#pragma once

#include "../gpu/gpu_init.h"
#include "../application/application.h"
#include "../application/pear_file.h"
#include "../gpu/gpu_context.h"
#include "../gpu/gpu_shader.h"
#include "../gpu/gpu_texture.h"
#include "../gpu/gpu_framebuffer.h"
#include "../gpu/gpu_includes.h"

#include "int_transform.h"
#include "int_line.h"

namespace gi {

#define MAX_LINE_BATCH_LINES						262144

	class LineBatch {
	public:
		static void Init ( ) ;
		static void Destroy ( ) ;

		static void Begin ( ) ;
		static void PushLine ( Transform transform , Line line , glm::vec4 color ) ;
		static void PushLine ( Transform transform , glm::vec3 begin , glm::vec3 end , glm::vec4 color ) ;
		static void Flush ( ) ;
	};

} ;