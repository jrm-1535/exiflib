#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exif.h"
#include "parse.h"

typedef void (parse_tag_fct)( ifd_desc_t *ifdd );

static inline bool check_entry_type( ifd_desc_t *ifdd )
{
    if ( ifdd->type < TIFF_UINT8 || ifdd->type > TIFF_DOUBLE ) {
        return false;
    }
    return true;
}

static uint32_t tiff_type_size[] = { /* unused */           0,
                                     /* TIFF_UINT8 */       BYTE_SIZE,
                                     /* TIFF_STRING */      BYTE_SIZE,
                                     /* TIFF_UINT16 */      SHORT_SIZE,
                                     /* TIFF_UINT32 */      LONG_SIZE,
                                     /* TIFF_URATIONAL */   RATIONAL_SIZE,
                                     /* TIFF_INT8 */        BYTE_SIZE,
                                     /* TIFF_UNDEFINED */   0,
                                     /* TIFF_INT16 */       SHORT_SIZE,
                                     /* TIFF_INT32 */       LONG_SIZE,
                                     /* TIFF_RATIONAL */    RATIONAL_SIZE,
                                     /* TIFF_FLOAT */       FLOAT_SIZE,
                                     /* TIFF_DOUBLE */      DOUBLE_SIZE };

static inline bool is_direct_value( ifd_desc_t *ifdd )
{
    uint32_t n_bytes = ifdd->count * tiff_type_size[ifdd->type];
    if ( n_bytes > 0 && n_bytes <= 4 ) {
        return true;
    }
    return false;
}

static inline void ifdd_map_insert_array( ifd_desc_t *ifdd, vector_t *array )
{
    size_t key = make_key_from_tag( ifdd->tag ); // force key to be non-zero
    if ( map_insert_entry( ifdd->content, (void *)key, array ) ) {
        return;     // success...
    }
    // failed to insert this entry, perhaps same as a previous one?
    vector_t *previous_array =
                    (vector_t *)map_lookup_entry( ifdd->content, (void *)key);
    if ( NULL != previous_array ) {
        map_delete_entry( ifdd->content, (void *)key ); // rid of previous entry
        vector_free( previous_array );
    }
    map_insert_entry( ifdd->content, (void *)key, array );
}

static inline void add_tag_direct_byte_values( ifd_desc_t *ifdd )
{
    uint8_t val[4] = { 0 };     // max 4 bytes in direct entry value
                                // bytes are left-justified in field
    *(uint32_t *)val = ifdd->valoff;
    assert( ifdd->count <= 4 );
    vector_t *array = new_vector_from_data( val, BYTE_SIZE, ifdd->count );
    ifdd_map_insert_array( ifdd, array );
}

static inline void add_tag_direct_short_values( ifd_desc_t *ifdd )
{
    uint16_t val[2] = { 0 };    // max 2 shorts in direct entry value
                                // shorts are left-justified in field
    val[0] = tiff_endianize_uint16(ifdd->desc, (uint16_t)(ifdd->valoff));
    val[1] = tiff_endianize_uint16(ifdd->desc, (uint16_t)(ifdd->valoff >> 16));
    assert( ifdd->count <= 2 );
    vector_t *array = new_vector_from_data( val, SHORT_SIZE, ifdd->count );
    ifdd_map_insert_array( ifdd, array );
}

static inline void add_tag_direct_long_values( ifd_desc_t *ifdd )
{
    uint32_t val;               // exactly 1 long in direct entry value

    val = tiff_endianize_uint32(ifdd->desc, ifdd->valoff);
    assert( ifdd->count == 1 );
    vector_t *array = new_vector_from_data( &val, LONG_SIZE, 1 );
    ifdd_map_insert_array( ifdd, array );
}

static inline void move_file_position_to_offset( ifd_desc_t *ifdd )
{
    uint32_t offset = tiff_endianize_uint32( ifdd->desc, ifdd->valoff );
    ifdd->saved_pos = ftell( ifdd->desc->file );
    fseek( ifdd->desc->file, ifdd->desc->header + offset, SEEK_SET );
}

static inline void restore_file_position( ifd_desc_t *ifdd )
{
    fseek( ifdd->desc->file, ifdd->saved_pos, SEEK_SET );
}

