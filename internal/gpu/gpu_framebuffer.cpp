#include "gpu_framebuffer.h"
#include "gpu_texture.h"
#include "gpu_context.h"

#include <algorithm>

#include <cassert>

#define GPU_FB_MAX_COLOR_ATTACHMENT (GPU_FB_MAX_ATTACHEMENT - GPU_FB_COLOR_ATTACHMENT0)

#define GPU_FB_DIRTY_DRAWBUFFER (1 << 15)

#define GPU_FB_ATTACHEMENT_IS_DIRTY(flag, type) ((flag & (1 << type)) != 0)
#define GPU_FB_ATTACHEMENT_SET_DIRTY(flag, type) (flag |= (1 << type))


static GLenum convert_attachment_type_to_gl ( GPUAttachmentType type ) {
	switch ( type ) {
		case GPU_FB_DEPTH_ATTACHMENT: return GL_DEPTH_ATTACHMENT ;
		case GPU_FB_DEPTH_STENCIL_ATTACHMENT: return GL_DEPTH_STENCIL_ATTACHMENT ;
		case GPU_FB_COLOR_ATTACHMENT0: return GL_COLOR_ATTACHMENT0 ;
		case GPU_FB_COLOR_ATTACHMENT1: return GL_COLOR_ATTACHMENT1 ;
		case GPU_FB_COLOR_ATTACHMENT2: return GL_COLOR_ATTACHMENT2 ;
		case GPU_FB_COLOR_ATTACHMENT3: return GL_COLOR_ATTACHMENT3 ;
		case GPU_FB_COLOR_ATTACHMENT4: return GL_COLOR_ATTACHMENT4 ;
		case GPU_FB_COLOR_ATTACHMENT5: return GL_COLOR_ATTACHMENT5 ;
	}
	return GL_INVALID_ENUM ;
}

static GPUAttachmentType attachment_type_from_tex ( GPU_Texture *tex , int slot ) {
	switch ( GPU_texture_format ( tex ) ) {
		case GPU_DEPTH_COMPONENT32F:
		case GPU_DEPTH_COMPONENT24:
		case GPU_DEPTH_COMPONENT16:
			return GPU_FB_DEPTH_ATTACHMENT;
		case GPU_DEPTH24_STENCIL8:
		case GPU_DEPTH32F_STENCIL8:
			return GPU_FB_DEPTH_STENCIL_ATTACHMENT;
		default:
			return GPUAttachmentType ( GPU_FB_COLOR_ATTACHMENT0 + slot ) ;
	}
}

static GLenum convert_buffer_bits_to_gl ( eGPUFrameBufferBits bits )
{
	GLbitfield mask = 0;
	mask |= ( bits & GPU_DEPTH_BIT ) ? GL_DEPTH_BUFFER_BIT : 0;
	mask |= ( bits & GPU_STENCIL_BIT ) ? GL_STENCIL_BUFFER_BIT : 0;
	mask |= ( bits & GPU_COLOR_BIT ) ? GL_COLOR_BUFFER_BIT : 0;
	return mask;
}

GPU_Texture *framebuffer_get_depth_tex ( GPU_Framebuffer *fb ) {
	if ( fb->attachments [ GPU_FB_DEPTH_ATTACHMENT ].tex )
		return fb->attachments [ GPU_FB_DEPTH_ATTACHMENT ].tex ;
	return fb->attachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].tex ;
}

GPU_Texture *framebuffer_get_color_tex ( GPU_Framebuffer *fb , int slot ) {
	return fb->attachments [ GPU_FB_COLOR_ATTACHMENT0 + slot ].tex ;
}

static void gpu_print_framebuffer_error ( GLenum status , char err_out [ 256 ] ) {
	const char *format = "GPU_Framebuffer : framebuffer status %s\n" ;
	const char *err = "unknown" ;

	switch ( status ) {
		case GL_FRAMEBUFFER_COMPLETE:
			err = "GL_FRAMEBUFFER_COMPLETE" ;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			err = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" ;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			err = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" ;
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			err = "GL_FRAMEBUFFER_UNSUPPORTED" ;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES:
			err = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES" ;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			err = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" ;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			err = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" ;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			err = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" ;
			break;
		case GL_FRAMEBUFFER_UNDEFINED:
			err = "GL_FRAMEBUFFER_UNDEFINED" ;
			break;
	}

	if ( err_out )
		sprintf_s ( err_out , 256 , format , err ) ;
	else
		fprintf ( stderr , format , err ) ;
}


