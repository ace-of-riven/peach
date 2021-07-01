#include "int_line_batch.h"
#include "../usr/usr_strings.h"

namespace gi {

	struct LineBatchData {

		GPU_Shader *shader = NULL ;

		GPU_Batch *batch ;
		GPU_VertBuf *vbo ;
		GPU_IndexBuf *ibo ;

		int aPos ;
		int aCol ;

		unsigned int vertices_count ;
		unsigned int indices_count ;			// = ( vertices_count / 4 ) * 6

	} LineBatchData ;

	void LineBatch::Init ( ) {
		LineBatchData.vertices_count = 0 ;
		LineBatchData.indices_count = 0 ;

		GPU_IndexBufBuilder *builder = new GPU_IndexBufBuilder ( ) ;
		GPU_indexbuf_init_ex ( builder , GPUPrimType::GPU_PRIM_LINES , 2 * MAX_LINE_BATCH_LINES , 2 * MAX_LINE_BATCH_LINES ) ;
		for ( int i = 0 ; i < MAX_LINE_BATCH_LINES ; i++ ) {
			unsigned int offset = i * 2 ;
			GPU_indexbuf_add_line_verts ( builder , offset + 0 , offset + 1 ) ;
		}

		LineBatchData.ibo = GPU_indexbuf_build ( builder ) ;

		if ( !LineBatchData.shader )
			LineBatchData.shader = GPU_shader_load_from_file ( SHADER_LOCATION_BY_NAME ( "LineBatch_Vert.glsl" ).c_str ( ) ,
									SHADER_LOCATION_BY_NAME ( "LineBatch_Frag.glsl" ).c_str ( ) , NULL ) ;

		GPU_VertFormat *format = new GPU_VertFormat ( ) ;
		GPU_vertformat_clear ( format ) ;
		GPU_vertformat_from_interface ( format , LineBatchData.shader->shaderface ) ;

		LineBatchData.aPos = GPU_vertformat_attr_id_get ( format , "aPos" ) ;
		LineBatchData.aCol = GPU_vertformat_attr_id_get ( format , "aCol" ) ;

		LineBatchData.vbo = GPU_vertbuf_create_with_format_ex ( format , GPUUsageType::GPU_USAGE_STREAM ) ;
		GPU_vertbuf_data_alloc ( LineBatchData.vbo , 2 * MAX_LINE_BATCH_LINES ) ;

		delete builder ;

		LineBatchData.batch = GPU_batch_create_ex ( GPUPrimType::GPU_PRIM_LINES , LineBatchData.vbo , LineBatchData.ibo , 0 ) ;
		GPU_batch_program_set_shader ( LineBatchData.batch , LineBatchData.shader ) ;
	}
	
	void LineBatch::Destroy ( ) {
		GPU_vertbuf_discard ( LineBatchData.vbo ) ;
		GPU_indexbuf_discard ( LineBatchData.ibo ) ;
		GPU_batch_discard ( LineBatchData.batch ) ;
	}

	void LineBatch::Begin ( ) {
		LineBatchData.vertices_count = 0 ;
		LineBatchData.indices_count = 0 ;
	}

	void LineBatch::PushLine ( Transform transform , Line line , glm::vec4 color ) {
		if ( LineBatchData.indices_count + 2 >= MAX_LINE_BATCH_LINES * 2 ) {
			Flush ( ) ;
			Begin ( ) ;
		}

		Transform res = transform * line ;
		glm::vec4 vLine [ ] = { glm::vec4 ( line.begin , 1.0f ) * ( res ).toMat4 ( ) , glm::vec4 ( line.end , 1.0f ) * ( res ).toMat4 ( ) } ;

		for ( int i = 0 ; i < 2 ; i++ ) {
			if ( LineBatchData.aPos >= 0 ) {
				GPU_vertbuf_attr_set ( LineBatchData.vbo , LineBatchData.aPos , LineBatchData.vertices_count , &vLine [ i ][ 0 ] ) ;
			}
			if ( LineBatchData.aCol >= 0 ) {
				GPU_vertbuf_attr_set ( LineBatchData.vbo , LineBatchData.aCol , LineBatchData.vertices_count , &color [ 0 ] ) ;
			}
			LineBatchData.vertices_count++;
		}

		LineBatchData.indices_count += 2 ;
	}

	void LineBatch::PushLine ( Transform transform , glm::vec3 begin , glm::vec3 end , glm::vec4 color ) {
		PushLine ( transform , Line ( begin , end ) , color ) ;
	}

	void LineBatch::Flush ( ) {
		glEnable ( GL_BLEND );
		glBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );

		GPU_shader_use ( LineBatchData.shader ) ;
		GPU_matrix_bind ( LineBatchData.shader->shaderface ) ;

		GPU_batch_draw_advanced ( LineBatchData.batch , 0 , LineBatchData.indices_count , 0 , 0 ) ;
	}

}
