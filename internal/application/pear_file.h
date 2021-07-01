#pragma once

#include <Windows.h>

void *API_file_read ( const char *filename , long long *size ) ;
void *API_file_read ( const wchar_t *filename , long long *size ) ;