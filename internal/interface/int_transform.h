#pragma once

#include "../gpu/gpu_init.h"
#include "../application/application.h"
#include "../application/pear_file.h"
#include "../gpu/gpu_context.h"
#include "../gpu/gpu_shader.h"
#include "../gpu/gpu_texture.h"
#include "../gpu/gpu_framebuffer.h"
#include "../gpu/gpu_includes.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#undef near
#undef far

namespace gi {

	class Transform {
	protected:
		glm::vec3 eulerAngles ;
		glm::vec3 position ;
		glm::vec3 scalation ;
	public:
		Transform ( ) ;

		void rotate ( float degree , float x , float y , float z ) ;
		void rotate ( float degree , glm::vec3 vec ) ;

		void translate ( float x , float y , float z ) ;
		void translate ( glm::vec3 vec ) ;

		// dir will be normalized
		void translate ( float distance , glm::vec3 dir ) ;

		static Transform identity ( ) ;

		void scale ( float x , float y , float z ) ;
		void scale ( glm::vec3 vec ) ;

		void set_translation ( float x , float y , float z ) ;
		void set_translation ( glm::vec3 vec ) ;

		void set_rotation ( float x , float y , float z ) ;
		void set_rotation ( glm::vec3 vec ) ;

		void set_scalation ( float x , float y , float z ) ;
		void set_scalation ( glm::vec3 vec ) ;

		Transform operator * ( const Transform &transform ) ;
		Transform &operator = ( const Transform &transform ) ;
		Transform &operator *= ( const Transform &transform ) ;

		inline glm::vec3 translation ( ) const { return position ; }
		inline glm::vec3 rotation ( ) const { return eulerAngles ; }
		inline glm::vec3 scale ( ) const { return scalation ; }

		virtual glm::mat4 toMat4 ( ) const ;
	};

	class Projection : public Transform {
		glm::mat4 proj ;
	public:
		void perspective ( float fov , float ratio , float near , float far ) ;
		void orthographic ( float left , float right , float bottom , float top ) ;
		void orthographic ( float left , float right , float bottom , float top , float near , float far ) ;
		
		virtual glm::mat4 toMat4 ( ) const ;
	};

}
