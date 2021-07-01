#include "../application/pear_file.h"
#include "gpu_shader.h"
#include "gpu_batch.h"
#include "gpu_texture.h"

#include <malloc.h>
#include <cstring>
#include <vector>
#include <cassert>

#include <string>
#include <fstream>
#include <streambuf>

#define GPU_SHADERFACE_NAME_HASH(n,l)	GPU_hash_string ( n , l , 256 - 'a' , 'a' - 1 ) ;

const char *GPU_builtin_uniform_name ( GPUUniformBuiltin u ) {
	switch ( u ) {
		case GPU_UNIFORM_NONE: return NULL;
		case GPU_UNIFORM_MODEL: return "ModelMatrix";
		case GPU_UNIFORM_VIEW: return "ViewMatrix";
		case GPU_UNIFORM_MODELVIEW: return "ModelViewMatrix";
		case GPU_UNIFORM_PROJECTION: return "ProjectionMatrix";
		case GPU_UNIFORM_VIEWPROJECTION: return "ViewProjectionMatrix";
		case GPU_UNIFORM_MVP: return "ModelViewProjectionMatrix";
		case GPU_UNIFORM_MODEL_INV: return "ModelMatrixInverse";
		case GPU_UNIFORM_VIEW_INV: return "ViewMatrixInverse";
		case GPU_UNIFORM_MODELVIEW_INV: return "ModelViewMatrixInverse";
		case GPU_UNIFORM_PROJECTION_INV: return "ProjectionMatrixInverse";
		case GPU_UNIFORM_VIEWPROJECTION_INV: return "ViewProjectionMatrixInverse";
		case GPU_UNIFORM_NORMAL: return "NormalMatrix";
		case GPU_UNIFORM_ORCO: return "OrcoTexCoFactors";
		case GPU_UNIFORM_CLIPPLANES: return "WorldClipPlanes";
		case GPU_UNIFORM_COLOR: return "color";
		case GPU_UNIFORM_BASE_INSTANCE: return "baseInstance";
		case GPU_UNIFORM_RESOURCE_CHUNK: return "resourceChunk";
		case GPU_UNIFORM_CUSTOM: return NULL;
		case GPU_NUM_UNIFORMS: return NULL;
	};
	return NULL ;
}

inline unsigned int GPU_get_max_ubo_name_len ( GLuint program ) {
	GLint data ;
	glGetProgramiv ( program , GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH , &data );
	if ( data <= 0 ) return 256 ;
	return data ;
}

inline unsigned int GPU_get_max_attr_name_len ( GLuint program ) {
	GLint data ;
	glGetProgramiv ( program , GL_ACTIVE_ATTRIBUTE_MAX_LENGTH , &data );
	if ( data <= 0 ) return 256 ;
	return data ;
}

void GPU_shader_input_set_name ( GPU_ShaderInterface *shaderface , GPU_ShaderInput *input , const char *name , unsigned int name_length ) {
	input->name_offset = shaderface->name_buffer_offset;
	input->name_hash = GPU_SHADERFACE_NAME_HASH ( name , name_length ) ;
	shaderface->name_buffer_offset += name_length + 1 ; /* include NULL terminator */
}

void GPU_shader_input_to_bucket ( GPU_ShaderInput *input , GPU_ShaderInput *buckets [ GPU_NUM_SHADERINTERFACE_BUCKETS ] ) {
	const unsigned int bucket_index = input->name_hash % GPU_NUM_SHADERINTERFACE_BUCKETS ;
	input->next = buckets [ bucket_index ] ;
	buckets [ bucket_index ] = input ;
}

void GPU_buckets_free ( GPU_ShaderInput *buckets [ GPU_NUM_SHADERINTERFACE_BUCKETS ] ) {
	for ( unsigned int bucket_index = 0; bucket_index < GPU_NUM_SHADERINTERFACE_BUCKETS; bucket_index++ ) {
		GPU_ShaderInput *input = buckets [ bucket_index ];
		while ( input != NULL ) {
			GPU_ShaderInput *input_next = input->next;
			delete input ;
			input = input_next;
		}
	}
}

