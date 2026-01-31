
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
    printf( "injexif [-h] [-v] -o=<out-path> -m=<exif-path> path\n\n" );
    printf( "injefif injects the provided exif metadata into the file\n" );
    printf( "specified by path and writes the comined files into the\n" );
    printf( "file specified as output.\n" );
    printf( "\noptions:\n" );
    printf( "  -h         print this help message and exit\n" );
    printf( "  -m=<path>  input exif metadata file path\n" );
    printf( "  -o=<path>  output file path\n" );
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
    char    *exif, *input, *output;
    bool    verbose;
} options_t;

static void get_args( int argc, char **argv, options_t *args )
{
    args->exif = NULL;
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

                case 'm': case 'M':
                    if ( '=' != arg[j+1]) {
                        arg_error_msg( "'=' is required for option -i" );
                    }
                    if ( NULL != args->exif ) {
                        arg_error_msg( "Only one exif path is possible" );
                    }
                    args->exif = &arg[j+2];
                    break;

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
    if ( NULL == args->exif || '\0' == args->output[0]  ) {
        arg_error_msg( "No exif metdata path was given\n" );
    }
    if ( NULL == args->output || '\0' == args->output[0] ) {
        arg_error_msg( "No output path was given" );
    }
}

static bool is_jfif_header( FILE *f )
{
    int c = getc( f );
    if ( c < 0 || c != 0xff ) {
        return false;
    }
    c = getc( f );
    if ( c < 0 || c != (int)0xd8 ) {
        return false;
    }
    return true;
}

#define BUFFER_SIZE 65536
static uint8_t buffer[ BUFFER_SIZE ];

// returns 0 is not a valid JPEG marker, or the 2nd byte of marker if valid
// (from 0xc0 to 0xff) and updates the segment size (0 if stand alone marker)
static uint8_t get_marker_n_data_size( FILE *f, size_t *size )
{
    int c = getc( f );
    if ( c < 0 || c != 0xff ) {
        return 0;
    }
    c = getc( f );
    if ( c < (int)0xc0 ) {
        return 0;
    } 
    uint8_t mcode = (uint8_t)c;
    uint16_t dlen = 0;
    if ( mcode < 0xd0 || mcode > 0xd9 ) {
        c = getc( f );
        if ( c < 0 ) {
            return 0;
        }
        dlen = (uint16_t)c << 8;
        c = getc( f );
        if ( c < 0 ) {
            return 0;
        }
        dlen += (uint16_t)c;
        dlen -= 2;              // dlen includes itself
    }
    *size = dlen;
    return mcode;
}

static bool copy_input_app0_to_output( FILE *inf, FILE *ouf )
{
    size_t dlen;
    uint8_t mcode = get_marker_n_data_size( inf, &dlen );
    if ( 0xe0 != mcode ) {  // must start with APP0
        printf( "injexif: error: invalid input file\n" );
        return false;
    }
    size_t l = fread( buffer, 1, dlen, inf );
    if ( dlen != l ) {
        printf( "injexif: error: unable to read APP0 (%s)\n", strerror(errno) );
        return false;
    }

    unsigned char *soiapp0 = (unsigned char *)"\xff\xd8\xff\xe0";
    errno = 0;
    size_t n = fwrite( soiapp0, 1, 4, ouf );
    if ( 4 != n ) {
        printf( "injexif: error: unable to write output file (%s)\n",
                strerror(errno) );
        return false;
    }

    dlen += 2;      // segment length includes itself
    fputc( dlen >> 8, ouf );
    fputc( dlen & 0xff, ouf );
    dlen -= 2;      // back to following data length

    errno = 0;
    l = fwrite( buffer, 1, dlen, ouf );
        if ( dlen != l ) {
        printf( "injexif: error: unable to write APP0 (%s)\n",
                strerror(errno) );
        return false;
    }

    // next segment may be APP1 in original file; in that case skip it as it
    // will be replaced with the new exif segment.
    mcode = get_marker_n_data_size( inf, &dlen );
    if ( 0xe1 == mcode ) {
        fseek( inf, (long)dlen, SEEK_SET );
    } else {
        fseek( inf, -4, SEEK_CUR );
    }
    return true;
}

static bool copy_exif_app1_to_output( FILE *exf, FILE *ouf )
{
    fseek( exf, 0L, SEEK_END );
    long len = ftell( exf );
    rewind( exf );

    unsigned char *app1 = (unsigned char *)"\xff\xe1";
    errno = 0;
    size_t n = fwrite( app1, 1, 2, ouf );
    if ( 2 != n ) {
        printf( "injexif: error: unable to write output file (%s)\n",
                strerror(errno) );
        return false;
    }
    len += 2;           // count itself in
    assert( len < 65536 );
    fputc( len >> 8, ouf );
    fputc( len & 0xff, ouf );

    len -= 2;           // back to actual data size
    size_t l = fread( buffer, 1, len, exf );
    if ( len != l ) {
        printf( "injexif: error: unable to read exif data (%s)\n",
                strerror(errno) );
        return false;
    }
    errno = 0;
    l = fwrite( buffer, 1, len, ouf );
        if ( len != l ) {
        printf( "injexif: error: unable to write exif data (%s)\n", 
                strerror(errno) );
        return false;
    }
    return true;
}

static bool copy_remaining_input_to_output( FILE *inf, FILE *ouf )
{
    while ( true ) {
        errno = 0;
        size_t nr = fread( buffer, 1, BUFFER_SIZE, inf );
        if ( BUFFER_SIZE != nr ) {
            if ( ferror( inf ) ) {
                printf( "injexif: error: unable to read remaining data (%s)\n",
                        strerror(errno) );
                return false;
            }
        }
        errno = 0;
        size_t nw = fwrite( buffer, 1, nr, ouf );
            if ( nr != nw ) {
            printf( "injexif: error: unable to write remaining data (%s)\n", 
                    strerror(errno) );
            return false;
        }
        if ( BUFFER_SIZE != nr ) {      // should be EOF
            break;
        }
    }
    return true;
}

extern int main( int argc, char **argv )
{
    options_t  args;
    get_args( argc, argv, &args );

    errno = 0;
    FILE *inf = fopen( args.input, "r" );
    if ( NULL == inf ) {
        printf( "injexif: error: unable to open %s (%s)\n",
                args.input, strerror(errno) );
        exit(2);
    }

    FILE *exf = fopen( args.exif, "r" );
    if ( NULL == exf ) {
        printf( "injexif: error: unable to open %s (%s)\n",
                args.exif, strerror(errno) );
        exit(2);
    }
    errno = 0;
    FILE *ouf = fopen( args.output, "w" );
    if ( NULL == ouf ) {
        printf( "injexif: error: unable to open %s (%s)\n",
                args.output, strerror(errno) );
        exit(2);
    }

    if ( ! is_jfif_header( inf ) ) {
        printf( "injexif: error: input file is not a JPEG file\n" );
        exit(2);
    }
    if ( ! copy_input_app0_to_output( inf, ouf ) ) {
        exit(3);
    }
    if ( ! copy_exif_app1_to_output( exf, ouf ) ) {
        exit(3);
    }
    if ( ! copy_remaining_input_to_output( inf, ouf ) ) {
        exit(3);
    }
    printf("Exif replacement completed\n" );

    fclose( ouf );
    fclose( exf ) ;
    fclose( inf );
 
    exit(0);
}
