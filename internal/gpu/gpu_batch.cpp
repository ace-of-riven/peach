#include "gpu_batch.h"
#include "gpu_matrix.h"

static void batch_update_program_bindings ( GPU_Batch *batch , unsigned int i_first );

void GPU_batch_vao_cache_clear ( GPU_Batch *batch )
{
	if ( batch->context == NULL ) {
		return;
	}
	if ( batch->is_dynamic_vao_count ) {
		for ( int i = 0; i < batch->dynamic_vaos.count; i++ ) {
			if ( batch->dynamic_vaos.vao_ids [ i ] ) {
				GPU_vao_free ( batch->dynamic_vaos.vao_ids [ i ] , batch->context );
			}
			if ( batch->dynamic_vaos.interfaces [ i ] ) {
				GPU_shaderinterface_remove_batch_ref ( ( GPU_ShaderInterface * ) batch->dynamic_vaos.interfaces [ i ] , batch );
			}
		}
		free ( ( void * ) batch->dynamic_vaos.interfaces );
		free ( batch->dynamic_vaos.vao_ids );
	}
	else {
		for ( int i = 0; i < GPU_BATCH_VAO_STATIC_LEN; i++ ) {
			if ( batch->static_vaos.vao_ids [ i ] ) {
				GPU_vao_free ( batch->static_vaos.vao_ids [ i ] , batch->context );
			}
			if ( batch->static_vaos.interfaces [ i ] ) {
				GPU_shaderinterface_remove_batch_ref ( ( GPU_ShaderInterface * ) batch->static_vaos.interfaces [ i ] , batch );
			}
		}
	}
	batch->is_dynamic_vao_count = false;
	for ( int i = 0; i < GPU_BATCH_VAO_STATIC_LEN; i++ ) {
		batch->static_vaos.vao_ids [ i ] = 0;
		batch->static_vaos.interfaces [ i ] = NULL;
	}
	gpu_context_remove_batch ( batch->context , batch );
	batch->context = NULL;
}

GPU_Batch *GPU_batch_create_ex ( GPUPrimType prim_type , GPU_VertBuf *verts , GPU_IndexBuf *elem , unsigned int owns_flag ) {
	GPU_Batch *batch = new GPU_Batch ( ) ;
	GPU_batch_init_ex ( batch , prim_type , verts , elem , owns_flag );
	return batch;
}

void GPU_batch_init_ex ( GPU_Batch *batch , GPUPrimType prim_type , GPU_VertBuf *verts , GPU_IndexBuf *elem , unsigned int owns_flag ) {
	batch->verts [ 0 ] = verts;
	for ( int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
		batch->verts [ v ] = NULL;
	}
	for ( int v = 0; v < GPU_BATCH_INST_VBO_MAX_LEN; v++ ) {
		batch->inst [ v ] = NULL;
	}
	batch->elem = elem;
	batch->gl_prim_type = convert_prim_type_to_gl ( prim_type );
	batch->phase = GPU_BATCH_READY_TO_DRAW;
	batch->is_dynamic_vao_count = false;
	batch->owns_flag = owns_flag;
	batch->free_callback = NULL;
}

/* This will share the VBOs with the new batch. */
void GPU_batch_copy ( GPU_Batch *batch_dst , GPU_Batch *batch_src ) {
	GPU_batch_init_ex ( batch_dst , GPU_PRIM_POINTS , batch_src->verts [ 0 ] , batch_src->elem , 0 );

	batch_dst->gl_prim_type = batch_src->gl_prim_type;
	for ( int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
		batch_dst->verts [ v ] = batch_src->verts [ v ];
	}
}

void GPU_batch_clear ( GPU_Batch *batch )
{
	if ( batch->owns_flag & GPU_BATCH_OWNS_INDEX ) {
		GPU_indexbuf_discard ( batch->elem );
	}
	if ( batch->owns_flag & GPU_BATCH_OWNS_INSTANCES ) {
		GPU_vertbuf_discard ( batch->inst [ 0 ] );
		if ( batch->inst [ 1 ] )
			GPU_vertbuf_discard ( batch->inst [ 1 ] ) ;
	}
	if ( ( batch->owns_flag & ~GPU_BATCH_OWNS_INDEX ) != 0 ) {
		for ( int v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
			if ( batch->verts [ v ] == NULL )
				break;
			if ( batch->owns_flag & ( 1 << v ) )
				GPU_vertbuf_discard ( batch->verts [ v ] );
		}
	}
	GPU_batch_vao_cache_clear ( batch );
	batch->phase = GPU_BATCH_UNUSED;
}

