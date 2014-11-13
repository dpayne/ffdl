//
// ffdl.c
//
// Created on: Oct 29, 2013
//     Author: Darby Payne
//

#include "ffdl.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

unsigned long long c_defaultChunkSize = 2 << 23; //default to 16MB chunks
long c_defaultMaxConnections = 32; //default number of simultaneous connections
long c_defaultTimeoutSecs = 600; //default timeout for a connection, 5 mins

size_t ffdl_write_data_to_file( void *ptr, size_t size, size_t nmemb, FILE *stream );

size_t ffdl_write_data_to_file( void *ptr, size_t size, size_t nmemb, FILE *stream )
{
    size_t written = fwrite( ptr, size, nmemb, stream );
    return written;
}

static size_t header_size( void *ptr, size_t size, size_t nmemb, void* data )
{
    (void) ptr;
    (void) data;

    return ( size_t )( size * nmemb );
}

size_t ffdl_get_file_size_bytes( char * url )
{
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    double filesize = 0.0;

    if ( curl )
    {
        curl_easy_setopt( curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
        curl_easy_setopt( curl, CURLOPT_URL, url );
        curl_easy_setopt( curl, CURLOPT_NOBODY, TRUE );
        curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, header_size );
        curl_easy_setopt( curl, CURLOPT_HEADER, FALSE );

        res = curl_easy_perform( curl );

        if ( res == 0 )
        {
            res = curl_easy_getinfo( curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize );
        }
    }

    curl_easy_cleanup( curl );

    return filesize;
}

int setup_curl_handlers( CURLM *curlMulti, char * url, char * filename, unsigned long long numberOfChunks, unsigned long long chunkSize, long timeout, long rateLimit, CURL ** curlHandles, FILE ** fileDescriptors )
{
    CURL * curl = NULL;
    size_t chunkFilenameSize = strnlen( filename, 2 << 16 ) + 15UL;
    char chunkFilename[chunkFilenameSize];
    int status = TRUE;
    unsigned long long rangeStart = 0;
    unsigned long long rangeEnd = chunkSize;
    size_t rangeBufferSize = 4096;
    char range[rangeBufferSize];
    unsigned long long i;
    for (i = 0; i < numberOfChunks; ++i )
    {
        //setup range in bytes
        snprintf( range, rangeBufferSize, "%llu-%llu", rangeStart, rangeEnd );
#ifdef DEBUG
        fprintf( stdout, "%s\n", range );
#endif

        rangeStart += chunkSize + 1;
        rangeEnd = rangeStart + chunkSize;

        curlHandles[i] = curl_easy_init();
        curl = curlHandles[i];
        if ( curl )
        {
            snprintf( chunkFilename, chunkFilenameSize, "%s.pt%llu", filename, i );

            fileDescriptors[i] = fopen( chunkFilename, "wb" );

            curl_easy_setopt( curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
            curl_easy_setopt( curl, CURLOPT_NOSIGNAL, 1 );
            curl_easy_setopt( curl, CURLOPT_URL, url );
            curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, ffdl_write_data_to_file );
            curl_easy_setopt( curl, CURLOPT_WRITEDATA, fileDescriptors[i] );
            curl_easy_setopt( curl, CURLOPT_RANGE, range );

            if ( timeout > 0 )
            {
                curl_easy_setopt( curl, CURLOPT_TIMEOUT, timeout );
            }

            if ( rateLimit > 0 )
            {
                curl_easy_setopt( curl, CURLOPT_MAX_RECV_SPEED_LARGE, rateLimit );
            }

            curl_multi_add_handle( curlMulti, curl );
        }
        else
        {
            status = FALSE;
        }
    }
    return status;
}

void clean_up_curl_multi_connections( CURLM *curlMulti, unsigned long long numberOfChunks, CURL * curlHandles[], FILE * fileDescriptors[] )
{
    CURL * curl = NULL;
    curl_multi_cleanup( curlMulti );

    unsigned long long i;
    for ( i = 0; i < numberOfChunks; ++i )
    {
        curl = curlHandles[i];
        if ( curl )
        {
            fclose( fileDescriptors[i] );
        }

        curl_easy_cleanup( curl );
    }
}