static inline void add_tag_indirect_byte_values( ifd_desc_t *ifdd )
{
    vector_t *array = new_vector( BYTE_SIZE, ifdd->count );
    if ( NULL != array ) {
        move_file_position_to_offset( ifdd );
        for ( uint32_t i = 0; i < ifdd->count; ++i ) {
            uint8_t byte = getc( ifdd->desc->file );
            vector_write_item_at( array, i, &byte );
        }
        ifdd_map_insert_array( ifdd, array );
        restore_file_position( ifdd );
    }
}

static inline void add_tag_indirect_short_values( ifd_desc_t *ifdd )
{
    vector_t *array = new_vector( SHORT_SIZE, ifdd->count );
    if ( NULL != array ) {
        move_file_position_to_offset( ifdd );
        for ( uint32_t i = 0; i < ifdd->count; ++i ) {
            uint16_t word = tiff_get_uint16( ifdd->desc );
            vector_write_item_at( array, i, &word );
        }
        ifdd_map_insert_array( ifdd, array );
        restore_file_position( ifdd );
    }
}

static inline void add_tag_indirect_long_values( ifd_desc_t *ifdd )
{
    vector_t *array = new_vector( SHORT_SIZE, ifdd->count );
    if ( NULL != array ) {
        move_file_position_to_offset( ifdd );
        for ( uint32_t i = 0; i < ifdd->count; ++i ) {
            uint32_t lword = tiff_get_uint32( ifdd->desc );
            vector_write_item_at( array, i, &lword );
        }
        ifdd_map_insert_array( ifdd, array );
        restore_file_position( ifdd );
    }
}

static void add_tag_byte_values( ifd_desc_t *ifdd )
{
    if ( is_direct_value( ifdd ) ) {
        add_tag_direct_byte_values( ifdd );
    } else {
        add_tag_indirect_byte_values( ifdd );
    }
}

static void add_tag_short_values( ifd_desc_t *ifdd )
{
    if ( is_direct_value( ifdd ) ) {
        add_tag_direct_short_values( ifdd );
    } else {
        add_tag_indirect_short_values( ifdd );
    }
}

static void add_tag_long_values( ifd_desc_t *ifdd )
{
    if ( is_direct_value( ifdd ) ) {
        add_tag_direct_long_values( ifdd );
    } else {
        add_tag_indirect_long_values( ifdd );
    }
}

// add a map entry with key=tag and count value=integers of 8, 16 or 32 bits
// item_size.
static void add_tag_int_values( ifd_desc_t *ifdd )
{
    // create a vector for count entries of tiff_type_size
    switch( tiff_type_size[ifdd->type] ) {
    case BYTE_SIZE:
        add_tag_byte_values( ifdd );
        break;
    case SHORT_SIZE:
        add_tag_short_values( ifdd );
        break;
    case LONG_SIZE:
        add_tag_long_values( ifdd );
        break;
    }
}

// signed and unsigned rationals are treated the same way her. It is just
// a matter of interpreting what has been stored here.
static void add_tag_rational_values( ifd_desc_t *ifdd )
{
    // since a rational id too big to fit in valoff, no direct values here
    vector_t *array = new_vector( RATIONAL_SIZE, ifdd->count );
    if ( NULL != array ) {
        move_file_position_to_offset( ifdd );
        for ( uint32_t i = 0; i < ifdd->count; ++i ) {
            uint32_t rational[2];
            rational[0] = tiff_get_uint32( ifdd->desc );
            rational[1] = tiff_get_uint32( ifdd->desc );
            vector_write_item_at( array, i, rational );
        }
        ifdd_map_insert_array( ifdd, array );
        restore_file_position( ifdd );
    }
}

static void process_n_unsigned_bytes( ifd_desc_t *ifdd, uint32_t n )
{
    if ( TIFF_UINT8 == ifdd->type && ( n == 0 || n == ifdd->count ) ) {
        add_tag_byte_values( ifdd );
    }
}