void GPU_batch_discard ( GPU_Batch *batch ) {
	if ( batch->free_callback ) {
		batch->free_callback ( batch , batch->callback_data );
	}

	GPU_batch_clear ( batch );
	free ( batch );
}

void GPU_batch_callback_free_set ( GPU_Batch *batch , void ( *callback )( GPU_Batch * , void * ) , void *user_data ) {
	batch->free_callback = callback;
	batch->callback_data = user_data;
}

void GPU_batch_instbuf_set ( GPU_Batch *batch , GPU_VertBuf *inst , bool own_vbo )
{
#ifdef TRUST_NO_ONE
	assert ( inst != NULL );
#endif
	/* redo the bindings */
	GPU_batch_vao_cache_clear ( batch );

	if ( batch->inst [ 0 ] != NULL && ( batch->owns_flag & GPU_BATCH_OWNS_INSTANCES ) ) {
		GPU_vertbuf_discard ( batch->inst [ 0 ] );
		if ( batch->inst [ 1 ] )
			GPU_vertbuf_discard ( batch->inst [ 1 ] );
	}
	batch->inst [ 0 ] = inst;

	if ( own_vbo ) {
		batch->owns_flag |= GPU_BATCH_OWNS_INSTANCES;
	}
	else {
		batch->owns_flag &= ~GPU_BATCH_OWNS_INSTANCES;
	}
}

void GPU_batch_elembuf_set ( GPU_Batch *batch , GPU_IndexBuf *elem , bool own_ibo )
{
	assert ( elem != NULL );
	/* redo the bindings */
	GPU_batch_vao_cache_clear ( batch );

	if ( batch->elem != NULL && ( batch->owns_flag & GPU_BATCH_OWNS_INDEX ) ) {
		GPU_indexbuf_discard ( batch->elem );
	}
	batch->elem = elem;

	if ( own_ibo ) {
		batch->owns_flag |= GPU_BATCH_OWNS_INDEX;
	}
	else {
		batch->owns_flag &= ~GPU_BATCH_OWNS_INDEX;
	}
}

/* A bit of a quick hack. Should be streamlined as the vbos handling */
int GPU_batch_instbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *insts , bool own_vbo )
{
	/* redo the bindings */
	GPU_batch_vao_cache_clear ( batch );

	for ( unsigned int v = 0; v < GPU_BATCH_INST_VBO_MAX_LEN; v++ ) {
		if ( batch->inst [ v ] == NULL ) {
			batch->inst [ v ] = insts;
			if ( own_vbo ) {
				batch->owns_flag |= GPU_BATCH_OWNS_INSTANCES;
			}
			return v;
		}
	}

	/* we only make it this far if there is no room for another GPUVertBuf */
	return -1;
}

/* Returns the index of verts in the batch. */
int GPU_batch_vertbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *verts , bool own_vbo )
{
	/* redo the bindings */
	GPU_batch_vao_cache_clear ( batch );

	for ( unsigned int v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
		if ( batch->verts [ v ] == NULL ) {
			batch->verts [ v ] = verts;
			/* TODO: mark dirty so we can keep attribute bindings up-to-date */
			if ( own_vbo ) {
				batch->owns_flag |= ( 1 << v );
			}
			return v;
		}
	}

	/* we only make it this far if there is no room for another GPUVertBuf */
#ifdef TRUST_NO_ONE
	assert ( false );
#endif
	return -1;
}

