#include "gpu_texture.h"
#include "gpu_framebuffer.h"
#include "gpu_context.h"

#include <cassert>
#include <algorithm>

struct GPUTextureGlobal {
	GPU_Texture *invalid_tex_1D ;
	GPU_Texture *invalid_tex_2D ;
	GPU_Texture *invalid_tex_3D ;

	GPU_Texture *empty_tex_1D ;
	GPU_Texture *empty_tex_2D ;
	GPU_Texture *empty_tex_3D ;
} GPUTextureGlobal ;

static unsigned int gpu_texture_memory_footprint_compute ( GPU_Texture *tex ) {
	int samp = std::max ( tex->samples , 1 ) ;
	switch ( tex->target_base ) {
		case GL_TEXTURE_1D:
			return tex->bytesize * tex->w * samp ;
		case GL_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_2D:
			return tex->bytesize * tex->w * tex->h * samp ;
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
			return tex->bytesize * tex->w * tex->h * tex->d * samp;
		case GL_TEXTURE_CUBE_MAP:
			return tex->bytesize * 6 * tex->w * tex->h * samp;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return tex->bytesize * 6 * tex->w * tex->h * tex->d * samp;
		default:
			return 0;
	}
}

static const char *gl_enum_to_str ( GLenum e ) {
	switch ( e ) {
		case GL_TEXTURE_CUBE_MAP: return "TEXTURE_CUBE_MAP" ;
		case GL_TEXTURE_2D: return "TEXTURE_2D" ;
		case GL_TEXTURE_2D_ARRAY: return "TEXTURE_2D_ARRAY" ;
		case GL_TEXTURE_1D: return "TEXTURE_1D" ;
		case GL_TEXTURE_1D_ARRAY: return "TEXTURE_1D_ARRAY" ;
		case GL_TEXTURE_3D: return "TEXTURE_3D" ;
		case GL_TEXTURE_2D_MULTISAMPLE: return "TEXTURE_2D_MULTISAMPLE" ;
		case GL_RGBA32F: return "RGBA32F" ;
		case GL_RGBA16F: return "RGBA16F" ;
		case GL_RGBA16: return "RGBA16" ;
		case GL_RG32F: return "RG32F" ;
		case GL_RGB16F: return "RGB16F" ;
		case GL_RG16F: return "RG16F" ;
		case GL_RG16I: return "RG16I" ;
		case GL_RG16: return "RG16" ;
		case GL_RGBA8: return "RGBA8" ;
		case GL_RGBA8UI: return "RGBA8UI" ;
		case GL_R32F: return "R32F" ;
		case GL_R32UI: return "R32UI" ;
		case GL_R32I: return "R32I" ;
		case GL_R16F: return "R16F" ;
		case GL_R16I: return "R16I" ;
		case GL_R16UI: return "R16UI" ;
		case GL_RG8: return "RG8" ;
		case GL_RG16UI: return "RG16UI" ;
		case GL_R16: return "R16" ;
		case GL_R8: return "R8" ;
		case GL_R8UI: return "R8UI" ;
		case GL_R11F_G11F_B10F: return "R11F_G11F_B10F" ;
		case GL_DEPTH24_STENCIL8: return "DEPTH24_STENCIL8" ;
		case GL_DEPTH32F_STENCIL8: return "DEPTH32F_STENCIL8" ;
		case GL_DEPTH_COMPONENT32F: return "DEPTH_COMPONENT32F" ;
		case GL_DEPTH_COMPONENT24: return "DEPTH_COMPONENT24" ;
		case GL_DEPTH_COMPONENT16: return "DEPTH_COMPONENT16" ;
	};
	return "INVALID_TEXTURE_ENUM" ;
}

static int gpu_get_component_count ( eGPUTextureFormat format ) {
	switch ( format ) {
		case GPU_RGBA8:
		case GPU_RGBA8UI:
		case GPU_RGBA16F:
		case GPU_RGBA16:
		case GPU_RGBA32F:
			return 4;
		case GPU_RGB16F:
		case GPU_R11F_G11F_B10F:
			return 3;
		case GPU_RG8:
		case GPU_RG16:
		case GPU_RG16F:
		case GPU_RG16I:
		case GPU_RG16UI:
		case GPU_RG32F:
			return 2;
		default:
			return 1;
	}
}

/* Definitely not complete, edit according to the gl specification. */
static void gpu_validate_data_format ( eGPUTextureFormat tex_format , eGPUDataFormat data_format ) {
	( void ) data_format ;

	if ( ELEM ( tex_format , GPU_DEPTH_COMPONENT24 , GPU_DEPTH_COMPONENT16 , GPU_DEPTH_COMPONENT32F ) ) {
		assert ( data_format == GPU_DATA_FLOAT );
	}
	else if ( ELEM ( tex_format , GPU_DEPTH24_STENCIL8 , GPU_DEPTH32F_STENCIL8 ) ) {
		assert ( data_format == GPU_DATA_UNSIGNED_INT_24_8 );
	}
	else {
		/* Integer formats */
		if ( ELEM ( tex_format , GPU_RG16I , GPU_R16I , GPU_RG16UI , GPU_R16UI , GPU_R8UI , GPU_R32UI ) ) {
			if ( ELEM ( tex_format , GPU_R8UI , GPU_R16UI , GPU_RG16UI , GPU_R32UI ) ) {
				assert ( data_format == GPU_DATA_UNSIGNED_INT );
			}
			else {
				assert ( data_format == GPU_DATA_INT );
			}
		}
		/* Byte formats */
		else if ( ELEM ( tex_format , GPU_R8 , GPU_RG8 , GPU_RGBA8 , GPU_RGBA8UI ) ) {
			assert ( ELEM ( data_format , GPU_DATA_UNSIGNED_BYTE , GPU_DATA_FLOAT ) );
		}
		/* Special case */
		else if ( ELEM ( tex_format , GPU_R11F_G11F_B10F ) ) {
			assert ( ELEM ( data_format , GPU_DATA_10_11_11_REV , GPU_DATA_FLOAT ) );
		}
		/* Float formats */
		else {
			assert ( ELEM ( data_format , GPU_DATA_FLOAT ) );
		}
	}
}

static eGPUDataFormat gpu_get_data_format_from_tex_format ( eGPUTextureFormat tex_format ) {
	if ( ELEM ( tex_format , GPU_DEPTH_COMPONENT24 , GPU_DEPTH_COMPONENT16 , GPU_DEPTH_COMPONENT32F ) ) {
		return GPU_DATA_FLOAT;
	}
	else if ( ELEM ( tex_format , GPU_DEPTH24_STENCIL8 , GPU_DEPTH32F_STENCIL8 ) ) {
		return GPU_DATA_UNSIGNED_INT_24_8;
	}
	else {
		/* Integer formats */
		if ( ELEM ( tex_format , GPU_RG16I , GPU_R16I , GPU_RG16UI , GPU_R8UI , GPU_R16UI , GPU_R32UI ) ) {
			if ( ELEM ( tex_format , GPU_R8UI , GPU_R16UI , GPU_RG16UI , GPU_R32UI ) ) {
				return GPU_DATA_UNSIGNED_INT;
			}
			else {
				return GPU_DATA_INT;
			}
		}
		/* Byte formats */
		else if ( ELEM ( tex_format , GPU_R8 ) ) {
			return GPU_DATA_UNSIGNED_BYTE;
		}
		/* Special case */
		else if ( ELEM ( tex_format , GPU_R11F_G11F_B10F ) ) {
			return GPU_DATA_10_11_11_REV;
		}
		else {
			return GPU_DATA_FLOAT;
		}
	}
}

