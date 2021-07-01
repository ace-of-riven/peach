#pragma once

#include "int_transform.h"

namespace gi {

	struct Line : public Transform {
		Line ( ) ;
		Line ( glm::vec3 begin , glm::vec3 end ) ;
		Line ( glm::vec3 begin , glm::vec3 dir , float length ) ;

		glm::vec3 begin ;
		glm::vec3 end ;

		glm::vec3 dir ( ) const ;

		float length ( ) const ;

		glm::vec3 pos_in_path ( float distance ) const ;
		glm::vec3 pos_in_path ( float val , float min , float max ) const ;
	};

}