static GLuint batch_vao_get ( GPU_Batch *batch )
{
	/* Search through cache */
	if ( batch->is_dynamic_vao_count ) {
		for ( int i = 0; i < batch->dynamic_vaos.count; i++ ) {
			if ( batch->dynamic_vaos.interfaces [ i ] == batch->shinterface ) {
				return batch->dynamic_vaos.vao_ids [ i ];
			}
		}
	}
	else {
		for ( int i = 0; i < GPU_BATCH_VAO_STATIC_LEN; i++ ) {
			if ( batch->static_vaos.interfaces [ i ] == batch->shinterface ) {
				return batch->static_vaos.vao_ids [ i ];
			}
		}
	}

	/* Set context of this batch.
	 * It will be bound to it until GPU_batch_vao_cache_clear is called.
	 * Until then it can only be drawn with this context. */
	if ( batch->context == NULL ) {
		batch->context = GPU_context_active_get ( );
		gpu_context_add_batch ( batch->context , batch );
	}

	/* Cache miss, time to add a new entry! */
	GLuint new_vao = 0;
	if ( !batch->is_dynamic_vao_count ) {
		int i; /* find first unused slot */
		for ( i = 0; i < GPU_BATCH_VAO_STATIC_LEN; i++ ) {
			if ( batch->static_vaos.vao_ids [ i ] == 0 ) {
				break;
			}
		}

		if ( i < GPU_BATCH_VAO_STATIC_LEN ) {
			batch->static_vaos.interfaces [ i ] = batch->shinterface;
			batch->static_vaos.vao_ids [ i ] = new_vao = GPU_vao_alloc ( );
		}
		else {
			/* Not enough place switch to dynamic. */
			batch->is_dynamic_vao_count = true;
			/* Erase previous entries, they will be added back if drawn again. */
			for ( int j = 0; j < GPU_BATCH_VAO_STATIC_LEN; j++ ) {
				GPU_shaderinterface_remove_batch_ref ( ( GPU_ShaderInterface * ) batch->static_vaos.interfaces [ j ] , batch );
				GPU_vao_free ( batch->static_vaos.vao_ids [ j ] , batch->context );
			}
			/* Init dynamic arrays and let the branch below set the values. */
			batch->dynamic_vaos.count = GPU_BATCH_VAO_DYN_ALLOC_COUNT;
			batch->dynamic_vaos.interfaces = ( const GPU_ShaderInterface ** ) malloc ( batch->dynamic_vaos.count * sizeof ( GPU_ShaderInterface * ) ) ;
			batch->dynamic_vaos.vao_ids = ( GLuint * ) malloc ( batch->dynamic_vaos.count * sizeof ( GLuint ) ) ;
		}
	}

	if ( batch->is_dynamic_vao_count ) {
		int i; /* find first unused slot */
		for ( i = 0; i < batch->dynamic_vaos.count; i++ ) {
			if ( batch->dynamic_vaos.vao_ids [ i ] == 0 ) {
				break;
			}
		}

		if ( i == batch->dynamic_vaos.count ) {
			/* Not enough place, realloc the array. */
			i = batch->dynamic_vaos.count;
			batch->dynamic_vaos.count += GPU_BATCH_VAO_DYN_ALLOC_COUNT;
			batch->dynamic_vaos.interfaces = ( const GPU_ShaderInterface ** ) realloc ( ( void * ) batch->dynamic_vaos.interfaces , sizeof ( GPU_ShaderInterface * ) * batch->dynamic_vaos.count );
			batch->dynamic_vaos.vao_ids = ( GLuint * ) realloc ( batch->dynamic_vaos.vao_ids , sizeof ( GLuint ) * batch->dynamic_vaos.count );
		}
		batch->dynamic_vaos.interfaces [ i ] = batch->shinterface;
		batch->dynamic_vaos.vao_ids [ i ] = new_vao = GPU_vao_alloc ( );
	}

	GPU_shaderinterface_add_batch_ref ( ( GPU_ShaderInterface * ) batch->shinterface , batch );

	/* We just got a fresh VAO we need to initialize it. */
	glBindVertexArray ( new_vao );
	batch_update_program_bindings ( batch , 0 );
	glBindVertexArray ( 0 );

	return new_vao;
}

void GPU_batch_program_set_no_use ( GPU_Batch *batch , GLuint program , const GPU_ShaderInterface *shaderface ) {
	batch->shinterface = shaderface;
	batch->program = program;
	batch->vao_id = batch_vao_get ( batch );
}

void GPU_batch_program_set ( GPU_Batch *batch , uint32_t program , const GPU_ShaderInterface *shaderface ) {
	GPU_batch_program_set_no_use ( batch , program , shaderface );
	GPU_batch_program_use_begin ( batch ); /* hack! to make Batch_Uniform* simpler */
}

