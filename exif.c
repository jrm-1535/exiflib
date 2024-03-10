
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <stdbool.h>

#include "exif.h"
#include "parse.h"
#include "print.h"
#include "_slice.h"

static exif_desc_t *new_exif_desc( void )
{
    exif_desc_t *desc = malloc( sizeof(exif_desc_t) );
    if ( NULL != desc ) {
        memset( desc, 0, sizeof(exif_desc_t) );
    }
    return desc;
}

// read a uint16_t according to endianess
extern uint16_t tiff_get_uint16( exif_desc_t *d )
{
    uint8_t data[2];
    fread( data, 1, 2, d->file );

    uint16_t val;
    if ( d->big_endian ) {
        val = data[0] << 8;
        val |= data[1];
    } else {
        val = data[0];
        val |= data[1] << 8;
    }
    return val;
}

// read a uint32_t according to endianess
extern uint32_t tiff_get_uint32( exif_desc_t *d )
{
    uint8_t data[4];
    fread( data, 1, 4, d->file );

    uint32_t val;
    if ( d->big_endian ) {
        val = data[0] << 24;
        val |= data[1] << 16;
        val |= data[2] << 8;
        val |= data[3];
    } else {
        val = data[0];
        val |= data[1] << 8;
        val |= data[2] << 16;
        val |= data[3] << 24;
    }
    return val;
}

// raw read 4 bytes without considering endianess
extern uint32_t tiff_get_raw_uint32( exif_desc_t *d )
{
    uint8_t data[4];
    fread( data, 1, 4, d->file );
    return *(uint32_t *)data;
}

extern uint32_t tiff_endianize_uint32( exif_desc_t *d, uint32_t raw )
{
    uint8_t data[4];
    *(uint32_t *)data = raw;

    uint32_t val;
    if ( d->big_endian ) {
        val = data[0] << 24;
        val |= data[1] << 16;
        val |= data[2] << 8;
        val |= data[3];
    } else {
        val = data[0];
        val |= data[1] << 8;
        val |= data[2] << 16;
        val |= data[3] << 24;
    }
    return val;
}

extern uint16_t tiff_endianize_uint16( exif_desc_t *d, uint16_t raw )
{
    uint8_t data[2];
    *(uint16_t *)data = raw;

    uint16_t val;
    if ( d->big_endian ) {
        val = data[0] << 8;
        val |= data[1];
    } else {
        val = data[0];
        val |= data[1] << 8;
    }
    return val;
}

// consumes the TIFF endianess marker (2 bytes), updates the descriptor
// big_endian value by side effect and returns true if the endianess is
// either big endian or little endian, or false otherwise. Any multi-byte
// value following this point must take in account the TIFF endianess.
static bool check_tiff_endianess( FILE *f, bool *is_big_endian )
{
    // TIFF header starts with 2 bytes indicating the byte ordering ("II" short
    // for Intel or "MM" short for Motorola, indicating little or big endian
    // respectively)

    unsigned char data[4];
    fread( data, 1, 2, f );
    if ( data[0] == 'I' && data[1] == 'I' ) {
        if ( NULL != is_big_endian ) {
            *is_big_endian = false;
        }
        return true;
    }
    if ( data[0] == 'M' && data[1] == 'M' ) {
        if ( NULL != is_big_endian ) {
            *is_big_endian = true;
        }
        return true;
    }
    return false;
}

// check if the tiff header has the correct validity marker (0x2a) and
// returns false if it does not. Otherwise it updates the ifd_offset by
// side effect and returns true.
static bool check_tiff_validity( exif_desc_t *d, uint32_t *ifd_offset )
{
    if ( 0x002a != tiff_get_uint16( d ) ) {
        return false;
    }
    // followed by Primary Image File directory (IFD) offset
    *ifd_offset = tiff_get_uint32( d );
    return true;
}

//  starting at the tiff header (all offsets are relative to the TIFF header)
static exif_desc_t *parse_tiff( FILE *f, exif_control_t *control )
{

    exif_desc_t *d = new_exif_desc( );
    if ( NULL == d ) {
        return NULL;
    }
    d->file = f;
    d->header = ftell( f );  // keep TIFF header location

    if ( ! check_tiff_endianess( f, &(d->big_endian) ) ) {
        free( d );
        return NULL;
    }
    d->control = *control;

    uint32_t ifd_offset;    // offset relative to the  TIF header
    if ( ! check_tiff_validity( d, &ifd_offset ) ) {
        free( d );
        return NULL;
    }
    fseek( f, d->header + ifd_offset, SEEK_SET );
    map_t *ifd_map = exif_parse_ifd( d, PRIMARY, &ifd_offset );
    if ( NULL == ifd_map ) {
        free( d );
        return NULL;
    }
    d->ifds[ PRIMARY ] = ifd_map;
    if ( 0 != ifd_offset ) {
        fseek( f, d->header + ifd_offset, SEEK_SET );
        map_t *ifd_map = exif_parse_ifd( d, THUMBNAIL, NULL );
        if ( NULL == ifd_map ) {
            free( d );
            return NULL;
        }
        d->ifds[ THUMBNAIL ] = ifd_map;
    }
    return d;
}

// bitap table for Exif
static unsigned char masks [256];

static void init_exif_bitap( void ) {
    for ( int i = 0; i < 256; ++i ) {
        masks[i] = 0xff;
    }

    masks['E'] = 0xfe;  // E position at bit 0 in pattern,
    masks['x'] = 0xfd;  // x position at bit 1,
    masks['i'] = 0xfb;  // i position at bit 2,
    masks['f'] = 0xf7;  // f position at bit 3,
    masks[ 0 ] = 0xcf;  // \0 at 2 positions, bits 4 and 5.
}

