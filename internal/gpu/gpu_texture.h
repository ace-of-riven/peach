#pragma once

#include "gpu_init.h"
#include "gpu_state.h"

struct GPU_Framebuffer ;

typedef enum eGPUTextureFormat {
	GPU_RGBA8UI ,
	GPU_RGBA8I ,
	GPU_RGBA8 ,
	GPU_RGBA32UI ,
	GPU_RGBA32I ,
	GPU_RGBA32F ,
	GPU_RGBA16UI ,
	GPU_RGBA16I ,
	GPU_RGBA16F ,
	GPU_RGBA16 ,
	GPU_RG8UI ,
	GPU_RG8I ,
	GPU_RG8 ,
	GPU_RG32UI ,
	GPU_RG32I ,
	GPU_RG32F ,
	GPU_RG16UI ,
	GPU_RG16I ,
	GPU_RG16F ,
	GPU_RG16 ,
	GPU_R8UI ,
	GPU_R8I ,
	GPU_R8 ,
	GPU_R32UI ,
	GPU_R32I ,
	GPU_R32F ,
	GPU_R16UI ,
	GPU_R16I ,
	GPU_R16F ,
	GPU_R16 , /* Max texture buffer format. */

	/* Special formats texture & renderbuffer */
#if 0
	GPU_RGB10_A2 ,
	GPU_RGB10_A2UI ,
#endif
	GPU_R11F_G11F_B10F ,
	GPU_DEPTH32F_STENCIL8 ,
	GPU_DEPTH24_STENCIL8 ,

	/* Texture only format */
	GPU_RGB16F ,
#if 0
	GPU_RGBA16_SNORM ,
	GPU_RGBA8_SNORM ,
	GPU_RGB32F ,
	GPU_RGB32I ,
	GPU_RGB32UI ,
	GPU_RGB16_SNORM ,
	GPU_RGB16I ,
	GPU_RGB16UI ,
	GPU_RGB16 ,
	GPU_RGB8_SNORM ,
	GPU_RGB8 ,
	GPU_RGB8I ,
	GPU_RGB8UI ,
	GPU_RG16_SNORM ,
	GPU_RG8_SNORM ,
	GPU_R16_SNORM ,
	GPU_R8_SNORM ,
#endif

	/* Special formats texture only */
#if 0
	GPU_SRGB8_A8 ,
	GPU_SRGB8 ,
	GPU_RGB9_E5 ,
	GPU_COMPRESSED_RG_RGTC2 ,
	GPU_COMPRESSED_SIGNED_RG_RGTC2 ,
	GPU_COMPRESSED_RED_RGTC1 ,
	GPU_COMPRESSED_SIGNED_RED_RGTC1 ,
#endif

	/* Depth Formats */
	GPU_DEPTH_COMPONENT32F ,
	GPU_DEPTH_COMPONENT24 ,
	GPU_DEPTH_COMPONENT16 ,
	GPU_INVALID_TEX_FORMAT
} eGPUTextureFormat;

typedef enum eGPUDataFormat {
	GPU_DATA_FLOAT ,
	GPU_DATA_INT ,
	GPU_DATA_UNSIGNED_INT ,
	GPU_DATA_UNSIGNED_BYTE ,
	GPU_DATA_UNSIGNED_INT_24_8 ,
	GPU_DATA_10_11_11_REV ,
} eGPUDataFormat;

typedef enum eGPUTextureFormatFlag {
	GPU_FORMAT_DEPTH = ( 1 << 0 ) ,
	GPU_FORMAT_STENCIL = ( 1 << 1 ) ,
	GPU_FORMAT_INTEGER = ( 1 << 2 ) ,
	GPU_FORMAT_FLOAT = ( 1 << 3 ) ,

	GPU_FORMAT_1D = ( 1 << 10 ) ,
	GPU_FORMAT_2D = ( 1 << 11 ) ,
	GPU_FORMAT_3D = ( 1 << 12 ) ,
	GPU_FORMAT_CUBE = ( 1 << 13 ) ,
	GPU_FORMAT_ARRAY = ( 1 << 14 ) ,
} eGPUTextureFormatFlag;

/* Maximum number of FBOs a texture can be attached to. */
#define GPU_TEX_MAX_FBO_ATTACHED 10

struct GPU_Texture {
	int w , h , d ;				// width / height / depth
	int orig_w , orig_h ;			// width / height ( of source data ), optional
	int number ;				// number for multitexture binding
	int refcount ;				// reference count
	GLenum target ;				// GL_TEXTURE_*
	GLenum target_base ;			// same as target, ( but no multisample ) use it for unbinding
	
	GLuint bindcode ;			// opengl identifier for texture

	eGPUTextureFormat format ;
	long long format_flag ;