void gpu_batch_remove_interface_ref ( GPU_Batch *batch , const GPU_ShaderInterface *interface ) {
	if ( batch->is_dynamic_vao_count ) {
		for ( int i = 0; i < batch->dynamic_vaos.count; i++ ) {
			if ( batch->dynamic_vaos.interfaces [ i ] == interface ) {
				GPU_vao_free ( batch->dynamic_vaos.vao_ids [ i ] , batch->context );
				batch->dynamic_vaos.vao_ids [ i ] = 0;
				batch->dynamic_vaos.interfaces [ i ] = NULL;
				break; /* cannot have duplicates */
			}
		}
	}
	else {
		int i;
		for ( i = 0; i < GPU_BATCH_VAO_STATIC_LEN; i++ ) {
			if ( batch->static_vaos.interfaces [ i ] == interface ) {
				GPU_vao_free ( batch->static_vaos.vao_ids [ i ] , batch->context );
				batch->static_vaos.vao_ids [ i ] = 0;
				batch->static_vaos.interfaces [ i ] = NULL;
				break; /* cannot have duplicates */
			}
		}
	}
}

static void create_bindings ( GPU_VertBuf *verts , const GPU_ShaderInterface *interface , unsigned int v_first , const bool use_instancing ) {
	const GPU_VertFormat *format = &verts->format;

	const unsigned int attr_len = format->attr_len;
	unsigned int stride = format->stride;
	unsigned int offset = 0;

	GPU_vertbuf_use ( verts );

	for ( unsigned int a_idx = 0; a_idx < attr_len; a_idx++ ) {
		const GPU_VertAttr *a = &format->attrs [ a_idx ];

		if ( format->deinterleaved ) {
			offset += ( ( a_idx == 0 ) ? 0 : format->attrs [ a_idx - 1 ].sz ) * verts->vertex_len;
			stride = a->sz;
		}
		else {
			offset = a->offset;
		}

		const GLvoid *pointer = ( const GLubyte * ) 0 + offset + v_first * stride;

		for ( unsigned int n_idx = 0; n_idx < a->name_len; n_idx++ ) {
			const char *name = GPU_vertformat_attr_name_get ( format , a , n_idx );
			const GPU_ShaderInput *input = GPU_shaderinterface_attr ( interface , name );

			if ( input == NULL ) {
				continue;
			}

			if ( a->comp_len == 16 || a->comp_len == 12 || a->comp_len == 8 ) {
				for ( int i = 0; i < a->comp_len / 4; i++ ) {
					glEnableVertexAttribArray ( input->location + i );
					glVertexAttribDivisor ( input->location + i , ( use_instancing ) ? 1 : 0 );
					glVertexAttribPointer ( input->location + i , 4 , a->gl_comp_type , GL_FALSE , stride , ( const GLubyte * ) pointer + i * 16 );
				}
			}
			else {
				glEnableVertexAttribArray ( input->location );
				glVertexAttribDivisor ( input->location , ( use_instancing ) ? 1 : 0 );

				switch ( a->fetch_mode ) {
					case GPU_FETCH_FLOAT:
					case GPU_FETCH_INT_TO_FLOAT:
						glVertexAttribPointer ( input->location , a->comp_len , a->gl_comp_type , GL_FALSE , stride , pointer );
						break;
					case GPU_FETCH_INT_TO_FLOAT_UNIT:
						glVertexAttribPointer ( input->location , a->comp_len , a->gl_comp_type , GL_TRUE , stride , pointer );
						break;
					case GPU_FETCH_INT:
						glVertexAttribIPointer ( input->location , a->comp_len , a->gl_comp_type , stride , pointer );
						break;
				}
			}
		}
	}
}

static void batch_update_program_bindings ( GPU_Batch *batch , unsigned int i_first ) {
	/* Reverse order so first vbos have more prevalence (in term of attrib override). */
	for ( int v = GPU_BATCH_VBO_MAX_LEN - 1; v > -1; v-- ) {
		if ( batch->verts [ v ] != NULL ) {
			create_bindings ( batch->verts [ v ] , batch->shinterface , 0 , false );
		}
	}
	for ( int v = GPU_BATCH_INST_VBO_MAX_LEN - 1; v > -1; v-- ) {
		if ( batch->inst [ v ] ) {
			create_bindings ( batch->inst [ v ] , batch->shinterface , i_first , true );
		}
	}
	if ( batch->elem ) {
		GPU_indexbuf_use ( batch->elem );
	}
}