// if n == 0 accept any count, otherwise accept only count == n
static void process_n_unsigned_shorts( ifd_desc_t *ifdd, uint32_t n )
{
    if ( TIFF_UINT16 == ifdd->type && ( n == 0 || n == ifdd->count ) ) {
        add_tag_short_values( ifdd );
    }
}

// if u == 0 accept only if l <= count otherwise accept only if l <= count <= u
static void process_min_max_unsigned_shorts( ifd_desc_t *ifdd,
                                             uint32_t l, uint32_t u )
{
    if ( TIFF_UINT16 == ifdd->type && l <= ifdd->count &&
        ( 0 == u || u >= ifdd->count ) ) {
        add_tag_short_values( ifdd );
    }
}

static void process_ascii_string( ifd_desc_t *ifdd )
{
    if ( TIFF_STRING == ifdd->type && ifdd->count > 0 ) {
        add_tag_byte_values( ifdd );
    }
}

// if n == 0 accept any count, otherwise accept only count == n
static void process_n_undefined_bytes( ifd_desc_t *ifdd, uint32_t n )
{
    if ( TIFF_UNDEFINED == ifdd->type && (n == 0 || n == ifdd->count ) ) {
        add_tag_byte_values( ifdd );
    }
}

#if 0
// if u == 0 accept only if l <= count otherwise accept only if l <= count <= u
static void process_min_max_undefined_bytes( ifd_desc_t *ifdd,
                                             uint32_t l, uint32_t u )
{
    if ( TIFF_UNDEFINED == ifdd->type && l <= ifdd->count &&
        ( 0 == u || u >= ifdd->count ) ) {
        add_tag_byte_values( ifdd );
    }
}
#endif
static void process_n_unsigned_shorts_longs( ifd_desc_t *ifdd, uint32_t n )
{
    switch ( ifdd->type ) {
    case TIFF_UINT16: case TIFF_UINT32:
        if ( 0 == n || n == ifdd->count ) {
            add_tag_int_values( ifdd );
        }
    }
}

static void process_n_unsigned_longs( ifd_desc_t *ifdd, uint32_t n )
{
    switch ( ifdd->type ) {
    case TIFF_UINT32:
        if ( 0 == n || n == ifdd->count ) {
            add_tag_int_values( ifdd );
        }
    }
}

static void process_n_urationals( ifd_desc_t *ifdd, uint32_t n )
{
    if ( TIFF_URATIONAL == ifdd->type && ( n == 0 || n == ifdd->count ) ) {
        add_tag_rational_values( ifdd );
    }
}

static void process_n_rationals( ifd_desc_t *ifdd, uint32_t n )
{
    if ( TIFF_RATIONAL == ifdd->type && ( n == 0 || n == ifdd->count ) ) {
        add_tag_rational_values( ifdd );
    }
}

static void process_jpeg_interchange_format( ifd_desc_t *ifdd )
{
    if ( TIFF_UINT32 == ifdd->type && 1 == ifdd->count ) {
        ifdd->desc->thumb_offset =
                    tiff_endianize_uint32( ifdd->desc, ifdd->valoff );
    }
}

static void process_jpeg_interchange_format_length( ifd_desc_t *ifdd )
{
    if ( TIFF_UINT32 == ifdd->type && 1 == ifdd->count ) {
        ifdd->desc->thumb_size =
                    tiff_endianize_uint32( ifdd->desc, ifdd->valoff );
    }
}

static void process_embedded_ifd( ifd_desc_t *ifdd, ifd_id_t id )
{
    if ( TIFF_UINT32 == ifdd->type && 1 == ifdd->count ) {
        move_file_position_to_offset( ifdd );
//printf("Switching to IFD if %d\n", id );
        ifdd->desc->ifds[id] = exif_parse_ifd( ifdd->desc, id, NULL );
//printf("Back from id %d\n", id );
        restore_file_position( ifdd );
    }
}

static void process_unknown_tag( ifd_desc_t *ifdd, ifd_id_t id )
{
    printf( "Unknown tag 0x%04x type %d count %d in IFD %d\n",
            ifdd->tag, ifdd->type, ifdd->count, id );
    exit(1);
}

