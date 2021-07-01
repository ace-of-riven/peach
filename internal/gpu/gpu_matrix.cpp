#include "gpu_matrix.h"
#include "gpu_context.h"
#include "gpu_shader.h"

#define DEBUG_MATRIX_BIND 0

#define MATRIX_STACK_DEPTH 32

typedef struct MatrixStack {
	glm::mat4 stack [ MATRIX_STACK_DEPTH ] ;
	unsigned int top ;
} MatrixStack ;

typedef struct GPU_MatrixState {
	MatrixStack model_view_stack ;
	MatrixStack projection_stack ;

	bool dirty ;
} GPU_MatrixState ;

#define ModelViewStack gpu_context_active_matrix_state_get()->model_view_stack
#define ModelView ModelViewStack.stack[ModelViewStack.top]

#define ProjectionStack gpu_context_active_matrix_state_get()->projection_stack
#define Projection ProjectionStack.stack[ProjectionStack.top]

GPU_MatrixState *GPU_matrix_state_create ( void )
{
	GPU_MatrixState *state = new GPU_MatrixState ( ) ;
	MatrixStack identity_stack ;
	identity_stack.stack [ identity_stack.top = 0 ] = glm::mat4 ( 1.0f ) ;

	state->model_view_stack = state->projection_stack = identity_stack;
	state->dirty = true;

	return state;
}

void GPU_matrix_state_discard ( GPU_MatrixState *state ) {
	delete state ;
}

static void gpu_matrix_state_active_set_dirty ( bool value ) {
	GPU_MatrixState *state = gpu_context_active_matrix_state_get ( );
	state->dirty = value;
}

