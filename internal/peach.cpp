#include "gpu/gpu_init.h"
#include "application/application.h"

#include <SOIL/SOIL.h>

#include <Windows.h>
#include <psapi.h>

#include "usr/usr_strings.h"
#include "interface/int_plane_batch.h"
#include "interface/int_line_batch.h"
#include "interface/int_line_strip_batch.h"
#include "interface/int_line.h"
#include "interface/int_path.h"

#include "game/gm_resource_manager.h"
#include "game/gm_enemy.h"
#include "game/gm_map.h"

class mainWindowFrame ;

mainWindowFrame *window ;
GPU_Context *context ;

class mainWindowFrame : public wxGLCanvas {
	wxFrame *frame_wnd ;
public:
	mainWindowFrame ( ) : wxGLCanvas ( ( frame_wnd = new wxFrame ( NULL , wxID_ANY , wxT ( "Rotten peach" ) , wxPoint ( 200 , 200 ) , wxSize ( 800 , 800 ) ) ) ) {
		frame_wnd->Show ( );
	}
	~mainWindowFrame ( ) {
		gi::PlaneBatch::Destroy ( ) ;
		gi::LineBatch::Destroy ( ) ;
		gi::LineStripBatch::Destroy ( ) ;
		window = nullptr ;
	}

	void OnResize ( wxSizeEvent &event ) {
		width = event.m_size.GetWidth ( );
		height = event.m_size.GetHeight ( );
	}

	void OnDestroy ( wxWindowDestroyEvent &event ) {
		GPU_exit ( ) ;
		exit ( 0 ) ;
	}

	int width ;
	int height ;

	GAME_map *sample_map ;

	wxDECLARE_EVENT_TABLE ( );
};

wxBEGIN_EVENT_TABLE ( mainWindowFrame , wxGLCanvas )
EVT_SIZE ( mainWindowFrame::OnResize )
EVT_WINDOW_DESTROY ( mainWindowFrame::OnDestroy )
wxEND_EVENT_TABLE ( )

float lastTick = 0.0f ;

void GPU_OnRenderInit ( ) {
	int width = window->width ;
	int height = window->height ;

	glViewport ( 0 , 0 , width , height ) ;

	glClearColor ( 0.0f , 0.0f , 0.0f , 1.0f ) ;
	glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

	GPU_matrix_push ( ) ;
	GPU_matrix_push_projection ( ) ;

	gi::PlaneBatch::Begin ( );

}

void GPU_OnRenderFinish ( ) {

	gi::PlaneBatch::Flush ( ) ;

	GPU_matrix_pop_projection ( ) ;
	GPU_matrix_pop ( ) ;

	window->SwapBuffers ( ) ;
}

#define TICK_MS					0.03f

void App_OnUpdate ( float deltaTime ) {
	GPU_OnRenderInit ( ) ;

	GPU_matrix_ortho_set ( 0.0f , 100.0f , 0.0f , 100.0f , 0.0f , 100.0f ) ;

	while ( lastTick + TICK_MS < api->Time ( ) ) {
		GAME_map_tick_step ( window->sample_map ) ;
		lastTick += TICK_MS ;
	}

	GAME_map_render ( window->sample_map ) ;

	GPU_OnRenderFinish ( ) ;
}

void App_OnInit ( ) {
	window = new mainWindowFrame ( ) ;
	window->Show ( ) ;

	context = GPU_context_create ( window ) ;
	GPU_context_active_set ( context ) ;

	GPU_init ( ) ;
	GPU_invalid_tex_init ( ) ;

	gi::PlaneBatch::Init ( ) ;
	gi::LineBatch::Init ( ) ;
	gi::LineStripBatch::Init ( ) ;

	GAME_resource_manager_init ( "resources.pack" ) ;
	window->sample_map = GAME_map_import_from_file ( "sample_map.map" ) ;
	GAME_map_build ( window->sample_map , GAME_resource_request ( "archer" ) , 0 ) ;

	lastTick = api->Time ( ) ;
}
