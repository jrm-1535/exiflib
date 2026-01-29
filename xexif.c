
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
//#include <unistd.h>
//#include <sys/stat.h>

#include "exif.h"

void help( void )
{
    printf( "xexif [-h] [-v] [-o=<path>] path\n\n" );
    printf( "xefif extracts exif metadata from the file specified by path.\n" );
    printf( "Exif metadata is usually found in JPG (JFIF) or HEIF files.\n" );
    printf( "If the Exif header is not found xexif looks for a TIFF header\n" );
    printf( "and extracts the tiff IFD.\n" );
    printf( "\noptions:\n" );
    printf( "  -h         print this help message and exit\n" );
    printf( "  -o=<path>  output file path (default to stdout)\n" );
    printf( "  -v         verbose.\n");
    printf( "path is the pathname of the input file.\n" );
}

static void arg_error_msg( const char *msg, ... )
{
    va_list ap;
    va_start( ap, msg );
    fputs( "rdir: ", stderr );
    vfprintf( stderr, msg, ap );
    putc( '\n', stderr );
    va_end( ap );
    help();
    exit(1);
}

typedef struct {
    char    *input, *output;
    bool    verbose;
} options_t;

static void get_args( int argc, char **argv, options_t *args )
{
    args->input = NULL;
    args->output = NULL;
    args->verbose = false;

    for ( int i = 1; i < argc; ++i ) {
        char *arg = argv[i];
        switch (*arg) {
        case '-':
            for ( int j = 1; 0 != arg[j]; ++j) {
                switch (arg[j]) {
                case 'h': case 'H':
                    help();
                    exit(0);

                case 'o': case 'O':
                    if ( '=' != arg[j+1]) {
                        arg_error_msg( "'=' is required for option -o" );
                    }
                    if ( NULL != args->output ) {
                        arg_error_msg( "Only one output path is possible" );
                    }
                    args->output = &arg[j+2];
                    break;

                case 'v': case 'V':
                    args->verbose = true;
                    break;

                default:
                    arg_error_msg( "unrecognized option -%s", &arg[i] );
                }
                j = strlen(arg) -1;
            }
            break;

        default:    // the input pathname
            if ( NULL != args->input ) {
                arg_error_msg( "Only one input path is possible" );
            }
            args->input = arg;
            break;
        }
    }
    if ( NULL == args->input ) {
        arg_error_msg( "No input path was given\n" );
    }
    if ( NULL == args->output || '\0' == args->output[0] ) {
        fprintf( stderr, "No output path was given, using stdout instead\n" );
        args->output = NULL;
    }
}

#define BUFFER_SIZE 65536

extern int main( int argc, char **argv )
{
    options_t  args;
    get_args( argc, argv, &args );

    errno = 0;
    FILE *inf = fopen( args.input, "r" );
    if ( NULL == inf ) {
        printf( "xexif: error: unable to open %s (%s)\n",
                args.input, strerror(errno) );
        exit(2);
    }

    FILE *outf;
    if ( args.output ) {
        errno = 0;
        outf = fopen( args.output, "w" );
        if ( NULL == outf ) {
            printf( "xexif: error: unable to open %s (%s)\n",
                    args.output, strerror(errno) );
            exit(2);
        }
    } else {
        outf = stdout;
    }
    exif_control_t control;
    control.skip_unknown_tags = true;
    control.warnings = true;
    control.parse_debug = true;

    uint8_t buffer[ BUFFER_SIZE ];

    exif_desc_t *desc = parse_exif( inf, 0, &control );
    if ( NULL != desc ) {

        size_t length;
        off_t  offset = exif_get_segment( desc, &length );

        if ( args.verbose ) {
            printf("Found exif/tiff @%ld, length %ld\n", offset, length );
        }

        fseek( inf, offset, SEEK_SET );
        for ( size_t count = 0; count < length;  ) {
            size_t n = ( length < BUFFER_SIZE ) ? length : BUFFER_SIZE;
            errno = 0;
            size_t r = fread( buffer, 1, n, inf );
            if ( r != n ) {
                fprintf( stderr, "Error reading input file (%s)\n",
                         strerror(errno) );
                break;
            }
            errno = 0;
            size_t w = fwrite( buffer, 1, r, outf );
            if ( w != r ) {
                fprintf( stderr, "Error writing output file (%s)\n",
                         strerror(errno) );
            }
            count += r;
        }
    }
    if ( args.input ) {
        fclose( inf );
    }
    if ( args.output ) {
        fclose( outf );
    }
}