static void parse_tiff_tags( ifd_desc_t *ifdd, ifd_id_t id )
{
    switch( ifdd->tag ) {
    case IMAGE_WIDTH_TAG: case IMAGE_LENGTH_TAG: case ROWS_PER_STRIP_TAG:
    case TILE_WIDTH_TAG: case TILE_LENGTH_TAG: case TILE_BYTE_COUNTS_TAG:
        process_n_unsigned_shorts_longs( ifdd, 1 );
        break;

    case STRIP_OFFSETS_TAG: case STRIP_BYTE_COUNTS_TAG:
        process_n_unsigned_shorts_longs( ifdd, 0 );
        break;

    case TILE_OFFSETS_TAG:
        process_n_unsigned_longs( ifdd, 1 );
        break;

    case BITS_PER_SAMPLE_TAG:
        add_tag_int_values( ifdd ); /* 1 short by color component */
        break;

    case COMPRESSION_TAG: case PHOTOMETRIC_INTERPRETATION_TAG:
    case ORIENTATION_TAG: case RESOLUTION_UNIT_TAG: case YCBCR_POSITIONING_TAG:
    case SAMPLES_PER_PIXEL_TAG: case PLANAR_CONFIGURATION_TAG:

    case RELATED_IMAGE_WIDTH_TAG:   // should not be in IFD 0, but sometimes is!
    case RELATED_IMAGE_HEIGHT_TAG:  // should not be in IFD 0, but sometimes is!
    case CUSTOM_RENDERED_TAG:   // should not be in IFD 0, but sometimes is!
    case EXPOSURE_MODE_TAG:     // should not be in IFD 0, but sometimes is!
    case WHITE_BALANCE_TAG:     // should not be in IFD 0, but sometimes is!
    case FOCAL_LENGTH_IN_35MM_FILM_TAG: // should not be in IFD 0, but...
    case SCENE_CAPTURE_TYPE_TAG:// should not be in IFD 0, but sometimes is!
    case GAIN_CONTROL_TAG:      // should not be in IFD 0, but sometimes is!
    case CONTRAST_TAG:          // should not be in IFD 0, but sometimes is!
    case SATURATION_TAG:        // should not be in IFD 0, but sometimes is!
    case SHARPNESS_TAG:         // should not be in IFD 0, but sometimes is!
    case SUBJECT_DISTANCE_RANGE_TAG:    // should not be in IFD 0, but...
        process_n_unsigned_shorts( ifdd, 1 );
        break;

    case YCBCR_SUBSAMPLING_TAG:
        process_n_unsigned_shorts( ifdd, 2 );
        break;

    case PROCESSING_SOFTWARE:
    case IMAGE_DESCRIPTION_TAG: case MAKE_TAG: case MODEL_TAG:
    case SOFTWARE_TAG:  case DATE_TIME_TAG: case ARTIST_TAG:
    case HOST_COMPUTER_TAG: case COPYRIGHT_TAG:
        process_ascii_string( ifdd );
        break;

    case X_RESOLUTION_TAG: case Y_RESOLUTION_TAG:
    case DIGITAL_ZOOM_RATIO_TAG: // should not be in IFD 0, but sometimes is!
        process_n_rationals( ifdd, 1 );
        break;

    case WHITE_POINT_TAG:
        process_n_rationals( ifdd, 2 );
        break;

    case YCBCR_COEFFICIENTS_TAG:
        process_n_rationals( ifdd, 3 );
        break;

    case PRIMARY_CHROMACITIES_TAG: case REFERENCE_BLACK_WHITE_TAG:
        process_n_rationals( ifdd, 6 );
        break;

    case JPEG_INTERCHANGE_FORMAT_TAG:
        process_jpeg_interchange_format( ifdd );
        break;

    case JPEG_INTERCHANGE_FORMAT_LENGTH_TAG:
        process_jpeg_interchange_format_length( ifdd );
        break;

    case EXIF_IFD_TAG:
        process_embedded_ifd( ifdd, EXIF );
        break;

    case  GPS_IFD_TAG:
        process_embedded_ifd( ifdd, GPS );
        break;

    case PANASONIC_TITLE: case PANASONIC_TITLE2: case XP_COMMENT:
    case UNKNOWN_TAG1: case UNKNOWN_TAG2: case UNKNOWN_TAG3:
    case UNKNOWN_TAG4: case UNKNOWN_TAG5: case UNKNOWN_TAG6:
    case PADDING_TAG:  // just ignore
        break;

    case PRINT_IM_TAG:
        process_n_undefined_bytes( ifdd, 0 );   // any number
        break;

    default:
        process_unknown_tag( ifdd, id );
    }
}