extern exif_desc_t *parse_exif( FILE *f, uint32_t start,
                                exif_control_t *control )
{
    if ( NULL == f ) return NULL;

    fseek( f, (long)start, SEEK_SET );

    init_exif_bitap( );
    unsigned char bit_mask = 0xfe;

    if ( NULL == control ) {
        exif_control_t default_control = { 0 };
        control = &default_control;
    }
    while ( true ) {

        int byte = getc( f );
        if ( EOF == byte ) {
            break;
        }

        bit_mask |= masks[byte];
        bit_mask <<= 1;
        if ( 0 == ( bit_mask & 64 ) ) {
            return parse_tiff( f, control );
        }
    }
    if ( control->warnings ) {
        printf( "Did not find EXIF header\n" );
    }
    exif_desc_t *desc = NULL;
    fseek( f, (long)start, SEEK_SET );
    if ( check_tiff_endianess( f, NULL ) ) {
        fseek( f, -2, SEEK_CUR );
        desc = parse_tiff( f, control );
    }
    if ( NULL == desc && control->warnings ) {
        printf( "Did not find TIFF header\n" );
    }
    return desc;
}

extern exif_desc_t *read_exif( char *path, uint32_t start,
                               exif_control_t *control )
{
    if ( NULL == path ) return NULL;

    FILE *f = fopen( path, "r" );
    if ( NULL == f ) {
        return NULL;
    }

    exif_desc_t *desc = parse_exif( f, start, control );

    fclose( f );
    return desc;
}

extern slice_t *exif_get_ifd_ids( exif_desc_t *desc )
{
    if ( NULL == desc ) {
        return NULL;
    }

    size_t n = 0;
    for ( ifd_id_t i = PRIMARY; i < _IFD_N; ++i ) {
        if ( NULL != desc->ifds[i] ) ++n;
    }

    slice_t *ids = new_slice( sizeof(ifd_id_t), n );
    if ( NULL == ids ) {
        return NULL;
    }

    for ( ifd_id_t i = PRIMARY; i < _IFD_N; ++i ) {
        if ( NULL != desc->ifds[i] ) {
            slice_append_item( ids, &i );
        }
    }
    return ids;
}

// cmparison for sorting by increasing tag values.
static int cmp( const void *item1, const void *item2 )
{
    return *(uint16_t *)item1 - *(uint16_t *)item2;
}

extern slice_t *exif_get_ifd_tags( exif_desc_t *desc, ifd_id_t id, comp_fct cmp )
{
    if ( NULL == desc || id < PRIMARY || id >= _IFD_N || NULL == desc->ifds[id] )
        return NULL;

    slice_t *keys = map_keys( desc->ifds[id], NULL );
    if ( NULL == keys ) {
        return NULL;
    }

    // in a map, keys are void pointers, whereas tags are uint16_t values,
    // which unfortunately requires a transfer to a different tag slice
    slice_t *tags = new_slice( sizeof(uint16_t), slice_len( keys ) );
    if ( NULL == tags ) {
        slice_free( keys );
        return NULL;
    }

    for ( size_t i = 0; i < slice_len( keys ); ++i ) {
        uint16_t tag = (uint16_t)((size_t)_pointer_slice_item_at( keys, i ));
        slice_append_item( tags, &tag );
    }
    slice_free( keys );
    if ( NULL != cmp ) {
        slice_sort_items( tags, cmp );
    }
    return tags;
}

extern exif_type_t exif_get_ifd_tag_type( exif_desc_t *desc, ifd_id_t id,
                                          uint16_t tag )
{
    if ( NULL == desc || id < PRIMARY || id >= _IFD_N || NULL == desc->ifds[id] ) {
        return NOT_A_TYPE;
    }

    size_t key = make_key_from_tag( tag );
    const void *res = map_lookup_entry( desc->ifds[id], (void *)key );
    if ( NULL == res ) {
        return NOT_A_TYPE;
    }
    // TODO make a sorted tag type array...
    return NOT_A_TYPE;
}

extern bool exif_get_ifd_tag_values( exif_desc_t *desc, ifd_id_t id,
                                     uint16_t tag, vector_t **values )
{
    if ( NULL == desc || id < PRIMARY || id >= _IFD_N || NULL == desc->ifds[id] ) {
        return false;
    }
    size_t key = make_key_from_tag( tag );
    const void *res = map_lookup_entry( desc->ifds[id], (void *)key );
    if ( NULL == res ) {
        return false;
    }
    if ( NULL != values ) {
        *values = (vector_t *)res;
    }
    return true;
}

extern bool exif_print_ifd_entries( exif_desc_t *desc, ifd_id_t id,
                                    char *indent_string )
{
    slice_t *tags = exif_get_ifd_tags( desc, id, cmp );
    if ( tags ) {
        print_ifd_tags( desc, id, tags, indent_string );
        slice_free( tags );
        return true;
    }
    return false;
}

static bool free_map_entry( uint32_t index,
                            const void *key, const void *data, void *context )
{
    vector_t *entry = (vector_t *)data;
    vector_free( entry );   // map entries are all vectors
    return false;
}

extern bool exif_free( exif_desc_t *desc )
{
    if ( NULL == desc ) {
        return false;
    }
    for ( uint8_t i = PRIMARY; i < _IFD_N; ++i ) {
        if ( NULL != desc->ifds[i] ) {
            map_process_entries( desc->ifds[i], free_map_entry, NULL );
            map_free( desc->ifds[i] );
        }
    }
    free( desc );
    return true;
}
