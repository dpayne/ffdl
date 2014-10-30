#include "ffdl.h"
#include "stdio.h"
#include "string.h"

int main( int argc, char * argv[] )
{
    char *filename = NULL;
    char *url = NULL;

    if ( argc == 3 )
    {
        url = argv[1];
        filename = argv[2];
    }
    //try to guess filename from url
    else if ( argc == 2 )
    {
        url = argv[1];
        filename = strrchr( url, '/' );

        if ( filename == NULL )
        {
            filename = url;
        }
        else
        {
            ++filename;
            if ( filename == '\0' )
            {
                filename = url;
            }
        }
    }
    else
    {
        fprintf( stderr, "error: wrong number of arguments\nUsage ffdl http://someurl.com/file /tmp/some_local_file\n" );
        return 1;
    }

    printf( "Url: %s\tfilename:%s\n", url, filename );

    if ( ffdl_download_to_file( url, filename ) == 1 )
    {
        //success!
        return 0;
    }

    //failed to download file
    return 2;
}
