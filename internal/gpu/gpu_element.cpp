#include "gpu_element.h"
#include "gpu_context.h"

#include <malloc.h>
#include <cassert>

#define KEEP_SINGLE_COPY 1

#define RESTART_INDEX 0xFFFFFFFF

#define GPU_TRACK_INDEX_RANGE

static GLenum convert_index_type_to_gl ( GPUIndexBufType type )
{
	switch ( type ) {
	    case GPU_INDEX_U16: return GL_UNSIGNED_SHORT;
	    case GPU_INDEX_U32: return GL_UNSIGNED_INT;
	};
	return GL_INVALID_ENUM ;
}

unsigned int GPU_indexbuf_size_get ( const GPU_IndexBuf *elem ) {
#ifdef GPU_TRACK_INDEX_RANGE
	unsigned int index_type_len = 1 ;
	switch ( elem->index_type ) {
		case GPU_INDEX_U16:
			index_type_len = sizeof ( GLushort );
			break;
		case GPU_INDEX_U32:
			index_type_len = sizeof ( GLuint );
			break;
	};
	return elem->index_len * index_type_len;
#else
	return elem->index_len * sizeof ( GLuint );
#endif
}

int GPU_indexbuf_primitive_len ( GPUPrimType prim_type ) {
	switch ( prim_type ) {
		case GPU_PRIM_POINTS:
			return 1;
		case GPU_PRIM_LINES:
			return 2;
		case GPU_PRIM_TRIS:
			return 3;
		case GPU_PRIM_LINES_ADJ:
			return 4;
		default:
			break;
	}
	return -1;
}

void GPU_indexbuf_init_ex ( GPU_IndexBufBuilder *builder , GPUPrimType prim_type , unsigned int index_len , unsigned int vertex_len ) {
	builder->max_allowed_index = vertex_len - 1;
	builder->max_index_len = index_len;
	builder->index_len = 0;  // start empty
	builder->prim_type = prim_type;
	builder->data = ( unsigned int * ) malloc ( builder->max_index_len * sizeof ( unsigned int ) ) ;
}

void GPU_indexbuf_init ( GPU_IndexBufBuilder *builder , GPUPrimType prim_type , unsigned int prim_len , unsigned int vertex_len ) {
	int verts_per_prim = GPU_indexbuf_primitive_len ( prim_type );
	GPU_indexbuf_init_ex ( builder , prim_type , prim_len * ( unsigned int ) verts_per_prim , vertex_len );
}

void GPU_indexbuf_add_generic_vert ( GPU_IndexBufBuilder *builder , unsigned int v ) {
	builder->data [ builder->index_len++ ] = v;
}

void GPU_indexbuf_add_primitive_restart ( GPU_IndexBufBuilder *builder ) {
	builder->data [ builder->index_len++ ] = RESTART_INDEX;
}

void GPU_indexbuf_add_point_vert ( GPU_IndexBufBuilder *builder , unsigned int v ) {
	GPU_indexbuf_add_generic_vert ( builder , v );
}

void GPU_indexbuf_add_line_verts ( GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 ) {
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
}

void GPU_indexbuf_add_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 , unsigned int v3 ) {
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
	GPU_indexbuf_add_generic_vert ( builder , v3 );
}

void GPU_indexbuf_add_line_adj_verts ( GPU_IndexBufBuilder *builder , unsigned int v1 , unsigned int v2 , unsigned int v3 , unsigned int v4 ) {
	GPU_indexbuf_add_generic_vert ( builder , v1 );
	GPU_indexbuf_add_generic_vert ( builder , v2 );
	GPU_indexbuf_add_generic_vert ( builder , v3 );
	GPU_indexbuf_add_generic_vert ( builder , v4 );
}

void GPU_indexbuf_set_point_vert ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 ) {
	assert ( builder->prim_type == GPU_PRIM_POINTS );
	assert ( elem < builder->max_index_len );
	builder->data [ elem++ ] = v1;
	if ( builder->index_len < elem ) {
		builder->index_len = elem;
	}
}