const GPU_ShaderInput *GPU_shader_buckets_lookup ( GPU_ShaderInput *const buckets [ GPU_NUM_SHADERINTERFACE_BUCKETS ] , const char *name_buffer , const char *name ) {
	const unsigned int name_hash = GPU_SHADERFACE_NAME_HASH ( name , -1 ) ;
	const unsigned int bucket_index = name_hash % GPU_NUM_SHADERINTERFACE_BUCKETS ;
	const GPU_ShaderInput *input = buckets [ bucket_index ] ;
	if ( input == NULL ) {
		return NULL ;
	}
	const GPU_ShaderInput *next = input ;
	while ( next != NULL ) {
		input = next ;
		next = input->next ;
		if ( input->name_hash != name_hash ) {
			continue ;
		}
		if ( strcmp ( name_buffer + input->name_offset , name ) == 0 ) {
			return input ;
		}
	}
	return NULL ;
}

GPU_ShaderInterface *GPU_shaderinterface_create ( GLuint program ) {
	GPU_ShaderInterface *shaderface = new GPU_ShaderInterface ( ) ;

	GLint attr_len = 0 , ubo_len = 0 ;
	glGetProgramiv ( program , GL_ACTIVE_ATTRIBUTES , &attr_len );
	glGetProgramiv ( program , GL_ACTIVE_UNIFORM_BLOCKS , &ubo_len );
	shaderface->attributes_count = attr_len ;

	shaderface->program = program ;

	const unsigned int name_buffer_len = attr_len * GPU_get_max_attr_name_len ( program ) + ubo_len * GPU_get_max_ubo_name_len ( program ) ;
	shaderface->name_buffer = ( char * ) malloc ( name_buffer_len ) ;

	for ( unsigned int i = 0; i < attr_len ; i++ ) {
		GPU_ShaderInput *input = new GPU_ShaderInput ( ) ;
		GLsizei remaining_buffer = name_buffer_len - shaderface->name_buffer_offset ;
		char *name = shaderface->name_buffer + shaderface->name_buffer_offset ;
		GLsizei name_len = 0;

		glGetActiveAttrib ( program , i , remaining_buffer , &name_len , &input->size , &input->gl_type , name ) ;

		/* remove "[0]" from array name */
		if ( name [ name_len - 1 ] == ']' ) {
			name [ name_len - 3 ] = '\0';
			name_len -= 3;
		}

		/* TODO: reject DOUBLE gl_types */
		assert ( input->gl_type != GL_DOUBLE ) ;

		input->location = glGetAttribLocation ( program , name ) ;

		GPU_shader_input_set_name ( shaderface , input , name , name_len ) ;
		GPU_shader_input_to_bucket ( input , shaderface->attr_buckets );

#ifdef DEBUG_SHADER_INTERFACE
		printf ( "GPU_ShaderInterface %p, program %d, attr[] '%s' at location %d (size %d , type %s)\n" , shaderface , shaderface->program , name , input->location , input->size , OpenGL_GLSL_type_to_string ( input->gl_type ) );
#endif
	}

#if 0	// only load builtin uniforms
	/* Builtin Uniforms */
	for ( unsigned int u = GPU_UNIFORM_NONE + 1; u < GPU_UNIFORM_CUSTOM; u++ ) {
		const char *builtin_name = GPU_builtin_uniform_name ( (GPUUniformBuiltin) u );
		if ( glGetUniformLocation ( program , builtin_name ) != -1 ) {
			GPU_shaderinterface_add_uniform ( shaderface , builtin_name );
		}
	}
#else
	glGetProgramiv ( program , GL_ACTIVE_UNIFORMS , &shaderface->uniforms_count );

	for ( int u = 0 ; u < shaderface->uniforms_count ; u++ ) {
		GPU_shaderinterface_add_uniform ( shaderface , u ) ;
	}
#endif

	return shaderface ;
}