/* Definitely not complete, edit according to the gl specification. */
static GLenum gpu_get_gl_dataformat ( eGPUTextureFormat data_type , long long *format_flag ) {
	if ( ELEM ( data_type , GPU_DEPTH_COMPONENT24 , GPU_DEPTH_COMPONENT16 , GPU_DEPTH_COMPONENT32F ) ) {
		*format_flag |= GPU_FORMAT_DEPTH;
		return GL_DEPTH_COMPONENT;
	}
	else if ( ELEM ( data_type , GPU_DEPTH24_STENCIL8 , GPU_DEPTH32F_STENCIL8 ) ) {
		*format_flag |= GPU_FORMAT_DEPTH | GPU_FORMAT_STENCIL;
		return GL_DEPTH_STENCIL;
	}
	else {
		/* Integer formats */
		if ( ELEM ( data_type , GPU_R8UI , GPU_RG16I , GPU_R16I , GPU_RG16UI , GPU_R16UI , GPU_R32UI ) ) {
			*format_flag |= GPU_FORMAT_INTEGER;

			switch ( gpu_get_component_count ( data_type ) ) {
				case 1:
					return GL_RED_INTEGER;
					break;
				case 2:
					return GL_RG_INTEGER;
					break;
				case 3:
					return GL_RGB_INTEGER;
					break;
				case 4:
					return GL_RGBA_INTEGER;
					break;
			}
		}
		else if ( ELEM ( data_type , GPU_R8 ) ) {
			*format_flag |= GPU_FORMAT_FLOAT;
			return GL_RED;
		}
		else {
			*format_flag |= GPU_FORMAT_FLOAT;

			switch ( gpu_get_component_count ( data_type ) ) {
				case 1:
					return GL_RED;
					break;
				case 2:
					return GL_RG;
					break;
				case 3:
					return GL_RGB;
					break;
				case 4:
					return GL_RGBA;
					break;
			}
		}
	}

	*format_flag |= GPU_FORMAT_FLOAT;
	return GL_RGBA;
}

static unsigned int gpu_get_bytesize ( eGPUTextureFormat data_type ) {
	switch ( data_type ) {
		case GPU_RGBA32F:
			return 32;
		case GPU_RG32F:
		case GPU_RGBA16F:
		case GPU_RGBA16:
			return 16;
		case GPU_RGB16F:
			return 12;
		case GPU_DEPTH32F_STENCIL8:
			return 8;
		case GPU_RG16F:
		case GPU_RG16I:
		case GPU_RG16UI:
		case GPU_RG16:
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH_COMPONENT32F:
		case GPU_RGBA8UI:
		case GPU_RGBA8:
		case GPU_R11F_G11F_B10F:
		case GPU_R32F:
		case GPU_R32UI:
		case GPU_R32I:
			return 4;
		case GPU_DEPTH_COMPONENT24:
			return 3;
		case GPU_DEPTH_COMPONENT16:
		case GPU_R16F:
		case GPU_R16UI:
		case GPU_R16I:
		case GPU_RG8:
		case GPU_R16:
			return 2;
		case GPU_R8:
		case GPU_R8UI:
			return 1;
		default:
			assert ( !"Texture format incorrect or unsupported\n" );
			return 0;
	}
}

static GLenum gpu_get_gl_internalformat ( eGPUTextureFormat format ) {
	/* You can add any of the available type to this list
	 * For available types see GPU_texture.h */
	switch ( format ) {
		/* Formats texture & renderbuffer */
		case GPU_RGBA32F:
			return GL_RGBA32F;
		case GPU_RGBA16F:
			return GL_RGBA16F;
		case GPU_RGBA16:
			return GL_RGBA16;
		case GPU_RG32F:
			return GL_RG32F;
		case GPU_RGB16F:
			return GL_RGB16F;
		case GPU_RG16F:
			return GL_RG16F;
		case GPU_RG16I:
			return GL_RG16I;
		case GPU_RG16:
			return GL_RG16;
		case GPU_RGBA8:
			return GL_RGBA8;
		case GPU_RGBA8UI:
			return GL_RGBA8UI;
		case GPU_R32F:
			return GL_R32F;
		case GPU_R32UI:
			return GL_R32UI;
		case GPU_R32I:
			return GL_R32I;
		case GPU_R16F:
			return GL_R16F;
		case GPU_R16I:
			return GL_R16I;
		case GPU_R16UI:
			return GL_R16UI;
		case GPU_RG8:
			return GL_RG8;
		case GPU_RG16UI:
			return GL_RG16UI;
		case GPU_R16:
			return GL_R16;
		case GPU_R8:
			return GL_R8;
		case GPU_R8UI:
			return GL_R8UI;
			/* Special formats texture & renderbuffer */
		case GPU_R11F_G11F_B10F:
			return GL_R11F_G11F_B10F;
		case GPU_DEPTH24_STENCIL8:
			return GL_DEPTH24_STENCIL8;
		case GPU_DEPTH32F_STENCIL8:
			return GL_DEPTH32F_STENCIL8;
			/* Texture only format */
			/* ** Add Format here */
			/* Special formats texture only */
			/* ** Add Format here */
			/* Depth Formats */
		case GPU_DEPTH_COMPONENT32F:
			return GL_DEPTH_COMPONENT32F;
		case GPU_DEPTH_COMPONENT24:
			return GL_DEPTH_COMPONENT24;
		case GPU_DEPTH_COMPONENT16:
			return GL_DEPTH_COMPONENT16;
		default:
			assert ( !"Texture format incorrect or unsupported\n" );
			return 0;
	}
}

static GLenum gpu_get_gl_datatype ( eGPUDataFormat format ) {
	switch ( format ) {
		case GPU_DATA_FLOAT:
			return GL_FLOAT;
		case GPU_DATA_INT:
			return GL_INT;
		case GPU_DATA_UNSIGNED_INT:
			return GL_UNSIGNED_INT;
		case GPU_DATA_UNSIGNED_BYTE:
			return GL_UNSIGNED_BYTE;
		case GPU_DATA_UNSIGNED_INT_24_8:
			return GL_UNSIGNED_INT_24_8;
		case GPU_DATA_10_11_11_REV:
			return GL_UNSIGNED_INT_10F_11F_11F_REV;
		default:
			assert ( !"Unhandled data format" );
			return GL_FLOAT;
	}
}

