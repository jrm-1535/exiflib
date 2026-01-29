
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "exif.h"

// list IFD tags before printing their values
#define LIST_IFD_TAGS 0

#if LIST_IFD_TAGS
static char *get_ifd_name( ifd_id_t id )
{
    switch ( id ) {
    case PRIMARY:   return "Primary IFD";
    case THUMBNAIL: return "Thumbnail IFD";
    case EXIF:      return "Exif IFD";
    case GPS:       return "GPS IFD";
    case IOP:       return "IOP_IFD";
    default:
        break;
    }
    return NULL;
}

// cmparison for sorting by increasing tag values.
static int cmp( const void *item1, const void *item2 )
{
    return *(uint16_t *)item1 - *(uint16_t *)item2;
}

static void print_ifd_keys( exif_desc_t *desc, ifd_id_t id )
{
    slice_t *tags = exif_get_ifd_tags( desc, id, cmp );
    if ( NULL == tags ) {
        printf( "Failed to get %s tags\n", get_ifd_name( id ) );
    } else {
        printf( "%s tags: ", get_ifd_name( id ) );
        for ( size_t i = 0; i < slice_len( tags ); ++i ) {
            // t are just the uint16_t exif/tiff tag
            uint16_t tag = *(uint16_t *)slice_item_at( tags, i );
            printf("0x%04x ", tag);
        }
        printf( "\n" );
        slice_free( tags );
    }
}
#endif
void help( void )
{
    printf( "showexif [-h] [-v] path\n\n" );
    printf( "showexif look up exif metadata from the file specified by path.\n" );
    printf( "\noptions:\n" );
    printf( "  -h         print this help message and exit\n" );
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
    char    *input;
    bool    verbose;
} options_t;

static void get_args( int argc, char **argv, options_t *args )
{
    args->input = NULL;
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
}

int main( int argc, char **argv )
{
    options_t  args;
    get_args( argc, argv, &args );

    exif_desc_t *desc = read_exif( args.input, 0, NULL );
    if ( NULL == desc ) {
        printf("Failed to read exif content\n" );
        exit(2);
    }
#if LIST_IFD_TAGS
    print_ifd_keys( desc, PRIMARY );
    print_ifd_keys( desc, THUMBNAIL );
    print_ifd_keys( desc, EXIF );
    print_ifd_keys( desc, GPS );
#endif
    printf( "\nPrimary Metadata:\n");
    exif_print_ifd_entries( desc, PRIMARY, "  " );

    printf( "\nThumbnail Metadata:\n");
    exif_print_ifd_entries( desc, THUMBNAIL, "  " );

    printf( "\nExif Metadata:\n");
    exif_print_ifd_entries( desc, EXIF, "  " );

    printf( "\nGPS Metadata:\n");
    exif_print_ifd_entries( desc, GPS, "  " );

    exif_free( desc );
    return 0;
}