void GPU_shaderinterface_discard ( GPU_ShaderInterface *shaderface ) {
	GPU_buckets_free ( shaderface->attr_buckets ) ;
	GPU_buckets_free ( shaderface->uniform_buckets ) ;

	free ( shaderface->name_buffer ) ;

	delete shaderface ;
}

static bool setup_builtin_uniform ( GPU_ShaderInput *input , const char *name ) {
	/* detect built-in uniforms (name must match) */
	for ( unsigned int u = GPU_UNIFORM_NONE + 1; u < GPU_UNIFORM_CUSTOM; u++ ) {
		const char *builtin_name = GPU_builtin_uniform_name ( ( GPUUniformBuiltin ) u ) ;
		if ( strcmp ( name , builtin_name ) == 0 ) {
			input->builtin_type = ( GPUUniformBuiltin ) u ;
			return true;
		}
	}
	input->builtin_type = GPU_UNIFORM_CUSTOM;
	return false;
}

const GPU_ShaderInput *GPU_shaderinterface_add_uniform ( GPU_ShaderInterface *shaderface , const char *name ) {
	GPU_ShaderInput *input = new GPU_ShaderInput ( ) ;
	input->location = glGetUniformLocation ( shaderface->program , name ) ;
	int nameLength ;
	glGetActiveUniform ( shaderface->program , ( GLuint ) input->location , 0 , NULL , &input->size , &input->gl_type , NULL );

	const unsigned int name_len = strlen ( name ) ;

	if ( input->gl_type == 0 ) {
		char *u_name = new char [ 64 ] ;
		strcpy ( u_name , name ) ;
		*std::find ( u_name , u_name + 64 , '[' ) = '\0' ;
		int location = glGetUniformLocation ( shaderface->program , u_name ) ;
		glGetActiveUniform ( shaderface->program , location , 0 , NULL , &input->size , &input->gl_type , NULL );
	}

	shaderface->name_buffer = ( char * ) realloc ( shaderface->name_buffer , shaderface->name_buffer_offset + name_len + 1 ) ;
	strcpy ( shaderface->name_buffer + shaderface->name_buffer_offset , name ) ;

	GPU_shader_input_set_name ( shaderface , input , name , name_len ) ;
	setup_builtin_uniform ( input , name ) ;

	GPU_shader_input_to_bucket ( input , shaderface->uniform_buckets ) ;
	if ( input->builtin_type != GPU_UNIFORM_NONE && input->builtin_type != GPU_UNIFORM_CUSTOM ) {
		shaderface->builtin_uniforms [ input->builtin_type ] = input ;
	}

#ifdef DEBUG_SHADER_INTERFACE
	printf ( "GPU_ShaderInterface %p, program %d, uniform[] '%s' at location %d (size %d , type %s)\n" , shaderface , shaderface->program , name , input->location , input->size , OpenGL_GLSL_type_to_string ( input->gl_type ) );
#endif
	return input ;
}

const GPU_ShaderInput *GPU_shaderinterface_add_uniform ( GPU_ShaderInterface *shaderface , int ulocation ) {
	GPU_ShaderInput *input = new GPU_ShaderInput ( ) ;
	input->location = ulocation ;
	
	int szName ;
	char *uName = new char [ 64 ] ;
	glGetActiveUniform ( shaderface->program , ( GLuint ) input->location , 64 , &szName , &input->size , &input->gl_type , uName );

	shaderface->name_buffer = ( char * ) realloc ( shaderface->name_buffer , shaderface->name_buffer_offset + szName + 1 ) ;
	strcpy ( shaderface->name_buffer + shaderface->name_buffer_offset , uName ) ;

	GPU_shader_input_set_name ( shaderface , input , uName , szName ) ;
	setup_builtin_uniform ( input , uName ) ;

	GPU_shader_input_to_bucket ( input , shaderface->uniform_buckets ) ;
	if ( input->builtin_type != GPU_UNIFORM_NONE && input->builtin_type != GPU_UNIFORM_CUSTOM ) {
		shaderface->builtin_uniforms [ input->builtin_type ] = input ;
	}

#ifdef DEBUG_SHADER_INTERFACE
	printf ( "GPU_ShaderInterface %p, program %d, uniform[] '%s' at location %d (size %d , type %s)\n" , shaderface , shaderface->program , uName , input->location , input->size , OpenGL_GLSL_type_to_string ( input->gl_type ) );
#endif
	return input ;
}

