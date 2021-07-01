#include "gpu_init.h"

#include <string>

__GPUGLEWExtensions__ GPUGLEWExtensions ;
__GPUPlatformSupportTest__ GPUPlatformSupportTest ;
__GPUPlatformGlobal__ GPUPlatformGlobal ;

char *gpu_platform_create_key ( eGPUSupportLevel support_level , const char *vendor , const char *renderer , const char *version ) {
	std::string ds ;

	ds += "{" ;
	ds += vendor ;
	ds += "/" ;
	ds += renderer ;
	ds += "/" ;
	ds += version ;
	ds += "}" ;
	ds += "=" ;

	if ( support_level == eGPUSupportLevel::GPU_SUPPORT_LEVEL_SUPPORTED ) ds += "SUPPORTED" ;
	else if ( support_level == eGPUSupportLevel::GPU_SUPPORT_LEVEL_LIMITED ) ds += "LIMITED" ;
	else ds += "UNSUPPORTED" ;

	for ( int index = 0 ; index < ds.length ( ) ; index++ )
		if ( ds [ index ] == '\n' or ds [ index ] == '\r' ) ds = ' ' ;

	char *str = new char [ ds.length ( ) + 1 ] ;
	memcpy ( str , ds.c_str ( ) , ds.length ( ) + 1 ) ;
	return str ;
}
char *gpu_platform_create_gpu_name ( const char *vendor , const char *renderer , const char *version ) {
	std::string ds ;
	ds += vendor ;
	ds += " " ;
	ds += renderer ;
	ds += " " ;
	ds += version ;

	for ( int index = 0 ; index < ds.length ( ) ; index++ )
		if ( ds [ index ] == '\n' or ds [ index ] == '\r' ) ds = ' ' ;
	char *str = new char [ ds.length ( ) + 1 ] ;
	memcpy ( str , ds.c_str ( ) , ds.length ( ) + 1 ) ;
	return str ;
}

