#include "gpu_vertformat.h"
#include "gpu_shader.h"

#include <algorithm>

void GPU_vertformat_clear ( GPU_VertFormat *format ) {
	memset ( format , 0 , sizeof ( GPU_VertFormat ) ) ;
}

void GPU_vertformat_copy ( GPU_VertFormat *dest , const GPU_VertFormat *src ) {
	memcpy ( dest , src , sizeof ( GPU_VertFormat ) ) ;
}

static GLenum convert_comp_type_to_gl ( GPUVertCompType type ) {
	switch ( type ) {
		case GPU_COMP_I8: return GL_BYTE ;
		case GPU_COMP_U8: return GL_UNSIGNED_BYTE ;
		case GPU_COMP_I16: return GL_SHORT ;
		case GPU_COMP_U16: return GL_UNSIGNED_SHORT ;
		case GPU_COMP_I32: return GL_INT ;
		case GPU_COMP_U32: return GL_UNSIGNED_INT ;
		case GPU_COMP_F32: return GL_FLOAT ;
		case GPU_COMP_I10: return GL_INT_2_10_10_10_REV;
	};
	return GL_INVALID_ENUM ;
}

static unsigned int comp_sz ( GPUVertCompType type ) {
	static const GLubyte sizes [ ] = { 1,1,2,2,4,4,4 } ;
	return sizes [ type ] ;
}

static unsigned int attr_sz ( const GPU_VertAttr *a ) {
	if ( a->comp_type == GPU_COMP_I10 )
		return 4 ;
	return a->comp_len * comp_sz ( ( GPUVertCompType ) a->comp_type ) ;
}

static unsigned int attr_align ( const GPU_VertAttr *a ) {
	if ( a->comp_type == GPU_COMP_I10 )
		return 4 ; /* always packed as 10_10_10_2 */
	unsigned int c = comp_sz ( ( GPUVertCompType ) a->comp_type ) ;
	if ( a->comp_len == 3 && c <= 2 )
		return 4 ; /* AMD HW can't fetch these well, so pad it out (other vendors too?) */
	return c ; /* most fetches are ok if components are naturally aligned */
}

unsigned int vertex_buffer_size ( const GPU_VertFormat *format , unsigned int vertex_len ) {
	if ( format->packed == false or format->stride <= 0 )
		fprintf ( stderr , "GPU_VertFormat is not packed and therefore vertex_buffer_size(); failed.\n" ) ;
	return format->stride * vertex_len ;
}

static unsigned char copy_attr_name ( GPU_VertFormat *format , const char *name ) {
	unsigned char name_offset = format->name_offset ;
	char *name_copy = format->names + name_offset ;
	unsigned int available = GPU_VERT_ATTR_NAMES_BUF_LEN - name_offset ;
	bool terminated = false ;

	for ( unsigned int i = 0 ; i < available ; i++ ) {
		const char c = name [ i ] ;
		name_copy [ i ] = c ;
		if ( c == '\0' ) {
			terminated = true ;
			format->name_offset += ( i + 1 ) ;
			break;
		}
	}

	assert ( terminated ) ;
	assert ( format->name_offset <= GPU_VERT_ATTR_NAMES_BUF_LEN ) ;

	return name_offset ;
}

unsigned int GPU_vertformat_attr_add ( GPU_VertFormat *format , const char *name , GPUVertCompType comp_type , unsigned int comp_len , GPUVertFetchMode fetch_mode ) {
	assert ( format->name_len < GPU_VERT_FORMAT_MAX_NAMES ); /* there's room for more */
	assert ( format->attr_len < GPU_VERT_ATTR_MAX_LEN );     /* there's room for more */
	assert ( !format->packed );                              /* packed means frozen/locked */
	assert ( ( comp_len >= 1 && comp_len <= 4 ) || comp_len == 8 || comp_len == 12 || comp_len == 16 );

	format->name_len++;

	const unsigned int attr_id = format->attr_len++ ;
	GPU_VertAttr *attr = &format->attrs [ attr_id ] ;

	attr->names [ attr->name_len++ ] = copy_attr_name ( format , name ) ;
	attr->comp_type = comp_type ;
	attr->gl_comp_type = convert_comp_type_to_gl ( comp_type ) ;
	attr->comp_len = ( comp_type == GPU_COMP_I10 ) ? 4 : comp_len ;
	attr->sz = attr_sz ( attr ) ;
	attr->offset = 0 ;
	attr->fetch_mode = fetch_mode ;

	return attr_id ;
}

void GPU_vertformat_alias_add ( GPU_VertFormat *format , const char *alias ) {
	GPU_VertAttr *attr = &format->attrs [ format->attr_len - 1 ];
	format->name_len++; /* multiname support */
	attr->names [ attr->name_len++ ] = copy_attr_name ( format , alias );
}

void GPU_vertformat_deinterleave ( GPU_VertFormat *format ) {
	/* Ideally we should change the stride and offset here. This would allow
	 * us to use GPU_vertbuf_attr_set / GPU_vertbuf_attr_fill. But since
	 * we use only 11 bits for attr->offset this limits the size of the
	 * buffer considerably. So instead we do the conversion when creating
	 * bindings in create_bindings(). */
	format->deinterleaved = true ;
}

int GPU_vertformat_attr_id_get ( const GPU_VertFormat *format , const char *name ) {
	for ( int i = 0; i < format->attr_len; i++ ) {
		const GPU_VertAttr *attr = &format->attrs [ i ];
		for ( int j = 0; j < attr->name_len; j++ ) {
			const char *attr_name = GPU_vertformat_attr_name_get ( format , attr , j );
			if ( STREQ ( name , attr_name ) and attr_name )
				return i;
		}
	}
	return -1;
}

