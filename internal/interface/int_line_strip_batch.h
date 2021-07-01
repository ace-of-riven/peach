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

#define MAX_LINE_STRIP_BATCH_POINTS						262144

	class LineStripBatch {
	public:
		static void Init ( ) ;
		static void Destroy ( ) ;

		static void Begin ( ) ;
		static void PushPoint ( Transform transform , glm::vec4 point , glm::vec4 color ) ;
		static void Flush ( ) ;
	};

} ;