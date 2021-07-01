#include "gpu_context.h"
#include "../application/application.h"

#include "gpu_matrix.h"
#include "gpu_batch.h"

#include <cassert>

static std::vector<GLuint> orphaned_buffer_ids;
static std::vector<GLuint> orphaned_texture_ids;

GPU_Context *current_ctx = nullptr ;

GPU_MatrixState *GPU_matrix_state_create ( void ) ;
void GPU_matrix_state_discard ( GPU_MatrixState *state ) ;

GPU_Context *GPU_context_create ( wxGLCanvas *frame , GLuint default_framebuffer ) {
	GPU_Context *ctx = new GPU_Context ( ) ;
	ctx->wx_context = new wxGLContext ( frame ) ;
	ctx->current_fbo = 0 ;
	ctx->default_framebuffer = default_framebuffer ;
	ctx->wx_canvas = frame ;
	ctx->matrix_state = GPU_matrix_state_create ( ) ;
	return ctx ;
}

GPU_Context *GPU_context_create ( wxGLCanvas *frame ) {
	return GPU_context_create ( frame , 0 ) ;
}

GPU_Context *GPU_context_active_get ( void ) {
	return current_ctx ;
}

void GPU_discard_context ( GPU_Context *ctx ) {
	assert ( ctx == GPU_context_active_get ( ) ) ;
	assert ( ctx->orphaned_vertarray_ids.empty ( ) ) ;

	GPU_matrix_state_discard ( ctx->matrix_state ) ;

	/* delete remaining vaos */
	while ( !ctx->batches.empty ( ) ) {
		/* this removes the array entry */
		GPU_batch_vao_cache_clear ( *ctx->batches.begin ( ) );
	}

	glDeleteVertexArrays ( 1 , &ctx->default_vao ) ;
	delete ctx->wx_context ;
	current_ctx = nullptr ;
}

static void GPU_context_orphans_clear ( GPU_Context *ctx ) {
	assert ( ctx ) ;

	if ( !ctx->orphaned_vertarray_ids.empty ( ) ) {
		unsigned int orphan_len = ( unsigned int ) ctx->orphaned_vertarray_ids.size ( ) ;
		glDeleteVertexArrays ( orphan_len , &ctx->orphaned_vertarray_ids [ 0 ] ) ;
		ctx->orphaned_vertarray_ids.clear ( ) ;
	}
	if ( !ctx->orphaned_framebuffer_ids.empty ( ) ) {
		unsigned int orphan_len = ( unsigned int ) ctx->orphaned_framebuffer_ids.size ( ) ;
		glDeleteFramebuffers ( orphan_len , &ctx->orphaned_framebuffer_ids [ 0 ] ) ;
		ctx->orphaned_framebuffer_ids.clear ( ) ;
	}

	if ( !orphaned_buffer_ids.empty ( ) ) {
		unsigned int orphan_len = ( unsigned int ) orphaned_buffer_ids.size ( ) ;
		glDeleteBuffers ( orphan_len , &orphaned_buffer_ids [ 0 ] ) ;
		orphaned_buffer_ids.clear ( ) ;
	}
	if ( !orphaned_texture_ids.empty ( ) ) {
		unsigned int orphan_len = ( unsigned int ) orphaned_texture_ids.size ( );
		glDeleteTextures ( orphan_len , orphaned_texture_ids.data ( ) );
		orphaned_texture_ids.clear ( );
	}
}

void GPU_context_active_set ( GPU_Context *ctx ) {
	if ( ctx ) {
		GPU_context_orphans_clear ( ctx ) ;
		ctx->wx_canvas->SetCurrent ( *ctx->wx_context ) ;
	}
	current_ctx = ctx ;
}

static void orphans_add ( GPU_Context *ctx , std::vector<GLuint> *orphan_list , GLuint id ) {
	printf ( "Orphan buffer has not been deleted.\n" ) ;
	orphan_list->push_back ( id ) ;
}

GLuint GPU_vao_default ( void ) {
	assert ( GPU_context_active_get ( ) );
	return GPU_context_active_get ( )->default_vao ;
}

GLuint GPU_framebuffer_default ( void ) {
	assert ( GPU_context_active_get ( ) );
	return GPU_context_active_get ( )->default_framebuffer ;
}

GLuint GPU_vao_alloc ( void ) {
	GLuint new_vao_id = 0;
	GPU_context_orphans_clear ( GPU_context_active_get ( ) ) ;
	glGenVertexArrays ( 1 , &new_vao_id );
	return new_vao_id;
}

GLuint GPU_fbo_alloc ( void ) {
	GLuint new_fbo_id = 0;
	GPU_context_orphans_clear ( GPU_context_active_get ( ) ) ;
	glGenFramebuffers ( 1 , &new_fbo_id );
	return new_fbo_id;
}

GLuint GPU_buf_alloc ( void ) {
	GLuint new_buffer_id = 0;
	GPU_context_orphans_clear ( GPU_context_active_get ( ) ) ;
	glGenBuffers ( 1 , &new_buffer_id );
	return new_buffer_id;
}

GLuint GPU_tex_alloc ( void ) {
	GLuint new_texture_id = 0;
	GPU_context_orphans_clear ( GPU_context_active_get ( ) ) ;
	glGenTextures ( 1 , &new_texture_id );
	return new_texture_id;
}

void GPU_vao_free ( GLuint vao_id , GPU_Context *ctx ) {
	assert ( ctx );
	if ( ctx == GPU_context_active_get ( ) ) {
		glDeleteVertexArrays ( 1 , &vao_id );
	}
	else {
		orphans_add ( ctx , &ctx->orphaned_vertarray_ids , vao_id );
	}
}

void GPU_fbo_free ( GLuint fbo_id , GPU_Context *ctx ) {
	assert ( ctx );
	if ( ctx == GPU_context_active_get ( ) ) {
		glDeleteFramebuffers ( 1 , &fbo_id );
	}
	else {
		orphans_add ( ctx , &ctx->orphaned_framebuffer_ids , fbo_id );
	}
}

void GPU_buf_free ( GLuint buf_id ) {
	if ( GPU_context_active_get ( ) ) {
		glDeleteBuffers ( 1 , &buf_id );
	}
	else {
		orphans_add ( NULL , &orphaned_buffer_ids , buf_id );
	}
}

void GPU_tex_free ( GLuint tex_id ) {
	if ( GPU_context_active_get ( ) ) {
		glDeleteTextures ( 1 , &tex_id );
	}
	else {
		orphans_add ( NULL , &orphaned_texture_ids , tex_id );
	}
}

void gpu_context_active_framebuffer_set ( GPU_Context *ctx , GPU_Framebuffer *fb ) {
	ctx->current_fbo = fb;
}

GPU_Framebuffer *gpu_context_active_framebuffer_get ( GPU_Context *ctx ) {
	return ctx->current_fbo;
}

struct GPU_MatrixState *gpu_context_active_matrix_state_get ( ) {
	return current_ctx->matrix_state;
}

void gpu_context_add_batch ( GPU_Context *ctx , GPU_Batch *batch ) {
	assert ( ctx );
	ctx->batches.emplace ( batch );
}

void gpu_context_remove_batch ( GPU_Context *ctx , GPU_Batch *batch ) {
	assert ( ctx );
	ctx->batches.erase ( batch );
}
