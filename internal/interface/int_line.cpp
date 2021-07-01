#include "int_line.h"

namespace gi {

	Line::Line ( ) {
		this->begin = glm::vec3 ( 0.0f ) ;
		this->end = glm::vec3 ( 0.0f ) ;
	}
	Line::Line ( glm::vec3 begin , glm::vec3 end ) {
		this->begin = begin ;
		this->end = end ;
	}
	Line::Line ( glm::vec3 begin , glm::vec3 dir , float length ) {
		this->begin = begin ;
		this->end = begin + glm::normalize ( dir ) * length ;
	}

	glm::vec3 Line::dir ( ) const {
		return glm::normalize ( this->end - this->begin ) ;
	}

	float Line::length ( ) const {
		return glm::length ( this->end - this->begin ) ;
	}

	glm::vec3 Line::pos_in_path ( float distance ) const {
		if ( distance > length ( ) )
			return glm::vec3 ( glm::vec4 ( end , 1.0f ) * toMat4 ( ) ) ;
		return glm::vec3 ( glm::vec4 ( begin + dir ( ) * distance , 1.0f ) * toMat4 ( ) ) ;
	}

	glm::vec3 Line::pos_in_path ( float val , float min , float max ) const {
		return pos_in_path ( ( length ( ) * ( val - min ) ) / ( max - min ) ) ;
	}

}