static float *GPU_texture_rescale_3d ( GPU_Texture *tex , int w , int h , int d , int channels , const float *fpixels ) {
	const unsigned int xf = w / tex->w , yf = h / tex->h , zf = d / tex->d;
	float *nfpixels = ( float * ) malloc ( channels * sizeof ( float ) * tex->w * tex->h * tex->d ) ;

	if ( nfpixels ) {
		fprintf ( stderr , "Scaling 3D texture!\n" );

		for ( unsigned int k = 0; k < tex->d; k++ ) {
			for ( unsigned int j = 0; j < tex->h; j++ ) {
				for ( unsigned int i = 0; i < tex->w; i++ ) {
					/* obviously doing nearest filtering here,
					 * it's going to be slow in any case, let's not make it worse */
					float xb = i * xf;
					float yb = j * yf;
					float zb = k * zf;
					unsigned int offset = k * ( tex->w * tex->h ) + i * tex->h + j;
					unsigned int offset_orig = ( zb ) * ( w * h ) + ( xb ) *h + ( yb );

					if ( channels == 4 ) {
						nfpixels [ offset * 4 ] = fpixels [ offset_orig * 4 ];
						nfpixels [ offset * 4 + 1 ] = fpixels [ offset_orig * 4 + 1 ];
						nfpixels [ offset * 4 + 2 ] = fpixels [ offset_orig * 4 + 2 ];
						nfpixels [ offset * 4 + 3 ] = fpixels [ offset_orig * 4 + 3 ];
					}
					else {
						nfpixels [ offset ] = fpixels [ offset_orig ];
					}
				}
			}
		}
	}

	return nfpixels;
}

// Implement gpu_texture_check_capacity /?
static bool gpu_texture_check_capacity ( GPU_Texture *tex , GLenum proxy , GLenum internalformat , GLenum data_format , GLenum data_type ) {
	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_WIN , GPU_DRIVER_ANY ) ||
		GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_UNIX , GPU_DRIVER_OFFICIAL ) ) {
		/* Some AMD drivers have a faulty `GL_PROXY_TEXTURE_..` check.
		 * (see T55888, T56185, T59351).
		 * Checking with `GL_PROXY_TEXTURE_..` doesn't prevent `Out Of Memory` issue,
		 * it just states that the OGL implementation can support the texture.
		 * So manually check the maximum size and maximum number of layers. */
		switch ( proxy ) {
			case GL_PROXY_TEXTURE_2D_ARRAY:
				if ( ( tex->d < 0 ) || ( tex->d > GPU_max_texture_layers ( ) ) ) {
					return false;
				}
				break;

			case GL_PROXY_TEXTURE_1D_ARRAY:
				if ( ( tex->h < 0 ) || ( tex->h > GPU_max_texture_layers ( ) ) ) {
					return false;
				}
				break;
		}

		switch ( proxy ) {
			case GL_PROXY_TEXTURE_3D:
				if ( ( tex->d < 0 ) || ( tex->d > GPU_max_texture_size ( ) ) ) {
					return false;
				}
				ATTR_FALLTHROUGH;

			case GL_PROXY_TEXTURE_2D:
			case GL_PROXY_TEXTURE_2D_ARRAY:
				if ( ( tex->h < 0 ) || ( tex->h > GPU_max_texture_size ( ) ) ) {
					return false;
				}
				ATTR_FALLTHROUGH;

			case GL_PROXY_TEXTURE_1D:
			case GL_PROXY_TEXTURE_1D_ARRAY:
				if ( ( tex->w < 0 ) || ( tex->w > GPU_max_texture_size ( ) ) ) {
					return false;
				}
				ATTR_FALLTHROUGH;
			default:
				break;
		}

		return true;
	}
	else {
		switch ( proxy ) {
			case GL_PROXY_TEXTURE_1D:
				glTexImage1D ( proxy , 0 , internalformat , tex->w , 0 , data_format , data_type , NULL );
				break;
			case GL_PROXY_TEXTURE_1D_ARRAY:
			case GL_PROXY_TEXTURE_2D:
				glTexImage2D ( proxy , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , NULL );
				break;
			case GL_PROXY_TEXTURE_2D_ARRAY:
			case GL_PROXY_TEXTURE_3D:
				glTexImage3D (
					proxy , 0 , internalformat , tex->w , tex->h , tex->d , 0 , data_format , data_type , NULL );
				break;
		}
		int width = 0;
		glGetTexLevelParameteriv ( proxy , 0 , GL_TEXTURE_WIDTH , &width );

		return ( width > 0 );
	}
}

/* This tries to allocate video memory for a given texture
 * if alloc fails, lower the resolution until it fits. */
static bool gpu_texture_try_alloc ( GPU_Texture *tex , GLenum proxy , GLenum internalformat , GLenum data_format , GLenum data_type , int channels , bool try_rescale , const float *fpixels , float **rescaled_fpixels ) {
	bool ret ;
	ret = gpu_texture_check_capacity ( tex , proxy , internalformat , data_format , data_type ) ;
	if ( !ret && try_rescale ) {
		assert ( !ELEM ( proxy , GL_PROXY_TEXTURE_1D_ARRAY , GL_PROXY_TEXTURE_2D_ARRAY ) );  // not implemented

		const int w = tex->w , h = tex->h , d = tex->d;

		/* Find largest texture possible */
		do {
			tex->w /= 2;
			tex->h /= 2;
			tex->d /= 2;

			/* really unlikely to happen but keep this just in case */
			if ( tex->w == 0 ) {
				break;
			}
			if ( tex->h == 0 && proxy != GL_PROXY_TEXTURE_1D ) {
				break;
			}
			if ( tex->d == 0 && proxy == GL_PROXY_TEXTURE_3D ) {
				break;
			}

			ret = gpu_texture_check_capacity ( tex , proxy , internalformat , data_format , data_type );
		} while ( ret == false );

		/* Rescale */
		if ( ret ) {
			switch ( proxy ) {
				case GL_PROXY_TEXTURE_1D:
				case GL_PROXY_TEXTURE_2D:
					/* Do nothing for now */
					return false;
				case GL_PROXY_TEXTURE_3D:
					assert ( data_type == GL_FLOAT );
					*rescaled_fpixels = GPU_texture_rescale_3d ( tex , w , h , d , channels , fpixels );
					return ( bool ) *rescaled_fpixels;
			}
		}
	}

	return ret;
}

