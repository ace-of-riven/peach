#include "application.h"
#include "pear_file.h"

#include <fstream>
#include <streambuf>

inline long long API_file_size ( const wchar_t *filename ) {
	HANDLE file = CreateFile ( filename , GENERIC_READ , FILE_SHARE_READ | FILE_SHARE_WRITE , NULL , OPEN_ALWAYS , FILE_ATTRIBUTE_NORMAL , NULL ) ;

	DWORD file_size_high = 0 , file_size_low = 0 ;
	file_size_low = GetFileSize ( ( HANDLE ) file , &file_size_high ) ;

	return ( ( long long ) file_size_low ) | ( ( ( long long ) ( file_size_high ) ) << 32 ) ;
}

inline long long API_file_size ( const char *filename ) {
	HANDLE file = CreateFileA ( filename , GENERIC_READ , FILE_SHARE_READ | FILE_SHARE_WRITE , NULL , OPEN_ALWAYS , FILE_ATTRIBUTE_NORMAL , NULL ) ;

	DWORD file_size_high = 0 , file_size_low = 0 ;
	file_size_low = GetFileSize ( ( HANDLE ) file , &file_size_high ) ;

	return ( ( long long ) file_size_low ) | ( ( ( long long ) ( file_size_high ) ) << 32 ) ;
}

void *API_file_read ( const wchar_t *filename , long long *size ) {
	std::ifstream t ( filename );
	std::string str ( ( std::istreambuf_iterator<char> ( t ) ) , std::istreambuf_iterator<char> ( ) );
	char *buffer = new char [ str.size ( ) + 1 ] ;
	memcpy ( buffer , str.c_str ( ) , str.size ( ) + 1 ) ;
	return ( void * ) buffer ;
}

void *API_file_read ( const char *filename , long long *size ) {
	std::ifstream t ( filename );
	std::string str ( ( std::istreambuf_iterator<char> ( t ) ) , std::istreambuf_iterator<char> ( ) );
	char *buffer = new char [ str.size ( ) + 1 ] ;
	memcpy ( buffer , str.c_str ( ) , str.size ( ) + 1 ) ;
	return ( void * ) buffer ;
}
