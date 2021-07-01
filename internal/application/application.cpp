#include "application.h"
#include "pear_file.h"

#include <Windows.h>
#include <chrono>

Application *api ;

wxBEGIN_EVENT_TABLE ( Application , wxApp )
EVT_IDLE ( Application::OnUpdate )
wxEND_EVENT_TABLE ( )

wxIMPLEMENT_APP ( Application ) ;

Application::Application ( ) : m_timer ( this , API_TIMER_ID ) {
	api = this ;
}

float Application::Time ( ) {
	return std::chrono::duration<float> ( std::chrono::system_clock::now ( ) - start ).count ( ) ;
}

bool Application::OnInit ( ) {
	if ( AttachConsole ( ATTACH_PARENT_PROCESS ) || AllocConsole ( ) ) {
		freopen ( "CONOUT$" , "w" , stdout );
		freopen ( "CONOUT$" , "w" , stderr );
	}

	start = std::chrono::system_clock::now ( ) ;

	App_OnInit ( ) ;

	last_update = std::chrono::system_clock::now ( ) ;
	last_update_log = std::chrono::system_clock::now ( ) ;

	m_timer.Start ( 1 , wxTIMER_CONTINUOUS ) ;

	return true ;
}

void Application::OnUpdate ( wxIdleEvent& evt ) {
	std::chrono::time_point<std::chrono::system_clock> curr_update = std::chrono::system_clock::now ( ) ;

	float deltaTime = std::chrono::duration<float> ( curr_update - last_update ).count ( ) ;
	if ( std::chrono::duration<float> ( curr_update - last_update_log ).count ( ) > 1.0f ) {
		// printf ( "Applicaiton::OnUpdate ( %.3fms , %.1f );\n" , deltaTime * 1000.0f , 1.0f / deltaTime ) ;
		last_update_log = curr_update ;
	}
	
	App_OnUpdate ( deltaTime ) ;

	last_update = curr_update ;
	evt.RequestMore ( true ) ;
}
