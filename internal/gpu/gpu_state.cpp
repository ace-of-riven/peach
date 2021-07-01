#include "gpu_init.h"
#include "gpu_state.h"

#include <cassert>

static GLenum gpu_get_gl_blendfunction ( eGPUBlendFunction blend )
{
	switch ( blend ) {
		case GPU_ONE:
			return GL_ONE;
		case GPU_SRC_ALPHA:
			return GL_SRC_ALPHA;
		case GPU_ONE_MINUS_SRC_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;
		case GPU_DST_COLOR:
			return GL_DST_COLOR;
		case GPU_ZERO:
			return GL_ZERO;
		default:
			fprintf ( stderr , "Unhandled blend mode\n" );
			return GL_ZERO;
	}
}

void GPU_blend ( bool enable ) {
	if ( enable ) glEnable ( GL_BLEND ) ;
	else glDisable ( GL_BLEND ) ;
}

void GPU_blend_set_func ( eGPUBlendFunction sfactor , eGPUBlendFunction dfactor ) {
	glBlendFunc ( gpu_get_gl_blendfunction ( sfactor ) , gpu_get_gl_blendfunction ( dfactor ) ) ;
}

void GPU_blend_set_func_separate ( eGPUBlendFunction src_rgb , eGPUBlendFunction dst_rgb , eGPUBlendFunction src_alpha , eGPUBlendFunction dst_alpha ) {
	glBlendFuncSeparate ( gpu_get_gl_blendfunction ( src_rgb ) ,
		gpu_get_gl_blendfunction ( dst_rgb ) ,
		gpu_get_gl_blendfunction ( src_alpha ) ,
		gpu_get_gl_blendfunction ( dst_alpha ) );
}

void GPU_depth_range ( float near , float far ) {
	glDepthRange ( near , far ) ;
}

void GPU_depth_test ( bool enable ) {
	if ( enable ) glEnable ( GL_DEPTH_TEST ) ;
	else glDisable ( GL_DEPTH_TEST ) ;
}

bool GPU_depth_test_enabled ( ) {
	return glIsEnabled ( GL_DEPTH_TEST );
}

void GPU_line_smooth ( bool enable ) {
	if ( enable ) glEnable ( GL_LINE_SMOOTH ) ;
	else glDisable ( GL_LINE_SMOOTH ) ;
}

void GPU_line_width ( float width ) {
	glLineWidth ( width ) ;
}

void GPU_point_size ( float size ) {
	glPointSize ( size ) ;
}

void GPU_polygon_smooth ( bool enable ) {
	if ( enable ) glEnable ( GL_POLYGON_SMOOTH ) ;
	else glDisable ( GL_POLYGON_SMOOTH ) ;
}

void GPU_scissor ( int x , int y , int width , int height ) {
	glScissor ( x , y , width , height );
}

void GPU_scissor_get_f ( float coords [ 4 ] ) {
	glGetFloatv ( GL_SCISSOR_BOX , coords );
}

void GPU_scissor_get_i ( int coords [ 4 ] ) {
	glGetIntegerv ( GL_SCISSOR_BOX , coords );
}

void GPU_viewport_size_get_f ( float coords [ 4 ] ) {
	glGetFloatv ( GL_VIEWPORT , coords );
}

void GPU_viewport_size_get_i ( int coords [ 4 ] ) {
	glGetIntegerv ( GL_VIEWPORT , coords );
}

void GPU_flush ( void ) {
	glFlush ( );
}

void GPU_finish ( void ) {
	glFinish ( );
}

void GPU_logic_op_invert_set ( bool enable ) {
	if ( enable ) {
		glLogicOp ( GL_INVERT );
		glEnable ( GL_COLOR_LOGIC_OP );
		glDisable ( GL_DITHER );
	}
	else {
		glLogicOp ( GL_COPY );
		glDisable ( GL_COLOR_LOGIC_OP );
		glEnable ( GL_DITHER );
	}
}