static void parse_primary_tags( ifd_desc_t *ifdd )
{
    parse_tiff_tags( ifdd, PRIMARY );
}

static void parse_thumbnail_tags( ifd_desc_t *ifdd )
{
    parse_tiff_tags( ifdd, THUMBNAIL );
}

static void process_version_string( ifd_desc_t *ifdd )
{
    if ( TIFF_UNDEFINED == ifdd->type && 4 == ifdd->count ) {
        // 4 ascii chars fitting in valoff (non-zero terminated ascii string)
        char buffer[5];
        memcpy( buffer, &ifdd->valoff, 4 );
        buffer[4] = 0;

        vector_t *array = new_vector_from_data( buffer, BYTE_SIZE, 5 );
        ifdd_map_insert_array( ifdd, array );
    }
}

static void process_components_configuration( ifd_desc_t *ifdd )
{
    if ( TIFF_UNDEFINED == ifdd->type && 4 >= ifdd->count ) {
        char buffer[8];            // assuming no more than 4 components and
        memset( buffer, 0, 8 );    // up to 2 char by component, e.g. "Cb"
        // 4 bytes fit directly in valoff
        char *config = (char *)(&ifdd->valoff);
        int j = 0;
        for ( size_t i = 0; i < ifdd->count; ++i ) {
            switch( config[i] ) {
            default: break;
            case 1: buffer[j++] = 'Y'; break;
            case 2: buffer[j++] = 'C'; buffer[j++] = 'b'; break;
            case 3: buffer[j++] = 'C'; buffer[j++] = 'r'; break;
            case 4: buffer[j++] = 'R'; break;
            case 5: buffer[j++] = 'G'; break;
            case 6: buffer[j++] = 'B'; break;
            }
        }
        buffer[j++] = '\0';
        vector_t *array = new_vector_from_data( buffer, BYTE_SIZE, j );
        ifdd_map_insert_array( ifdd, array );
    }
}

static void process_user_comment( ifd_desc_t *ifdd )
{
    if ( TIFF_UNDEFINED == ifdd->type && 8 <= ifdd->count ) {
        // add a terminating 0
        vector_t *array = new_vector( BYTE_SIZE, ifdd->count + 1 );
        if ( NULL != array ) {
            move_file_position_to_offset( ifdd );
            uint32_t i = 0;
            uint8_t byte;
            for ( ; i < ifdd->count; ++i ) {
                byte = getc( ifdd->desc->file );
                vector_write_item_at( array, i, &byte );
            }
            byte = 0;
            vector_write_item_at( array, i, &byte );
            ifdd_map_insert_array( ifdd, array );
            restore_file_position( ifdd );
        }
    }
}

