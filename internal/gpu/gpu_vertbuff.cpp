#include "gpu_vertbuff.h"
#include "gpu_shader.h"
#include "gpu_context.h"

static GLenum convert_usage_type_to_gl ( GPUUsageType type ) {
	switch ( type ) {
		case GPU_USAGE_DYNAMIC: return GL_DYNAMIC_DRAW ;
		case GPU_USAGE_STATIC: return GL_STATIC_DRAW ;
		case GPU_USAGE_STREAM: return GL_STREAM_DRAW ;
	}
	return GL_INVALID_ENUM ;
}

GPU_VertBuf *GPU_vertbuf_create ( GPUUsageType usage ) {
	GPU_VertBuf *verts = new GPU_VertBuf ( ) ;
	GPU_vertbuf_init ( verts , usage ) ;
	return verts ;
}

GPU_VertBuf *GPU_vertbuf_create_with_format_ex ( const GPU_VertFormat *format , GPUUsageType usage ) {
	GPU_VertBuf *verts = GPU_vertbuf_create ( usage ) ;
	GPU_vertformat_copy ( &verts->format , format ) ;
	if ( !format->packed )
		VertexFormat_pack ( &verts->format ) ;
	return verts ;
}

void GPU_vertbuf_init ( GPU_VertBuf *verts , GPUUsageType type ) {
	memset ( verts , 0 , sizeof ( GPU_VertBuf ) ) ;
	verts->usage = type ;
	verts->dirty = true ;
}

void GPU_vertbuf_init_with_format_ex ( GPU_VertBuf *verts , const GPU_VertFormat *format , GPUUsageType usage ) {
	GPU_vertbuf_init ( verts , usage ) ;
	GPU_vertformat_copy ( &verts->format , format ) ;
	if ( !format->packed )
		VertexFormat_pack ( &verts->format ) ;
}

// Same as discard but does not free
void GPU_vertbuf_clear ( GPU_VertBuf *verts ) {
	if ( verts->vbo_id ) {
		GPU_buf_free ( verts->vbo_id ) ;
		verts->vbo_id = 0 ;
	}
	if ( verts->data ) {
		free ( verts->data ) ;
		verts->data = NULL ;
	}
}

void GPU_vertbuf_discard ( GPU_VertBuf *verts ) {
	GPU_vertbuf_clear ( verts ) ;
	free ( verts ) ;
}

unsigned int GPU_vertbuf_size_get ( const GPU_VertBuf *verts ) {
	return vertex_buffer_size ( &verts->format , verts->vertex_len ) ;
}

void GPU_vertbuf_data_alloc ( GPU_VertBuf *verts , unsigned int v_len ) {
	GPU_VertFormat *format = &verts->format ;
	if ( !format->packed )
		VertexFormat_pack ( format ) ;
	// discard previous data if any
	if ( verts->data )
		free ( verts->data ) ;
	verts->dirty = true ;
	verts->vertex_len = verts->vertex_alloc = v_len ;
	verts->data = ( unsigned char * ) malloc ( GPU_vertbuf_size_get ( verts ) ) ;
}


/* resize buffer keeping existing data */
void GPU_vertbuf_data_resize ( GPU_VertBuf *verts , unsigned int v_len )
{
	verts->dirty = true;
	verts->vertex_len = verts->vertex_alloc = v_len;
	verts->data = ( unsigned char * ) realloc ( verts->data , sizeof ( GLubyte ) * GPU_vertbuf_size_get ( verts ) ) ;
}

/* Set vertex count but does not change allocation.
 * Only this many verts will be uploaded to the GPU and rendered.
 * This is useful for streaming data. */
void GPU_vertbuf_data_len_set ( GPU_VertBuf *verts , unsigned int v_len ) {
	verts->vertex_len = v_len;
}

void GPU_vertbuf_attr_set ( GPU_VertBuf *verts , unsigned int a_idx , unsigned int v_idx , const void *data ) {
	const GPU_VertFormat *format = &verts->format;
	const GPU_VertAttr *a = &format->attrs [ a_idx ];

	verts->dirty = true;
	memcpy ( ( GLubyte * ) verts->data + a->offset + v_idx * format->stride , data , a->sz );
}

void GPU_vertbuf_attr_fill ( GPU_VertBuf *verts , unsigned int a_idx , const void *data )
{
	const GPU_VertFormat *format = &verts->format;
	const GPU_VertAttr *a = &format->attrs [ a_idx ];

	const unsigned int stride = a->sz; /* tightly packed input data */

	GPU_vertbuf_attr_fill_stride ( verts , a_idx , stride , data );
}

/** Fills a whole vertex (all attribs). Data must match packed layout.  */
void GPU_vertbuf_vert_set ( GPU_VertBuf *verts , unsigned int v_idx , const void *data ) {
	const GPU_VertFormat *format = &verts->format;

	verts->dirty = true;
	memcpy ( ( GLubyte * ) verts->data + v_idx * format->stride , data , format->stride );
}

void GPU_vertbuf_attr_fill_stride ( GPU_VertBuf *verts , unsigned int a_idx , unsigned int stride , const void *data ) {
	const GPU_VertFormat *format = &verts->format;
	const GPU_VertAttr *a = &format->attrs [ a_idx ];

	verts->dirty = true;
	const unsigned int vertex_len = verts->vertex_len;

	if ( format->attr_len == 1 && stride == format->stride ) {
		/* we can copy it all at once */
		memcpy ( verts->data , data , vertex_len * a->sz );
	}
	else {
		/* we must copy it per vertex */
		for ( unsigned int v = 0; v < vertex_len; v++ ) {
			memcpy ( ( GLubyte * ) verts->data + a->offset + v * format->stride ,
				( const GLubyte * ) data + v * stride ,
				a->sz );
		}
	}
}

void GPU_vertbuf_attr_get_raw_data ( GPU_VertBuf *verts , unsigned int a_idx , GPU_VertBufRaw *access ) {
	const GPU_VertFormat *format = &verts->format;
	const GPU_VertAttr *a = &format->attrs [ a_idx ];

	verts->dirty = true;

	access->size = a->sz;
	access->stride = format->stride;
	access->data = ( GLubyte * ) verts->data + a->offset;
	access->data_init = access->data;
}

static void VertBuffer_upload_data ( GPU_VertBuf *verts ) {
	if ( verts->dirty == false ) return ;
	unsigned int buffer_sz = GPU_vertbuf_size_get ( verts );

	/* orphan the vbo to avoid sync */
	glBufferData ( GL_ARRAY_BUFFER , buffer_sz , NULL , convert_usage_type_to_gl ( ( GPUUsageType ) verts->usage ) ) ;
	/* upload data */
	glBufferSubData ( GL_ARRAY_BUFFER , 0 , buffer_sz , verts->data );

	if ( verts->usage == GPU_USAGE_STATIC ) {
		free ( verts->data );
		verts->data = NULL;
	}
	verts->dirty = false;
}

void GPU_vertbuf_use ( GPU_VertBuf *verts ) {
	/* only create the buffer the 1st time */
	if ( verts->vbo_id == 0 ) {
		verts->vbo_id = GPU_buf_alloc ( );
	}
	glBindBuffer ( GL_ARRAY_BUFFER , verts->vbo_id );
	if ( verts->dirty ) {
		VertBuffer_upload_data ( verts );
	}
}
