#pragma once

/* These map directly to the GL_ blend functions, to minimize API add as needed*/
typedef enum eGPUBlendFunction {
	GPU_ONE ,
	GPU_SRC_ALPHA ,
	GPU_ONE_MINUS_SRC_ALPHA ,
	GPU_DST_COLOR ,
	GPU_ZERO ,
} eGPUBlendFunction;

/* These map directly to the GL_ filter functions, to minimize API add as needed*/
typedef enum eGPUFilterFunction {
	GPU_NEAREST ,
	GPU_LINEAR ,
} eGPUFilterFunction;

void GPU_blend ( bool enable );
void GPU_blend_set_func ( eGPUBlendFunction sfactor , eGPUBlendFunction dfactor );
void GPU_blend_set_func_separate ( eGPUBlendFunction src_rgb , eGPUBlendFunction dst_rgb , eGPUBlendFunction src_alpha , eGPUBlendFunction dst_alpha );
void GPU_depth_range ( float near , float far );
void GPU_depth_test ( bool enable );
bool GPU_depth_test_enabled ( void );
void GPU_line_smooth ( bool enable );
void GPU_line_width ( float width );
void GPU_point_size ( float size );
void GPU_polygon_smooth ( bool enable );
void GPU_program_point_size ( bool enable );
void GPU_scissor ( int x , int y , int width , int height );
void GPU_scissor_get_f ( float coords [ 4 ] );
void GPU_scissor_get_i ( int coords [ 4 ] );
void GPU_viewport_size_get_f ( float coords [ 4 ] );
void GPU_viewport_size_get_i ( int coords [ 4 ] );

void GPU_flush ( void );
void GPU_finish ( void );

void GPU_logic_op_invert_set ( bool enable );
