#pragma once

#include "int_transform.h"
#include "int_line.h"

namespace gi {

	class Path : public Transform {
		struct Point {
			float path_length ;			// up to this point
			glm::vec3 point ;
		};

		float path_length ;
		std::vector<Point> points ;

		// curve pipeline data
		std::vector<glm::vec3> pipeline ;
	public:
		Path ( ) ;

		void push_point ( glm::vec3 p ) ;
		void push_curve ( const std::vector<glm::vec3> &pt , float unit = 0.0125f ) ;

		// Begin Curve Pipeline
		void begin_curve ( ) ;
		void add_curve_point ( glm::vec3 p ) ;
		void add_curve_point ( glm::vec2 p ) ;
		// End Curve Pipeline
		void end_curve ( float unit = 0.0125f ) ;

		void reset ( ) ;

		float length ( ) const ;
		glm::vec3 pos_in_path ( float distance ) const ;
		glm::vec3 pos_in_path ( float val , float min , float max ) const ;

		glm::vec3 dir_in_path ( float distance ) const ;
		glm::vec3 dir_in_path ( float val , float min , float max ) const ;

		void sample_render ( ) ;
	};

}