void GPU_init ( ) {
	glewInit ( ) ;

	glGetIntegerv ( GL_MAX_TEXTURE_IMAGE_UNITS , &GPUGLEWExtensions.maxtexturesfrag );
	glGetIntegerv ( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS , &GPUGLEWExtensions.maxtexturesvert );
	glGetIntegerv ( GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS , &GPUGLEWExtensions.maxtexturesgeom );
	glGetIntegerv ( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS , &GPUGLEWExtensions.maxtextures );

	glGetIntegerv ( GL_MAX_TEXTURE_SIZE , &GPUGLEWExtensions.maxtexsize );
	glGetIntegerv ( GL_MAX_ARRAY_TEXTURE_LAYERS , &GPUGLEWExtensions.maxtexlayers );
	glGetIntegerv ( GL_MAX_CUBE_MAP_TEXTURE_SIZE , &GPUGLEWExtensions.maxcubemapsize );

	if ( GLEW_EXT_texture_filter_anisotropic ) {
		glGetFloatv ( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT , &GPUGLEWExtensions.max_anisotropy );
	}
	else {
		GPUGLEWExtensions.max_anisotropy = 1.0f;
	}

	glGetIntegerv ( GL_MAX_FRAGMENT_UNIFORM_BLOCKS , &GPUGLEWExtensions.maxubobinds );
	glGetIntegerv ( GL_MAX_UNIFORM_BLOCK_SIZE , &GPUGLEWExtensions.maxubosize );

	glGetFloatv ( GL_ALIASED_LINE_WIDTH_RANGE , GPUGLEWExtensions.line_width_range );

	glGetIntegerv ( GL_MAX_COLOR_TEXTURE_SAMPLES , &GPUGLEWExtensions.samples_color_texture_max );

	GPUGLEWExtensions.vendor = ( char * ) glGetString ( GL_VENDOR ) ;
	GPUGLEWExtensions.renderer = ( char * ) glGetString ( GL_RENDERER ) ;
	GPUGLEWExtensions.version = ( char * ) glGetString ( GL_VERSION ) ;

	printf ( "------ [  GLEW EXTENSIONS  ] ------\n" ) ;
	printf ( "GL_VENDOR = %s\n" , glGetString ( GL_VENDOR ) ) ;
	printf ( "GL_RENDERER = %s\n" , glGetString ( GL_RENDERER ) ) ;
	printf ( "GL_VERSION = %s\n" , glGetString ( GL_VERSION ) ) ;


#ifdef _WIN32
	GPUPlatformGlobal.os = GPU_OS_WIN;
#elif defined(__APPLE__)
	GPUPlatformGlobal.os = GPU_OS_MAC;
#else
	GPUPlatformGlobal.os = GPU_OS_UNIX;
#endif

	const char *vendor = ( const char * ) glGetString ( GL_VENDOR );
	const char *renderer = ( const char * ) glGetString ( GL_RENDERER );
	const char *version = ( const char * ) glGetString ( GL_VERSION );

	if ( strstr ( vendor , "ATI" ) || strstr ( vendor , "AMD" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_ATI;
		GPUPlatformGlobal.driver = GPU_DRIVER_OFFICIAL;
	}
	else if ( strstr ( vendor , "NVIDIA" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_NVIDIA;
		GPUPlatformGlobal.driver = GPU_DRIVER_OFFICIAL;
	}
	else if ( strstr ( vendor , "Intel" ) ||
		/* src/mesa/drivers/dri/intel/intel_context.c */
		strstr ( renderer , "Mesa DRI Intel" ) || strstr ( renderer , "Mesa DRI Mobile Intel" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_INTEL;
		GPUPlatformGlobal.driver = GPU_DRIVER_OFFICIAL;
	}
	else if ( ( strstr ( renderer , "Mesa DRI R" ) ) ||
		( strstr ( renderer , "Radeon" ) && strstr ( vendor , "X.Org" ) ) ||
		( strstr ( renderer , "AMD" ) && strstr ( vendor , "X.Org" ) ) ||
		( strstr ( renderer , "Gallium " ) && strstr ( renderer , " on ATI " ) ) ||
		( strstr ( renderer , "Gallium " ) && strstr ( renderer , " on AMD " ) ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_ATI;
		GPUPlatformGlobal.driver = GPU_DRIVER_OPENSOURCE;
	}
	else if ( strstr ( renderer , "Nouveau" ) || strstr ( vendor , "nouveau" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_NVIDIA;
		GPUPlatformGlobal.driver = GPU_DRIVER_OPENSOURCE;
	}
	else if ( strstr ( vendor , "Mesa" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_SOFTWARE;
		GPUPlatformGlobal.driver = GPU_DRIVER_SOFTWARE;
	}
	else if ( strstr ( vendor , "Microsoft" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_SOFTWARE;
		GPUPlatformGlobal.driver = GPU_DRIVER_SOFTWARE;
	}
	else if ( strstr ( renderer , "Apple Software Renderer" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_SOFTWARE;
		GPUPlatformGlobal.driver = GPU_DRIVER_SOFTWARE;
	}
	else if ( strstr ( renderer , "llvmpipe" ) || strstr ( renderer , "softpipe" ) ) {
		GPUPlatformGlobal.device = GPU_DEVICE_SOFTWARE;
		GPUPlatformGlobal.driver = GPU_DRIVER_SOFTWARE;
	}
	else {
		printf ( "Warning: Could not find a matching GPU name. Things may not behave as expected.\n" );
		printf ( "Detected OpenGL configuration:\n" );
		printf ( "Vendor: %s\n" , vendor );
		printf ( "Renderer: %s\n" , renderer );
		GPUPlatformGlobal.device = GPU_DEVICE_ANY;
		GPUPlatformGlobal.driver = GPU_DRIVER_ANY;
	}

	/* Detect support level */
	if ( !GLEW_VERSION_3_3 ) {
		GPUPlatformGlobal.support_level = GPU_SUPPORT_LEVEL_UNSUPPORTED;
	}
	else {
		if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_ANY ) ) {
			/* Old Intel drivers with known bugs that cause material properties to crash.
			 * Version Build 10.18.14.5067 is the latest available and appears to be working
			 * ok with our workarounds, so excluded from this list. */
			if ( strstr ( version , "Build 7.14" ) || strstr ( version , "Build 7.15" ) ||
				strstr ( version , "Build 8.15" ) || strstr ( version , "Build 9.17" ) ||
				strstr ( version , "Build 9.18" ) || strstr ( version , "Build 10.18.10.3" ) ||
				strstr ( version , "Build 10.18.10.4" ) || strstr ( version , "Build 10.18.10.5" ) ||
				strstr ( version , "Build 10.18.14.4" ) ) {
				GPUPlatformGlobal.support_level = GPU_SUPPORT_LEVEL_LIMITED;
			}
		}
	}

	/* df/dy calculation factors, those are dependent on driver */
	GPUPlatformGlobal.dfdyfactors [ 0 ] = 1.0;
	GPUPlatformGlobal.dfdyfactors [ 1 ] = 1.0;

	GPUPlatformGlobal.context_local_shaders_workaround = false ;
	GPUPlatformGlobal.unused_fb_slot_workaround = false ;

	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_WIN , GPU_DRIVER_OFFICIAL ) ) {
		if ( strstr ( version , "4.5.13399" ) || strstr ( version , "4.5.13417" ) ||
			strstr ( version , "4.5.13422" ) ) {
			/* The renderers include:
			 *   Mobility Radeon HD 5000;
			 *   Radeon HD 7500M;
			 *   Radeon HD 7570M;
			 *   Radeon HD 7600M;
			 * And many others... */

			GPUPlatformGlobal.unused_fb_slot_workaround = true;
		}
	}

	if ( GPU_type_matches ( GPU_DEVICE_ATI , GPU_OS_ANY , GPU_DRIVER_ANY ) &&
		strstr ( version , "3.3.10750" ) ) {
		GPUPlatformGlobal.dfdyfactors [ 0 ] = 1.0;
		GPUPlatformGlobal.dfdyfactors [ 1 ] = -1.0;
	}
	else if ( GPU_type_matches ( GPU_DEVICE_INTEL , GPU_OS_WIN , GPU_DRIVER_ANY ) ) {
		if ( strstr ( version , "4.0.0 - Build 10.18.10.3308" ) ||
			strstr ( version , "4.0.0 - Build 9.18.10.3186" ) ||
			strstr ( version , "4.0.0 - Build 9.18.10.3165" ) ||
			strstr ( version , "3.1.0 - Build 9.17.10.3347" ) ||
			strstr ( version , "3.1.0 - Build 9.17.10.4101" ) ||
			strstr ( version , "3.3.0 - Build 8.15.10.2618" ) ) {
			GPUPlatformGlobal.dfdyfactors [ 0 ] = -1.0;
			GPUPlatformGlobal.dfdyfactors [ 1 ] = 1.0;
		}
		if ( strstr ( version , "Build 10.18.10.3" ) || strstr ( version , "Build 10.18.10.4" ) ||
			strstr ( version , "Build 10.18.10.5" ) || strstr ( version , "Build 10.18.14.4" ) ||
			strstr ( version , "Build 10.18.14.5" ) ) {
			/* Maybe not all of these drivers have problems with `GLEW_ARB_base_instance`.
			 * But it's hard to test each case. */
			GPUPlatformGlobal.glew_arb_base_instance_is_supported = false;
			GPUPlatformGlobal.context_local_shaders_workaround = true;
		}

		if ( strstr ( version , "Build 20.19.15.4285" ) ) {
			/* Somehow fixes armature display issues (see T69743). */
			GPUPlatformGlobal.context_local_shaders_workaround = true;
		}
	}

	GPUPlatformGlobal.support_key = gpu_platform_create_key ( GPUPlatformGlobal.support_level , vendor , renderer , version );
	GPUPlatformGlobal.gpu_name = gpu_platform_create_gpu_name ( vendor , renderer , version );
	GPUPlatformGlobal.initialized = true;

	printf ( "------ [ OPERATING  SYSTEM ] ------\n" ) ;
	printf ( "OS_GPU_NAME = %s\n" , GPUPlatformGlobal.gpu_name ) ;
	printf ( "OS_GPU_SUPPORT_KEY = %s\n" , GPUPlatformGlobal.support_key ) ;

	printf ( "\n" ) ;
}

void GPU_exit ( ) {
}
