#pragma once

#include "gpu_init.h"

struct GPU_Texture ;
struct GPU_Context ;

typedef struct GPUAttachment {
	GPU_Texture *tex;
	int mip , layer;
} GPUAttachment;

typedef enum eGPUFrameBufferBits {
	GPU_COLOR_BIT = ( 1 << 0 ) ,
	GPU_DEPTH_BIT = ( 1 << 1 ) ,
	GPU_STENCIL_BIT = ( 1 << 2 ) ,
} eGPUFrameBufferBits;

typedef enum {
	GPU_FB_DEPTH_ATTACHMENT = 0 ,
	GPU_FB_DEPTH_STENCIL_ATTACHMENT ,
	GPU_FB_COLOR_ATTACHMENT0 ,
	GPU_FB_COLOR_ATTACHMENT1 ,
	GPU_FB_COLOR_ATTACHMENT2 ,
	GPU_FB_COLOR_ATTACHMENT3 ,
	GPU_FB_COLOR_ATTACHMENT4 ,
	GPU_FB_COLOR_ATTACHMENT5 ,
	GPU_FB_MAX_ATTACHEMENT ,
} GPUAttachmentType;

struct GPU_Framebuffer {
	GPU_Context *ctx ;
	GLuint object ;

	GPUAttachment attachments [ GPU_FB_MAX_ATTACHEMENT ] ;

	unsigned int dirty_flag ;
	int width , height ;
	bool multisample ;
};

GPU_Framebuffer *GPU_framebuffer_create ( void ) ;
void GPU_framebuffer_free ( GPU_Framebuffer *fb ) ;
void GPU_framebuffer_bind ( GPU_Framebuffer *fb ) ;
void GPU_framebuffer_restore ( void ) ;

bool GPU_framebuffer_bound ( GPU_Framebuffer *fb ) ;
bool GPU_framebuffer_check_valid ( GPU_Framebuffer *fb , char *err_out ) ;

GPU_Framebuffer *GPU_framebuffer_active_get ( void ) ;

#define GPU_FRAMEBUFFER_FREE_SAFE(fb) if ( fb ) GPU_framebuffer_free ( fb )

void GPU_framebuffer_texture_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int mip ) ;
void GPU_framebuffer_texture_layer_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int layer , int mip ) ;
void GPU_framebuffer_texture_cubeface_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int face , int mip ) ;
void GPU_framebuffer_texture_detach ( GPU_Framebuffer *fb , GPU_Texture *tex ) ;
void GPU_framebuffer_texture_detach_slot ( GPU_Framebuffer *fb , GPU_Texture *tex , int type ) ;

/**
 * How to use #GPU_framebuffer_ensure_config ( ).
 * 
 * Example:
 * \code { .c }
 * GPU_framebuffer_ensure_config ( &fb , {
 *	GPU_ATTACHMENT_TEXTURE ( depth ), // must be depth buffer
 *	GPU_ATTACHMENT_TEXTURE ( tex1 ),
 *	GPU_ATTACHMENT_TEXTURE_CUBEFACE ( tex2, 0 ),
 *	GPU_ATTACHMENT_TEXTURE_LAYER_MIP ( tex2, 0, 0 )
 *  } )
 * \encode
 *
 * \note Unspecified attachments ( i.e: those beyond the last
 * GPU_ATTACHMENT_* in GPU_framebuffer_ensure_config list ) are left unchanged.
 * 
 * \note Make sure that the dimensions of your textures matches
 * otherwise you will have an invalid framebuffer error.
 */

void GPU_framebuffer_config_array ( GPU_Framebuffer * fb , const GPUAttachment * config , int config_len );

#define GPU_ATTACHMENT_NONE \
  { \
    .tex = NULL, .layer = -1, .mip = 0, \
  }
#define GPU_ATTACHMENT_LEAVE \
  { \
    .tex = NULL, .layer = -1, .mip = -1, \
  }
#define GPU_ATTACHMENT_TEXTURE(_tex) \
  { \
    .tex = _tex, .layer = -1, .mip = 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_MIP(_tex, _mip) \
  { \
    .tex = _tex, .layer = -1, .mip = _mip, \
  }
#define GPU_ATTACHMENT_TEXTURE_LAYER(_tex, _layer) \
  { \
    .tex = _tex, .layer = _layer, .mip = 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_LAYER_MIP(_tex, _layer, _mip) \
  { \
    .tex = _tex, .layer = _layer, .mip = _mip, \
  }
#define GPU_ATTACHMENT_TEXTURE_CUBEFACE(_tex, _face) \
  { \
    .tex = _tex, .layer = _face, .mip = 0, \
  }
#define GPU_ATTACHMENT_TEXTURE_CUBEFACE_MIP(_tex, _face, _mip) \
  { \
    .tex = _tex, .layer = _face, .mip = _mip, \
  }

/* Framebuffer operations */

void GPU_framebuffer_viewport_set ( GPU_Framebuffer *fb , int x , int y , int w , int h );

void GPU_framebuffer_clear ( GPU_Framebuffer *fb , eGPUFrameBufferBits buffers , const float clear_col [ 4 ] , float clear_depth , unsigned int clear_stencil );

#define GPU_framebuffer_clear_color(fb, col) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT, col, 0.0f, 0x00)

#define GPU_framebuffer_clear_depth(fb, depth) \
  GPU_framebuffer_clear(fb, GPU_DEPTH_BIT, NULL, depth, 0x00)

#define GPU_framebuffer_clear_color_depth(fb, col, depth) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT | GPU_DEPTH_BIT, col, depth, 0x00)

#define GPU_framebuffer_clear_stencil(fb, stencil) \
  GPU_framebuffer_clear(fb, GPU_STENCIL_BIT, NULL, 0.0f, stencil)

#define GPU_framebuffer_clear_depth_stencil(fb, depth, stencil) \
  GPU_framebuffer_clear(fb, GPU_DEPTH_BIT | GPU_STENCIL_BIT, NULL, depth, stencil)

#define GPU_framebuffer_clear_color_depth_stencil(fb, col, depth, stencil) \
  GPU_framebuffer_clear(fb, GPU_COLOR_BIT | GPU_DEPTH_BIT | GPU_STENCIL_BIT, col, depth, stencil)

void GPU_framebuffer_read_depth ( GPU_Framebuffer *fb , int x , int y , int w , int h , float *data );
void GPU_framebuffer_read_color ( GPU_Framebuffer *fb , int x , int y , int w , int h , int channels , int slot , float *data );

void GPU_framebuffer_blit ( GPU_Framebuffer *fb_read , int read_slot , GPU_Framebuffer *fb_write , int write_slot , eGPUFrameBufferBits blit_buffers );

void GPU_framebuffer_recursive_downsample ( GPU_Framebuffer *fb , int max_lvl , void ( *callback )( void *userData , int level ) , void *userData );