GPU_Framebuffer *GPU_framebuffer_active_get ( void ) {
	GPU_Context *ctx = GPU_context_active_get ( ) ;
	if ( ctx )
		return gpu_context_active_framebuffer_get ( ctx ) ;
	return 0 ;
}

static void gpu_framebuffer_current_set ( GPU_Framebuffer *fb ) {
	GPU_Context *ctx = GPU_context_active_get ( ) ;
	if ( ctx )
		gpu_context_active_framebuffer_set ( ctx , fb ) ;
}

//

GPU_Framebuffer *GPU_framebuffer_create ( void ) {
	return new GPU_Framebuffer ( ) ;
}

static void GPU_framebuffer_init ( GPU_Framebuffer *fb ) {
	fb->object = GPU_fbo_alloc ( ) ;
	fb->ctx = GPU_context_active_get ( ) ;
}

void GPU_framebuffer_free ( GPU_Framebuffer *fb ) {
	for ( unsigned int type = GPU_FB_DEPTH_ATTACHMENT ; type < GPU_FB_MAX_ATTACHEMENT ; type++ ) {
		if ( fb->attachments [ type ].tex )
			GPU_framebuffer_texture_detach ( fb , fb->attachments [ type ].tex ) ;
	}

	if ( fb->object ) {
		GPU_fbo_free ( fb->object , fb->ctx ) ;
	}
	if ( GPU_framebuffer_active_get ( ) == fb ) {
		gpu_framebuffer_current_set ( NULL ) ;
	}

	delete fb ;
}

// Attach

static void gpu_framebuffer_texture_attach_ex ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int layer , int mip ) {
	if ( slot >= GPU_FB_MAX_COLOR_ATTACHMENT ) {
		fprintf ( stderr , "Attaching to index %d framebuffer slot unsupported.\n" , slot ) ;
		return ;
	}

	GPUAttachmentType type = attachment_type_from_tex ( tex , slot ) ;
	GPUAttachment *attachment = &fb->attachments [ type ] ;

	if ( attachment->tex == tex and attachment->mip == mip and attachment->layer == layer )
		return ;
	else if ( attachment->tex )
		GPU_framebuffer_texture_detach ( fb , attachment->tex ) ;

	if ( !attachment->tex )
		GPU_texture_attach_framebuffer ( tex , fb , type ) ;

	attachment->tex = tex ;
	attachment->mip = mip ;
	attachment->layer = layer ;
	GPU_FB_ATTACHEMENT_SET_DIRTY ( fb->dirty_flag , type ) ;
}

void GPU_framebuffer_texture_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int mip )
{
	gpu_framebuffer_texture_attach_ex ( fb , tex , slot , -1 , mip );
}

void GPU_framebuffer_texture_layer_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int layer , int mip )
{
	/* NOTE: We could support 1D ARRAY texture. */
	assert ( GPU_texture_target ( tex ) == GL_TEXTURE_2D_ARRAY );
	gpu_framebuffer_texture_attach_ex ( fb , tex , slot , layer , mip );
}

void GPU_framebuffer_texture_cubeface_attach ( GPU_Framebuffer *fb , GPU_Texture *tex , int slot , int face , int mip )
{
	assert ( GPU_texture_cube ( tex ) );
	gpu_framebuffer_texture_attach_ex ( fb , tex , slot , face , mip );
}

// Detach


void GPU_framebuffer_texture_detach_slot ( GPU_Framebuffer *fb , GPU_Texture *tex , int type ) {
	GPUAttachment *attachment = &fb->attachments [ type ];

	if ( attachment->tex != tex ) {
		fprintf ( stderr , "Warning, attempting to detach Texture %p from framebuffer %p but texture is not attached.\n" , tex , fb );
		return;
	}

	attachment->tex = NULL;
	GPU_FB_ATTACHEMENT_SET_DIRTY ( fb->dirty_flag , type );
}

