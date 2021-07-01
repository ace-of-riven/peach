#pragma once

#include "gpu_init.h"

#include <glm/vec3.hpp>

#define GPU_VERT_ATTR_MAX_LEN 16
#define GPU_VERT_ATTR_MAX_NAMES 6
#define GPU_VERT_ATTR_NAMES_BUF_LEN 256
#define GPU_VERT_FORMAT_MAX_NAMES 63 /* More than enough, actual max is ~30. */
/* Computed as GPU_VERT_ATTR_NAMES_BUF_LEN / 30 (actual max format name). */
#define GPU_MAX_SAFE_ATTRIB_NAME 12

typedef enum {
	GPU_COMP_I8 ,
	GPU_COMP_U8 ,
	GPU_COMP_I16 ,
	GPU_COMP_U16 ,
	GPU_COMP_I32 ,
	GPU_COMP_U32 ,
	GPU_COMP_F32 ,
	GPU_COMP_I10 ,
} GPUVertCompType ;

typedef enum {
	GPU_FETCH_FLOAT ,
	GPU_FETCH_INT ,
	GPU_FETCH_INT_TO_FLOAT_UNIT , /* 127 (ubyte) -> 0.5 (and so on for other int types) */
	GPU_FETCH_INT_TO_FLOAT ,      /* 127 (any int type) -> 127.0 */
} GPUVertFetchMode ;

typedef struct GPU_VertAttr {
	unsigned int fetch_mode : 2 ;
	unsigned int comp_type : 3 ;
	// 1 or 4 or 8 or 12 or 16
	unsigned int comp_len : 5 ;
	// size in bytes, 1 to 64
	unsigned int sz : 7 ;
	// from beginning of vertex, in bytes
	unsigned int offset : 11 ;
	// up to GPU_VERT_ATTR_MAX_NAMES
	unsigned int name_len : 3 ;
	unsigned int gl_comp_type ;
	// 8 bytes
	unsigned char names [ GPU_VERT_ATTR_MAX_NAMES ] ;
} GPU_VertAttr ;

typedef struct GPU_VertFormat {
	/** 0 to 16 (GPU_VERT_ATTR_MAX_LEN). */
	unsigned int attr_len : 5;
	/** Total count of active vertex attribute names. (max GPU_VERT_FORMAT_MAX_NAMES) */
	unsigned int name_len : 6;
	/** Stride in bytes, 1 to 1024. */
	unsigned int stride : 11;
	/** Has the format been packed. */
	unsigned int packed : 1;
	/** Current offset in names[]. */
	unsigned int name_offset : 8;
	/** Store each attrib in one contiguous buffer region. */
	unsigned int deinterleaved : 1;

	GPU_VertAttr attrs [ GPU_VERT_ATTR_MAX_LEN ];
	char names [ GPU_VERT_ATTR_NAMES_BUF_LEN ];
} GPU_VertFormat ;

struct GPU_ShaderInterface ;

void GPU_vertformat_clear ( GPU_VertFormat * ) ;
void GPU_vertformat_copy ( GPU_VertFormat *dest , const GPU_VertFormat *src ) ;
void GPU_vertformat_from_interface ( GPU_VertFormat *format , const GPU_ShaderInterface *shaderface ) ;

unsigned int GPU_vertformat_attr_add ( GPU_VertFormat *, const char *name , GPUVertCompType , unsigned int , GPUVertFetchMode ) ;

void GPU_vertformat_alias_add ( GPU_VertFormat * , const char *alias ) ;
void GPU_vertformat_deinterleave ( GPU_VertFormat * ) ;

int GPU_vertformat_attr_id_get ( const GPU_VertFormat * , const char *name ) ;

inline const char *GPU_vertformat_attr_name_get ( const GPU_VertFormat *format , const GPU_VertAttr *attr , unsigned int n_idx ) {
	return format->names + attr->names [ n_idx ] ;
}

typedef struct GPU_PackedNormal {
	int x : 10 ;
	int y : 10 ;
	int z : 10 ;
	int w : 2 ;	// 0 by default, can manually set to { -2 , -1 , 0 , 1 }
} GPU_PackedNormal ;

#define SIGNED_INT_10_MAX 511
#define SIGNED_INT_10_MIN -512

inline int clampi ( int x , int min_allowed , int max_allowed ) {
	if ( x < min_allowed ) return min_allowed ;
	else if ( x > max_allowed ) return max_allowed ;
	return x ;
}

inline int gpu_convert_normalized_f32_to_i10 ( float x ) {
	int qx = x * 511.0f ;
	return clampi ( qx , SIGNED_INT_10_MIN , SIGNED_INT_10_MAX ) ;
}

inline int gpu_convert_i16_to_i10 ( short x ) {
	// 16-bit signed --> 10-bit signed ( round ? )
	return x >> 6 ;
}

inline GPU_PackedNormal GPU_normal_convert_i10_v3 ( const glm::vec3 &data ) {
	GPU_PackedNormal n = {
		gpu_convert_normalized_f32_to_i10 ( data.x ) ,
		gpu_convert_normalized_f32_to_i10 ( data.y ) ,
		gpu_convert_normalized_f32_to_i10 ( data.z )
	};
	return n ;
}

// internal

void VertexFormat_pack ( GPU_VertFormat *format ) ;
unsigned int vertex_buffer_size ( const GPU_VertFormat *format , unsigned int vertex_len ) ;

// TODO: Add GPU_normal_convert_i10_s3 ?
