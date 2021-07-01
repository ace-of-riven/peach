#pragma once

#include <cstring>
#include <gl/glew.h>
#include <SOIL/SOIL.h>
#include "gpu_defs.h"

typedef unsigned char byte ;

typedef enum eGPUDeviceType {
	GPU_DEVICE_NVIDIA = ( 1 << 0 ) ,
	GPU_DEVICE_ATI = ( 1 << 1 ) ,
	GPU_DEVICE_INTEL = ( 1 << 2 ) ,
	GPU_DEVICE_INTEL_UHD = ( 1 << 3 ) ,
	GPU_DEVICE_SOFTWARE = ( 1 << 4 ) ,
	GPU_DEVICE_UNKNOWN = ( 1 << 5 ) ,
	GPU_DEVICE_ANY = ( 0xff ) ,
} eGPUDeviceType;

typedef enum eGPUOSType {
	GPU_OS_WIN = ( 1 << 8 ) ,
	GPU_OS_MAC = ( 1 << 9 ) ,
	GPU_OS_UNIX = ( 1 << 10 ) ,
	GPU_OS_ANY = ( 0xff00 ) ,
} eGPUOSType;

typedef enum eGPUDriverType {
	GPU_DRIVER_OFFICIAL = ( 1 << 16 ) ,
	GPU_DRIVER_OPENSOURCE = ( 1 << 17 ) ,
	GPU_DRIVER_SOFTWARE = ( 1 << 18 ) ,
	GPU_DRIVER_ANY = ( 0xff0000 ) ,
} eGPUDriverType;

typedef enum eGPUSupportLevel {
	GPU_SUPPORT_LEVEL_SUPPORTED ,
	GPU_SUPPORT_LEVEL_LIMITED ,
	GPU_SUPPORT_LEVEL_UNSUPPORTED ,
} eGPUSupportLevel;

struct __GPUGLEWExtensions__ {

	GLint maxtexsize;
	GLint maxtexlayers;
	GLint maxcubemapsize;
	GLint maxtextures;
	GLint maxtexturesfrag;
	GLint maxtexturesgeom;
	GLint maxtexturesvert;
	GLint maxubosize;
	GLint maxubobinds;

	int samples_color_texture_max;

	float line_width_range [ 2 ];
	float max_anisotropy ;

	char *vendor ;
	char *version ;
	char *renderer ;

} ;

struct __GPUPlatformGlobal__ {
	bool initialized;

	eGPUDeviceType device;
	eGPUOSType os;
	eGPUDriverType driver;
	eGPUSupportLevel support_level;

	char *support_key;
	char *gpu_name;

	float dfdyfactors [ 2 ] ;

	bool glew_arb_base_instance_is_supported ;
	bool context_local_shaders_workaround ;
	bool unused_fb_slot_workaround ;
} ;

struct __GPUPlatformSupportTest__ {
	eGPUSupportLevel support_level;
	eGPUDeviceType device;
	eGPUOSType os;
	eGPUDriverType driver;
	char *vendor;
	char *renderer;
	char *version;
} ;

extern __GPUGLEWExtensions__ GPUGLEWExtensions ;
extern __GPUPlatformSupportTest__ GPUPlatformSupportTest ;
extern __GPUPlatformGlobal__ GPUPlatformGlobal ;

void GPU_init ( ) ;
void GPU_exit ( ) ;

inline int GPU_max_texture_size ( void ) {
	return GPUGLEWExtensions.maxtexsize;
}
inline int GPU_max_texture_layers ( void ) {
	return GPUGLEWExtensions.maxtexlayers;
}
inline int GPU_max_textures ( void ) {
	return GPUGLEWExtensions.maxtextures;
}
inline int GPU_max_textures_frag ( void ) {
	return GPUGLEWExtensions.maxtexturesfrag;
}
inline int GPU_max_textures_geom ( void ) {
	return GPUGLEWExtensions.maxtexturesgeom;
}
inline int GPU_max_textures_vert ( void ) {
	return GPUGLEWExtensions.maxtexturesvert;
}
inline float GPU_max_texture_anisotropy ( void ) {
	return GPUGLEWExtensions.max_anisotropy;
}
inline int GPU_max_color_texture_samples ( void ) {
	return GPUGLEWExtensions.samples_color_texture_max;
}
inline int GPU_max_cube_map_size ( void ) {
	return GPUGLEWExtensions.maxcubemapsize;
}
inline int GPU_max_ubo_binds ( void ) {
	return GPUGLEWExtensions.maxubobinds;
}
inline int GPU_max_ubo_size ( void ) {
	return GPUGLEWExtensions.maxubosize;
}
inline float GPU_max_line_width ( void ) {
	return GPUGLEWExtensions.line_width_range [ 1 ];
}
inline void GPU_get_dfdy_factors ( float factors [ 2 ] ) {
	memcpy ( factors , GPUPlatformGlobal.dfdyfactors , sizeof ( float ) * 2 ) ;
}
inline bool GPU_arb_base_instance_is_supported ( void ) {
	return GPUPlatformGlobal.glew_arb_base_instance_is_supported ;
}
inline bool GPU_context_local_shaders_workaround ( void ) {
	return GPUPlatformGlobal.context_local_shaders_workaround ;
}
inline bool GPU_unused_fb_slot_workaround ( ) {
	return GPUPlatformGlobal.unused_fb_slot_workaround ;
}
inline bool GPU_crappy_amd_driver ( void ) {
	/* Currently are the same drivers with the `unused_fb_slot` problem. */
	return GPU_unused_fb_slot_workaround ( );
}

inline bool GPU_type_matches ( eGPUDeviceType device , eGPUOSType os , eGPUDriverType driver ) {
	return ( GPUPlatformGlobal.device & device ) &&
		( GPUPlatformGlobal.os & os ) &&
		( GPUPlatformGlobal.driver & driver );
}

inline eGPUSupportLevel GPU_platform_support_level ( void ) {
	return GPUPlatformGlobal.support_level;
}
inline const char *GPU_platform_support_level_key ( void ) {
	return GPUPlatformGlobal.support_key;
}
inline const char *GPU_platform_gpu_name ( void ) {
	return GPUPlatformGlobal.gpu_name;
}
