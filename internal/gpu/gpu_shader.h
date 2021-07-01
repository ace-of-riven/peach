#pragma once

#include "gpu_init.h"

#define DEBUG_SHADER_INTERFACE

typedef enum {
	GPU_UNIFORM_NONE = 0 , /* uninitialized/unknown */

	GPU_UNIFORM_MODEL ,          /* mat4 ModelMatrix */
	GPU_UNIFORM_VIEW ,           /* mat4 ViewMatrix */
	GPU_UNIFORM_MODELVIEW ,      /* mat4 ModelViewMatrix */
	GPU_UNIFORM_PROJECTION ,     /* mat4 ProjectionMatrix */
	GPU_UNIFORM_VIEWPROJECTION , /* mat4 ViewProjectionMatrix */
	GPU_UNIFORM_MVP ,            /* mat4 ModelViewProjectionMatrix */

	GPU_UNIFORM_MODEL_INV ,          /* mat4 ModelMatrixInverse */
	GPU_UNIFORM_VIEW_INV ,           /* mat4 ViewMatrixInverse */
	GPU_UNIFORM_MODELVIEW_INV ,      /* mat4 ModelViewMatrixInverse */
	GPU_UNIFORM_PROJECTION_INV ,     /* mat4 ProjectionMatrixInverse */
	GPU_UNIFORM_VIEWPROJECTION_INV , /* mat4 ViewProjectionMatrixInverse */

	GPU_UNIFORM_NORMAL ,     /* mat3 NormalMatrix */
	GPU_UNIFORM_ORCO ,       /* vec4 OrcoTexCoFactors[] */
	GPU_UNIFORM_CLIPPLANES , /* vec4 WorldClipPlanes[] */

	GPU_UNIFORM_COLOR ,          /* vec4 color */
	GPU_UNIFORM_BASE_INSTANCE ,  /* int baseInstance */
	GPU_UNIFORM_RESOURCE_CHUNK , /* int resourceChunk */

	GPU_UNIFORM_CUSTOM , /* custom uniform, not one of the above built-ins */

	GPU_NUM_UNIFORMS , /* Special value, denotes number of builtin uniforms. */
} GPUUniformBuiltin;

struct GPU_ShaderInput {
	GPU_ShaderInput *next ;

	GLuint location ;
	GLenum gl_type ;
	unsigned int name_offset ;
	unsigned int name_hash ;

	GPUUniformBuiltin builtin_type ;
	GLsizei size ;
};

#define GPU_NUM_SHADERINTERFACE_BUCKETS 257
#define GPU_SHADERINTERFACE_REF_ALLOC_COUNT 16

struct GPU_ShaderInterface {
	GPU_ShaderInput *builtin_uniforms [ GPU_NUM_UNIFORMS ] ;
	GPU_ShaderInput *uniform_buckets [ GPU_NUM_SHADERINTERFACE_BUCKETS ] ;
	GPU_ShaderInput *attr_buckets [ GPU_NUM_SHADERINTERFACE_BUCKETS ] ;
	GLuint program ;
	unsigned int batches_len ;
	int uniforms_count ;
	int attributes_count ;
	struct GPU_Batch **batches ;
	char *name_buffer ;
	unsigned int name_buffer_offset ;
};

GPU_ShaderInterface *GPU_shaderinterface_create ( GLuint program_id );
void GPU_shaderinterface_discard ( GPU_ShaderInterface *shaderface ) ;

const GPU_ShaderInput *GPU_shaderinterface_add_uniform ( GPU_ShaderInterface *shaderface , const char *name ) ;
const GPU_ShaderInput *GPU_shaderinterface_add_uniform ( GPU_ShaderInterface *shaderface , int ulocation ) ;

const GPU_ShaderInput *GPU_shaderinterface_uniform ( const GPU_ShaderInterface *shaderface , const char *name ) ;
const GPU_ShaderInput *GPU_shaderinterface_uniform ( const GPU_ShaderInterface *shaderface , int location ) ;
const GPU_ShaderInput *GPU_shaderinterface_uniform_ensure ( const GPU_ShaderInterface *shaderface , const char *name ) ;
const GPU_ShaderInput *GPU_shaderinterface_uniform_builtin ( const GPU_ShaderInterface *shaderface , GPUUniformBuiltin builtin ) ;

const GPU_ShaderInput *GPU_shaderinterface_attr ( const GPU_ShaderInterface *shaderface , const char *name ) ;

struct GPU_Shader {
	/** Handle for full program (links shader stages below). */
	GLuint program;

	/** Handle for vertex shader. */
	GLuint vertex;
	/** Handle for geometry shader. */
	GLuint geometry;
	/** Handle for fragment shader. */
	GLuint fragment;

	GPU_ShaderInterface *shaderface ;
};

GPU_Shader *GPU_shader_create ( void ) ;

GPU_Shader *GPU_shader_load_from_file ( const char *vert , const char *frag , const char *geom ) ;
GPU_Shader *GPU_shader_load_from_binary ( const char *binary , const int binary_format , const int binary_len , const char *shname ) ;

void GPU_shader_discard ( GPU_Shader *shader ) ;

void GPU_shader_uniform ( const GPU_Shader *shader , const GPU_ShaderInput *uniform , const void *data ) ;
void GPU_shader_uniform_vector ( const GPU_Shader *shader , int location , int length , int arraysize , const float *value ) ;
void GPU_shader_uniform_vector_int ( const GPU_Shader *shader , int location , int length , int arraysize , const int *value ) ;
void GPU_shader_uniform_int ( const GPU_Shader *shader , int location , int value ) ;
void GPU_shader_uniform_matrix ( const GPU_Shader *shader , int location , int length , int arraysize , const float *value ) ;
// void GPU_shader_uniform_buffer ( GPU_Shader *shader , int location , struct GPU_UniformBuffer *ubo ) ;
void GPU_shader_uniform_texture ( const GPU_Shader *shader , const GPU_ShaderInput *uniform , struct GPU_Texture *tex ) ;
int GPU_shader_get_attribute ( const GPU_Shader *shader , const char *name ) ;
char *GPU_shader_get_binary ( const GPU_Shader *shader , unsigned int *r_binary_format , int *r_binary_len ) ;

void GPU_shader_use ( const GPU_Shader *shader ) ;

void GPU_shaderinterface_add_batch_ref ( GPU_ShaderInterface *shaderface , struct GPU_Batch *batch ) ;
void GPU_shaderinterface_remove_batch_ref ( GPU_ShaderInterface *shaderface , struct GPU_Batch *batch ) ;

const char *GPU_builtin_uniform_name ( GPUUniformBuiltin u ) ;
const char *OpenGL_GLSL_type_to_string ( GLenum type ) ;
