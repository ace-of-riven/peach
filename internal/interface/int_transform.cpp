#include "int_transform.h"

namespace gi {

	Transform::Transform ( ) {
		this->position = glm::vec3 ( 0.0f ) ;
		this->eulerAngles = glm::vec3 ( 0.0f ) ;
		this->scalation = glm::vec3 ( 1.0f ) ;
	}

	void Transform::rotate ( float degree , float x , float y , float z ) {
		this->eulerAngles += degree * glm::vec3 ( x , y , z ) ;
	}

	void Transform::rotate ( float degree , glm::vec3 vec ) {
		this->eulerAngles += degree * vec ;
	}

	void Transform::translate ( float x , float y , float z ) {
		this->position += glm::vec3 ( x , y , z ) ;
	}

	void Transform::translate ( glm::vec3 vec ) {
		this->position += vec ;
	}

	// dir will be normalized
	void Transform::translate ( float distance , glm::vec3 dir ) {
		this->position += distance * glm::normalize ( dir ) ;
	}

	Transform Transform::identity ( ) {
		return Transform ( ) ;
	}

	void Transform::scale ( float x , float y , float z ) {
		this->scalation *= glm::vec3 ( x , y , z ) ;
	}

	void Transform::scale ( glm::vec3 vec ) {
		this->scalation *= vec ;
	}

	void Transform::set_translation ( float x , float y , float z ) {
		this->position = glm::vec3 ( x , y , z ) ;
	}

	void Transform::set_translation ( glm::vec3 vec ) {
		this->position = vec ;
	}
	
	void Transform::set_rotation ( float x , float y , float z ) {
		this->eulerAngles = glm::vec3 ( x , y , z ) ;
	}

	void Transform::set_rotation ( glm::vec3 vec ) {
		this->eulerAngles = vec ;
	}

	void Transform::set_scalation ( float x , float y , float z ) {
		this->scalation = glm::vec3 ( x , y , z ) ;
	}

	void Transform::set_scalation ( glm::vec3 vec ) {
		this->scalation = vec ;
	}

	Transform Transform::operator * ( const Transform &transform ) {
		Transform result ;
		result.set_rotation ( this->eulerAngles + transform.eulerAngles ) ;
		result.set_translation ( this->position + transform.position ) ;
		result.set_scalation ( this->scalation * transform.scalation ) ;
		return result ;
	}

	Transform &Transform::operator = ( const Transform &transform ) {
		this->position = transform.position ;
		this->eulerAngles = transform.eulerAngles ;
		this->scalation = transform.eulerAngles ;
		return *this ;
	}

	Transform &Transform::operator *= ( const Transform &transform ) {
		this->set_rotation ( this->eulerAngles + transform.eulerAngles ) ;
		this->set_translation ( this->position + transform.position ) ;
		this->set_scalation ( this->scalation * transform.scalation ) ;
		return *this ;
	}

	glm::mat4 Transform::toMat4 ( ) const {
		return glm::translate ( position ) * glm::orientate4 ( glm::radians ( this->eulerAngles ) ) * glm::scale ( scalation ) ;
	}

	//

	void Projection::perspective ( float fov , float ratio , float near , float far ) {
		proj = glm::perspective ( glm::radians ( fov ) , ratio , near , far ) ;
	}

	void Projection::orthographic ( float left , float right , float bottom , float top ) {
		proj = glm::ortho ( left , right , bottom , top ) ;
	}

	void Projection::orthographic ( float left , float right , float bottom , float top , float near , float far ) {
		proj = glm::ortho ( left , right , bottom , top , near , far ) ;
	}

	glm::mat4 Projection::toMat4 ( ) const {
		return this->proj * glm::translate ( position ) * glm::orientate4 ( glm::radians ( this->eulerAngles ) ) * glm::scale ( scalation ) ;
	}

}