GPU_Texture *GPU_texture_create_nD ( int w , int h , int d , int n , const void *pixels ,
	eGPUTextureFormat tex_format , eGPUDataFormat gpu_data_format , int samples , const bool can_rescale ) {
	if ( samples ) {
		samples = std::min ( samples , GPU_max_color_texture_samples ( ) ) ;
	}

	GPU_Texture *tex = new GPU_Texture ( ) ;
	tex->w = w;
	tex->h = h;
	tex->d = d;
	tex->samples = samples;
	tex->number = -1;
	tex->refcount = 1;
	tex->format = tex_format;
	tex->components = gpu_get_component_count ( tex_format );
	tex->bytesize = gpu_get_bytesize ( tex_format );
	tex->format_flag = 0;

	if ( n == 2 ) {
		if ( d == 0 )
			tex->target_base = tex->target = GL_TEXTURE_2D;
		else
			tex->target_base = tex->target = GL_TEXTURE_2D_ARRAY;
	}
	else if ( n == 1 ) {
		if ( h == 0 )
			tex->target_base = tex->target = GL_TEXTURE_1D;
		else
			tex->target_base = tex->target = GL_TEXTURE_1D_ARRAY;
	}
	else if ( n == 3 ) {
		tex->target_base = tex->target = GL_TEXTURE_3D;
	}
	else {
		/* should never happen */
		fprintf ( stderr , "Invalid GPU_Texture format cannot create %dD texture.\n" , n ) ;
		delete tex ;
		return NULL;
	}

	gpu_validate_data_format ( tex_format , gpu_data_format ) ;

	if ( samples && n == 2 && d == 0 ) {
		tex->target = GL_TEXTURE_2D_MULTISAMPLE ;
	}

	GLenum internalformat = gpu_get_gl_internalformat ( tex_format ) ;
	GLenum data_format = gpu_get_gl_dataformat ( tex_format , &tex->format_flag ) ;
	GLenum data_type = gpu_get_gl_datatype ( gpu_data_format ) ;

	/* Generate Texture object */
	tex->bindcode = GPU_tex_alloc ( ) ;

	if ( !tex->bindcode ) {
		fprintf ( stderr , "GPUTexture: texture create failed\n" );
		GPU_texture_free ( tex );
		return NULL;
	}

	glBindTexture ( tex->target , tex->bindcode );

	/* Check if texture fit in VRAM */
	GLenum proxy = GL_PROXY_TEXTURE_2D;

	if ( n == 2 ) {
		if ( d > 1 )
			proxy = GL_PROXY_TEXTURE_2D_ARRAY;
	}
	else if ( n == 1 ) {
		if ( h == 0 )
			proxy = GL_PROXY_TEXTURE_1D;
		else
			proxy = GL_PROXY_TEXTURE_1D_ARRAY;
	}
	else if ( n == 3 ) {
		proxy = GL_PROXY_TEXTURE_3D;
	}

	float *rescaled_pixels = NULL;
	bool valid = gpu_texture_try_alloc ( tex ,
		proxy ,
		internalformat ,
		data_format ,
		data_type ,
		tex->components ,
		can_rescale ,
		( const float * ) pixels ,
		&rescaled_pixels ) ;

#ifdef GPU_DEBUG
	printf ( "GPUTexture: create : %s, %s, w : %d, h : %d, d : %d, comp : %d, size : %.2f MiB\n" ,
		gl_enum_to_str ( tex->target ) ,
		gl_enum_to_str ( internalformat ) ,
		w ,
		h ,
		d ,
		tex->components ,
		gpu_texture_memory_footprint_compute ( tex ) / 1048576.0f );
#endif

	if ( !valid ) {
		fprintf ( stderr , "GPUTexture: texture alloc failed. Likely not enough Video Memory.\n" );
		fprintf ( stderr ,
			"Current texture memory usage : %.2f MiB.\n" ,
			gpu_texture_memory_footprint_compute ( tex ) / 1048576.0f );
		GPU_texture_free ( tex );
		return NULL;
	}

	/* Upload Texture */
	const float *pix = ( const float * ) ( ( rescaled_pixels ) ? rescaled_pixels : pixels ) ;

	if ( tex->target == GL_TEXTURE_2D || tex->target == GL_TEXTURE_2D_MULTISAMPLE ||
		tex->target == GL_TEXTURE_1D_ARRAY ) {
		if ( samples ) {
			glTexImage2DMultisample ( tex->target , samples , internalformat , tex->w , tex->h , true );
			if ( pix ) {
				glTexSubImage2D ( tex->target , 0 , 0 , 0 , tex->w , tex->h , data_format , data_type , pix );
			}
		}
		else {
			glTexImage2D ( tex->target , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , pix );
		}
	}
	else if ( tex->target == GL_TEXTURE_1D ) {
		glTexImage1D ( tex->target , 0 , internalformat , tex->w , 0 , data_format , data_type , pix );
	}
	else {
		glTexImage3D (
			tex->target , 0 , internalformat , tex->w , tex->h , tex->d , 0 , data_format , data_type , pix );
	}

	if ( rescaled_pixels ) {
		free ( rescaled_pixels ) ;
	}

	/* Texture Parameters */
	if ( GPU_texture_stencil ( tex ) || /* Does not support filtering */
		GPU_texture_integer ( tex ) || /* Does not support filtering */
		GPU_texture_depth ( tex ) ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , GL_NEAREST );
		glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , GL_NEAREST );
	}
	else {
		glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , GL_LINEAR );
		glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
	}

	if ( GPU_texture_depth ( tex ) ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_COMPARE_MODE , GL_NONE );
		glTexParameteri ( tex->target_base , GL_TEXTURE_COMPARE_FUNC , GL_LEQUAL );
	}

	glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_S , GL_CLAMP_TO_EDGE );
	if ( n > 1 ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_T , GL_CLAMP_TO_EDGE );
	}
	if ( n > 2 ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_R , GL_CLAMP_TO_EDGE );
	}

	glBindTexture ( tex->target , 0 );

	return tex;
}


static GPU_Texture *GPU_texture_cube_create ( int w ,
	int d ,
	const float *fpixels_px ,
	const float *fpixels_py ,
	const float *fpixels_pz ,
	const float *fpixels_nx ,
	const float *fpixels_ny ,
	const float *fpixels_nz ,
	eGPUTextureFormat tex_format ,
	eGPUDataFormat gpu_data_format )
{
	GPU_Texture *tex = new GPU_Texture ( ) ;
	tex->w = w;
	tex->h = w;
	tex->d = d;
	tex->samples = 0;
	tex->number = -1;
	tex->refcount = 1;
	tex->format = tex_format;
	tex->components = gpu_get_component_count ( tex_format );
	tex->bytesize = gpu_get_bytesize ( tex_format );
	tex->format_flag = GPU_FORMAT_CUBE;

	if ( d == 0 ) {
		tex->target_base = tex->target = GL_TEXTURE_CUBE_MAP;
	}
	else {
		assert ( false && "Cubemap array Not implemented yet" );
		// tex->target_base = tex->target = GL_TEXTURE_CUBE_MAP_ARRAY;
	}

	GLenum internalformat = gpu_get_gl_internalformat ( tex_format );
	GLenum data_format = gpu_get_gl_dataformat ( tex_format , &tex->format_flag );
	GLenum data_type = gpu_get_gl_datatype ( gpu_data_format );

	/* Generate Texture object */
	tex->bindcode = GPU_tex_alloc ( );

	if ( !tex->bindcode ) {
		fprintf ( stderr , "GPUTexture: texture create failed\n" );
		GPU_texture_free ( tex );
		return NULL;
	}

#ifdef GPU_DEBUG
	printf ( "GPUTexture: create : %s, %s, w : %d, h : %d, d : %d, comp : %d, size : %.2f MiB\n" ,
		gl_enum_to_str ( tex->target ) ,
		gl_enum_to_str ( internalformat ) ,
		w ,
		w ,
		d ,
		tex->components ,
		gpu_texture_memory_footprint_compute ( tex ) / 1048576.0f );
#endif

	glBindTexture ( tex->target , tex->bindcode );

	/* Upload Texture */
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_X , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_px );
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Y , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_py );
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Z , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_pz );
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_X , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_nx );
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_ny );
	glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z , 0 , internalformat , tex->w , tex->h , 0 , data_format , data_type , fpixels_nz );

	/* Texture Parameters */
	if ( GPU_texture_stencil ( tex ) || /* Does not support filtering */
		GPU_texture_integer ( tex ) || /* Does not support filtering */
		GPU_texture_depth ( tex ) ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , GL_NEAREST );
		glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , GL_NEAREST );
	}
	else {
		glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , GL_LINEAR );
		glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , GL_LINEAR );
	}

	if ( GPU_texture_depth ( tex ) ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_COMPARE_MODE , GL_NONE );
		glTexParameteri ( tex->target_base , GL_TEXTURE_COMPARE_FUNC , GL_LEQUAL );
	}

	glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_S , GL_CLAMP_TO_EDGE );
	glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_T , GL_CLAMP_TO_EDGE );
	glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_R , GL_CLAMP_TO_EDGE );

	glBindTexture ( tex->target , 0 );

	return tex;
}


