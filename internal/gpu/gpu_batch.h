#pragma once

#include "gpu_init.h"
#include "gpu_vertbuff.h"
#include "gpu_vertformat.h"
#include "gpu_element.h"
#include "gpu_shader.h"
#include "gpu_context.h"

typedef enum GPU_BatchPhase {
	GPU_BATCH_UNUSED ,
	GPU_BATCH_READY_TO_FORMAT ,
	GPU_BATCH_READY_TO_BUILD ,
	GPU_BATCH_BUILDING ,
	GPU_BATCH_READY_TO_DRAW
} GPU_BatchPhase ;

#define GPU_BATCH_VBO_MAX_LEN 6
#define GPU_BATCH_INST_VBO_MAX_LEN 2
#define GPU_BATCH_VAO_STATIC_LEN 3
#define GPU_BATCH_VAO_DYN_ALLOC_COUNT 16

typedef struct GPU_Batch {
	// geometry

	// verts [0] is required, others can be NULL
	GPU_VertBuf *verts [ GPU_BATCH_VBO_MAX_LEN ] ;
	// instance attributes
	GPU_VertBuf *inst [ GPU_BATCH_INST_VBO_MAX_LEN ] ;
	// NULL if element list not needed
	GPU_IndexBuf *elem ;
	GLenum gl_prim_type ;

	// cached calues ( avoid dereferencing later )
	GLuint vao_id ;
	GLuint program ;

	const GPU_ShaderInterface *shinterface ;

	// book-keeping
	unsigned int owns_flag ;
	// used to free all vaos. this implies all vaos were created under the same context.
	GPU_Context *context ;
	GPU_BatchPhase phase ;
	bool program_in_use ;

	/* Vao management: remembers all geometry state (vertex attribute bindings & element buffer)
	 * for each shader interface. Start with a static number of vaos and fallback to dynamic count
	 * if necessary. Once a batch goes dynamic it does not go back. */
	bool is_dynamic_vao_count ;
	union {
		// static handle count
		struct {
			const GPU_ShaderInterface *interfaces [ GPU_BATCH_VAO_STATIC_LEN ] ;
			GLuint vao_ids [ GPU_BATCH_VAO_STATIC_LEN ] ;
		} static_vaos ;
		// dynamic handle count
		struct {
			const GPU_ShaderInterface **interfaces ;
			GLuint *vao_ids ;
			unsigned int count ;
		} dynamic_vaos ;
	};

	// This is the only solution if we want to have some data structure using batches as key to identify nodes. We must destroy these nodes with this callback
	void ( *free_callback )( GPU_Batch * , void * ) ;
	void *callback_data ;
} GPU_Batch ;

enum {
	GPU_BATCH_OWNS_VBO = ( 1 << 0 ) ,
	// each vbo index gets bit-shifted
	GPU_BATCH_OWNS_INSTANCES = ( 1 << 30 ) ,
	GPU_BATCH_OWNS_INDEX = ( 1u << 31 ) ,
};

GPU_Batch *GPU_batch_create_ex ( GPUPrimType , GPU_VertBuf * , GPU_IndexBuf * , unsigned int owns_flag ) ;
void GPU_batch_init_ex ( GPU_Batch * , GPUPrimType , GPU_VertBuf * , GPU_IndexBuf * , unsigned int owns_flag );
void GPU_batch_copy ( GPU_Batch *batch_dst , GPU_Batch *batch_src );

#define GPU_batch_create(prim, verts, elem) GPU_batch_create_ex(prim, verts, elem, 0)
#define GPU_batch_init(batch, prim, verts, elem) GPU_batch_init_ex(batch, prim, verts, elem, 0)

/* Same as discard but does not free. (does not call free callback). */
void GPU_batch_clear ( GPU_Batch * );

void GPU_batch_discard ( GPU_Batch * ); /* verts & elem are not discarded */

void GPU_batch_vao_cache_clear ( GPU_Batch * );