void GPU_indexbuf_set_line_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 ) {
	assert ( builder->prim_type == GPU_PRIM_LINES );
	assert ( v1 != v2 );
	assert ( v1 <= builder->max_allowed_index );
	assert ( v2 <= builder->max_allowed_index );
	assert ( ( elem + 1 ) * 2 <= builder->max_index_len );
	unsigned int idx = elem * 2;
	builder->data [ idx++ ] = v1;
	builder->data [ idx++ ] = v2;
	if ( builder->index_len < idx ) {
		builder->index_len = idx;
	}
}

void GPU_indexbuf_set_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 , unsigned int v3 ) {
	assert ( builder->prim_type == GPU_PRIM_TRIS );
	assert ( v1 != v2 && v2 != v3 && v3 != v1 );
	assert ( v1 <= builder->max_allowed_index );
	assert ( v2 <= builder->max_allowed_index );
	assert ( v3 <= builder->max_allowed_index );
	assert ( ( elem + 1 ) * 3 <= builder->max_index_len );
	unsigned int idx = elem * 3;
	builder->data [ idx++ ] = v1;
	builder->data [ idx++ ] = v2;
	builder->data [ idx++ ] = v3;
	if ( builder->index_len < idx ) {
		builder->index_len = idx;
	}
}

void GPU_indexbuf_set_point_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->prim_type == GPU_PRIM_POINTS );
	assert ( elem < builder->max_index_len );
	builder->data [ elem++ ] = RESTART_INDEX;
	if ( builder->index_len < elem ) {
		builder->index_len = elem;
	}
}

void GPU_indexbuf_set_line_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->prim_type == GPU_PRIM_LINES );
	assert ( ( elem + 1 ) * 2 <= builder->max_index_len );
	unsigned int idx = elem * 2;
	builder->data [ idx++ ] = RESTART_INDEX;
	builder->data [ idx++ ] = RESTART_INDEX;
	if ( builder->index_len < idx ) {
		builder->index_len = idx;
	}
}

void GPU_indexbuf_set_tri_restart ( GPU_IndexBufBuilder *builder , unsigned int elem ) {
	assert ( builder->prim_type == GPU_PRIM_TRIS );
	assert ( ( elem + 1 ) * 3 <= builder->max_index_len );
	unsigned int idx = elem * 3;
	builder->data [ idx++ ] = RESTART_INDEX;
	builder->data [ idx++ ] = RESTART_INDEX;
	builder->data [ idx++ ] = RESTART_INDEX;
	if ( builder->index_len < idx ) {
		builder->index_len = idx;
	}
}

GPU_IndexBuf *GPU_indexbuf_create_subrange ( GPU_IndexBuf *elem_src , unsigned int start , unsigned int length ) {
	GPU_IndexBuf *elem = new GPU_IndexBuf ( ) ;
	GPU_indexbuf_create_subrange_in_place ( elem , elem_src , start , length );
	return elem;
}

void GPU_indexbuf_create_subrange_in_place ( GPU_IndexBuf *elem , GPU_IndexBuf *elem_src , unsigned int start , unsigned int length ) {
	assert ( elem_src && !elem_src->is_subrange );
	assert ( ( length == 0 ) || ( start + length <= elem_src->index_len ) );
#ifdef GPU_TRACK_INDEX_RANGE
	elem->index_type = elem_src->index_type;
	elem->gl_index_type = elem_src->gl_index_type;
	elem->base_index = elem_src->base_index;
#endif
	elem->is_subrange = true;
	elem->src = elem_src;
	elem->index_start = start;
	elem->index_len = length;
}

#ifdef GPU_TRACK_INDEX_RANGE
/* Everything remains 32 bit while building to keep things simple.
 * Find min/max after, then convert to smallest index type possible. */