void GPU_batch_program_use_begin ( GPU_Batch *batch ) {
	/* NOTE: use_program & done_using_program are fragile, depend on staying in sync with
	 *       the GL context's active program.
	 *       use_program doesn't mark other programs as "not used". */
	 /* TODO: make not fragile (somehow) */

	if ( !batch->program_in_use ) {
		glUseProgram ( batch->program );
		batch->program_in_use = true;
	}
}

void GPU_batch_program_use_end ( GPU_Batch *batch )
{
	if ( batch->program_in_use ) {
#ifdef PROGRAM_NO_OPTI
		glUseProgram ( 0 );
#endif
		batch->program_in_use = false;
	}
}

#define GET_UNIFORM	const GPU_ShaderInput *uniform = GPU_shaderinterface_uniform_ensure(batch->shinterface, name)

void GPU_batch_uniform_1ui ( GPU_Batch *batch , const char *name , unsigned int value ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform1ui ( uniform->location , value );
}

void GPU_batch_uniform_1i ( GPU_Batch *batch , const char *name , int value ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform1i ( uniform->location , value );
}

void GPU_batch_uniform_1iv ( GPU_Batch *batch , const char *name , int count , int *value ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform1iv ( uniform->location , count , value );
}

void GPU_batch_uniform_1b ( GPU_Batch *batch , const char *name , bool value ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform1i ( uniform->location , value ? GL_TRUE : GL_FALSE );
}

void GPU_batch_uniform_2f ( GPU_Batch *batch , const char *name , float x , float y ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform2f ( uniform->location , x , y );
}

void GPU_batch_uniform_3f ( GPU_Batch *batch , const char *name , float x , float y , float z ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform3f ( uniform->location , x , y , z );
}

void GPU_batch_uniform_4f ( GPU_Batch *batch , const char *name , float x , float y , float z , float w ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform4f ( uniform->location , x , y , z , w );
}

void GPU_batch_uniform_1f ( GPU_Batch *batch , const char *name , float x ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform1f ( uniform->location , x );
}

void GPU_batch_uniform_2fv ( GPU_Batch *batch , const char *name , const float data [ 2 ] ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform2fv ( uniform->location , 1 , data );
}

void GPU_batch_uniform_3fv ( GPU_Batch *batch , const char *name , const float data [ 3 ] ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform3fv ( uniform->location , 1 , data );
}

void GPU_batch_uniform_4fv ( GPU_Batch *batch , const char *name , const float data [ 4 ] ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform4fv ( uniform->location , 1 , data );
}

void GPU_batch_uniform_2fv_array ( GPU_Batch *batch , const char *name , const int len , const float *data ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform2fv ( uniform->location , len , data );
}

void GPU_batch_uniform_4fv_array ( GPU_Batch *batch , const char *name , const int len , const float *data ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniform4fv ( uniform->location , len , data );
}

void GPU_batch_uniform_mat4 ( GPU_Batch *batch , const char *name , const float data [ 4 ][ 4 ] ) {
	GET_UNIFORM ;
	if ( !uniform or uniform->location == -1 ) return ;
	glUniformMatrix4fv ( uniform->location , 1 , GL_FALSE , ( const float * ) data );
}

static void *elem_offset ( const GPU_IndexBuf *el , int v_first )
{
#ifdef GPU_TRACK_INDEX_RANGE
	if ( el->index_type == GPU_INDEX_U16 ) {
		return ( GLushort * ) 0 + v_first + el->index_start;
	}
#endif
	return ( GLuint * ) 0 + v_first + el->index_start;
}

/* Use when drawing with GPU_batch_draw_advanced */
void GPU_batch_bind ( GPU_Batch *batch )
{
	glBindVertexArray ( batch->vao_id );

#ifdef GPU_TRACK_INDEX_RANGE
	/* Can be removed if GL 4.3 is required. */
	if ( !GLEW_ARB_ES3_compatibility && batch->elem != NULL ) {
		GLuint restart_index = ( batch->elem->index_type == GPU_INDEX_U16 ) ? ( GLuint ) 0xFFFF :
			( GLuint ) 0xFFFFFFFF;
		glPrimitiveRestartIndex ( restart_index );
	}
#endif
}