/* Special buffer textures. tex_format must be compatible with the buffer content. */
GPU_Texture *GPU_texture_create_buffer ( eGPUTextureFormat tex_format , const GLuint buffer )
{
	GPU_Texture *tex = new GPU_Texture ( ) ;
	tex->number = -1;
	tex->refcount = 1;
	tex->format = tex_format;
	tex->components = gpu_get_component_count ( tex_format );
	tex->format_flag = 0;
	tex->target_base = tex->target = GL_TEXTURE_BUFFER;
	tex->bytesize = gpu_get_bytesize ( tex_format );

	GLenum internalformat = gpu_get_gl_internalformat ( tex_format );

	gpu_get_gl_dataformat ( tex_format , &tex->format_flag );

	if ( !( ELEM ( tex_format , GPU_R8 , GPU_R16 ) || ELEM ( tex_format , GPU_R16F , GPU_R32F ) ||
		ELEM ( tex_format , GPU_R8I , GPU_R16I , GPU_R32I ) ||
		ELEM ( tex_format , GPU_R8UI , GPU_R16UI , GPU_R32UI ) || ELEM ( tex_format , GPU_RG8 , GPU_RG16 ) ||
		ELEM ( tex_format , GPU_RG16F , GPU_RG32F ) ||
		ELEM ( tex_format , GPU_RG8I , GPU_RG16I , GPU_RG32I ) ||
		ELEM ( tex_format , GPU_RG8UI , GPU_RG16UI , GPU_RG32UI ) ||
		/* Not available until gl 4.0 */
		// ELEM(tex_format, GPU_RGB32F, GPU_RGB32I, GPU_RGB32UI) ||
		ELEM ( tex_format , GPU_RGBA8 , GPU_RGBA16 ) || ELEM ( tex_format , GPU_RGBA16F , GPU_RGBA32F ) ||
		ELEM ( tex_format , GPU_RGBA8I , GPU_RGBA16I , GPU_RGBA32I ) ||
		ELEM ( tex_format , GPU_RGBA8UI , GPU_RGBA16UI , GPU_RGBA32UI ) ) ) {
		fprintf ( stderr , "GPUTexture: invalid format for texture buffer\n" );
		GPU_texture_free ( tex );
		return NULL;
	}

	/* Generate Texture object */
	tex->bindcode = GPU_tex_alloc ( );

	if ( !tex->bindcode ) {
		fprintf ( stderr , "GPUTexture: texture create failed\n" );
		GPU_texture_free ( tex );
		assert ( 0 && "glGenTextures failed: Are you sure a valid OGL context is active on this thread?\n" );
		return NULL;
	}

	glBindTexture ( tex->target , tex->bindcode );
	glTexBuffer ( tex->target , internalformat , buffer );
	glBindTexture ( tex->target , 0 );

	return tex;
}


GPU_Texture *GPU_texture_from_bindcode ( int textarget , int bindcode )
{
	/* see GPUInput::textarget: it can take two values - GL_TEXTURE_2D and GL_TEXTURE_CUBE_MAP
	 * these values are correct for glDisable, so textarget can be safely used in
	 * GPU_texture_bind/GPU_texture_unbind through tex->target_base */
	 /* (is any of this obsolete now that we don't glEnable/Disable textures?) */
	GPU_Texture *tex = new GPU_Texture ( ) ;
	tex->bindcode = bindcode;
	tex->number = -1;
	tex->refcount = 1;
	tex->target = textarget;
	tex->target_base = textarget;
	tex->format = eGPUTextureFormat::GPU_INVALID_TEX_FORMAT ;
	tex->components = -1;
	tex->samples = 0;

	if ( !glIsTexture ( tex->bindcode ) ) {
		fprintf ( stderr , "GPU_Texture Not Loaded, invalid bind code!\n" );
	}
	else {
		GLint w , h;

		GLenum gettarget;

		if ( textarget == GL_TEXTURE_2D ) {
			gettarget = GL_TEXTURE_2D;
		}
		else {
			gettarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		}

		glBindTexture ( textarget , tex->bindcode );
		glGetTexLevelParameteriv ( gettarget , 0 , GL_TEXTURE_WIDTH , &w );
		glGetTexLevelParameteriv ( gettarget , 0 , GL_TEXTURE_HEIGHT , &h );
		tex->w = w;
		tex->h = h;
		glBindTexture ( textarget , 0 );
	}

	return tex;
}


