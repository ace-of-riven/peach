#include "gpu_primitive.h"

GPUPrimClass GPU_primtype_class ( GPUPrimType prim_type ) {
	switch ( prim_type ) {
		case GPU_PRIM_POINTS: return GPU_PRIM_CLASS_POINT;
		case GPU_PRIM_LINES: return GPU_PRIM_CLASS_LINE;
		case GPU_PRIM_LINE_STRIP: return GPU_PRIM_CLASS_LINE;
		case GPU_PRIM_LINE_LOOP: return GPU_PRIM_CLASS_LINE;
		case GPU_PRIM_TRIS: return GPU_PRIM_CLASS_SURFACE;
		case GPU_PRIM_TRI_STRIP: return GPU_PRIM_CLASS_SURFACE;
		case GPU_PRIM_TRI_FAN: return GPU_PRIM_CLASS_SURFACE;
		case GPU_PRIM_LINES_ADJ: return GPU_PRIM_CLASS_LINE;
		case GPU_PRIM_LINE_STRIP_ADJ: return GPU_PRIM_CLASS_LINE;
		case GPU_PRIM_TRIS_ADJ: return GPU_PRIM_CLASS_SURFACE;
		case GPU_PRIM_NONE: return GPU_PRIM_CLASS_NONE;
	};
	return GPU_PRIM_CLASS_NONE;
}

bool GPU_primtype_belongs_to_class ( GPUPrimType prim_type , GPUPrimClass prim_class ) {
	if ( prim_class == GPU_PRIM_CLASS_NONE && prim_type == GPU_PRIM_NONE )
		return true;
	return prim_class & GPU_primtype_class ( prim_type ) ;
}

GLenum convert_prim_type_to_gl ( GPUPrimType prim_type ) {
	switch ( prim_type ) {
		case GPU_PRIM_POINTS: return GL_POINTS;
		case GPU_PRIM_LINES: return GL_LINES;
		case GPU_PRIM_LINE_STRIP: return GL_LINE_STRIP;
		case GPU_PRIM_LINE_LOOP: return GL_LINE_LOOP;
		case GPU_PRIM_TRIS: return GL_TRIANGLES;
		case GPU_PRIM_TRI_STRIP: return GL_TRIANGLE_STRIP;
		case GPU_PRIM_TRI_FAN: return GL_TRIANGLE_FAN;
		case GPU_PRIM_LINES_ADJ: return GL_LINES_ADJACENCY;
		case GPU_PRIM_LINE_STRIP_ADJ: return GL_LINE_STRIP_ADJACENCY;
		case GPU_PRIM_TRIS_ADJ: return GL_TRIANGLES_ADJACENCY;
	}
	return GL_INVALID_ENUM ;
}
