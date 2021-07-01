#include "gpu_defs.h"

#include <cstring>

unsigned int GPU_hash_string ( const char *str , int length , unsigned int base , unsigned int offset ) {
	if ( length < 0 ) length = strlen ( str ) ;
	if ( offset < 0 ) offset = 0 ;
	unsigned int result = 0 ;
	for ( int i = 0 ; i < length ; i++ )
		result = ( result * base ) % 1'000'007 + ( str [ i ] - offset ) ;
	return result % 1'000'007 ;
}