int perform_multi_curl_download( CURLM *curlMulti, unsigned long long numberOfChunks, unsigned long long timeout, CURL *handles[] )
{
    int status = TRUE;
    int stillRunning = 0;
    CURLMsg *msg;
    int msgsLeft;
    curl_multi_perform( curlMulti, &stillRunning );

    do
    {
        struct timeval timeoutVal;
        int returnCode;

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = -1;

        long curl_timeo = -1;

        FD_ZERO( &fdread );
        FD_ZERO( &fdwrite );
        FD_ZERO( &fdexcep );

        timeoutVal.tv_sec = timeout;
        timeoutVal.tv_usec = 0;

        curl_multi_timeout( curlMulti, &curl_timeo );
        if ( curl_timeo >= 0 )
        {
            timeoutVal.tv_sec = curl_timeo / 1000;
            if ( timeoutVal.tv_sec > 1 )
                timeoutVal.tv_sec = 1;
            else
                timeoutVal.tv_usec = ( curl_timeo % 1000 ) * 1000;
        }

        curl_multi_fdset( curlMulti, &fdread, &fdwrite, &fdexcep, &maxfd );

        returnCode = select( maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeoutVal );

        switch ( returnCode )
        {
            case -1:
                fprintf( stderr, "error: downloading multi part file %d", returnCode );
                status = FALSE;
                break;
            case 0: //timeout
            default: //action
                curl_multi_perform( curlMulti, &stillRunning );
                break;
        }
    }
    while ( stillRunning );

    //check transfers
    while ( ( msg = curl_multi_info_read( curlMulti, &msgsLeft ) ) )
    {
        if ( msg->msg == CURLMSG_DONE )
        {
            int idx, found = 0;

            /* Find out which handle this message is about */
            for ( idx = 0; idx < numberOfChunks; idx++ )
            {
                found = ( msg->easy_handle == handles[idx] );
                if ( found ) break;
            }
        }
    }

    return status;
}

void merge_chunks( unsigned long long numberOfChunks, unsigned long long chunkSize, char * filename )
{
    FILE * finalFile = fopen( filename, "w" );

    if ( finalFile == NULL )
    {
        fprintf( stderr, "error: could not open file %s", filename );
        return;
    }

    size_t chunkFilenameSize = strnlen( filename, 2 << 16 ) + 15UL;
    char chunkFilename[chunkFilenameSize];
    char *buf = (char *) malloc( chunkSize + 1);
    size_t n = 0;

    unsigned long long i;
    for ( i = 0; i < numberOfChunks; ++i )
    {
        snprintf( chunkFilename, chunkFilenameSize, "%s.pt%llu", filename, i );

        FILE * chunkedFile = fopen( chunkFilename, "r" );

        while( ( n = fread( buf, 1, chunkSize, chunkedFile ) ) > 0 )
        {
            if ( fwrite( buf, 1, n, finalFile ) != n )
            {
                fprintf( stderr, "error: merging chunk %s to file %s", chunkFilename, filename );
            }
        }

        fclose( chunkedFile );

        //error when removing chunked file
        if ( remove( chunkFilename ) != 0 )
        {
            fprintf( stderr, "error: could not remove chunked file %s", chunkFilename );
        }
    }

    fclose( finalFile );
    free( buf );
}

int ffdl_download_to_file_with_options( char * url, char * filename, unsigned long long chunkSize, long maxConnections, long timeout, long rateLimit )
{
    //initialize curl
    curl_global_init (CURL_GLOBAL_ALL);
    int result = TRUE;
    double filesize = ffdl_get_file_size_bytes( url );

#ifdef DEBUG
    fprintf( stdout, "filesize: %f\n", filesize );
#endif

    if ( chunkSize == 0 )
    {
        chunkSize = c_defaultChunkSize;
    }

    if ( maxConnections == 0 )
    {
        maxConnections = c_defaultMaxConnections;
    }

    if ( timeout < 0 )
    {
        timeout = c_defaultTimeoutSecs;
    }

    if ( rateLimit < 0 )
    {
        rateLimit = 0;
    }

#ifdef DEBUG
    fprintf( stdout, "Options: chunksize: %llu\tmaxConnections: %ld\ttimeout: %ld\trateLimit: %ld\n", chunkSize, maxConnections, timeout, rateLimit );
#endif

    unsigned long long numberOfChunks = ceil( filesize / (double)( chunkSize ) );

    CURL ** curlHandles = (CURL **) malloc( numberOfChunks * sizeof( CURL * ) );
    FILE ** fileDescriptors = (FILE **) malloc( numberOfChunks * sizeof( FILE * ) );
    CURLM * curlMulti = curl_multi_init();

    curl_multi_setopt( curlMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxConnections );

    setup_curl_handlers( curlMulti, url, filename, numberOfChunks, chunkSize, timeout, rateLimit, curlHandles, fileDescriptors );

    perform_multi_curl_download( curlMulti, numberOfChunks, timeout, curlHandles );

    clean_up_curl_multi_connections( curlMulti, numberOfChunks, curlHandles, fileDescriptors );

    merge_chunks( numberOfChunks, chunkSize, filename );

    curl_global_cleanup();
    free( curlHandles );
    free( fileDescriptors );
    return result;
}

int ffdl_download_to_file( char * url, char * filename )
{
    return ffdl_download_to_file_with_options( url, filename, 0, 0, 0, 0 );
}
