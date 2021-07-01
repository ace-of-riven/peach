#pragma once

#include "gpu_primitive.h"

#define GPU_TRACK_INDEX_RANGE

typedef enum {
	GPU_INDEX_U16,
	GPU_INDEX_U32
} GPUIndexBufType ;

typedef struct GPU_IndexBuf {
	unsigned int index_start;
	unsigned int index_len ;
	bool is_subrange ;
#ifdef GPU_TRACK_INDEX_RANGE
	GPUIndexBufType index_type ;
	GLuint gl_index_type ;
	unsigned int base_index ;
#endif
	GLuint ibo_id ; /* 0 indicates not yet sent to VRAM */
	union {
		void *data ;			/* non-NULL indicates not yet sent to VRAM */
		struct GPU_IndexBuf *src ;	/* if is_subrange is true, this is the source buffer. */
	};
} GPU_IndexBuf ;

void GPU_indexbuf_use ( GPU_IndexBuf * ) ;
unsigned int GPU_indexbuf_size_get ( const GPU_IndexBuf * ) ;

typedef struct GPU_IndexBufBuilder {
	unsigned int max_allowed_index ;
	unsigned int max_index_len ;
	unsigned int index_len ;
	GPUPrimType prim_type ;
	unsigned int *data ;
} GPU_IndexBufBuilder ;

// supports all primitive types
void GPU_indexbuf_init_ex ( GPU_IndexBufBuilder * , GPUPrimType , unsigned int index_len , unsigned int vertex_len ) ;

// supports only GPU_PRIM_POINTS , GPU_PRIM_LINES and GPU_PRIM_TRIS
void GPU_indexbuf_init ( GPU_IndexBufBuilder * , GPUPrimType , unsigned int prim_len , unsigned int vertex_len ) ;

void GPU_indexbuf_add_generic_vert ( GPU_IndexBufBuilder * , unsigned int v );
void GPU_indexbuf_add_primitive_restart ( GPU_IndexBufBuilder * );

void GPU_indexbuf_add_point_vert ( GPU_IndexBufBuilder * , unsigned int v );
void GPU_indexbuf_add_line_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 );
void GPU_indexbuf_add_tri_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 , unsigned int v3 );
void GPU_indexbuf_add_line_adj_verts ( GPU_IndexBufBuilder * , unsigned int v1 , unsigned int v2 , unsigned int v3 , unsigned int v4 );

void GPU_indexbuf_set_point_vert ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 );
void GPU_indexbuf_set_line_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 );
void GPU_indexbuf_set_tri_verts ( GPU_IndexBufBuilder *builder , unsigned int elem , unsigned int v1 , unsigned int v2 , unsigned int v3 );

/* Skip primitive rendering at the given index. */
void GPU_indexbuf_set_point_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );
void GPU_indexbuf_set_line_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );
void GPU_indexbuf_set_tri_restart ( GPU_IndexBufBuilder *builder , unsigned int elem );

GPU_IndexBuf *GPU_indexbuf_build ( GPU_IndexBufBuilder * );
void GPU_indexbuf_build_in_place ( GPU_IndexBufBuilder * , GPU_IndexBuf * );

/* Create a subrange of an existing indexbuffer. */
GPU_IndexBuf *GPU_indexbuf_create_subrange ( GPU_IndexBuf *ibo , unsigned int start , unsigned int length );
void GPU_indexbuf_create_subrange_in_place ( GPU_IndexBuf *r_ibo , GPU_IndexBuf *ibo , unsigned int start , unsigned int length );

void GPU_indexbuf_discard ( GPU_IndexBuf * );

int GPU_indexbuf_primitive_len ( GPUPrimType prim_type );

inline void GPU_INDEXBUF_DISCARD_SAFE ( GPU_IndexBuf *ibo ) {
	if ( ibo )
		GPU_indexbuf_discard ( ibo ) ;
}