void GPU_framebuffer_texture_detach ( GPU_Framebuffer *fb , GPU_Texture *tex ) {
	GPUAttachmentType type = ( GPUAttachmentType ) GPU_texture_detach_framebuffer ( tex , fb ) ;
	GPU_framebuffer_texture_detach_slot ( fb , tex , type );
}

void GPU_framebuffer_config_array ( GPU_Framebuffer *fb , const GPUAttachment *config , int config_len ) {
	if ( config [ 0 ].tex ) {
		assert ( GPU_texture_depth ( config [ 0 ].tex ) );
		gpu_framebuffer_texture_attach_ex ( fb , config [ 0 ].tex , 0 , config [ 0 ].layer , config [ 0 ].mip );
	}
	else if ( config [ 0 ].mip == -1 ) {
		/* Leave texture attached */
	}
	else if ( fb->attachments [ GPU_FB_DEPTH_ATTACHMENT ].tex != NULL ) {
		GPU_framebuffer_texture_detach ( fb , fb->attachments [ GPU_FB_DEPTH_ATTACHMENT ].tex );
	}
	else if ( fb->attachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].tex != NULL ) {
		GPU_framebuffer_texture_detach ( fb , fb->attachments [ GPU_FB_DEPTH_STENCIL_ATTACHMENT ].tex );
	}

	int slot = 0;
	for ( int i = 1; i < config_len; i++ , slot++ ) {
		if ( config [ i ].tex != NULL ) {
			assert ( GPU_texture_depth ( config [ i ].tex ) == false );
			gpu_framebuffer_texture_attach_ex ( fb , config [ i ].tex , slot , config [ i ].layer , config [ i ].mip );
		}
		else if ( config [ i ].mip != -1 ) {
			GPU_Texture *tex = framebuffer_get_color_tex ( fb , slot );
			if ( tex != NULL )
				GPU_framebuffer_texture_detach ( fb , tex );
		}
	}
}

/* ---------- Bind / Restore ----------- */

static void gpu_framebuffer_attachment_attach ( GPUAttachment *attach , GPUAttachmentType attach_type ) {
	int tex_bind = GPU_texture_opengl_bindcode ( attach->tex );
	GLenum gl_attachment = convert_attachment_type_to_gl ( attach_type );

	if ( attach->layer > -1 ) {
		if ( GPU_texture_cube ( attach->tex ) )
			glFramebufferTexture2D ( GL_FRAMEBUFFER , gl_attachment , GL_TEXTURE_CUBE_MAP_POSITIVE_X + attach->layer , tex_bind , attach->mip );
		else
			glFramebufferTextureLayer ( GL_FRAMEBUFFER , gl_attachment , tex_bind , attach->mip , attach->layer );
	}
	else {
		glFramebufferTexture ( GL_FRAMEBUFFER , gl_attachment , tex_bind , attach->mip );
	}
}


static void gpu_framebuffer_attachment_detach ( GPUAttachment *UNUSED ( attachment ) ,
	GPUAttachmentType attach_type )
{
	GLenum gl_attachment = convert_attachment_type_to_gl ( attach_type );
	glFramebufferTexture ( GL_FRAMEBUFFER , gl_attachment , 0 , 0 );
}

static void gpu_framebuffer_update_attachments ( GPU_Framebuffer *fb )
{
	GLenum gl_attachments [ GPU_FB_MAX_COLOR_ATTACHMENT ];
	int numslots = 0;

	assert ( GPU_framebuffer_active_get ( ) == fb );

	/* Update attachments */
	for ( int type = 0; type < GPU_FB_MAX_ATTACHEMENT; type++ ) {

		if ( type >= GPU_FB_COLOR_ATTACHMENT0 ) {
			if ( fb->attachments [ type ].tex ) {
				gl_attachments [ numslots ] = convert_attachment_type_to_gl ( ( GPUAttachmentType ) type );
			}
			else {
				gl_attachments [ numslots ] = GL_NONE;
			}
			numslots++;
		}

		if ( GPU_FB_ATTACHEMENT_IS_DIRTY ( fb->dirty_flag , type ) == false ) {
			continue;
		}
		else if ( fb->attachments [ type ].tex != NULL ) {
			gpu_framebuffer_attachment_attach ( &fb->attachments [ type ] , ( GPUAttachmentType ) type );

			fb->multisample = ( GPU_texture_samples ( fb->attachments [ type ].tex ) > 0 );
			fb->width = GPU_texture_width ( fb->attachments [ type ].tex );
			fb->height = GPU_texture_height ( fb->attachments [ type ].tex );
		}
		else {
			gpu_framebuffer_attachment_detach ( &fb->attachments [ type ] , ( GPUAttachmentType ) type );
		}
	}
	fb->dirty_flag = 0;

	/* Update draw buffers (color targets)
	 * This state is saved in the FBO */
	if ( numslots ) {
		glDrawBuffers ( numslots , gl_attachments );
	}
	else {
		glDrawBuffer ( GL_NONE );
	}
}

