#include "int_plane_batch.h"
#include "../usr/usr_strings.h"

#include <algorithm>

namespace gi {

	struct PlaneBatchData {

		GPU_Shader *shader = NULL ;

		GPU_Batch *batch ;
		GPU_VertBuf *vbo ;
		GPU_IndexBuf *ibo ;

		int aPos ;
		int aNorm ;
		int aUV ;
		int aTex ;
		int aCol ;

		std::vector<GPU_Texture *> textures ;
		unsigned int vertices_count ;
		unsigned int indices_count ;			// = ( vertices_count / 4 ) * 6

		int texIDArr [ 32 ] ;

	} PlaneBatchData ;

	void PlaneBatch::Init ( ) {
		PlaneBatchData.vertices_count = 0 ;
		PlaneBatchData.indices_count = 0 ;

		GPU_IndexBufBuilder *builder = new GPU_IndexBufBuilder ( ) ;
		GPU_indexbuf_init_ex ( builder , GPUPrimType::GPU_PRIM_TRIS , 6 * MAX_PLANE_BATCH_QUADS , 4 * MAX_PLANE_BATCH_QUADS ) ;
		for ( int i = 0 ; i < MAX_PLANE_BATCH_QUADS ; i++ ) {
			unsigned int offset = i * 4 ;
			GPU_indexbuf_add_tri_verts ( builder , offset + 0 , offset + 1 , offset + 2 ) ;
			GPU_indexbuf_add_tri_verts ( builder , offset + 2 , offset + 3 , offset + 0 ) ;
		}

		PlaneBatchData.ibo = GPU_indexbuf_build ( builder ) ;

		if ( !PlaneBatchData.shader )
			PlaneBatchData.shader = GPU_shader_load_from_file ( SHADER_LOCATION_BY_NAME ( "PlaneBatch_Vert.glsl" ).c_str ( ) ,
								SHADER_LOCATION_BY_NAME ( "PlaneBatch_Frag.glsl" ).c_str ( ) , NULL ) ;

		GPU_VertFormat *format = new GPU_VertFormat ( ) ;
		GPU_vertformat_clear ( format ) ;
		GPU_vertformat_from_interface ( format , PlaneBatchData.shader->shaderface ) ;

		PlaneBatchData.aPos = GPU_vertformat_attr_id_get ( format , "aPos" ) ;
		PlaneBatchData.aNorm = GPU_vertformat_attr_id_get ( format , "aNorm" ) ;
		PlaneBatchData.aUV = GPU_vertformat_attr_id_get ( format , "aUV" ) ;
		PlaneBatchData.aTex = GPU_vertformat_attr_id_get ( format , "aTex" ) ;
		PlaneBatchData.aCol = GPU_vertformat_attr_id_get ( format , "aCol" ) ;

		printf ( "aPos : %d , aNorm : %d , aUV : %d , aTex : %d , aCol : %d\n" , PlaneBatchData.aPos , PlaneBatchData.aNorm , PlaneBatchData.aUV , PlaneBatchData.aTex , PlaneBatchData.aCol ) ;

		PlaneBatchData.vbo = GPU_vertbuf_create_with_format_ex ( format , GPUUsageType::GPU_USAGE_STREAM ) ;
		GPU_vertbuf_data_alloc ( PlaneBatchData.vbo , 4 * MAX_PLANE_BATCH_QUADS ) ;

		delete builder ;

		PlaneBatchData.batch = GPU_batch_create_ex ( GPUPrimType::GPU_PRIM_TRIS , PlaneBatchData.vbo , PlaneBatchData.ibo , 0 ) ;
		GPU_batch_program_set_shader ( PlaneBatchData.batch , PlaneBatchData.shader ) ;

		for ( int i = 0 ; i < 32 ; i++ )
			PlaneBatchData.texIDArr [ i ] = i ;
	}

	void PlaneBatch::Destroy ( ) {
		GPU_vertbuf_discard ( PlaneBatchData.vbo ) ;
		GPU_indexbuf_discard ( PlaneBatchData.ibo ) ;
		GPU_batch_discard ( PlaneBatchData.batch ) ;
	}

	void PlaneBatch::Begin ( ) {
		PlaneBatchData.vertices_count = 0 ;
		PlaneBatchData.indices_count = 0 ;
		PlaneBatchData.textures.clear ( ) ;
	}