const GPU_ShaderInput *GPU_shaderinterface_uniform ( const GPU_ShaderInterface *shaderface , const char *name ) {
	return GPU_shader_buckets_lookup ( shaderface->uniform_buckets , shaderface->name_buffer , name ) ;
}

const GPU_ShaderInput *GPU_shaderinterface_uniform ( const GPU_ShaderInterface *shaderface , int location ) {
	int szName ;
	char *uName = new char [ 64 ] ;
	glGetActiveUniform ( shaderface->program , ( GLuint ) location , 64 , &szName , NULL , NULL , uName );
	return GPU_shaderinterface_uniform ( shaderface , uName ) ;
}

const GPU_ShaderInput *GPU_shaderinterface_uniform_ensure ( const GPU_ShaderInterface *shaderface , const char *name ) {
	const GPU_ShaderInput *input = GPU_shaderinterface_uniform ( shaderface , name ) ;
	if ( input == NULL ) {
		input = GPU_shaderinterface_add_uniform ( ( GPU_ShaderInterface * ) shaderface , name ) ;
		if ( ( input->location == -1 ) ) {
			fprintf ( stderr , "GPU_ShaderInterface: Warning: Uniform '%s' not found!\n" , name );
		}
	}
	return ( input->location != -1 ) ? input : NULL ;
}

const GPU_ShaderInput *GPU_shaderinterface_uniform_builtin ( const GPU_ShaderInterface *shaderface , GPUUniformBuiltin builtin ) {
	assert ( builtin != GPU_UNIFORM_NONE );
	assert ( builtin != GPU_UNIFORM_CUSTOM );
	assert ( builtin != GPU_NUM_UNIFORMS );
	return shaderface->builtin_uniforms [ builtin ] ;
}

const GPU_ShaderInput *GPU_shaderinterface_attr ( const GPU_ShaderInterface *shaderface , const char *name ) {
	return GPU_shader_buckets_lookup ( shaderface->attr_buckets , shaderface->name_buffer , name ) ;
}

// GPU_Shader

GPU_Shader *GPU_shader_create ( void ) {
	return new GPU_Shader ( ) ;
}

void *GPU_shader_source_from_file ( const char *file , long long *length ) {
	return API_file_read ( file , length ) ;
}

bool GPU_shader_create_from_source ( GLuint shader , const char *code , const char *shader_type ) {
	glShaderSource ( shader , 1 , &code , NULL );
	glCompileShader ( shader );

	GLint result = GL_FALSE , logLength ;

	glGetShaderiv ( shader , GL_COMPILE_STATUS , &result );
	glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &logLength );
	std::vector<char> shaderError ( ( logLength > 1 ) ? logLength : 1 );
	glGetShaderInfoLog ( shader , logLength , NULL , &shaderError [ 0 ] );
	if ( result != GL_TRUE ) {
		fprintf ( stdout , "[%s] Shader Error :\n%s\n" , shader_type , &shaderError [ 0 ] ) ;
		fprintf ( stdout , "[%s] Shader Code :\n%s\n" , shader_type , code ) ;
	}
	return result == GL_TRUE ;
}