/**
 * Hack to solve the problem of some bugged AMD GPUs (see `GPU_unused_fb_slot_workaround`).
 * If there is an empty color slot between the color slots,
 * all textures after this slot are apparently skipped/discarded.
 */
static void gpu_framebuffer_update_attachments_and_fill_empty_slots ( GPU_Framebuffer *fb )
{
	GLenum gl_attachments [ GPU_FB_MAX_COLOR_ATTACHMENT ];
	int dummy_tex = 0;

	assert ( GPU_framebuffer_active_get ( ) == fb );

	/* Update attachments */
	for ( int type = GPU_FB_MAX_ATTACHEMENT; type--;) {
		GPU_Texture *tex = fb->attachments [ type ].tex;

		if ( type >= GPU_FB_COLOR_ATTACHMENT0 ) {
			int slot = type - GPU_FB_COLOR_ATTACHMENT0;
			if ( tex != NULL || ( dummy_tex != 0 ) ) {
				gl_attachments [ slot ] = convert_attachment_type_to_gl ( ( GPUAttachmentType ) type );

				if ( dummy_tex == 0 ) {
					dummy_tex = GPU_texture_opengl_bindcode ( tex );
				}
			}
			else {
				gl_attachments [ slot ] = GL_NONE;
			}
		}
		else {
			dummy_tex = 0;
		}

		if ( ( dummy_tex != 0 ) && tex == NULL ) {
			/* Fill empty slot */
			glFramebufferTexture ( GL_FRAMEBUFFER , convert_attachment_type_to_gl ( ( GPUAttachmentType ) type ) , dummy_tex , 0 );
		}
		else if ( GPU_FB_ATTACHEMENT_IS_DIRTY ( fb->dirty_flag , type ) ) {
			if ( tex != NULL ) {
				gpu_framebuffer_attachment_attach ( &fb->attachments [ type ] , ( GPUAttachmentType ) type );

				fb->multisample = ( GPU_texture_samples ( tex ) > 0 );
				fb->width = GPU_texture_width ( tex );
				fb->height = GPU_texture_height ( tex );
			}
			else {
				gpu_framebuffer_attachment_detach ( &fb->attachments [ type ] , ( GPUAttachmentType ) type );
			}
		}
	}
	fb->dirty_flag = 0;

	/* Update draw buffers (color targets)
	 * This state is saved in the FBO */
	glDrawBuffers ( GPU_FB_MAX_COLOR_ATTACHMENT , gl_attachments );
}

#define FRAMEBUFFER_STACK_DEPTH 16

static struct {
	GPU_Framebuffer *framebuffers [ FRAMEBUFFER_STACK_DEPTH ];
	unsigned int top;
} FrameBufferStack = { {0} };

static void gpuPushFrameBuffer ( GPU_Framebuffer *fbo )
{
	assert ( FrameBufferStack.top < FRAMEBUFFER_STACK_DEPTH );
	FrameBufferStack.framebuffers [ FrameBufferStack.top ] = fbo;
	FrameBufferStack.top++;
}

static GPU_Framebuffer *gpuPopFrameBuffer ( void )
{
	assert ( FrameBufferStack.top > 0 );
	FrameBufferStack.top--;
	return FrameBufferStack.framebuffers [ FrameBufferStack.top ];
}

#undef FRAMEBUFFER_STACK_DEPTH