void GPU_matrix_reset ( void ) {
	GPU_MatrixState *state = gpu_context_active_matrix_state_get ( );
	state->model_view_stack.top = 0;
	state->projection_stack.top = 0;
	ModelView = glm::mat4 ( 1.0f ) ;
	Projection = glm::mat4 ( 1.0f ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_push ( void ) {
	assert ( ModelViewStack.top + 1 < MATRIX_STACK_DEPTH );
	ModelViewStack.top++;
	ModelView = ModelViewStack.stack [ ModelViewStack.top - 1 ] ;
}

void GPU_matrix_pop ( void ) {
	assert ( ModelViewStack.top > 0 );
	ModelViewStack.top--;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_push_projection ( void ) {
	assert ( ProjectionStack.top + 1 < MATRIX_STACK_DEPTH );
	ProjectionStack.top++;
	Projection = ProjectionStack.stack [ ProjectionStack.top - 1 ] ;
}

void GPU_matrix_pop_projection ( void ) {
	assert ( ProjectionStack.top > 0 );
	ProjectionStack.top--;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_set ( const glm::mat4 &m ) {
	ModelView = m ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_identity_projection_set ( void ) {
	Projection = glm::mat4 ( 1.0f ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_projection_set ( const glm::mat4& m ) {
	Projection = m ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_identity_set ( void ) {
	ModelView = glm::mat4 ( 1.0f ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_translate_2f ( float x , float y ) {
	GPU_matrix_mul ( glm::translate ( glm::vec3 ( x , y , 0.0f ) ) ) ;
}

void GPU_matrix_translate_2fv ( const glm::vec2& vec ) {
	GPU_matrix_translate_2f ( vec.x , vec.y );
}

void GPU_matrix_translate_3f ( float x , float y , float z ) {
	ModelView *= glm::translate ( glm::vec3 ( x , y , z ) ) ;
	gpu_matrix_state_active_set_dirty ( true ) ;
}

void GPU_matrix_translate_3fv ( const glm::vec3 &vec ) {
	GPU_matrix_translate_3f ( vec.x , vec.y , vec.z ) ;
}

void GPU_matrix_scale_1f ( float factor ) {
	GPU_matrix_mul ( glm::scale ( glm::vec3 ( factor ) ) ) ;
}

void GPU_matrix_scale_2f ( float x , float y ) {
	GPU_matrix_mul ( glm::scale ( glm::vec3 ( x , y , 1.0f ) ) ) ;
}

void GPU_matrix_scale_3f ( float x , float y , float z ) {
	GPU_matrix_mul ( glm::scale ( glm::vec3 ( x , y , z ) ) ) ;
}

void GPU_matrix_scale_2fv ( const glm::vec2 &v ) {
	GPU_matrix_mul ( glm::scale ( glm::vec3 ( v , 1.0f ) ) ) ;
}

void GPU_matrix_scale_3fv ( const glm::vec3 &v ) {
	GPU_matrix_mul ( glm::scale ( v ) ) ;
}

/* Axis of rotation should be a unit vector. */
void GPU_matrix_rotate_3f ( float deg , float x , float y , float z ) {
	GPU_matrix_mul ( glm::rotate ( glm::radians ( deg ) , glm::vec3 ( x , y , z ) ) ) ;
}

/* Axis of rotation should be a unit vector. */
void GPU_matrix_rotate_3fv ( float deg , const glm::vec3 &a ) {
	GPU_matrix_mul ( glm::rotate ( glm::radians ( deg ) , a ) ) ;
}

void GPU_matrix_rotate_2d ( float deg ) {
	GPU_matrix_mul ( glm::rotate ( glm::radians ( deg ) , glm::vec3 ( 0.0f , 0.0f , 1.0f ) ) ) ;
}

void GPU_matrix_look_at ( float eyeX , float eyeY , float eyeZ , float centerX , float centerY , float centerZ , float upX , float upY , float upZ ) {
	GPU_matrix_mul ( glm::lookAt ( glm::vec3 ( eyeX , eyeY , eyeZ ) , glm::vec3 ( centerX , centerY , centerZ ) , glm::vec3 ( upX , upY , upZ ) ) ) ;
}

void GPU_matrix_ortho_set ( float left , float right , float bottom , float top , float near , float far ) {
	Projection = glm::ortho ( left , right , bottom , top , near , far ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_ortho_2d_set ( float left , float right , float bottom , float top ) {
	Projection = glm::ortho ( left , right , bottom , top ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_frustum_set ( float left , float right , float bottom , float top , float near , float far ) {
	Projection = glm::frustum ( left , right , bottom , top , near , far ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_perspective_set ( float fovy , float aspect , float near , float far ) {
	Projection = glm::perspective ( glm::radians ( fovy ) , aspect , near , far ) ;
	gpu_matrix_state_active_set_dirty ( true );
}

void GPU_matrix_project ( const glm::vec3 &world , const glm::mat4 &model , const glm::mat4 &proj , const glm::ivec4 &view , glm::vec3 &r_win ) {
	r_win = proj * model * glm::vec4 ( world , 0.0f ) ;
}

static void gpu_mul_invert_projmat_m4_unmapped_v3_with_precalc ( const GPU_MatrixUnproject_Precalc *precalc , glm::vec3& co ) {
	/* 'precalc->dims' is the result of 'projmat_dimensions(proj, ...)'. */
	co [ 0 ] = precalc->dims.xmin + co [ 0 ] * ( precalc->dims.xmax - precalc->dims.xmin );
	co [ 1 ] = precalc->dims.ymin + co [ 1 ] * ( precalc->dims.ymax - precalc->dims.ymin );

	if ( precalc->is_persp ) {
		co [ 2 ] = precalc->dims.zmax * precalc->dims.zmin /
			( precalc->dims.zmax + co [ 2 ] * ( precalc->dims.zmin - precalc->dims.zmax ) );
		co [ 0 ] *= co [ 2 ];
		co [ 1 ] *= co [ 2 ];
	}
	else {
		co [ 2 ] = precalc->dims.zmin + co [ 2 ] * ( precalc->dims.zmax - precalc->dims.zmin );
	}
	co [ 2 ] *= -1;
}

static void projmat_dimensions ( const float **projmat ,
	float *r_left ,
	float *r_right ,
	float *r_bottom ,
	float *r_top ,
	float *r_near ,
	float *r_far )
{
	bool is_persp = projmat [ 3 ][ 3 ] == 0.0f;

	if ( is_persp ) {
		*r_left = ( projmat [ 2 ][ 0 ] - 1.0f ) / projmat [ 0 ][ 0 ];
		*r_right = ( projmat [ 2 ][ 0 ] + 1.0f ) / projmat [ 0 ][ 0 ];
		*r_bottom = ( projmat [ 2 ][ 1 ] - 1.0f ) / projmat [ 1 ][ 1 ];
		*r_top = ( projmat [ 2 ][ 1 ] + 1.0f ) / projmat [ 1 ][ 1 ];
		*r_near = projmat [ 3 ][ 2 ] / ( projmat [ 2 ][ 2 ] - 1.0f );
		*r_far = projmat [ 3 ][ 2 ] / ( projmat [ 2 ][ 2 ] + 1.0f );
	}
	else {
		*r_left = ( -projmat [ 3 ][ 0 ] - 1.0f ) / projmat [ 0 ][ 0 ];
		*r_right = ( -projmat [ 3 ][ 0 ] + 1.0f ) / projmat [ 0 ][ 0 ];
		*r_bottom = ( -projmat [ 3 ][ 1 ] - 1.0f ) / projmat [ 1 ][ 1 ];
		*r_top = ( -projmat [ 3 ][ 1 ] + 1.0f ) / projmat [ 1 ][ 1 ];
		*r_near = ( projmat [ 3 ][ 2 ] + 1.0f ) / projmat [ 2 ][ 2 ];
		*r_far = ( projmat [ 3 ][ 2 ] - 1.0f ) / projmat [ 2 ][ 2 ];
	}
}

bool GPU_matrix_unproject_precalc ( GPU_MatrixUnproject_Precalc *precalc , const glm::mat4 &model , const glm::mat4 &proj , const glm::ivec4 &view ) {
	precalc->is_persp = proj [ 3 ][ 3 ] == 0.0f;
	projmat_dimensions ( ( const float ** ) &proj [ 0 ][ 0 ] ,
		&precalc->dims.xmin , &precalc->dims.xmax ,
		&precalc->dims.ymin , &precalc->dims.ymax ,
		&precalc->dims.zmin , &precalc->dims.zmax ) ;
	if ( isinf ( precalc->dims.zmax ) ) {
		/* We cannot retrieve the actual value of the clip_end.
		 * Use `FLT_MAX` to avoid nans. */
		precalc->dims.zmax = FLT_MAX;
	}
	precalc->view = view ;
	precalc->model_inverted = glm::inverse ( model ) ;
	return true;
}

void GPU_matrix_unproject_with_precalc ( const struct GPU_MatrixUnproject_Precalc *precalc , const glm::vec3& win , glm::vec3& r_world ) {
	glm::vec3 in ( ( win.x - precalc->view.x ) / precalc->view.z , ( win.y - precalc->view.y ) / precalc->view.w , win.z ) ;
	gpu_mul_invert_projmat_m4_unmapped_v3_with_precalc ( precalc , in );
	r_world = glm::vec3 ( precalc->model_inverted * glm::vec4 ( in , 0.0f ) ) ;
}

bool GPU_matrix_unproject ( const glm::vec3 &win , const glm::mat4 &model , const glm::mat4 &proj , const glm::ivec4 &view , glm::vec3 &r_world ) {
	struct GPU_MatrixUnproject_Precalc precalc;
	if ( !GPU_matrix_unproject_precalc ( &precalc , model , proj , view ) ) {
		r_world = glm::vec3 ( 0.0f ) ;
		return false;
	}
	GPU_matrix_unproject_with_precalc ( &precalc , win , r_world );
	return true;
}

const glm::mat4 &GPU_matrix_model_view_get ( ) {
	return ModelView ;
}

const glm::mat4 &GPU_matrix_projection_get ( ) {
	return Projection ;
}

const glm::mat4 &GPU_matrix_model_view_projection_get ( ) {
	return Projection * ModelView ;
}

const glm::mat3 &GPU_matrix_normal_get ( ) {
	return glm::mat3 ( glm::transpose ( glm::inverse ( ModelView ) ) ) ;
}

const glm::mat3 &GPU_matrix_normal_inverse_get ( ) {
	return glm::inverse ( GPU_matrix_normal_get ( ) ) ;
}

void GPU_matrix_bind ( const GPU_ShaderInterface *shaderface ) {
	/* set uniform values to matrix stack values
	 * call this before a draw call if desired matrices are dirty
	 * call glUseProgram before this, as glUniform expects program to be bound
	 */

	const GPU_ShaderInput *MV = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_MODELVIEW );
	const GPU_ShaderInput *P = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_PROJECTION );
	const GPU_ShaderInput *MVP = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_MVP );
	const GPU_ShaderInput *N = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_NORMAL );
	const GPU_ShaderInput *MV_inv = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_MODELVIEW_INV );
	const GPU_ShaderInput *P_inv = GPU_shaderinterface_uniform_builtin ( shaderface , GPU_UNIFORM_PROJECTION_INV );

	if ( MV ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting MV matrix" );
#endif
		glUniformMatrix4fv ( MV->location , 1 , GL_FALSE , &GPU_matrix_model_view_get ( ) [ 0 ][ 0 ] ) ;
	}

	if ( P ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting P matrix" );
#endif
		glUniformMatrix4fv ( P->location , 1 , GL_FALSE , &GPU_matrix_projection_get ( ) [ 0 ][ 0 ] ) ;
	}

	if ( MVP ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting MVP matrix" );
#endif
		glUniformMatrix4fv ( MVP->location , 1 , GL_FALSE , &GPU_matrix_model_view_projection_get ( ) [ 0 ][ 0 ] ) ;
	}

	if ( N ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting normal matrix" );
#endif
		glUniformMatrix3fv ( N->location , 1 , GL_FALSE , &GPU_matrix_normal_get ( ) [ 0 ][ 0 ] );
	}

	if ( MV_inv ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting MV matrix" );
#endif
		glUniformMatrix4fv ( MV_inv->location , 1 , GL_FALSE , &glm::inverse ( GPU_matrix_model_view_get ( ) ) [ 0 ][ 0 ] ) ;
	}

	if ( P_inv ) {
#if DEBUG_MATRIX_BIND
		puts ( "setting P matrix" );
#endif
		glUniformMatrix4fv ( P_inv->location , 1 , GL_FALSE , &glm::inverse ( GPU_matrix_projection_get ( ) ) [ 0 ][ 0 ] );
	}

	gpu_matrix_state_active_set_dirty ( false );
}

bool GPU_matrix_dirty_get ( void ) {
	return gpu_context_active_matrix_state_get ( )->dirty ;
}

void GPU_matrix_mul ( const glm::mat4 &m ) {
	ModelView *= m ;
	gpu_matrix_state_active_set_dirty ( true );
}