GPU_Texture *GPU_texture_create_1d ( int w , eGPUTextureFormat tex_format , const float *pixels )
{
	assert ( w > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD ( w , 0 , 0 , 1 , pixels , tex_format , data_format , 0 , false );
}

GPU_Texture *GPU_texture_create_1d_array ( int w , int h , eGPUTextureFormat tex_format , const float *pixels )
{
	assert ( w > 0 && h > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD ( w , h , 0 , 1 , pixels , tex_format , data_format , 0 , false );
}

GPU_Texture *GPU_texture_create_2d ( int w , int h , eGPUTextureFormat tex_format , const float *pixels )
{
	assert ( w > 0 && h > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD ( w , h , 0 , 2 , pixels , tex_format , data_format , 0 , false );
}

GPU_Texture *GPU_texture_create_2d_multisample ( int w , int h , eGPUTextureFormat tex_format , const float *pixels , int samples )
{
	assert ( w > 0 && h > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD (
		w , h , 0 , 2 , pixels , tex_format , data_format , samples , false );
}

GPU_Texture *GPU_texture_create_2d_array (
	int w , int h , int d , eGPUTextureFormat tex_format , const float *pixels )
{
	assert ( w > 0 && h > 0 && d > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD ( w , h , d , 2 , pixels , tex_format , data_format , 0 , false );
}

GPU_Texture *GPU_texture_create_3d ( int w , int h , int d , eGPUTextureFormat tex_format , const float *pixels )
{
	assert ( w > 0 && h > 0 && d > 0 );
	eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex_format );
	return GPU_texture_create_nD ( w , h , d , 3 , pixels , tex_format , data_format , 0 , true );
}

GPU_Texture *GPU_texture_create_cube ( int w , eGPUTextureFormat tex_format , const float *fpixels ) {
	assert ( w > 0 );
	const float *fpixels_px , *fpixels_py , *fpixels_pz , *fpixels_nx , *fpixels_ny , *fpixels_nz;
	const int channels = gpu_get_component_count ( tex_format );

	if ( fpixels ) {
		int face_ofs = w * w * channels;
		fpixels_px = fpixels + 0 * face_ofs;
		fpixels_nx = fpixels + 1 * face_ofs;
		fpixels_py = fpixels + 2 * face_ofs;
		fpixels_ny = fpixels + 3 * face_ofs;
		fpixels_pz = fpixels + 4 * face_ofs;
		fpixels_nz = fpixels + 5 * face_ofs;
	}
	else {
		fpixels_px = fpixels_py = fpixels_pz = fpixels_nx = fpixels_ny = fpixels_nz = NULL;
	}

	return GPU_texture_cube_create ( w ,
		0 ,
		fpixels_px ,
		fpixels_py ,
		fpixels_pz ,
		fpixels_nx ,
		fpixels_ny ,
		fpixels_nz ,
		tex_format ,
		GPU_DATA_FLOAT );
}


void GPU_texture_add_mipmap ( GPU_Texture *tex , eGPUDataFormat gpu_data_format , int miplvl , const void *pixels )
{
	assert ( ( int ) tex->format > -1 );
	assert ( tex->components > -1 );

	gpu_validate_data_format ( tex->format , gpu_data_format );

	GLenum internalformat = gpu_get_gl_internalformat ( tex->format );
	GLenum data_format = gpu_get_gl_dataformat ( tex->format , &tex->format_flag );
	GLenum data_type = gpu_get_gl_datatype ( gpu_data_format );

	glBindTexture ( tex->target , tex->bindcode );

	int size [ 3 ];
	GPU_texture_get_mipmap_size ( tex , miplvl , size );

	switch ( tex->target ) {
		case GL_TEXTURE_1D:
			glTexImage1D ( tex->target , miplvl , internalformat , size [ 0 ] , 0 , data_format , data_type , pixels );
		break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_1D_ARRAY:
			glTexImage2D ( tex->target , miplvl , internalformat , size [ 0 ] , size [ 1 ] , 0 , data_format , data_type , pixels );
		break;
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
			glTexImage3D ( tex->target , miplvl , internalformat , size [ 0 ] , size [ 1 ] , size [ 2 ] , 0 , data_format , data_type , pixels );
		break;
		case GL_TEXTURE_2D_MULTISAMPLE:
			/* Multisample textures cannot have mipmaps. */
		default:
			assert ( !"tex->target mode not supported" );
	}

	glTexParameteri ( GPU_texture_target ( tex ) , GL_TEXTURE_MAX_LEVEL , miplvl );

	glBindTexture ( tex->target , 0 );
}


void GPU_texture_update_sub ( GPU_Texture *tex ,
	eGPUDataFormat gpu_data_format ,
	const void *pixels ,
	int offset_x ,
	int offset_y ,
	int offset_z ,
	int width ,
	int height ,
	int depth )
{
	assert ( ( int ) tex->format > -1 );
	assert ( tex->components > -1 );

	GLenum data_format = gpu_get_gl_dataformat ( tex->format , &tex->format_flag );
	GLenum data_type = gpu_get_gl_datatype ( gpu_data_format );
	GLint alignment;

	/* The default pack size for textures is 4, which won't work for byte based textures */
	if ( tex->bytesize == 1 ) {
		glGetIntegerv ( GL_UNPACK_ALIGNMENT , &alignment );
		glPixelStorei ( GL_UNPACK_ALIGNMENT , 1 );
	}

	glBindTexture ( tex->target , tex->bindcode );
	switch ( tex->target ) {
		case GL_TEXTURE_1D:
			glTexSubImage1D ( tex->target , 0 , offset_x , width , data_format , data_type , pixels );
			break;
		case GL_TEXTURE_2D:
		case GL_TEXTURE_2D_MULTISAMPLE:
		case GL_TEXTURE_1D_ARRAY:
			glTexSubImage2D ( tex->target , 0 , offset_x , offset_y , width , height , data_format , data_type , pixels );
			break;
		case GL_TEXTURE_3D:
		case GL_TEXTURE_2D_ARRAY:
			glTexSubImage3D ( tex->target , 0 , offset_x , offset_y , offset_z , width , height , depth , data_format , data_type , pixels );
			break;
		default:
			assert ( !"tex->target mode not supported" );
	}

	if ( tex->bytesize == 1 ) {
		glPixelStorei ( GL_UNPACK_ALIGNMENT , alignment );
	}

	glBindTexture ( tex->target , 0 );
}


void *GPU_texture_read ( GPU_Texture *tex , eGPUDataFormat gpu_data_format , int miplvl )
{
	int size [ 3 ] = { 0, 0, 0 };
	GPU_texture_get_mipmap_size ( tex , miplvl , size );

	gpu_validate_data_format ( tex->format , gpu_data_format );

	size_t buf_size = gpu_texture_memory_footprint_compute ( tex );
	size_t samples_count = std::max ( 1 , tex->samples );

	samples_count *= size [ 0 ];
	samples_count *= std::max ( 1 , size [ 1 ] );
	samples_count *= std::max ( 1 , size [ 2 ] );
	samples_count *= ( GPU_texture_cube ( tex ) ) ? 6 : 1;

	switch ( gpu_data_format ) {
		case GPU_DATA_FLOAT:
			buf_size = sizeof ( float ) * samples_count * tex->components;
			break;
		case GPU_DATA_INT:
		case GPU_DATA_UNSIGNED_INT:
			buf_size = sizeof ( int ) * samples_count * tex->components;
			break;
		case GPU_DATA_UNSIGNED_INT_24_8:
		case GPU_DATA_10_11_11_REV:
			buf_size = sizeof ( int ) * samples_count;
			break;
		case GPU_DATA_UNSIGNED_BYTE:
			break;
	}

	/* AMD Pro driver have a bug that write 8 bytes past buffer size
	 * if the texture is big. (see T66573) */
	void *buf = malloc ( buf_size + 8 );

	GLenum data_format = gpu_get_gl_dataformat ( tex->format , &tex->format_flag );
	GLenum data_type = gpu_get_gl_datatype ( gpu_data_format );

	glBindTexture ( tex->target , tex->bindcode );

	if ( GPU_texture_cube ( tex ) ) {
		int cube_face_size = buf_size / 6;
		for ( int i = 0; i < 6; i++ ) {
			glGetTexImage ( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i ,
				miplvl ,
				data_format ,
				data_type ,
				( ( char * ) buf ) + cube_face_size * i );
		}
	}
	else {
		glGetTexImage ( tex->target , miplvl , data_format , data_type , buf );
	}

	glBindTexture ( tex->target , 0 );

	return buf;
}


void GPU_texture_update ( GPU_Texture *tex , eGPUDataFormat data_format , const void *pixels )
{
	GPU_texture_update_sub ( tex , data_format , pixels , 0 , 0 , 0 , tex->w , tex->h , tex->d );
}

inline bool is_even_tile ( int x , int y , int w ) {
	return ( ( ( x % w ) < w / 2 and ( y % w ) < w / 2 ) or ( ( x % w ) >= w / 2 and ( y % w ) >= w / 2 ) ) ;
}

inline bool is_even_tile ( int x , int y , int z , int w ) {
	return ( ( ( x % w ) < w / 2 and ( y % w ) < w / 2 and ( z % w ) < w / 2 ) or ( ( x % w ) >= w / 2 and ( y % w ) >= w / 2 and ( z % w ) >= w / 2 ) ) ;
}

void GPU_invalid_tex_init ( void )
{
	const float color [ 4 ] = { 1.0f, 0.0f, 1.0f, 1.0f };

	const unsigned int tiles = 2 ;
	const unsigned int resolution = tiles * 2 ;
	static float texture_data2D [ resolution ][ resolution ][ 4 ] ;
	static float texture_data3D [ resolution ][ resolution ][ resolution ][ 4 ] ;

	for ( int x = 0 ; x < resolution ; x++ ) {
		for ( int y = 0 ; y < resolution ; y++ ) {
			int w = ( resolution / tiles ) * 2 ;
			texture_data2D [ x ][ y ][ 0 ] = 0.02f + is_even_tile ( x , y , w ) * 0.88f ;
			texture_data2D [ x ][ y ][ 1 ] = 0.02f + is_even_tile ( x , y , w ) * 0.08f ;
			texture_data2D [ x ][ y ][ 2 ] = 0.02f + is_even_tile ( x , y , w ) * 0.83f ;
			texture_data2D [ x ][ y ][ 3 ] = 1.0f ;
		}
	}

	for ( int x = 0 ; x < resolution ; x++ ) {
		for ( int y = 0 ; y < resolution ; y++ ) {
			for ( int z = 0 ; z < resolution ; z++ ) {
				int w = ( resolution / tiles ) * 2 ;
				texture_data3D [ x ][ y ][ z ][ 0 ] = 0.02f + is_even_tile ( x , y , z , w ) * 0.88f ;
				texture_data3D [ x ][ y ][ z ][ 1 ] = 0.02f + is_even_tile ( x , y , z , w ) * 0.08f ;
				texture_data3D [ x ][ y ][ z ][ 2 ] = 0.02f + is_even_tile ( x , y , z , w ) * 0.83f ;
				texture_data3D [ x ][ y ][ z ][ 3 ] = 1.0f ;
			}
		}
	}

	GPUTextureGlobal.invalid_tex_1D = GPU_texture_create_1d ( 1 , GPU_RGBA8 , color );
	GPUTextureGlobal.invalid_tex_2D = GPU_texture_create_2d ( resolution , resolution , GPU_RGBA8 , ( const float * ) texture_data2D );
	GPUTextureGlobal.invalid_tex_3D = GPU_texture_create_3d ( resolution , resolution , resolution , GPU_RGBA8 , ( const float * ) texture_data3D );

	GPU_texture_bind ( GPUTextureGlobal.invalid_tex_1D , 0 ) ;
	GPU_texture_filters ( GPUTextureGlobal.invalid_tex_1D , eGPUFilterFunction::GPU_NEAREST , eGPUFilterFunction::GPU_NEAREST ) ;
	GPU_texture_bind ( GPUTextureGlobal.invalid_tex_2D , 1 ) ;
	GPU_texture_filters ( GPUTextureGlobal.invalid_tex_2D , eGPUFilterFunction::GPU_NEAREST , eGPUFilterFunction::GPU_NEAREST ) ;
	GPU_texture_bind ( GPUTextureGlobal.invalid_tex_3D , 2 ) ;
	GPU_texture_filters ( GPUTextureGlobal.invalid_tex_3D , eGPUFilterFunction::GPU_NEAREST , eGPUFilterFunction::GPU_NEAREST ) ;

	const float color_white [ 4 ] = { 1.0f, 1.0f, 1.0f, 1.0f };

	GPUTextureGlobal.empty_tex_1D = GPU_texture_create_1d ( 1 , GPU_RGBA8 , color );
	GPUTextureGlobal.empty_tex_2D = GPU_texture_create_2d ( 1 , 1 , GPU_RGBA8 , color_white );
	GPUTextureGlobal.empty_tex_3D = GPU_texture_create_3d ( 1 , 1 , resolution , GPU_RGBA8 , color_white );
}

void GPU_invalid_tex_bind ( int mode )
{
	switch ( mode ) {
		case GL_TEXTURE_1D:
			glBindTexture ( GL_TEXTURE_1D , GPUTextureGlobal.invalid_tex_1D->bindcode );
			break;
		case GL_TEXTURE_2D:
			glBindTexture ( GL_TEXTURE_2D , GPUTextureGlobal.invalid_tex_2D->bindcode );
			break;
		case GL_TEXTURE_3D:
			glBindTexture ( GL_TEXTURE_3D , GPUTextureGlobal.invalid_tex_3D->bindcode );
			break;
	}
}

void GPU_invalid_tex_free ( void )
{
	if ( GPUTextureGlobal.invalid_tex_1D ) {
		GPU_texture_free ( GPUTextureGlobal.invalid_tex_1D );
	}
	if ( GPUTextureGlobal.invalid_tex_2D ) {
		GPU_texture_free ( GPUTextureGlobal.invalid_tex_2D );
	}
	if ( GPUTextureGlobal.invalid_tex_3D ) {
		GPU_texture_free ( GPUTextureGlobal.invalid_tex_3D );
	}
}


void GPU_texture_bind ( GPU_Texture *tex , int number )
{
	assert ( number >= 0 );

	if ( number >= GPU_max_textures ( ) ) {
		fprintf ( stderr , "Not enough texture slots.\n" );
		return;
	}

#ifdef GPU_DEBUG
	for ( int i = 0; i < GPU_TEX_MAX_FBO_ATTACHED; i++ ) {
		if ( tex->fb [ i ] && GPU_framebuffer_bound ( tex->fb [ i ] ) ) {
			fprintf ( stderr ,
				"Feedback loop warning!: Attempting to bind "
				"texture attached to current framebuffer!\n" );
			assert ( 0 ); /* Should never happen! */
			break;
		}
	}
#endif

	glActiveTexture ( GL_TEXTURE0 + number );

	if ( tex->bindcode != 0 ) {
		glBindTexture ( tex->target , tex->bindcode );
	}
	else {
		GPU_invalid_tex_bind ( tex->target_base );
	}

	tex->number = number;
}


void GPU_texture_unbind ( GPU_Texture *tex )
{
	if ( tex->number == -1 ) {
		return;
	}

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glBindTexture ( tex->target , 0 );

	tex->number = -1;
}

int GPU_texture_bound_number ( GPU_Texture *tex )
{
	return tex->number;
}


#define WARN_NOT_BOUND(_tex) \
  { \
    if (_tex->number == -1) { \
      fprintf(stderr, "Warning : Trying to set parameter on a texture not bound.\n"); \
      assert(0); \
      return; \
    } \
  } \
  ((void)0)

void GPU_texture_generate_mipmap ( GPU_Texture *tex )
{
	WARN_NOT_BOUND ( tex );

	glActiveTexture ( GL_TEXTURE0 + tex->number );

	if ( GPU_texture_depth ( tex ) ) {
		/* Some drivers have bugs when using glGenerateMipmap with depth textures (see T56789).
		 * In this case we just create a complete texture with mipmaps manually without down-sampling.
		 * You must initialize the texture levels using other methods like
		 * GPU_framebuffer_recursive_downsample(). */
		int levels = 1 + floor ( log2 ( std::max ( tex->w , tex->h ) ) );
		eGPUDataFormat data_format = gpu_get_data_format_from_tex_format ( tex->format );
		for ( int i = 1; i < levels; i++ ) {
			GPU_texture_add_mipmap ( tex , data_format , i , NULL );
		}
		glBindTexture ( tex->target , tex->bindcode );
	}
	else {
		glGenerateMipmap ( tex->target_base );
	}
}

void GPU_texture_compare_mode ( GPU_Texture *tex , bool use_compare )
{
	WARN_NOT_BOUND ( tex );

	/* Could become an assertion ? (fclem) */
	if ( !GPU_texture_depth ( tex ) ) {
		return;
	}

	GLenum mode = ( use_compare ) ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE;

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glTexParameteri ( tex->target_base , GL_TEXTURE_COMPARE_MODE , mode );
}

void GPU_texture_filter_mode ( GPU_Texture *tex , bool use_filter )
{
	WARN_NOT_BOUND ( tex );

	/* Stencil and integer format does not support filtering. */
	assert ( !use_filter || !( GPU_texture_stencil ( tex ) || GPU_texture_integer ( tex ) ) );

	GLenum filter = ( use_filter ) ? GL_LINEAR : GL_NEAREST;

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , filter );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , filter );
}

void GPU_texture_mipmap_mode ( GPU_Texture *tex , bool use_mipmap , bool use_filter )
{
	WARN_NOT_BOUND ( tex );

	/* Stencil and integer format does not support filtering. */
	assert ( ( !use_filter && !use_mipmap ) ||
		!( GPU_texture_stencil ( tex ) || GPU_texture_integer ( tex ) ) );

	GLenum filter = ( use_filter ) ? GL_LINEAR : GL_NEAREST;
	GLenum mipmap = ( ( use_filter ) ? ( use_mipmap ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR :
		( use_mipmap ) ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST );

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , mipmap );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , filter );
}

void GPU_texture_wrap_mode ( GPU_Texture *tex , bool use_repeat )
{
	WARN_NOT_BOUND ( tex );

	GLenum repeat = ( use_repeat ) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_S , repeat );
	if ( tex->target_base != GL_TEXTURE_1D ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_T , repeat );
	}
	if ( tex->target_base == GL_TEXTURE_3D ) {
		glTexParameteri ( tex->target_base , GL_TEXTURE_WRAP_R , repeat );
	}
}