void GPU_framebuffer_bind ( GPU_Framebuffer *fb )
{
	if ( fb->object == 0 ) {
		GPU_framebuffer_init ( fb );
	}

	if ( GPU_framebuffer_active_get ( ) != fb ) {
		glBindFramebuffer ( GL_FRAMEBUFFER , fb->object );
	}

	gpu_framebuffer_current_set ( fb );

	if ( fb->dirty_flag != 0 ) {
		if ( GPU_unused_fb_slot_workaround ( ) ) {
			/* XXX: Please AMD, fix this. */
			gpu_framebuffer_update_attachments_and_fill_empty_slots ( fb );
		}
		else {
			gpu_framebuffer_update_attachments ( fb );
		}
	}

	/* TODO manually check for errors? */
#if 0
	char err_out [ 256 ];
	if ( !GPU_framebuffer_check_valid ( fb , err_out ) ) {
		printf ( "Invalid %s\n" , err_out );
	}
#endif

	if ( fb->multisample ) {
		glEnable ( GL_MULTISAMPLE );
	}

	glViewport ( 0 , 0 , fb->width , fb->height );
}

void GPU_framebuffer_restore ( void )
{
	if ( GPU_framebuffer_active_get ( ) != NULL ) {
		glBindFramebuffer ( GL_FRAMEBUFFER , GPU_framebuffer_default ( ) );
		gpu_framebuffer_current_set ( NULL );
	}
}

bool GPU_framebuffer_bound ( GPU_Framebuffer *fb )
{
	return ( fb == GPU_framebuffer_active_get ( ) ) && ( fb->object != 0 );
}

bool GPU_framebuffer_check_valid ( GPU_Framebuffer *fb , char *err_out )
{
	if ( !GPU_framebuffer_bound ( fb ) ) {
		GPU_framebuffer_bind ( fb );
	}

	GLenum status = glCheckFramebufferStatus ( GL_FRAMEBUFFER );

	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		GPU_framebuffer_restore ( );
		gpu_print_framebuffer_error ( status , err_out );
		return false;
	}

	return true;
}

/* ---------- Framebuffer Operations ----------- */

#define CHECK_FRAMEBUFFER_IS_BOUND(_fb) \
  assert(GPU_framebuffer_bound(_fb)); \
  UNUSED_VARS_NDEBUG(_fb); \
  ((void)0)

/* Needs to be done after binding. */
void GPU_framebuffer_viewport_set ( GPU_Framebuffer *fb , int x , int y , int w , int h )
{
	CHECK_FRAMEBUFFER_IS_BOUND ( fb );

	glViewport ( x , y , w , h );
}

void GPU_framebuffer_clear ( GPU_Framebuffer *fb ,
	eGPUFrameBufferBits buffers ,
	const float clear_col [ 4 ] ,
	float clear_depth ,
	unsigned int clear_stencil )
{
	CHECK_FRAMEBUFFER_IS_BOUND ( fb );

	if ( buffers & GPU_COLOR_BIT ) {
		glColorMask ( GL_TRUE , GL_TRUE , GL_TRUE , GL_TRUE );
		glClearColor ( clear_col [ 0 ] , clear_col [ 1 ] , clear_col [ 2 ] , clear_col [ 3 ] );
	}
	if ( buffers & GPU_DEPTH_BIT ) {
		glDepthMask ( GL_TRUE );
		glClearDepth ( clear_depth );
	}
	if ( buffers & GPU_STENCIL_BIT ) {
		glStencilMask ( 0xFF );
		glClearStencil ( clear_stencil );
	}

	GLbitfield mask = convert_buffer_bits_to_gl ( buffers );
	glClear ( mask );
}

void GPU_framebuffer_read_depth ( GPU_Framebuffer *fb , int x , int y , int w , int h , float *data )
{
	CHECK_FRAMEBUFFER_IS_BOUND ( fb );

	GLenum type = GL_DEPTH_COMPONENT;
	glReadBuffer ( GL_COLOR_ATTACHMENT0 ); /* This is OK! */
	glReadPixels ( x , y , w , h , type , GL_FLOAT , data );
}

void GPU_framebuffer_read_color (
	GPU_Framebuffer *fb , int x , int y , int w , int h , int channels , int slot , float *data )
{
	CHECK_FRAMEBUFFER_IS_BOUND ( fb );

	GLenum type;
	switch ( channels ) {
		case 1:
			type = GL_RED;
			break;
		case 2:
			type = GL_RG;
			break;
		case 3:
			type = GL_RGB;
			break;
		case 4:
			type = GL_RGBA;
			break;
		default:
			assert ( false && "wrong number of read channels" );
			return;
	}
	glReadBuffer ( GL_COLOR_ATTACHMENT0 + slot );
	glReadPixels ( x , y , w , h , type , GL_FLOAT , data );
}

