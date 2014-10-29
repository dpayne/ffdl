//
// ffdl.h
//
// Created on: Oct 29, 2013
//     Author: Darby Payne
//

#ifndef _FFDL_H_
#define _FFDL_H_

#include <curl/curl.h>
#include <curl/easy.h>

int ffdl_download_to_file( char * url, char * filename );
int ffdl_download_to_file_with_options( char * url, char * filename, unsigned long long chunkSize, long maxConnections, long timeout, long rateLimit );

#endif