void GPU_batch_draw ( GPU_Batch *batch )
{
#ifdef TRUST_NO_ONE
	assert ( batch->phase == GPU_BATCH_READY_TO_DRAW );
	assert ( batch->verts [ 0 ]->vbo_id != 0 );
#endif
	GPU_batch_program_use_begin ( batch );
	GPU_matrix_bind ( batch->shinterface );  // external call.

	GPU_batch_bind ( batch );
	GPU_batch_draw_advanced ( batch , 0 , 0 , 0 , 0 );

	GPU_batch_program_use_end ( batch );
}

#ifdef GPU_TRACK_INDEX_RANGE
#  define BASE_INDEX(el) ((el)->base_index)
#  define INDEX_TYPE(el) ((el)->gl_index_type)
#else
#  define BASE_INDEX(el) 0
#  define INDEX_TYPE(el) GL_UNSIGNED_INT
#endif

void GPU_batch_draw_advanced ( GPU_Batch *batch , int v_first , int v_count , int i_first , int i_count )
{
	assert ( batch->program_in_use );
	/* TODO could assert that VAO is bound. */

	if ( v_count == 0 ) {
		v_count = ( batch->elem ) ? batch->elem->index_len : batch->verts [ 0 ]->vertex_len;
	}
	if ( i_count == 0 ) {
		i_count = ( batch->inst [ 0 ] ) ? batch->inst [ 0 ]->vertex_len : 1;
		/* Meh. This is to be able to use different numbers of verts in instance vbos. */
		if ( batch->inst [ 1 ] && i_count > batch->inst [ 1 ]->vertex_len ) {
			i_count = batch->inst [ 1 ]->vertex_len;
		}
	}

	if ( v_count == 0 || i_count == 0 ) {
		/* Nothing to draw. */
		return;
	}

	/* Verify there is enough data do draw. */
	/* TODO(fclem) Nice to have but this is invalid when using procedural drawcalls.
	 * The right assert would be to check if there is an enabled attrib from each VBO
	 * and check their length. */
	 // assert(i_first + i_count <= (batch->inst ? batch->inst->vertex_len : INT_MAX));
	 // assert(v_first + v_count <=
	 //            (batch->elem ? batch->elem->index_len : batch->verts[0]->vertex_len));

#ifdef __APPLE__
	GLuint vao = 0;
#endif

	if ( !GPU_arb_base_instance_is_supported ( ) ) {
		if ( i_first > 0 ) {
#ifdef __APPLE__
			/**
			 * There seems to be a nasty bug when drawing using the same VAO reconfiguring. (see T71147)
			 * We just use a throwaway VAO for that. Note that this is likely to degrade performance.
			 **/
			glGenVertexArrays ( 1 , &vao );
			glBindVertexArray ( vao );
#else
			/* If using offset drawing with instancing, we must
			 * use the default VAO and redo bindings. */
			glBindVertexArray ( GPU_vao_default ( ) );
#endif
			batch_update_program_bindings ( batch , i_first );
		}
		else {
			/* Previous call could have bind the default vao
			 * see above. */
			glBindVertexArray ( batch->vao_id );

			batch_update_program_bindings ( batch , i_first );
		}
	}

	if ( batch->elem ) {
		const GPU_IndexBuf *el = batch->elem;
		GLenum index_type = INDEX_TYPE ( el );
		GLint base_index = BASE_INDEX ( el );
		void *v_first_ofs = elem_offset ( el , v_first );

		if ( GPU_arb_base_instance_is_supported ( ) ) {
			glDrawElementsInstancedBaseVertexBaseInstance (
				batch->gl_prim_type , v_count , index_type , v_first_ofs , i_count , base_index , i_first );
		}
		else {
			glDrawElementsInstancedBaseVertex (
				batch->gl_prim_type , v_count , index_type , v_first_ofs , i_count , base_index );
		}
	}
	else {
#ifdef __APPLE__
		glDisable ( GL_PRIMITIVE_RESTART );
#endif
		if ( GPU_arb_base_instance_is_supported ( ) ) {
			glDrawArraysInstancedBaseInstance ( batch->gl_prim_type , v_first , v_count , i_count , i_first );
		}
		else {
			glDrawArraysInstanced ( batch->gl_prim_type , v_first , v_count , i_count );
		}
#ifdef __APPLE__
		glEnable ( GL_PRIMITIVE_RESTART );
#endif
	}

#ifdef __APPLE__
	if ( vao != 0 ) {
		glDeleteVertexArrays ( 1 , &vao );
	}
#endif
}