/* read_slot and write_slot are only used for color buffers. */
void GPU_framebuffer_blit ( GPU_Framebuffer *fb_read ,
	int read_slot ,
	GPU_Framebuffer *fb_write ,
	int write_slot ,
	eGPUFrameBufferBits blit_buffers )
{
	assert ( blit_buffers != 0 );

	GPU_Framebuffer *prev_fb = GPU_framebuffer_active_get ( );

	/* Framebuffers must be up to date. This simplify this function. */
	if ( fb_read->dirty_flag != 0 || fb_read->object == 0 ) {
		GPU_framebuffer_bind ( fb_read );
	}
	if ( fb_write->dirty_flag != 0 || fb_write->object == 0 ) {
		GPU_framebuffer_bind ( fb_write );
	}

	const bool do_color = ( blit_buffers & GPU_COLOR_BIT );
	const bool do_depth = ( blit_buffers & GPU_DEPTH_BIT );
	const bool do_stencil = ( blit_buffers & GPU_STENCIL_BIT );

	GPU_Texture *read_tex = ( ( do_depth || do_stencil ) ?
		framebuffer_get_depth_tex ( fb_read ) :
		framebuffer_get_color_tex ( fb_read , read_slot ) );
	GPU_Texture *write_tex = ( ( do_depth || do_stencil ) ?
		framebuffer_get_depth_tex ( fb_write ) :
		framebuffer_get_color_tex ( fb_write , read_slot ) );

	if ( do_depth ) {
		assert ( GPU_texture_depth ( read_tex ) && GPU_texture_depth ( write_tex ) );
		assert ( GPU_texture_format ( read_tex ) == GPU_texture_format ( write_tex ) );
	}
	if ( do_stencil ) {
		assert ( GPU_texture_stencil ( read_tex ) && GPU_texture_stencil ( write_tex ) );
		assert ( GPU_texture_format ( read_tex ) == GPU_texture_format ( write_tex ) );
	}
	if ( GPU_texture_samples ( write_tex ) != 0 || GPU_texture_samples ( read_tex ) != 0 ) {
		/* Can only blit multisample textures to another texture of the same size. */
		assert ( ( fb_read->width == fb_write->width ) && ( fb_read->height == fb_write->height ) );
	}

	glBindFramebuffer ( GL_READ_FRAMEBUFFER , fb_read->object );
	glBindFramebuffer ( GL_DRAW_FRAMEBUFFER , fb_write->object );

	if ( do_color ) {
		glReadBuffer ( GL_COLOR_ATTACHMENT0 + read_slot );
		glDrawBuffer ( GL_COLOR_ATTACHMENT0 + write_slot );
		/* XXX we messed with the glDrawBuffer, this will reset the
		 * glDrawBuffers the next time we bind fb_write. */
		fb_write->dirty_flag = GPU_FB_DIRTY_DRAWBUFFER;
	}

	GLbitfield mask = convert_buffer_bits_to_gl ( blit_buffers );

	glBlitFramebuffer ( 0 ,
		0 ,
		fb_read->width ,
		fb_read->height ,
		0 ,
		0 ,
		fb_write->width ,
		fb_write->height ,
		mask ,
		GL_NEAREST );

	/* Restore previous framebuffer */
	if ( fb_write == prev_fb ) {
		GPU_framebuffer_bind ( fb_write ); /* To update drawbuffers */
	}
	else if ( prev_fb ) {
		glBindFramebuffer ( GL_FRAMEBUFFER , prev_fb->object );
		gpu_framebuffer_current_set ( prev_fb );
	}
	else {
		glBindFramebuffer ( GL_FRAMEBUFFER , GPU_framebuffer_default ( ) );
		gpu_framebuffer_current_set ( NULL );
	}
}

/**
 * Use this if you need to custom down-sample your texture and use the previous mip level as input.
 * This function only takes care of the correct texture handling.
 * It execute the callback for each texture level.
 */