unsigned int padding ( unsigned int offset , unsigned int alignment ) {
	const unsigned int mod = offset % alignment;
	return ( mod == 0 ) ? 0 : ( alignment - mod );
}

void VertexFormat_pack ( GPU_VertFormat *format ) {
	/* For now, attributes are packed in the order they were added,
	 * making sure each attribute is naturally aligned (add padding where necessary)
	 * Later we can implement more efficient packing w/ reordering
	 * (keep attribute ID order, adjust their offsets to reorder in buffer). */

	 /* TODO: realloc just enough to hold the final combo string. And just enough to
	  * hold used attributes, not all 16. */

	GPU_VertAttr *a0 = &format->attrs [ 0 ];
	a0->offset = 0;
	unsigned int offset = a0->sz;

	for ( unsigned int a_idx = 1; a_idx < format->attr_len; a_idx++ ) {
		GPU_VertAttr *a = &format->attrs [ a_idx ];
		unsigned int mid_padding = padding ( offset , attr_align ( a ) );
		offset += mid_padding;
		a->offset = offset;
		offset += a->sz;
	}

	unsigned int end_padding = padding ( offset , attr_align ( a0 ) );

	format->stride = offset + end_padding;
	format->packed = true;
}

//


static unsigned int calc_input_component_size ( const GPU_ShaderInput *input )
{
	int size = input->size;
	switch ( input->gl_type ) {
		case GL_FLOAT_VEC2:
		case GL_INT_VEC2:
		case GL_UNSIGNED_INT_VEC2:
			return size * 2;
		case GL_FLOAT_VEC3:
		case GL_INT_VEC3:
		case GL_UNSIGNED_INT_VEC3:
			return size * 3;
		case GL_FLOAT_VEC4:
		case GL_FLOAT_MAT2:
		case GL_INT_VEC4:
		case GL_UNSIGNED_INT_VEC4:
			return size * 4;
		case GL_FLOAT_MAT3:
			return size * 9;
		case GL_FLOAT_MAT4:
			return size * 16;
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT3x2:
			return size * 6;
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT4x2:
			return size * 8;
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x3:
			return size * 12;
		default:
			return size;
  }
}

static void get_fetch_mode_and_comp_type ( int gl_type , GPUVertCompType *r_comp_type , GPUVertFetchMode *r_fetch_mode ) {
	switch ( gl_type ) {
		case GL_FLOAT:
		case GL_FLOAT_VEC2:
		case GL_FLOAT_VEC3:
		case GL_FLOAT_VEC4:
		case GL_FLOAT_MAT2:
		case GL_FLOAT_MAT3:
		case GL_FLOAT_MAT4:
		case GL_FLOAT_MAT2x3:
		case GL_FLOAT_MAT2x4:
		case GL_FLOAT_MAT3x2:
		case GL_FLOAT_MAT3x4:
		case GL_FLOAT_MAT4x2:
		case GL_FLOAT_MAT4x3:
			*r_comp_type = GPU_COMP_F32;
			*r_fetch_mode = GPU_FETCH_FLOAT;
			break;
		case GL_INT:
		case GL_INT_VEC2:
		case GL_INT_VEC3:
		case GL_INT_VEC4:
			*r_comp_type = GPU_COMP_I32;
			*r_fetch_mode = GPU_FETCH_INT;
			break;
		case GL_UNSIGNED_INT:
		case GL_UNSIGNED_INT_VEC2:
		case GL_UNSIGNED_INT_VEC3:
		case GL_UNSIGNED_INT_VEC4:
			*r_comp_type = GPU_COMP_U32;
			*r_fetch_mode = GPU_FETCH_INT;
			break;
		default:
			assert ( 0 );
	}
}

void GPU_vertformat_from_interface ( GPU_VertFormat *format , const GPU_ShaderInterface *shaderface ) {
	const char *name_buffer = shaderface->name_buffer;

	GPU_vertformat_clear ( format ) ;

	for ( int i = 0; i < GPU_NUM_SHADERINTERFACE_BUCKETS; i++ ) {
		const GPU_ShaderInput *input = shaderface->attr_buckets [ i ];
		if ( input == NULL ) {
			continue;
		}

		const GPU_ShaderInput *next = input;
		while ( next != NULL ) {
			input = next;
			next = input->next;

			/* OpenGL attributes such as `gl_VertexID` have a location of -1. */
			if ( input->location < 0 ) {
				continue;
			}

			format->name_len++; /* multiname support */
			format->attr_len++;

			GPUVertCompType comp_type;
			GPUVertFetchMode fetch_mode;
			get_fetch_mode_and_comp_type ( input->gl_type , &comp_type , &fetch_mode );

			GPU_VertAttr *attr = &format->attrs [ input->location ];

			attr->names [ attr->name_len++ ] = copy_attr_name ( format , name_buffer + input->name_offset );
			attr->offset = 0; /* offsets & stride are calculated later (during pack) */
			attr->comp_len = calc_input_component_size ( input );
			attr->sz = attr->comp_len * 4;
			attr->fetch_mode = fetch_mode;
			attr->comp_type = comp_type;
			attr->gl_comp_type = convert_comp_type_to_gl ( comp_type );

			printf ( "VertFormat , Attr %d(%s) is of type %s and has %d components.\n" , input->location , name_buffer + input->name_offset , OpenGL_GLSL_type_to_string ( input->gl_type ) , attr->comp_len ) ;
		}
	}
}