// the exif CFA pattern structure is transformed to a simple ascii string:
// original structure: (uint16_t)n_h_repeat, (uin16_t)n_v_repeat, followed
// by a n_h_repeat * n_v_repeat array of bytes is transformed into a zero
// terminated string: "c .... c, ... , c .... c", where the comma ',' appears
// after each group of n_h_repeat.
static void process_cfa_pattern( ifd_desc_t *ifdd )
{
    if ( TIFF_UNDEFINED == ifdd->type && 4 < ifdd->count ) {
        move_file_position_to_offset( ifdd );
        uint32_t hz = (uint32_t)tiff_get_uint16( ifdd->desc );
        uint32_t vt = (uint32_t)tiff_get_uint16( ifdd->desc );

        // as it is a byte array and endianess is not clearly specified,
        // different tools generate different data: it seems that older
        // microsoft tools do not use the proper endianess, so check here if
        // the values are consistent with the total count:

        if ( hz * vt != ifdd->count - 4 ) {
            // need to change the endianess
            uint32_t h = ((hz & 0xff) << 8) + (hz >> 8);
            uint32_t v = ((vt & 0xff) << 8) + (vt >> 8 );
            if ( h * v != ifdd->count - 4 ) {
                restore_file_position( ifdd );
                printf("Invalid repeat patterns: hz=%d, vt=%d\n", hz, vt );
                return;     // invalif repeat patterns
            }
            hz = h;
            vt = v;
        }

        uint32_t n = ( hz + 1 ) * vt;
        vector_t *array = new_vector( BYTE_SIZE, n );
        if ( NULL != array ) {
            uint32_t k = 0;
            for ( uint32_t i = 0; i < vt; ++i ) {
                char c;
                for ( uint32_t j = 0; j < hz; ++j ) {
                    uint8_t byte = getc( ifdd->desc->file );
                    switch ( byte ) {
                    case 0: c = 'R'; break;
                    case 1: c = 'G'; break;
                    case 2: c = 'B'; break;
                    case 3: c = 'C'; break;
                    case 4: c = 'M'; break;
                    case 5: c = 'Y'; break;
                    case 6: c = 'W'; break;
                    default: c = '-'; break;
                    }
                    vector_write_item_at( array, k++, &c );
                }
                if ( k == n-1 ) {
                    c = 0;
                } else {
                    c = ',';
                }
                vector_write_item_at( array, k++, &c );
            }
            ifdd_map_insert_array( ifdd, array );
        }
        restore_file_position( ifdd );
    }
}

static void parse_exif_tags( ifd_desc_t *ifdd )
{
    switch ( ifdd->tag ) {
    case EXPOSURE_TIME_TAG: case FNUMBER_TAG:
    case COMPRESSED_BITS_PER_PIXEL_TAG:
    case APERTURE_VALUE_TAG:
    case MAX_APERTURE_VALUE_TAG:
    case SUBJECT_DISTANCE_TAG:
    case FOCAL_LENGTH_TAG:
    case EXPOSURE_INDEX_TAG:
    case DIGITAL_ZOOM_RATIO_TAG:
    case FOCAL_PLANE_X_RESOLUTION_TAG:
    case FOCAL_PLANE_Y_RESOLUTION_TAG:
        process_n_urationals( ifdd, 1 );
        break;

    case SHUTTER_SPEED_VALUE_TAG:   // signed rational
    case BRIGHTNESS_VALUE_TAG:      // signed rational
    case EXPOSURE_BIAS_VALUE_TAG:   // signed rational
        process_n_rationals( ifdd, 1 );
        break;

    case EXPOSURE_PROGRAM_TAG: case ISO_SPEED_RATINGS_TAG:
    case METERING_MODE_TAG: case LIGHT_SOURCE_TAG: case FLASH_TAG:
    case COLOR_SPACE_TAG: case SENSING_METHOD_TAG:
    case CUSTOM_RENDERED_TAG: case EXPOSURE_MODE_TAG:
    case WHITE_BALANCE_TAG: case FOCAL_LENGTH_IN_35MM_FILM_TAG:
    case SCENE_CAPTURE_TYPE_TAG: case GAIN_CONTROL_TAG:
    case CONTRAST_TAG: case SATURATION_TAG: case SHARPNESS_TAG:
    case SUBJECT_DISTANCE_RANGE_TAG: case FOCAL_PLANE_RESOLUTION_UNIT_TAG:
    case COMPOSITE_IMAGE_TAG: case SENSITIVITY_TYPE_TAG:
        process_n_unsigned_shorts( ifdd, 1 );
        break;

    case SPATIAL_FREQUENCY_RESPONSE_TAG:
        process_n_unsigned_bytes( ifdd, 0 );
        break;

    case STANDARD_OUTPUT_SENSITIVITY_TAG: case RECOMMENDED_EXPOSURE_INDEX_TAG:
        process_n_unsigned_longs( ifdd, 1 );
        break;

    case EXIF_VERSION_TAG: case FLASHPIX_VERSION_TAG:
        process_version_string( ifdd );
        break;

    case DATE_TIME_ORIGINAL_TAG: case DATE_TIME_DIGITIZED_TAG:
    case OFFSET_TIME_TAG: case OFFSET_TIME_ORIGINAL_TAG:
    case OFFSET_TIME_DIGITIZED_TAG:
    case SUBSEC_TIME_TAG: case SUBSEC_TIME_ORIGINAL_TAG:
    case SUBSEC_TIME_DIGITIZED_TAG: case IMAGE_UNIQUE_ID_TAG:
    case LENS_MAKE_TAG: case LENS_MODEL_TAG: case LENS_SERIAL_NUMBER_TAG:
    case RELATED_SOUND_FILE_TAG:
    case OWNER_NAME_TAG: case BODY_SERIAL_NUMBER_TAG:
        process_ascii_string( ifdd );
        break;

    case COMPONENTS_CONFIGURATION_TAG:
        process_components_configuration( ifdd );
        break;

     case SUBJECT_AREA_TAG:
        process_min_max_unsigned_shorts( ifdd, 2, 4 );
        break;

    case USER_COMMENT_TAG:
        process_user_comment( ifdd );
        break;

    case PIXEL_X_DIMENSION_TAG: case PIXEL_Y_DIMENSION_TAG:
        process_n_unsigned_shorts_longs( ifdd, 1 );

    case SUBJECT_LOCATION_TAG: case COMPOSITE_IMAGE_COUNT_TAG:
        process_n_unsigned_shorts( ifdd, 2 );
        break;

    case FILE_SOURCE_TAG: case SCENE_TYPE_TAG:
        process_n_undefined_bytes( ifdd, 1 );
        break;

    case CFA_PATTERN_TAG:
        process_cfa_pattern( ifdd );
        break;

    case LENS_SPECIFICATION_TAG:
        process_n_urationals( ifdd, 4 );
        break;

    case COMPOSITE_IMAGE_EXPOSURE_TIME_TAG:
        process_n_undefined_bytes( ifdd, 0 );
        break;

    case INTEROPERABILITY_IFD_TAG:
        process_embedded_ifd( ifdd, IOP );
        break;

    case MAKER_NOTE_TAG: case OFFSET_SCHEMA_TAG:
    case UNKNOWN_TAG5: case UNKNOWN_TAG6:

    case PADDING_TAG:
        break;              // just ignore

    default:
        process_unknown_tag( ifdd, EXIF );
        break;
    }
}