	void PlaneBatch::PushPlane ( glm::mat4 matrix , GPU_Texture *texture , glm::vec4 color ) {

		if ( PlaneBatchData.textures.size ( ) + 1 > MAX_PLANE_BATCH_TEXTURES or
			PlaneBatchData.vertices_count + 4 > MAX_PLANE_BATCH_QUADS * 4 or
			PlaneBatchData.indices_count + 6 > MAX_PLANE_BATCH_QUADS * 6 ) {
			Flush ( ) ;
			Begin ( ) ;
		}

		float textureID = PlaneBatchData.textures.size ( ) ;
		for ( int i = 0 ; i < PlaneBatchData.textures.size ( ) ; i++ ) {
			if ( texture == PlaneBatchData.textures [ i ] ) {
				textureID = i ;
				break;
			}
		}
		if ( textureID == PlaneBatchData.textures.size ( ) ) {
			PlaneBatchData.textures.push_back ( texture ) ;
		}

		glm::vec4 vTri [ ] = { matrix * glm::vec4 ( -1.0f , 1.0f , 0.0f , 1.0f ) ,
					matrix * glm::vec4 ( 1.0f , 1.0f , 0.0f , 1.0f ) ,
					matrix * glm::vec4 ( 1.0f ,-1.0f , 0.0f , 1.0f ) ,
					matrix * glm::vec4 ( -1.0f ,-1.0f , 0.0f , 1.0f ) } ;

		glm::vec2 vUV [ ] = { glm::vec2 ( 0.0f , 0.0f ) ,
					glm::vec2 ( 1.0f , 0.0f ) ,
					glm::vec2 ( 1.0f , 1.0f ) ,
					glm::vec2 ( 0.0f , 1.0f ) } ;

		glm::vec3 nTri = glm::mat3 ( glm::inverse ( matrix ) ) * glm::vec3 ( 0.0f , 0.0f , 1.0f ) ;

		for ( int i = 0 ; i < 4 ; i++ ) {
			if ( PlaneBatchData.aPos >= 0 ) {
				GPU_vertbuf_attr_set ( PlaneBatchData.vbo , PlaneBatchData.aPos , PlaneBatchData.vertices_count , &vTri [ i ][ 0 ] ) ;
			}
			if ( PlaneBatchData.aNorm >= 0 ) {
				GPU_vertbuf_attr_set ( PlaneBatchData.vbo , PlaneBatchData.aNorm , PlaneBatchData.vertices_count , &nTri [ 0 ] ) ;
			}
			if ( PlaneBatchData.aUV >= 0 ) {
				GPU_vertbuf_attr_set ( PlaneBatchData.vbo , PlaneBatchData.aUV , PlaneBatchData.vertices_count , &vUV [ i ][ 0 ] ) ;
			}
			if ( PlaneBatchData.aTex >= 0 ) {
				GPU_vertbuf_attr_set ( PlaneBatchData.vbo , PlaneBatchData.aTex , PlaneBatchData.vertices_count , &textureID ) ;
			}
			if ( PlaneBatchData.aCol >= 0 ) {
				GPU_vertbuf_attr_set ( PlaneBatchData.vbo , PlaneBatchData.aCol , PlaneBatchData.vertices_count , &color [ 0 ] ) ;
			}
			PlaneBatchData.vertices_count++;
		}

		PlaneBatchData.indices_count += 6 ;
	}

	void PlaneBatch::PushPlane ( Transform transform , GPU_Texture *texture , glm::vec4 color ) {
		PushPlane ( transform.toMat4 ( ) , texture , color ) ;
	}

	void PlaneBatch::PushPlane ( glm::vec3 pos , glm::vec3 scale , glm::vec3 rot , GPU_Texture *texture , glm::vec4 color ) {
		Transform transform ;
		transform.translate ( pos ) ;
		transform.rotate ( 1.0f , rot ) ;
		transform.scale ( scale ) ;
		PushPlane ( transform.toMat4 ( ) , texture , color ) ;
	}

	void PlaneBatch::Flush ( ) {
		glEnable ( GL_BLEND );
		glBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );

		GPU_shader_use ( PlaneBatchData.shader ) ;
		GPU_matrix_bind ( PlaneBatchData.shader->shaderface ) ;

		char uniform_name [ 64 ] ;
		for ( int tex = 0 ; tex < PlaneBatchData.textures.size ( ) ; tex++ ) {
			sprintf_s ( uniform_name , 64 , "texUniform[%d]" , tex ) ;
			if ( PlaneBatchData.textures [ tex ] ) {
				GPU_texture_bind ( PlaneBatchData.textures [ tex ] , tex ) ;
				GPU_batch_uniform_1i ( PlaneBatchData.batch , uniform_name , tex ) ;
			}
			else {
				GPU_texture_bind ( GPU_texture_get_empty ( 2 ) , tex ) ;
				GPU_batch_uniform_1i ( PlaneBatchData.batch , uniform_name , tex ) ;
			}
		}

		GPU_batch_draw_advanced ( PlaneBatchData.batch , 0 , PlaneBatchData.indices_count , 0 , 0 ) ;
	}

};