static GLenum gpu_get_gl_filterfunction ( eGPUFilterFunction filter )
{
	switch ( filter ) {
		case GPU_NEAREST:
			return GL_NEAREST;
		case GPU_LINEAR:
			return GL_LINEAR;
		default:
			assert ( !"Unhandled filter mode" );
			return GL_NEAREST;
	}
}

void GPU_texture_filters ( GPU_Texture *tex ,
	eGPUFilterFunction min_filter ,
	eGPUFilterFunction mag_filter )
{
	WARN_NOT_BOUND ( tex );

	/* Stencil and integer format does not support filtering. */
	assert ( !( GPU_texture_stencil ( tex ) || GPU_texture_integer ( tex ) ) );
	assert ( mag_filter == GPU_NEAREST || mag_filter == GPU_LINEAR );

	glActiveTexture ( GL_TEXTURE0 + tex->number );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MIN_FILTER , gpu_get_gl_filterfunction ( min_filter ) );
	glTexParameteri ( tex->target_base , GL_TEXTURE_MAG_FILTER , gpu_get_gl_filterfunction ( mag_filter ) );
}

void GPU_texture_free ( GPU_Texture *tex )
{
	tex->refcount--;

	if ( tex->refcount < 0 ) {
		fprintf ( stderr , "GPU_Texture: negative refcount\n" );
	}

	if ( tex->refcount == 0 ) {
		for ( int i = 0; i < GPU_TEX_MAX_FBO_ATTACHED; i++ ) {
			if ( tex->fb [ i ] != NULL ) {
				GPU_framebuffer_texture_detach_slot ( tex->fb [ i ] , tex , tex->fb_attachment [ i ] );
			}
		}

		if ( tex->bindcode ) {
			GPU_tex_free ( tex->bindcode );
		}

		delete tex ;
	}
}