static void parse_gps_tags( ifd_desc_t *ifdd )
{
    switch ( ifdd->tag ) {
    case GPS_VERSION_ID_TAG:
        process_version_string( ifdd );
        break;

    case GPS_LATITUDE_REF_TAG:              // "N\0" or "S\0"
        process_ascii_string( ifdd );
        break;
    case GPS_LATITUDE_TAG:
        process_n_rationals( ifdd, 3 );
        break;
    case GPS_LONGITUDE_REF_TAG:             // "E\0" or "W\0"
        process_ascii_string( ifdd );
        break;
    case GPS_LONGITUDE_TAG:
        process_n_rationals( ifdd, 3 );
        break;
    case GPS_ALTITUDE_REF_TAG:              // 0 = above sea level, 1 = below sea level
        process_n_unsigned_bytes( ifdd, 1 );
        break;
    case GPS_ALTITUDE_TAG:                  // altitude in meters
        process_n_rationals( ifdd, 1 );
        break;
    case GPS_TIME_STAMP_TAG:                // UTC time, hh, mm, ss (atomic time)
        process_n_rationals( ifdd, 3 );
        break;
    case GPS_SATELLITES_TAG:                // whatever the device can indicate
        process_ascii_string( ifdd );
        break;
    case GPS_STATUS_TAG:                    // "A\0" (measurement) or "V\0" (done)
        process_ascii_string( ifdd );
        break;
    case GPS_MEASURE_MODE_TAG:              // "2\0" or "3\0""
        process_ascii_string( ifdd );
        break;
    case GPS_DOP_TAG:
        process_n_rationals( ifdd, 1 );
        break;
    case GPS_SPEED_REF_TAG:                 // "K\0" (km/h), "M\0" (Miles/h) or "N\0" (Knots/h)
        process_ascii_string( ifdd );
        break;
    case GPS_SPEED_TAG:
        process_n_rationals( ifdd, 1 );
        break;
    case GPS_TRACK_REF_TAG:                 // "T\0" (true North) or "M\0" (magnetic)
        process_ascii_string( ifdd );
        break;
    case GPS_TRACK_TAG:
        process_n_rationals( ifdd, 1 );
        break;
    case GPS_IMG_DIRECTION_REF_TAG:         // "T\0" (true North) or "M\0" (magnetic)
        process_ascii_string( ifdd );
        break;
    case GPS_IMG_DIRECTION_TAG:
        process_n_rationals( ifdd, 1 );
        break;

// TODO
    case GPS_MAP_DATUM_TAG:
    case GPS_DEST_LATITUDE_REF_TAG:
    case GPS_DEST_LATITUDE_TAG:
    case GPS_DEST_LONGITUDE_REF_TAG:
    case GPS_DEST_LONGITUDE_TAG:
    case GPS_DEST_BEARING_REF_TAG:
    case GPS_DEST_BEARING_TAG:
    case GPS_DEST_DISTANCE_REF_TAG:
    case GPS_DEST_DISTANCE_TAG:
    case GPS_PROCESSING_METHOD_TAG:
    case GPS_AREA_INFORMATION_TAG:
    case GPS_DATE_STAMP_TAG:
    case GPS_DIFFERENTIAL_TAG:
    case GPS_H_POSITIONING_ERROR_TAG:
        break;

    case PADDING_TAG:
        break;              // just ignore

    default:
        process_unknown_tag( ifdd, GPS );
        break;
    }
}