GPU_Shader *GPU_shader_load_from_file ( const char *vert , const char *frag , const char *geom ) {
	GPU_Shader *shader = new GPU_Shader ( ) ;
	shader->program = glCreateProgram ( ) ;

	bool compilation = true ;

	if ( vert ) {
		long long code_sz ;
		char *code = ( char * ) GPU_shader_source_from_file ( vert , &code_sz ) ;
		if ( code_sz <= 0 ) {
			fprintf ( stdout , "Failed to load shader file. (%s)\n" , vert ) ;
			glDeleteProgram ( shader->program ) ;
			return NULL ;
		}
		fprintf ( stdout , "Successfully loaded shader file. (%s)\n" , vert ) ;
		compilation = compilation & GPU_shader_create_from_source ( shader->vertex = glCreateShader ( GL_VERTEX_SHADER ) , code , "GLSL_VERTEX_SHADER" ) ;
		free ( code ) ;
	}
	if ( frag ) {
		long long code_sz ;
		char *code = ( char * ) GPU_shader_source_from_file ( frag , &code_sz ) ;
		if ( code_sz <= 0 ) {
			fprintf ( stdout , "Failed to load shader file. (%s)\n" , frag ) ;
			glDeleteProgram ( shader->program ) ;
			return NULL ;
		}
		fprintf ( stdout , "Successfully loaded shader file. (%s)\n" , frag ) ;
		compilation = compilation & GPU_shader_create_from_source ( shader->fragment = glCreateShader ( GL_FRAGMENT_SHADER ) , code , "GLSL_FRAGMENT_SHADER" ) ;
		free ( code ) ;
	}
	if ( geom ) {
		long long code_sz ;
		char *code = ( char * ) GPU_shader_source_from_file ( geom , &code_sz ) ;
		if ( code_sz <= 0 ) {
			fprintf ( stdout , "Failed to load shader file. (%s)\n" , geom ) ;
			glDeleteProgram ( shader->program ) ;
			return NULL ;
		}
		fprintf ( stdout , "Successfully loaded shader file. (%s)\n" , geom ) ;
		compilation = compilation & GPU_shader_create_from_source ( shader->geometry = glCreateShader ( GL_GEOMETRY_SHADER ) , code , "GLSL_GEOMETRY_SHADER" ) ;
		free ( code ) ;
	}

	if ( !compilation ) {
		fprintf ( stdout , "Shader Compilation Failed\n" ) ;
		return NULL ;
	}

	glAttachShader ( shader->program , shader->vertex );
	glAttachShader ( shader->program , shader->fragment );
	glAttachShader ( shader->program , shader->geometry );
	glLinkProgram ( shader->program );

	shader->shaderface = GPU_shaderinterface_create ( shader->program ) ;

	glFinish ( ) ;
	return shader ;
}

GPU_Shader *GPU_shader_load_from_binary ( const char *binary , const int binary_format , const int binary_len , const char *shname ) {
	int success;
	int program = glCreateProgram ( );

	glProgramBinary ( program , binary_format , binary , binary_len );
	glGetProgramiv ( program , GL_LINK_STATUS , &success );

	if ( success ) {
		GPU_Shader *shader = new GPU_Shader ( ) ;
		shader->shaderface = GPU_shaderinterface_create ( program );
		shader->program = program;

		return shader;
	}
	glDeleteProgram ( program ) ;
	return NULL ;
}

void GPU_shader_discard ( GPU_Shader *shader ) {
	glDeleteShader ( shader->vertex ) ;
	glDeleteShader ( shader->fragment ) ;
	glDeleteShader ( shader->geometry ) ;
	glDeleteProgram ( shader->program ) ;
}

void GPU_shader_use ( const GPU_Shader *shader ) {
	glUseProgram ( shader->program ) ;
}

