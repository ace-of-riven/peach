#include "../application/application.h"
#include "usr_strings.h"

#include <map>

std::map<int , char *> USR_strings ;

const char *USR_loadstring ( unsigned int define ) {
	if ( USR_strings.find ( define ) == USR_strings.end ( ) ) {
		USR_strings [ define ] = new char [ 512 ] ;
		memset ( USR_strings [ define ] , 0 , 512 ) ;
		LoadStringA ( GetModuleHandleW ( NULL ) , define , ( LPSTR ) USR_strings [ define ] , 256 ) ;
	}
	return USR_strings [ define ] ;
}
