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

namespace gi {

#define MAX_PLANE_BATCH_QUADS						8192
#define MAX_PLANE_BATCH_TEXTURES					32

	class PlaneBatch {
	public:
		static void Init ( ) ;
		static void Destroy ( ) ;

		static void Begin ( ) ;
		static void PushPlane ( glm::mat4 matrix , GPU_Texture *texture , glm::vec4 color = glm::vec4 ( 1.0f ) ) ;
		static void PushPlane ( Transform matrix , GPU_Texture *texture , glm::vec4 color = glm::vec4 ( 1.0f ) ) ;
		static void PushPlane ( glm::vec3 pos , glm::vec3 scale , glm::vec3 rot , GPU_Texture *texture , glm::vec4 color = glm::vec4 ( 1.0f ) ) ;
		static void Flush ( ) ;
	};

} ;