	unsigned int bytesize ;			// number of byte for one pixel
	int components ;			// number of color / alpha channels
	int samples ;				// number of samples for multisamples textures. 0 if not multisample target

	int fb_attachment [ GPU_TEX_MAX_FBO_ATTACHED ] ;
	GPU_Framebuffer *fb [ GPU_TEX_MAX_FBO_ATTACHED ] ;
};

GPU_Texture *GPU_texture_create_nD ( int w , int h , int d , int n , const void *pixels , eGPUTextureFormat tex_format , eGPUDataFormat gpu_data_format , int samples , const bool can_rescale ) ;
GPU_Texture *GPU_texture_create_1d ( int w , eGPUTextureFormat data_type , const float *pixels ) ;
GPU_Texture *GPU_texture_create_1d_array ( int w , int h , eGPUTextureFormat data_type , const float *pixel ) ;
GPU_Texture *GPU_texture_create_2d ( int w , int h , eGPUTextureFormat data_type , const float *pixel ) ;
GPU_Texture *GPU_texture_create_2d_multisample ( int w , int h , eGPUTextureFormat data_type , const float *pixel , int samples ) ;

GPU_Texture *GPU_texture_create_2d_array ( int w , int h , int d , eGPUTextureFormat data_type , const float *pixel ) ;
GPU_Texture *GPU_texture_create_3d ( int w , int h , int d , eGPUTextureFormat data_type , const float *pixel ) ;

GPU_Texture *GPU_texture_create_cube ( int w , eGPUTextureFormat data_type , const float *pixels ) ;

GPU_Texture *GPU_texture_from_bindcode ( int textarget , int bindcode );

void GPU_texture_add_mipmap ( GPU_Texture *tex , eGPUDataFormat gpu_data_format , int miplvl , const void *pixels ) ;
void GPU_texture_update ( GPU_Texture *tex , eGPUDataFormat data_format , const void *pixels ) ;
void GPU_texture_update_sub ( GPU_Texture *tex , eGPUDataFormat gpu_data_format , const void *pixels , int offset_x , int offset_y , int offset_z , int width , int height , int depth ) ;

void *GPU_texture_read ( GPU_Texture *tex , eGPUDataFormat gpu_data_format , int miplvl ) ;

void GPU_invalid_tex_init ( void ) ;
void GPU_invalid_tex_bind ( int mode ) ;
void GPU_invalid_tex_free ( void ) ;

void GPU_texture_free ( GPU_Texture *tex ) ;

void GPU_texture_ref ( GPU_Texture *tex ) ;
void GPU_texture_bind ( GPU_Texture *tex , int number ) ;
void GPU_texture_unbind ( GPU_Texture *tex ) ;
int GPU_texture_bound_number ( GPU_Texture *tex ) ;

void GPU_texture_generate_mipmap ( GPU_Texture *tex );
void GPU_texture_compare_mode ( GPU_Texture *tex , bool use_compare );
void GPU_texture_filter_mode ( GPU_Texture *tex , bool use_filter );
void GPU_texture_mipmap_mode ( GPU_Texture *tex , bool use_mipmap , bool use_filter );
void GPU_texture_wrap_mode ( GPU_Texture *tex , bool use_repeat );
void GPU_texture_filters ( GPU_Texture *tex , eGPUFilterFunction min_filter , eGPUFilterFunction mag_filter );

int GPU_texture_target ( const GPU_Texture *tex );
int GPU_texture_width ( const GPU_Texture *tex );
int GPU_texture_height ( const GPU_Texture *tex );
int GPU_texture_orig_width ( const GPU_Texture *tex );
int GPU_texture_orig_height ( const GPU_Texture *tex );
void GPU_texture_orig_size_set ( GPU_Texture *tex , int w , int h );
int GPU_texture_layers ( const GPU_Texture *tex );
eGPUTextureFormat GPU_texture_format ( const GPU_Texture *tex );
int GPU_texture_samples ( const GPU_Texture *tex );
bool GPU_texture_cube ( const GPU_Texture *tex );
bool GPU_texture_depth ( const GPU_Texture *tex );
bool GPU_texture_stencil ( const GPU_Texture *tex );
bool GPU_texture_integer ( const GPU_Texture *tex );
int GPU_texture_opengl_bindcode ( const GPU_Texture *tex );

void GPU_texture_get_mipmap_size ( GPU_Texture *tex , int lvl , int *size );

void GPU_texture_attach_framebuffer ( GPU_Texture *tex , GPU_Framebuffer *fb , int attachment ) ;
int GPU_texture_detach_framebuffer ( GPU_Texture *tex , GPU_Framebuffer *fb ) ;

GPU_Texture *GPU_texture_get_invalid ( int dimensions ) ;
GPU_Texture *GPU_texture_get_empty ( int dimensions ) ;