static void parse_iop_tags( ifd_desc_t *ifdd )
{
    switch ( ifdd->tag ) {
    case INTEROPERABILITY_INDEX_TAG: case RELATED_IMAGE_FILE_FORMAT_TAG:
        process_ascii_string( ifdd );
        break;

    case INTEROPERABILITY_VERSION_TAG:
        process_version_string( ifdd );
        break;

    case RELATED_IMAGE_WIDTH_TAG: case RELATED_IMAGE_HEIGHT_TAG:
        process_n_unsigned_shorts( ifdd, 1 );
        break;

    default:
        process_unknown_tag( ifdd, IOP );
        break;
    }
}

extern map_t *exif_parse_ifd( exif_desc_t *desc, ifd_id_t id, uint32_t *next )
{
    parse_tag_fct *parse_tag;

    switch( id ) {
    case PRIMARY:
        parse_tag = parse_primary_tags;
        break;
    case THUMBNAIL:
        parse_tag = parse_thumbnail_tags;
        break;
    case EXIF:
        parse_tag = parse_exif_tags;
        break;
    case GPS:
        parse_tag = parse_gps_tags;
        break;
    case IOP:
        parse_tag = parse_iop_tags;
        break;
//  case MAKER:  case EMBEDDED:
    default:
        printf( "Request for ifd %d is not implemented\n", id );
        return NULL;
    }

    ifd_desc_t ifdd;
    ifdd.id = id;
    ifdd.desc = desc;
    ifdd.content = new_map( NULL, NULL, 0, 32 );

    uint16_t n_entries = tiff_get_uint16( desc );
//    printf( "ifd id %d: number of entries=%d\n", id, n_entries );
    for ( uint16_t i = 0; i < n_entries; ++i ) {
        ifdd.tag = tiff_get_uint16( desc );     // field tag
        ifdd.type = tiff_get_uint16( desc );    // field type
        ifdd.count = tiff_get_uint32( desc );   // field count
        ifdd.valoff = tiff_get_raw_uint32( desc ); // field valoff (no endianess)
//        printf("Parsing tag=0x%04x, type=0x%04x, count=%d, valoff=0x%08x\n",
//                ifdd.tag, ifdd.type, ifdd.count, ifdd.valoff);
        if ( ! check_entry_type( &ifdd ) ) {
            printf( "Illegal tiff type: 0x%04x\n", ifdd.type );
            map_free( ifdd.content );
            return NULL;
        }
        parse_tag( &ifdd );
    }

    uint32_t next_offset = tiff_get_uint32( desc);
//    printf( "ifd id %d: next offset=0x%08x\n", id, next_offset );
    if ( NULL != next ) {
        *next = next_offset;
    }
    return ifdd.content;
}