/* just draw some vertices and let shader place them where we want. */
void GPU_draw_primitive ( GPUPrimType prim_type , int v_count )
{
	/* we cannot draw without vao ... annoying ... */
	glBindVertexArray ( GPU_vao_default ( ) );

	GLenum type = convert_prim_type_to_gl ( prim_type );
	glDrawArrays ( type , 0 , v_count );

	/* Performance hog if you are drawing with the same vao multiple time.
	 * Only activate for debugging.*/
	 // glBindVertexArray(0);
}

/* -------------------------------------------------------------------- */
/** \name Indirect Draw Calls
 * \{ */

#define USE_MULTI_DRAW_INDIRECT		(GL_ARB_multi_draw_indirect && GPU_arb_base_instance_is_supported() && !GPU_type_matches(GPU_DEVICE_NVIDIA, GPU_OS_ANY, GPU_DRIVER_OFFICIAL))

typedef struct GPUDrawCommand {
	unsigned int v_count;
	unsigned int i_count;
	unsigned int v_first;
	unsigned int i_first;
} GPUDrawCommand;

typedef struct GPUDrawCommandIndexed {
	unsigned int v_count;
	unsigned int i_count;
	unsigned int v_first;
	unsigned int base_index;
	unsigned int i_first;
} GPUDrawCommandIndexed;

struct GPUDrawList {
	GPU_Batch *batch;
	unsigned int base_index;  /* Avoid dereferencing batch. */
	unsigned int cmd_offset;  /* in bytes, offset  inside indirect command buffer. */
	unsigned int cmd_len;     /* Number of used command for the next call. */
	unsigned int buffer_size; /* in bytes, size of indirect command buffer. */
	GLuint buffer_id; /* Draw Indirect Buffer id */
	union {
		GPUDrawCommand *commands;
		GPUDrawCommandIndexed *commands_indexed;
	};
};

GPUDrawList *GPU_draw_list_create ( int length )
{
	GPUDrawList *list = new GPUDrawList ( ) ;
	/* Alloc the biggest possible command list which is indexed. */
	list->buffer_size = sizeof ( GPUDrawCommandIndexed ) * length;
	if ( USE_MULTI_DRAW_INDIRECT ) {
		list->buffer_id = GPU_buf_alloc ( );
		glBindBuffer ( GL_DRAW_INDIRECT_BUFFER , list->buffer_id );
		glBufferData ( GL_DRAW_INDIRECT_BUFFER , list->buffer_size , NULL , GL_DYNAMIC_DRAW );
	}
	else {
		list->commands = ( GPUDrawCommand * ) malloc ( list->buffer_size ) ;
	}
	return list;
}

void GPU_draw_list_discard ( GPUDrawList *list )
{
	if ( list->buffer_id ) {
		GPU_buf_free ( list->buffer_id );
	}
	else {
		if ( list->commands )
			free ( list->commands ) ;
	}
	free ( list );
}

void GPU_draw_list_init ( GPUDrawList *list , GPU_Batch *batch )
{
	assert ( batch->phase == GPU_BATCH_READY_TO_DRAW );
	list->batch = batch;
	list->base_index = batch->elem ? BASE_INDEX ( batch->elem ) : UINT_MAX;
	list->cmd_len = 0;

	if ( USE_MULTI_DRAW_INDIRECT ) {
		if ( list->commands == NULL ) {
			glBindBuffer ( GL_DRAW_INDIRECT_BUFFER , list->buffer_id );
			if ( list->cmd_offset >= list->buffer_size ) {
				/* Orphan buffer data and start fresh. */
				glBufferData ( GL_DRAW_INDIRECT_BUFFER , list->buffer_size , NULL , GL_DYNAMIC_DRAW );
				list->cmd_offset = 0;
			}
			GLenum flags = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
			list->commands = ( GPUDrawCommand * ) glMapBufferRange ( GL_DRAW_INDIRECT_BUFFER , list->cmd_offset , list->buffer_size - list->cmd_offset , flags ) ;
		}
	}
	else {
		list->cmd_offset = 0;
	}
}

