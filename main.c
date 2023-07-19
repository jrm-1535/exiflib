

#include <stdio.h>
#include "exif.h"

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

int main( int argc, char **argv )
{
    if ( argc < 2 ) {
        printf("Expect a picture file name\n");
        return 1;
    }

    exif_desc_t *desc = read_exif( argv[1], 0, NULL );
    if ( NULL == desc ) {
        printf("Failed to read exif content\n" );
        return 2;
    }

    print_ifd_keys( desc, PRIMARY );
    print_ifd_keys( desc, THUMBNAIL );
    print_ifd_keys( desc, EXIF );

    printf( "\nPrimary Metadata:\n");
    exif_print_ifd_entries( desc, PRIMARY, "  " );

    printf( " \nThumbnail Metadata:\n");
    exif_print_ifd_entries( desc, THUMBNAIL, "  " );

    printf( " \nExif Metadata:\n");
    exif_print_ifd_entries( desc, EXIF, "  " );

    exif_free( desc );
    return 0;
}
