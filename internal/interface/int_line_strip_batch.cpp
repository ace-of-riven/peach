#include "int_line_strip_batch.h"
#include "../usr/usr_strings.h"

namespace gi {

	struct LineStripBatchData {

		GPU_Shader *shader = NULL ;

		GPU_Batch *batch ;
		GPU_VertBuf *vbo ;
		GPU_IndexBuf *ibo ;

		int aPos ;
		int aCol ;

		unsigned int vertices_count ;
		unsigned int indices_count ;			// = ( vertices_count / 4 ) * 6

	} LineStripBatchData ;

	void LineStripBatch::Init ( ) {
		LineStripBatchData.vertices_count = 0 ;
		LineStripBatchData.indices_count = 0 ;

		GPU_IndexBufBuilder *builder = new GPU_IndexBufBuilder ( ) ;
		GPU_indexbuf_init_ex ( builder , GPUPrimType::GPU_PRIM_LINE_STRIP , MAX_LINE_STRIP_BATCH_POINTS , MAX_LINE_STRIP_BATCH_POINTS ) ;
		for ( int i = 0 ; i < MAX_LINE_STRIP_BATCH_POINTS ; i++ ) {
			GPU_indexbuf_add_point_vert ( builder , i ) ;
		}

		LineStripBatchData.ibo = GPU_indexbuf_build ( builder ) ;

		if ( !LineStripBatchData.shader )
			LineStripBatchData.shader = GPU_shader_load_from_file ( SHADER_LOCATION_BY_NAME ( "LineBatch_Vert.glsl" ).c_str ( ) ,
									SHADER_LOCATION_BY_NAME ( "LineBatch_Frag.glsl" ).c_str ( ) , NULL ) ;

		GPU_VertFormat *format = new GPU_VertFormat ( ) ;
		GPU_vertformat_clear ( format ) ;
		GPU_vertformat_from_interface ( format , LineStripBatchData.shader->shaderface ) ;

		LineStripBatchData.aPos = GPU_vertformat_attr_id_get ( format , "aPos" ) ;
		LineStripBatchData.aCol = GPU_vertformat_attr_id_get ( format , "aCol" ) ;

		LineStripBatchData.vbo = GPU_vertbuf_create_with_format_ex ( format , GPUUsageType::GPU_USAGE_STREAM ) ;
		GPU_vertbuf_data_alloc ( LineStripBatchData.vbo , MAX_LINE_STRIP_BATCH_POINTS ) ;

		delete builder ;

		LineStripBatchData.batch = GPU_batch_create_ex ( GPUPrimType::GPU_PRIM_LINE_STRIP , LineStripBatchData.vbo , LineStripBatchData.ibo , 0 ) ;
		GPU_batch_program_set_shader ( LineStripBatchData.batch , LineStripBatchData.shader ) ;
	}

	void LineStripBatch::Destroy ( ) {
		GPU_vertbuf_discard ( LineStripBatchData.vbo ) ;
		GPU_indexbuf_discard ( LineStripBatchData.ibo ) ;
		GPU_batch_discard ( LineStripBatchData.batch ) ;
	}

	void LineStripBatch::Begin ( ) {
		LineStripBatchData.vertices_count = 0 ;
		LineStripBatchData.indices_count = 0 ;
	}

	void LineStripBatch::PushPoint ( Transform transform , glm::vec4 point , glm::vec4 color ) {
		if ( LineStripBatchData.indices_count + 1 >= MAX_LINE_STRIP_BATCH_POINTS ) {
			Flush ( ) ;
			Begin ( ) ;
		}

		point = point * transform.toMat4 ( ) ;

		if ( LineStripBatchData.aPos >= 0 ) {
			GPU_vertbuf_attr_set ( LineStripBatchData.vbo , LineStripBatchData.aPos , LineStripBatchData.vertices_count , &point [ 0 ] ) ;
		}
		if ( LineStripBatchData.aCol >= 0 ) {
			GPU_vertbuf_attr_set ( LineStripBatchData.vbo , LineStripBatchData.aCol , LineStripBatchData.vertices_count , &color [ 0 ] ) ;
		}
		LineStripBatchData.vertices_count++;
		LineStripBatchData.indices_count++;
	}

	void LineStripBatch::Flush ( ) {
		glEnable ( GL_BLEND );
		glBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );

		GPU_shader_use ( LineStripBatchData.shader ) ;
		GPU_matrix_bind ( LineStripBatchData.shader->shaderface ) ;

		GPU_batch_draw_advanced ( LineStripBatchData.batch , 0 , LineStripBatchData.indices_count , 0 , 0 ) ;
	}

}