static unsigned int index_range ( const unsigned int values [ ] , unsigned int value_len , unsigned int *min_out , unsigned int *max_out ) {
	if ( value_len == 0 ) {
		*min_out = 0;
		*max_out = 0;
		return 0;
	}
	unsigned int min_value = RESTART_INDEX;
	unsigned int max_value = 0;
	for ( unsigned int i = 0; i < value_len; i++ ) {
		const unsigned int value = values [ i ];
		if ( value == RESTART_INDEX ) {
			continue;
		}
		else if ( value < min_value ) {
			min_value = value;
		}
		else if ( value > max_value ) {
			max_value = value;
		}
	}
	if ( min_value == RESTART_INDEX ) {
		*min_out = 0;
		*max_out = 0;
		return 0;
	}
	else {
		*min_out = min_value;
		*max_out = max_value;
		return max_value - min_value;
	}
}

static void squeeze_indices_short ( GPU_IndexBufBuilder *builder , GPU_IndexBuf *elem , unsigned int min_index , unsigned int max_index ) {
	const unsigned int *values = builder->data;
	const unsigned int index_len = elem->index_len;

	/* data will never be *larger* than builder->data...
	 * converting in place to avoid extra allocation */
	GLushort *data = ( GLushort * ) builder->data;

	if ( max_index >= 0xFFFF ) {
		elem->base_index = min_index;
		for ( unsigned int i = 0; i < index_len; i++ ) {
			data [ i ] = ( values [ i ] == RESTART_INDEX ) ? 0xFFFF : ( GLushort ) ( values [ i ] - min_index );
		}
	}
	else {
		elem->base_index = 0;
		for ( unsigned int i = 0; i < index_len; i++ ) {
			data [ i ] = ( GLushort ) ( values [ i ] );
		}
	}
}

#endif /* GPU_TRACK_INDEX_RANGE */

GPU_IndexBuf *GPU_indexbuf_build ( GPU_IndexBufBuilder *builder ) {
	GPU_IndexBuf *elem = new GPU_IndexBuf ( ) ;
	GPU_indexbuf_build_in_place ( builder , elem );
	return elem;
}

void GPU_indexbuf_build_in_place ( GPU_IndexBufBuilder *builder , GPU_IndexBuf *elem ) {
	elem->index_len = builder->index_len;
	elem->ibo_id = 0; /* Created at first use. */

#ifdef GPU_TRACK_INDEX_RANGE
	unsigned int min_index , max_index;
	unsigned int range = index_range ( builder->data , builder->index_len , &min_index , &max_index );

	/* count the primitive restart index. */
	range += 1;

	if ( range <= 0xFFFF ) {
		elem->index_type = GPU_INDEX_U16;
		squeeze_indices_short ( builder , elem , min_index , max_index );
	}
	else {
		elem->index_type = GPU_INDEX_U32;
		elem->base_index = 0;
	}
	elem->gl_index_type = convert_index_type_to_gl ( elem->index_type );
#endif

	/* Transfer data ownership to GPUIndexBuf.
	 * It will be uploaded upon first use. */
	elem->data = builder->data;
	builder->data = NULL;
	/* other fields are safe to leave */
}

static void indexbuf_upload_data ( GPU_IndexBuf *elem )
{
	/* send data to GPU */
	glBufferData ( GL_ELEMENT_ARRAY_BUFFER , GPU_indexbuf_size_get ( elem ) , elem->data , GL_STATIC_DRAW );
	/* No need to keep copy of data in system memory. */
	free ( elem->data );
	elem->data = NULL;
}

void GPU_indexbuf_use ( GPU_IndexBuf *elem ) {
	if ( elem->is_subrange ) {
		GPU_indexbuf_use ( elem->src );
		return;
	}
	if ( elem->ibo_id == 0 )
		elem->ibo_id = GPU_buf_alloc ( );
	glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER , elem->ibo_id );
	if ( elem->data != NULL ) {
		indexbuf_upload_data ( elem );
	}
}

void GPU_indexbuf_discard ( GPU_IndexBuf *elem ) {
	if ( elem->ibo_id )
		GPU_buf_free ( elem->ibo_id );
	if ( !elem->is_subrange && elem->data )
		free ( elem->data );
	free ( elem );
}