void GPU_texture_attach_framebuffer ( GPU_Texture *tex , GPU_Framebuffer *fb , int attachment ) {
	for ( int i = 0; i < GPU_TEX_MAX_FBO_ATTACHED; i++ ) {
		if ( tex->fb [ i ] == NULL ) {
			tex->fb [ i ] = fb;
			tex->fb_attachment [ i ] = attachment;
			return;
		}
	}

	fprintf ( stderr , "Error: Texture: Not enough Framebuffer slots.\n" ) ;
	assert ( !"Error: Texture: Not enough Framebuffer slots" );
}

int GPU_texture_detach_framebuffer ( GPU_Texture *tex , GPU_Framebuffer *fb ) {
	for ( int i = 0; i < GPU_TEX_MAX_FBO_ATTACHED; i++ ) {
		if ( tex->fb [ i ] == fb ) {
			tex->fb [ i ] = NULL;
			return tex->fb_attachment [ i ];
		}
	}

	fprintf ( stderr , "Error: Texture: Framebuffer is not attached.\n" ) ;
	assert ( !"Error: Texture: Framebuffer is not attached" );
	return 0;
}

void GPU_texture_ref ( GPU_Texture *tex )
{
	tex->refcount++;
}

int GPU_texture_target ( const GPU_Texture *tex )
{
	return tex->target;
}

int GPU_texture_width ( const GPU_Texture *tex )
{
	return tex->w;
}

int GPU_texture_height ( const GPU_Texture *tex )
{
	return tex->h;
}

int GPU_texture_orig_width ( const GPU_Texture *tex )
{
	return tex->orig_w;
}

int GPU_texture_orig_height ( const GPU_Texture *tex )
{
	return tex->orig_h;
}

void GPU_texture_orig_size_set ( GPU_Texture *tex , int w , int h )
{
	tex->orig_w = w;
	tex->orig_h = h;
}

int GPU_texture_layers ( const GPU_Texture *tex )
{
	return tex->d;
}

eGPUTextureFormat GPU_texture_format ( const GPU_Texture *tex )
{
	return tex->format;
}

int GPU_texture_samples ( const GPU_Texture *tex )
{
	return tex->samples;
}

bool GPU_texture_depth ( const GPU_Texture *tex )
{
	return ( tex->format_flag & GPU_FORMAT_DEPTH ) != 0;
}

bool GPU_texture_stencil ( const GPU_Texture *tex )
{
	return ( tex->format_flag & GPU_FORMAT_STENCIL ) != 0;
}

bool GPU_texture_integer ( const GPU_Texture *tex )
{
	return ( tex->format_flag & GPU_FORMAT_INTEGER ) != 0;
}

bool GPU_texture_cube ( const GPU_Texture *tex )
{
	return ( tex->format_flag & GPU_FORMAT_CUBE ) != 0;
}

int GPU_texture_opengl_bindcode ( const GPU_Texture *tex )
{
	return tex->bindcode;
}

void GPU_texture_get_mipmap_size ( GPU_Texture *tex , int lvl , int *size )
{
	/* TODO assert if lvl is below the limit of 1px in each dimension. */
	int div = 1 << lvl;
	size [ 0 ] = std::max ( 1 , tex->w / div );

	if ( tex->target == GL_TEXTURE_1D_ARRAY ) {
		size [ 1 ] = tex->h;
	}
	else if ( tex->h > 0 ) {
		size [ 1 ] = std::max ( 1 , tex->h / div );
	}

	if ( tex->target == GL_TEXTURE_2D_ARRAY ) {
		size [ 2 ] = tex->d;
	}
	else if ( tex->d > 0 ) {
		size [ 2 ] = std::max ( 1 , tex->d / div );
	}
}

GPU_Texture *GPU_texture_get_invalid ( int dimensions ) {
	switch ( dimensions ) {
		case 1: return GPUTextureGlobal.invalid_tex_1D ;
		case 2: return GPUTextureGlobal.invalid_tex_2D ;
		case 3: return GPUTextureGlobal.invalid_tex_3D ;
	}
	return NULL ;
}

GPU_Texture *GPU_texture_get_empty ( int dimensions ) {
	switch ( dimensions ) {
		case 1: return GPUTextureGlobal.empty_tex_1D ;
		case 2: return GPUTextureGlobal.empty_tex_2D ;
		case 3: return GPUTextureGlobal.empty_tex_3D ;
	}
	return NULL ;
}
