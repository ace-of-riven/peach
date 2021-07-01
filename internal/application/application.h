#pragma once

#define __WXMSW__
#define WXUSINGDLL
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/glcanvas.h>
#include <wx/font.h>
#include <wx/fontpicker.h>
#include <wx/wrapsizer.h>
#include <wx/gbsizer.h>
#include <wx/combo.h>

#include "resource.h"

#include <chrono>

#define THROTLE					while(1)

class Application : public wxApp {
	wxTimer m_timer ;
	std::chrono::time_point<std::chrono::system_clock> last_update ;
	std::chrono::time_point<std::chrono::system_clock> last_update_log ;
	std::chrono::time_point<std::chrono::system_clock> start ;
public:
	Application ( ) ;

	bool OnInit ( ) ;
	void OnUpdate ( wxIdleEvent &evt ) ;

	float Time ( ) ;

	wxDECLARE_EVENT_TABLE ( );
};

extern Application *api ;

void App_OnInit ( ) ;
void App_OnUpdate ( float deltaTime ) ;