void GPU_framebuffer_recursive_downsample ( GPU_Framebuffer *fb ,
	int max_lvl ,
	void ( *callback )( void *userData , int level ) ,
	void *userData )
{
	/* Framebuffer must be up to date and bound. This simplify this function. */
	if ( GPU_framebuffer_active_get ( ) != fb || fb->dirty_flag != 0 || fb->object == 0 ) {
		GPU_framebuffer_bind ( fb );
	}
	/* HACK: We make the framebuffer appear not bound in order to
	 * not trigger any error in GPU_texture_bind().  */
	GPU_Framebuffer *prev_fb = GPU_framebuffer_active_get ( );
	gpu_framebuffer_current_set ( NULL );

	int levels = floor ( log2 ( std::max ( fb->width , fb->height ) ) );
	max_lvl = std::min ( max_lvl , levels );

	int i;
	int current_dim [ 2 ] = { fb->width, fb->height };
	for ( i = 1; i < max_lvl + 1; i++ ) {
		/* calculate next viewport size */
		current_dim [ 0 ] = std::max ( current_dim [ 0 ] / 2 , 1 );
		current_dim [ 1 ] = std::max ( current_dim [ 1 ] / 2 , 1 );

		for ( int type = 0; type < GPU_FB_MAX_ATTACHEMENT; type++ ) {
			if ( fb->attachments [ type ].tex != NULL ) {
				/* Some Intel HDXXX have issue with rendering to a mipmap that is below
				 * the texture GL_TEXTURE_MAX_LEVEL. So even if it not correct, in this case
				 * we allow GL_TEXTURE_MAX_LEVEL to be one level lower. In practice it does work! */
				int next_lvl = i - 1 ;
				/* bind next level for rendering but first restrict fetches only to previous level */
				GPU_Texture *tex = fb->attachments [ type ].tex;
				GPU_texture_bind ( tex , 0 );
				glTexParameteri ( GPU_texture_target ( tex ) , GL_TEXTURE_BASE_LEVEL , i - 1 );
				glTexParameteri ( GPU_texture_target ( tex ) , GL_TEXTURE_MAX_LEVEL , next_lvl );
				GPU_texture_unbind ( tex );
				/* copy attachment and replace miplevel. */
				GPUAttachment attachment = fb->attachments [ type ];
				attachment.mip = i;
				gpu_framebuffer_attachment_attach ( &attachment , ( GPUAttachmentType ) type );
			}
		}

		assert ( GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus ( GL_FRAMEBUFFER ) );

		glViewport ( 0 , 0 , current_dim [ 0 ] , current_dim [ 1 ] );
		callback ( userData , i );

		if ( current_dim [ 0 ] == 1 && current_dim [ 1 ] == 1 ) {
			break;
		}
	}

	for ( int type = 0; type < GPU_FB_MAX_ATTACHEMENT; type++ ) {
		if ( fb->attachments [ type ].tex != NULL ) {
			/* reset mipmap level range */
			GPU_Texture *tex = fb->attachments [ type ].tex;
			GPU_texture_bind ( tex , 0 );
			glTexParameteri ( GPU_texture_target ( tex ) , GL_TEXTURE_BASE_LEVEL , 0 );
			glTexParameteri ( GPU_texture_target ( tex ) , GL_TEXTURE_MAX_LEVEL , i - 1 );
			GPU_texture_unbind ( tex );
			/* Reattach original level */
			/* NOTE: This is not necessary but this makes the FBO config
			 *       remain in sync with the GPU_Framebuffer config. */
			gpu_framebuffer_attachment_attach ( &fb->attachments [ type ] , ( GPUAttachmentType ) type ) ;
		}
	}

	gpu_framebuffer_current_set ( prev_fb );
}

/* GPUOffScreen */

#define MAX_CTX_FB_LEN 3

struct GPUOffScreen {
	struct {
		GPU_Context *ctx;
		GPU_Framebuffer *fb;
	} framebuffers [ MAX_CTX_FB_LEN ];

	GPU_Texture *color;
	GPU_Texture *depth;
};

void GPU_clear_color ( float red , float green , float blue , float alpha )
{
	glClearColor ( red , green , blue , alpha );
}

void GPU_clear_depth ( float depth )
{
	glClearDepth ( depth );
}

void GPU_clear ( eGPUFrameBufferBits flags )
{
	glClear ( convert_buffer_bits_to_gl ( flags ) );
}
