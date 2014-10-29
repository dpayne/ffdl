#include "ffdl.h"
#include "stdio.h"

int main( int argc, char * argv[] )
{
    if ( argc == 3 )
    {
        if ( ffdl_download_to_file( argv[1], argv[2] ) == 1 )
        {
            //success!
            return 0;
        }
        else
        {
            //failed to download file
            return 2;
        }
    }
    else
    {
        fprintf( stderr, "error: wrong number of arguments\nUsage ffdl http://someurl.com/file /tmp/some_local_file\n" );
    }

    //failed
    return 1;
}
