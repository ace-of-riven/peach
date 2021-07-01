#pragma once

#include "../../resource.h"

#include <string>
#include <cstring>

const char *USR_loadstring ( unsigned int define ) ;

inline std::string COMBINE_DIRECTORIES ( const std::string &dir , const std::string &file ) {
	if ( dir.back ( ) == '/' )
		return ( dir + file ) ;
	if ( dir.size ( ) )
		return ( dir + '/' + file ) ;
	return file.c_str ( ) ;
}

inline std::string TEXTURE_LOCATION_BY_NAME ( const std::string &name ) {
	return COMBINE_DIRECTORIES ( USR_loadstring ( IDS_USR_TEXTURE_DIR ) , name ) ;
}

inline std::string MODEL_LOCATION_BY_NAME ( const std::string &name ) {
	return COMBINE_DIRECTORIES ( USR_loadstring ( IDS_USR_MODEL_DIR ) , name ) ;
}

inline std::string SHADER_LOCATION_BY_NAME ( const std::string &name ) {
	return COMBINE_DIRECTORIES ( USR_loadstring ( IDS_USR_SHADER_DIR ) , name ) ;
}

inline std::string RESOURCE_LOCATION_BY_NAME ( const std::string &name ) {
	return COMBINE_DIRECTORIES ( USR_loadstring ( IDS_USR_RESOURCE_DIR ) , name ) ;
}
