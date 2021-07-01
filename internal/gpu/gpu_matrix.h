#pragma once

#include "gpu_init.h"

#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

void GPU_matrix_reset ( void ) ;

/* ModelView Matrix (2D or 3D) */

void GPU_matrix_push ( void ) ;
void GPU_matrix_pop ( void ) ;

void GPU_matrix_identity_set ( void ) ;

void GPU_matrix_scale_1f ( float factor ) ;

/* 3D ModelView Matrix */

void GPU_matrix_set ( const glm::mat4 &m ) ;
void GPU_matrix_mul ( const glm::mat4 &m ) ;

void GPU_matrix_translate_3f ( float x , float y , float z ) ;
void GPU_matrix_translate_3fv ( const glm::vec3& v ) ;

void GPU_matrix_scale_3f ( float x , float y , float z ) ;
void GPU_matrix_scale_3fv ( const glm::vec3 &v ) ;

/* Axis of rotation should be a unit vector. */
void GPU_matrix_rotate_3f ( float deg , float x , float y , float z ) ;
/* Axis of rotation should be a unit vector. */
void GPU_matrix_rotate_3fv ( float deg , const glm::vec3 &a ) ;

void GPU_matrix_look_at ( float eyeX , float eyeY , float eyeZ , float centerX , float centerY , float centerZ , float upX , float upY , float upZ ) ;

/* 2D ModelView Matrix */

void GPU_matrix_translate_2f ( float x , float y );
void GPU_matrix_translate_2fv ( const glm::vec2& vec );
void GPU_matrix_scale_2f ( float x , float y );
void GPU_matrix_scale_2fv ( const glm::vec2 &vec );
void GPU_matrix_rotate_2d ( float deg );

/* Projection Matrix (2D or 3D) */

void GPU_matrix_push_projection ( void );
void GPU_matrix_pop_projection ( void );

/* 3D Projection Matrix */

void GPU_matrix_identity_projection_set ( void );
void GPU_matrix_projection_set ( const glm::mat4& m );

void GPU_matrix_ortho_set ( float left , float right , float bottom , float top , float near , float far );
void GPU_matrix_frustum_set ( float left , float right , float bottom , float top , float near , float far );
void GPU_matrix_perspective_set ( float fovy , float aspect , float near , float far );

/* 3D Projection between Window and World Space */

struct GPU_MatrixUnproject_Precalc {
	glm::mat4 model_inverted ;
	glm::ivec4 view ;
	bool is_persp;
	/** Result of 'projmat_dimensions'. */
	struct {
		float xmin , xmax;
		float ymin , ymax;
		float zmin , zmax;
	} dims;
};

bool GPU_matrix_unproject_precalc ( struct GPU_MatrixUnproject_Precalc *unproj_precalc ,
	const glm::mat4 &model ,
	const glm::mat4 &proj ,
	const glm::ivec4 &view );

void GPU_matrix_project ( const glm::vec3 &world ,
	const glm::mat4 &model ,
	const glm::mat4 &proj ,
	const glm::ivec4 &view ,
	glm::vec3 &r_win );

bool GPU_matrix_unproject ( const glm::vec3 &win ,
	const glm::mat4 &model ,
	const glm::mat4 &proj ,
	const glm::ivec4 &view ,
	glm::vec3 &r_world ) ;

void GPU_matrix_unproject_with_precalc ( const struct GPU_MatrixUnproject_Precalc *unproj_precalc , const glm::vec3& win , glm::vec3& r_world );

/* 2D Projection Matrix */

void GPU_matrix_ortho_2d_set ( float left , float right , float bottom , float top );

/* functions to get matrix values */
const glm::mat4 &GPU_matrix_model_view_get ( ) ;
const glm::mat4 &GPU_matrix_projection_get ( ) ;
const glm::mat4 &GPU_matrix_model_view_projection_get ( ) ;

const glm::mat3 &GPU_matrix_normal_get ( ) ;
const glm::mat3 &GPU_matrix_normal_inverse_get ( ) ;

/* set uniform values for currently bound shader */
void GPU_matrix_bind ( const struct GPU_ShaderInterface * );
bool GPU_matrix_dirty_get ( void ); /* since last bind */
