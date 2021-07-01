#pragma once

#include "gpu_init.h"
#include "gpu_vertformat.h"

typedef enum {
	/* can be extended to support more types */
	GPU_USAGE_STREAM ,
	GPU_USAGE_STATIC , /* do not keep data in memory */
	GPU_USAGE_DYNAMIC ,
} GPUUsageType;

typedef struct GPU_VertBuf {
	GPU_VertFormat format ;
	/** Number of verts we want to draw. */
	unsigned int vertex_len;
	/** Number of verts data. */
	unsigned int vertex_alloc;
	/** 0 indicates not yet allocated. */
	GLuint vbo_id;
	/** Usage hint for GL optimisation. */
	unsigned int usage : 2;
	/** Data has been touched and need to be reuploaded to GPU. */
	unsigned int dirty : 1;
	unsigned char *data; /* NULL indicates data in VRAM (unmapped) */
} GPU_VertBuf ;

GPU_VertBuf *GPU_vertbuf_create ( GPUUsageType ) ;
GPU_VertBuf *GPU_vertbuf_create_with_format_ex ( const GPU_VertFormat * , GPUUsageType ) ;

#define GPU_vertbuf_create_with_format(format)		GPU_vertbuf_create_with_format_ex(format, GPU_USAGE_STATIC)

void GPU_vertbuf_clear ( GPU_VertBuf *verts ) ;
void GPU_vertbuf_discard ( GPU_VertBuf * ) ;

void GPU_vertbuf_init ( GPU_VertBuf * , GPUUsageType ) ;
void GPU_vertbuf_init_with_format_ex ( GPU_VertBuf * , const GPU_VertFormat * , GPUUsageType ) ;

#define GPU_vertbuf_init_with_format(verts, format)	GPU_vertbuf_init_with_format_ex(verts, format, GPU_USAGE_STATIC)

unsigned int GPU_vertbuf_size_get ( const GPU_VertBuf * ) ;
void GPU_vertbuf_data_alloc ( GPU_VertBuf * , unsigned int v_len ) ;
void GPU_vertbuf_data_resize ( GPU_VertBuf * , unsigned int v_len ) ;
void GPU_vertbuf_data_len_set ( GPU_VertBuf * , unsigned int v_len ) ;

/* The most important #set_attr variant is the untyped one. Get it right first.
 * It takes a void* so the app developer is responsible for matching their app data types
 * to the vertex attribute's type and component count. They're in control of both, so this
 * should not be a problem. */

void GPU_vertbuf_attr_set ( GPU_VertBuf * , unsigned int a_idx , unsigned int v_idx , const void *data );

void GPU_vertbuf_vert_set ( GPU_VertBuf *verts , unsigned int v_idx , const void *data );

/* Tightly packed, non interleaved input data. */
void GPU_vertbuf_attr_fill ( GPU_VertBuf * , unsigned int a_idx , const void *data );

void GPU_vertbuf_attr_fill_stride ( GPU_VertBuf * , unsigned int a_idx , unsigned int stride , const void *data );

/* For low level access only */
typedef struct GPU_VertBufRaw {
	unsigned int size;
	unsigned int stride;
	unsigned char *data;
	unsigned char *data_init;
#if TRUST_NO_ONE
	/* Only for overflow check */
	unsigned char *_data_end;
#endif
} GPU_VertBufRaw;

inline void *GPU_vertbuf_raw_step ( GPU_VertBufRaw *a ) {
	unsigned char *data = a->data;
	a->data += a->stride;
	return ( void * ) data;
}

inline unsigned int GPU_vertbuf_raw_used ( GPU_VertBufRaw *a ) {
	return ( ( a->data - a->data_init ) / a->stride );
}

void GPU_vertbuf_attr_get_raw_data ( GPU_VertBuf * , unsigned int a_idx , GPU_VertBufRaw *access );

void GPU_vertbuf_use ( GPU_VertBuf * );
