#include "int_path.h"
#include "int_line_strip_batch.h"

namespace gi {

	Path::Path ( ) {
		this->path_length = 0.0f ;
	}

	void Path::push_point ( glm::vec3 p ) {
		if ( points.size ( ) ) {
			this->path_length += glm::length ( this->points.back ( ).point - p ) ;
		}
		
		Point pt ;
		pt.path_length = this->path_length ;
		pt.point = p ;

		this->points.push_back ( pt ) ;
	}

	double nCr ( int n , int r ) {
		double nf = tgamma ( n + 1 ) ;
		double rf = tgamma ( r + 1 ) ;
		double nrf = tgamma ( ( n - r ) + 1 ) ;
		double ncr = ( nf / ( rf * nrf ) ) ;
		return ncr;
	}

	void Path::push_curve ( const std::vector<glm::vec3> &pt , float unit ) {
		push_point ( pt.front ( ) ) ;
		for ( float u = 0.0f; u <= 1; u += unit )
		{
			float x = 0;
			float y = 0;
			float z = 0;

			for ( int k = 0; k < pt.size ( ); k++ )
			{
				x += ( pt [ k ][ 0 ] * nCr ( 3 , k ) * pow ( u , k ) * powl ( ( 1 - u ) , ( 3 - k ) ) );
				y += ( pt [ k ][ 1 ] * nCr ( 3 , k ) * pow ( u , k ) * powl ( ( 1 - u ) , ( 3 - k ) ) );
				z += ( pt [ k ][ 2 ] * nCr ( 3 , k ) * pow ( u , k ) * powl ( ( 1 - u ) , ( 3 - k ) ) );
			}

			push_point ( glm::vec3 ( x , y , z ) ) ;
		}
		push_point ( pt.back ( ) ) ;
	}

	// Begin Curve Pipeline
	void Path::begin_curve ( ) {
		pipeline.clear ( ) ;
	}

	void Path::add_curve_point ( glm::vec3 p ) {
		pipeline.push_back ( p ) ;
	}

	void Path::add_curve_point ( glm::vec2 p ) {
		pipeline.push_back ( glm::vec3 ( p , 0.0f ) ) ;
	}

	// End Curve Pipeline
	void Path::end_curve ( float unit ) {
		push_curve ( pipeline , unit ) ;
	}

	float Path::length ( ) const {
		return this->path_length ;
	}

	glm::vec3 Path::pos_in_path ( float distance ) const {
		int left = 0 , right = this->points.size ( ) - 2 ;
		while ( left < right ) {
			int mid = ( left + right ) / 2 ;
			float lo = this->points [ mid ].path_length ;
			float hi = this->points [ mid + 1 ].path_length ;
			if ( lo <= distance and distance <= hi ) {
				return Line ( this->points [ mid ].point , this->points [ mid + 1 ].point ).pos_in_path ( distance - lo ) ;
			}
			if ( distance < lo )
				right = mid - 1 ;
			else
				left = mid + 1 ;
		}
		return Line ( this->points [ left ].point , this->points [ left + 1 ].point ).pos_in_path ( distance - this->points [ left ].path_length ) ;
	}

	glm::vec3 Path::dir_in_path ( float distance ) const {
		int left = 0 , right = this->points.size ( ) - 2 ;
		while ( left < right ) {
			int mid = ( left + right ) / 2 ;
			float lo = this->points [ mid ].path_length ;
			float hi = this->points [ mid + 1 ].path_length ;
			if ( lo <= distance and distance <= hi ) {
				return Line ( this->points [ mid ].point , this->points [ mid + 1 ].point ).dir ( ) ;
			}
			if ( distance < lo )
				right = mid - 1 ;
			else
				left = mid + 1 ;
		}
		return Line ( this->points [ left ].point , this->points [ left + 1 ].point ).dir ( ) ;
	}

	glm::vec3 Path::dir_in_path ( float val , float min , float max ) const {
		float v = ( val - min ) / ( max - min ) ;
		return dir_in_path ( v * length ( ) ) ;
	}

	void Path::reset ( ) {
		this->points.clear ( ) ;
		this->path_length = 0.0f ;
	}

	glm::vec3 Path::pos_in_path ( float val , float min , float max ) const {
		float v = ( val - min ) / ( max - min ) ;
		return pos_in_path ( v * length ( ) ) ;
	}

	void Path::sample_render ( ) {
		gi::LineStripBatch::Begin ( ) ;
		Transform transform = *this ;
		for ( int i = 0 ; i < points.size ( ) ; i++ ) {
			gi::LineStripBatch::PushPoint ( transform , glm::vec4 ( points [ i ].point , 1.0f ) , glm::vec4 ( 1.0f , 1.0f , 1.0f , 1.0f ) ) ;
		}
		gi::LineStripBatch::Flush ( ) ;
	}

}