void GPU_draw_list_command_add ( GPUDrawList *list , int v_first , int v_count , int i_first , int i_count ) {
	assert ( list->commands );

	if ( v_count == 0 || i_count == 0 ) {
		return;
	}

	if ( list->base_index != UINT_MAX ) {
		GPUDrawCommandIndexed *cmd = list->commands_indexed + list->cmd_len;
		cmd->v_first = v_first;
		cmd->v_count = v_count;
		cmd->i_count = i_count;
		cmd->base_index = list->base_index;
		cmd->i_first = i_first;
	}
	else {
		GPUDrawCommand *cmd = list->commands + list->cmd_len;
		cmd->v_first = v_first;
		cmd->v_count = v_count;
		cmd->i_count = i_count;
		cmd->i_first = i_first;
	}

	list->cmd_len++;
	unsigned int offset = list->cmd_offset + list->cmd_len * sizeof ( GPUDrawCommandIndexed );

	if ( offset == list->buffer_size ) {
		GPU_draw_list_submit ( list );
		GPU_draw_list_init ( list , list->batch );
	}
}

void GPU_draw_list_submit ( GPUDrawList *list )
{
	GPU_Batch *batch = list->batch;

	if ( list->cmd_len == 0 ) {
		return;
	}

	assert ( list->commands );
	assert ( batch->program_in_use );
	/* TODO could assert that VAO is bound. */

	/* TODO We loose a bit of memory here if we only draw arrays. Fix that. */
	uintptr_t offset = list->cmd_offset;
	unsigned int cmd_len = list->cmd_len;
	size_t bytes_used = cmd_len * sizeof ( GPUDrawCommandIndexed );
	list->cmd_len = 0; /* Avoid reuse. */

	/* Only do multi-draw indirect if doing more than 2 drawcall.
	 * This avoids the overhead of buffer mapping if scene is
	 * not very instance friendly. */
	if ( USE_MULTI_DRAW_INDIRECT && cmd_len > 2 ) {
		GLenum prim = batch->gl_prim_type;

		glBindBuffer ( GL_DRAW_INDIRECT_BUFFER , list->buffer_id );
		glFlushMappedBufferRange ( GL_DRAW_INDIRECT_BUFFER , 0 , bytes_used );
		glUnmapBuffer ( GL_DRAW_INDIRECT_BUFFER );
		list->commands = NULL; /* Unmapped */
		list->cmd_offset += bytes_used;

		if ( batch->elem ) {
			glMultiDrawElementsIndirect ( prim , INDEX_TYPE ( batch->elem ) , ( void * ) offset , cmd_len , 0 );
		}
		else {
			glMultiDrawArraysIndirect ( prim , ( void * ) offset , cmd_len , 0 );
		}
	}
	else {
		/* Fallback */
		if ( batch->elem ) {
			GPUDrawCommandIndexed *cmd = list->commands_indexed;
			for ( int i = 0; i < cmd_len; i++ , cmd++ ) {
				/* Index start was added by Draw manager. Avoid counting it twice. */
				cmd->v_first -= batch->elem->index_start;
				GPU_batch_draw_advanced ( batch , cmd->v_first , cmd->v_count , cmd->i_first , cmd->i_count );
			}
		}
		else {
			GPUDrawCommand *cmd = list->commands;
			for ( int i = 0; i < cmd_len; i++ , cmd++ ) {
				GPU_batch_draw_advanced ( batch , cmd->v_first , cmd->v_count , cmd->i_first , cmd->i_count );
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */

void GPU_batch_program_set_shader ( GPU_Batch *batch , GPU_Shader *shader ) {
	GPU_batch_program_set ( batch , shader->program , shader->shaderface );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Init/Exit
 * \{ */

void gpu_batch_init ( void ) {
	// gpu_batch_presets_init ( );
}

void gpu_batch_exit ( void ) {
	// gpu_batch_presets_exit ( );
}