void GPU_batch_callback_free_set ( GPU_Batch * , void ( *callback )( GPU_Batch * , void * ) , void * );

void GPU_batch_instbuf_set ( GPU_Batch * , GPU_VertBuf * , bool own_vbo ); /* Instancing */
void GPU_batch_elembuf_set ( GPU_Batch *batch , GPU_IndexBuf *elem , bool own_ibo );

int GPU_batch_instbuf_add_ex ( GPU_Batch * , GPU_VertBuf * , bool own_vbo );
int GPU_batch_vertbuf_add_ex ( GPU_Batch * , GPU_VertBuf * , bool own_vbo );

#define GPU_batch_vertbuf_add(batch, verts) GPU_batch_vertbuf_add_ex(batch, verts, false)

void GPU_batch_program_set_no_use ( GPU_Batch * , uint32_t program , const GPU_ShaderInterface * );
void GPU_batch_program_set ( GPU_Batch * , uint32_t program , const GPU_ShaderInterface * );
void GPU_batch_program_set_shader ( GPU_Batch * , GPU_Shader *shader );
/* Entire batch draws with one shader program, but can be redrawn later with another program. */
/* Vertex shader's inputs must be compatible with the batch's vertex format. */

void GPU_batch_program_use_begin ( GPU_Batch * ); /* call before Batch_Uniform (temp hack?) */
void GPU_batch_program_use_end ( GPU_Batch * );

void GPU_batch_uniform_1ui ( GPU_Batch * , const char *name , unsigned int value );
void GPU_batch_uniform_1i ( GPU_Batch * , const char *name , int value );
void GPU_batch_uniform_1iv ( GPU_Batch *batch , const char *name , int count , int *value ) ;
void GPU_batch_uniform_1b ( GPU_Batch * , const char *name , bool value );
void GPU_batch_uniform_1f ( GPU_Batch * , const char *name , float value );
void GPU_batch_uniform_2f ( GPU_Batch * , const char *name , float x , float y );
void GPU_batch_uniform_3f ( GPU_Batch * , const char *name , float x , float y , float z );
void GPU_batch_uniform_4f ( GPU_Batch * , const char *name , float x , float y , float z , float w );
void GPU_batch_uniform_2fv ( GPU_Batch * , const char *name , const float data [ 2 ] );
void GPU_batch_uniform_3fv ( GPU_Batch * , const char *name , const float data [ 3 ] );
void GPU_batch_uniform_4fv ( GPU_Batch * , const char *name , const float data [ 4 ] );
void GPU_batch_uniform_2fv_array ( GPU_Batch * , const char *name , const int len , const float *data );
void GPU_batch_uniform_4fv_array ( GPU_Batch * , const char *name , const int len , const float *data );
void GPU_batch_uniform_mat4 ( GPU_Batch * , const char *name , const float data [ 4 ][ 4 ] );

void GPU_batch_draw ( GPU_Batch * );

/* Needs to be called before GPU_batch_draw_advanced. */
void GPU_batch_bind ( GPU_Batch * );
/* This does not bind/unbind shader and does not call GPU_matrix_bind() */
void GPU_batch_draw_advanced ( GPU_Batch * , int v_first , int v_count , int i_first , int i_count );

/* Does not even need batch */
void GPU_draw_primitive ( GPUPrimType , int v_count );

/**
 * #GPUDrawList is an API to do lots of similar draw-calls very fast using multi-draw-indirect.
 * There is a fallback if the feature is not supported.
 */
typedef struct GPUDrawList GPUDrawList;

GPUDrawList *GPU_draw_list_create ( int length );
void GPU_draw_list_discard ( GPUDrawList *list );
void GPU_draw_list_init ( GPUDrawList *list , GPU_Batch *batch );
void GPU_draw_list_command_add ( GPUDrawList *list , int v_first , int v_count , int i_first , int i_count );
void GPU_draw_list_submit ( GPUDrawList *list );

void gpu_batch_init ( void );
void gpu_batch_exit ( void );