void GPU_shader_uniform ( const GPU_Shader *shader , const GPU_ShaderInput *uniform , const void *data ) {
	if ( !uniform )
		return ;
	if ( uniform->location == -1 or data == NULL )
		return ;

	switch ( uniform->gl_type ) {
		case GL_FLOAT: {
			GPU_shader_uniform_vector ( shader , uniform->location , 1 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_INT: {
			GPU_shader_uniform_vector_int ( shader , uniform->location , 1 , uniform->size , ( const int * ) data ) ;
		}break;
		case GL_FLOAT_VEC2: {
			GPU_shader_uniform_vector ( shader , uniform->location , 2 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_FLOAT_VEC3: {
			GPU_shader_uniform_vector ( shader , uniform->location , 3 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_FLOAT_VEC4: {
			GPU_shader_uniform_vector ( shader , uniform->location , 4 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_INT_VEC2: {
			GPU_shader_uniform_vector_int ( shader , uniform->location , 2 , uniform->size , ( const int * ) data ) ;
		}break;
		case GL_INT_VEC3: {
			GPU_shader_uniform_vector_int ( shader , uniform->location , 3 , uniform->size , ( const int * ) data ) ;
		}break;
		case GL_INT_VEC4: {
			GPU_shader_uniform_vector_int ( shader , uniform->location , 4 , uniform->size , ( const int * ) data ) ;
		}break;
		case GL_FLOAT_MAT2: {
			GPU_shader_uniform_matrix ( shader , uniform->location , 2 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_FLOAT_MAT3: {
			GPU_shader_uniform_matrix ( shader , uniform->location , 3 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_FLOAT_MAT4: {
			GPU_shader_uniform_matrix ( shader , uniform->location , 4 , uniform->size , ( const float * ) data ) ;
		}break;
		case GL_SAMPLER_2D: {
			GPU_shader_uniform_int ( shader , uniform->location , *( const int * ) data ) ;
		}break;
		default: {
			printf ( "GPU_shader_uniform() does not have implementation for glsl type %d.\n" , uniform->gl_type ) ;
		}break;
	}
}

void GPU_shader_uniform_texture ( const GPU_Shader *shader , const GPU_ShaderInput *uniform , struct GPU_Texture *tex ) {
	int number = GPU_texture_bound_number ( tex );

	if ( number == -1 ) {
		fprintf ( stderr , "Texture is not bound.\n" );
		assert ( 0 );
		return;
	}

	if ( uniform->location == -1 or !uniform ) {
		return;
	}

	glUniform1i ( uniform->location , number );
}

void GPU_shader_uniform_vector ( const GPU_Shader *shader , int location , int length , int arraysize , const float *value ) {
	if ( location == -1 || value == NULL ) {
		return;
	}

	switch ( length ) {
		case 1:
			glUniform1fv ( location , arraysize , value );
			break;
		case 2:
			glUniform2fv ( location , arraysize , value );
			break;
		case 3:
			glUniform3fv ( location , arraysize , value );
			break;
		case 4:
			glUniform4fv ( location , arraysize , value );
			break;
		case 9:
			glUniformMatrix3fv ( location , arraysize , 0 , value );
			break;
		case 16:
			glUniformMatrix4fv ( location , arraysize , 0 , value );
			break;
		default:
			assert ( 0 );
			break;
	}
}

void GPU_shader_uniform_vector_int ( const GPU_Shader *shader , int location , int length , int arraysize , const int *value ) {
	if ( location == -1 ) {
		return;
	}

	switch ( length ) {
		case 1:
			glUniform1iv ( location , arraysize , value );
			break;
		case 2:
			glUniform2iv ( location , arraysize , value );
			break;
		case 3:
			glUniform3iv ( location , arraysize , value );
			break;
		case 4:
			glUniform4iv ( location , arraysize , value );
			break;
		default:
			assert ( 0 );
			break;
	}
}

void GPU_shader_uniform_matrix ( const GPU_Shader *shader , int location , int length , int arraysize , const float *value ) {
	if ( location == -1 ) {
		return;
	}

	switch ( length ) {
		case 2:
			glUniformMatrix2fv ( location , arraysize , GL_FALSE , value );
			break;
		case 3:
			glUniformMatrix3fv ( location , arraysize , GL_FALSE , value );
			break;
		case 4:
			glUniformMatrix4fv ( location , arraysize , GL_FALSE , value );
			break;
		default:
			assert ( 0 );
			break;
	}
}

void GPU_shader_uniform_int ( const GPU_Shader *shader , int location , int value ) {
	if ( location == -1 ) {
		return;
	}

	glUniform1i ( location , value );
}

int GPU_shader_get_attribute ( const GPU_Shader *shader , const char *name ) {
	assert ( shader && shader->program );
	const GPU_ShaderInput *attr = GPU_shaderinterface_attr ( shader->shaderface , name );
	return attr ? attr->location : -1;
}

char *GPU_shader_get_binary ( GPU_Shader *shader , unsigned int *r_binary_format , int *r_binary_len ) {
	assert ( GLEW_ARB_get_program_binary );
	char *r_binary;
	int binary_len = 0;

	glGetProgramiv ( shader->program , GL_PROGRAM_BINARY_LENGTH , &binary_len );
	r_binary = ( char * ) malloc ( binary_len ) ;
	glGetProgramBinary ( shader->program , binary_len , NULL , r_binary_format , r_binary );

	if ( r_binary_len ) {
		*r_binary_len = binary_len;
	}

	return r_binary;
}

void GPU_shaderinterface_add_batch_ref ( GPU_ShaderInterface *shaderface , GPU_Batch *batch ) {
	int i; /* find first unused slot */
	for ( i = 0; i < shaderface->batches_len; i++ ) {
		if ( shaderface->batches [ i ] == NULL ) {
			break;
		}
	}
	if ( i == shaderface->batches_len ) {
		/* Not enough place, realloc the array. */
		i = shaderface->batches_len;
		shaderface->batches_len += GPU_SHADERINTERFACE_REF_ALLOC_COUNT;
		shaderface->batches = ( GPU_Batch ** ) realloc ( shaderface->batches , sizeof ( GPU_Batch * ) * shaderface->batches_len ) ;
	}
	shaderface->batches [ i ] = batch;
}

void GPU_shaderinterface_remove_batch_ref ( GPU_ShaderInterface *shaderface , GPU_Batch *batch ) {
	for ( int i = 0; i < shaderface->batches_len; i++ ) {
		if ( shaderface->batches [ i ] == batch ) {
			shaderface->batches [ i ] = NULL;
			break; /* cannot have duplicates */
		}
	}
}

//

const char *OpenGL_GLSL_type_to_string ( GLenum type ) {
	switch ( type ) {
		case GL_FLOAT: return "GL_FLOAT" ;
		case GL_INT: return "GL_INT" ;
		case GL_BOOL: return "GL_BOOL" ;
		case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT" ;
		case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE" ;
		case GL_BYTE: return "GL_BYTE" ;
		case GL_FLOAT_VEC2: return "GL_FLOAT_VEC2" ;
		case GL_FLOAT_VEC3: return "GL_FLOAT_VEC3" ;
		case GL_FLOAT_VEC4: return "GL_FLOAT_VEC4" ;
		case GL_FLOAT_MAT2: return "GL_FLOAT_MAT2" ;
		case GL_FLOAT_MAT3: return "GL_FLOAT_MAT3" ;
		case GL_FLOAT_MAT4: return "GL_FLOAT_MAT4" ;
		case GL_INT_VEC2: return "GL_INT_VEC2" ;
		case GL_INT_VEC3: return "GL_INT_VEC3" ;
		case GL_INT_VEC4: return "GL_INT_VEC4" ;
		case GL_UNSIGNED_INT_VEC2: return "GL_UNSIGNED_INT_VEC2" ;
		case GL_UNSIGNED_INT_VEC3: return "GL_UNSIGNED_INT_VEC3" ;
		case GL_UNSIGNED_INT_VEC4: return "GL_UNSIGNED_INT_VEC4" ;
		case GL_SAMPLER_2D: return "GL_SAMPLER_2D" ;
	}
	printf ( "Cannot recognize enum %d\n" , type ) ;
	return "GL_INALID_ENUM" ;
}