#pragma once

#include "gpu_init.h"

#include <vector>
#include <unordered_set>

struct GPU_Framebuffer ;

struct GPU_Context {
	GLuint default_vao ;
	GLuint default_framebuffer ;
	std::vector<GLuint> orphaned_vertarray_ids ;
	std::vector<GLuint> orphaned_framebuffer_ids ;

	std::unordered_set<struct GPU_Batch *> batches; /* Batches that have VAOs from this context */

	GPU_Framebuffer *current_fbo ;

	struct GPU_MatrixState *matrix_state ;

	class wxGLContext *wx_context ;
	class wxGLCanvas *wx_canvas ;
};

GPU_Context *GPU_context_create ( class wxGLCanvas *frame , GLuint default_framebuffer ) ;
GPU_Context *GPU_context_create ( class wxGLCanvas *frame ) ;
GPU_Context *GPU_context_active_get ( void ) ;
void GPU_context_active_set ( GPU_Context *ctx ) ;

GLuint GPU_vao_default ( void ) ;
GLuint GPU_framebuffer_default ( void ) ;
GLuint GPU_vao_alloc ( void ) ;
GLuint GPU_buf_alloc ( void ) ;
GLuint GPU_tex_alloc ( void ) ;
GLuint GPU_fbo_alloc ( void ) ;

void GPU_vao_free ( GLuint vao_id , GPU_Context *ctx ) ;
void GPU_fbo_free ( GLuint fbo_id , GPU_Context *ctx ) ;
void GPU_buf_free ( GLuint buf_id ) ;
void GPU_tex_free ( GLuint tex_id ) ;

void GPU_discard_context ( GPU_Context *ctx ) ;

void gpu_context_active_framebuffer_set ( GPU_Context *ctx , GPU_Framebuffer *fb ) ;
GPU_Framebuffer *gpu_context_active_framebuffer_get ( GPU_Context *ctx ) ;

struct GPU_MatrixState *gpu_context_active_matrix_state_get ( void );

void gpu_context_add_batch ( GPU_Context *ctx , struct GPU_Batch *batch ) ;
void gpu_context_remove_batch ( GPU_Context *ctx , struct GPU_Batch *batch ) ;
