
#include <stdio.h>
#include "print.h"

typedef void (*formated_print_fct)( uint32_t num, uint32_t den );

// Note: this can be used for signed values with an appropriate format _print
static void print_unsigned_values( vector_t *v, formated_print_fct format_print )
{
    size_t size = vector_item_size( v );
    uint32_t n_items = vector_cap( v );

    for ( uint32_t i = 0; i < n_items ; ++i ) {
        switch ( size ) {
        case sizeof( uint8_t ):
            {
                uint8_t *val = vector_item_at( v, 0 );
                if ( format_print ) {
                    format_print( (uint32_t)*val, 0 );
                } else {
                    printf( "%u", (uint32_t)*val );
                }
                break;
            }
        case sizeof( uint16_t ):
            {
                uint16_t *val = vector_item_at( v, 0 );
                if ( format_print ) {
                    format_print( (uint32_t)*val, 0 );
                } else {
                    printf( "%u", (uint32_t)*val );
                }
                break;
            }
        case sizeof( uint32_t ):
            {
                uint32_t *val = vector_item_at( v, 0 );
                if ( format_print ) {
                    format_print( (uint32_t)*val, 0 );
                } else {
                    printf( "%u", (uint32_t)*val );
                }
                break;
            }
        case sizeof( rational_t ):
            {
                urational_t *val = vector_item_at( v, 0 );
                if ( format_print ) {
                    format_print( val->numerator, val->denominator );
                } else if ( 0 == val->denominator ) {
                    printf( "%u/%u", val->numerator, val->denominator );
                } else {
                    printf( "%f (%u/%u)",
                        (float)(val->numerator)/(float)(val->denominator),
                        (uint32_t)val->numerator, (uint32_t)val->denominator );
                }
                break;
            }
        }
        if ( i != n_items-1 ) {
            printf( ", ");
        }
    }
}

static void print_string_tag( exif_desc_t *desc, ifd_id_t id, uint16_t tag,
                              char *indent, char *name,
                              formated_print_fct format )
{
    vector_t *v;
    bool success = exif_get_ifd_tag_values( desc, id, tag, &v );

    if ( success ) {
        printf( "%s%s: \"%s\"\n", indent, name, vector_read_string( v ) );
    }
}

static void print_uint_tag_array( exif_desc_t *desc, ifd_id_t id, uint16_t tag,
                                  char *indent, char *name,
                                  formated_print_fct format )
{
    vector_t *v;
    bool success = exif_get_ifd_tag_values( desc, id, tag, &v );

    if ( success ) {
        printf( "%s%s: ", indent, name );
        print_unsigned_values( v, format );
        printf( "\n" );
    }
}

static void print_user_comment( exif_desc_t *desc, ifd_id_t id, uint16_t tag,
                                char *indent, char *name,
                                formated_print_fct format )
{
    vector_t *v;
    bool success = exif_get_ifd_tag_values( desc, id, tag, &v );

    if ( success ) {
        char *encoding = (char *)vector_item_at( v, 0 );
        char *text = (char *)vector_item_at( v, 8 );
        if ( 0 == *encoding ) {
            encoding = "Undefined";
            text = "";
        }
        printf( "%s%s: (Encoding %s)\n%s  \"%s\"\n",
                indent, name, encoding, indent, text );
    }
}

static void format_hex_bytes( uint32_t num, uint32_t den )
{
    printf("0x%02x ", (uint8_t)num );
}

static void format_srational( uint32_t num, uint32_t den )
{
    printf( "%f", (float)(int32_t)num / (float)(int32_t)den );
}

static void format_compression( uint32_t num, uint32_t den ) {

    switch( num ) {
    default:                    printf("unknown (%d)", num); break;
    case NOT_COMPRESSED:        printf("none"); break;
    case CCITT_1D:              printf("CCITT 1D"); break;
    case CCITT_Group3:          printf("CCITT Group3"); break;
    case CCITT_Group4:          printf("CCITT Group4"); break;
    case LZW:                   printf("LZW"); break;
    case JPEG:                  printf("JPEG"); break;
    case JPEG_Technote2:        printf("JPEG Techmote2"); break;
    case Deflate:               printf("Deflate"); break;
    case RFC_2301_BW_JBIG:      printf("RFC 2301 BW JBIG"); break;
    case RFC_2301_Color_JBIG:   printf("RFC 2301 Color JBIG"); break;
    case PACKBITS:              printf("Packbits (Apple)"); break;
    }
}

static void format_photographic_interpretation( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf( "Unknown (%d)\n", num ); break;
    case BILEVEL_OR_GRAYSCALE_0_WHITE:
        printf( "Bilevel or Grayscale, 0 is white\n" ); break;
    case BILEVEL_OR_GRAYSCALE_0_BLACK:
        printf( "Bilevel or Grayscale, 0 is black\n" ); break;
    case RGB:
        printf( "RGB components\n" ); break;
    case PALETTE:
        printf( "Palette indexes\n"); break;
    }
}

static void format_orientation( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf( "unknown (%d)\n", num ); break;
    case ROW_0_TOP_COL_0_LEFT:
        printf( "Row 0 on top, Col 0 on left" ); break;
    case ROW_0_TOP_COL_0_RIGHT:
        printf( "Row 0 on top, Col 0 on right" ); break;
    case ROW_0_BOTTOM_COL0_RIGHT:
        printf( "Row 0 on bottom, Col 0 on right" ); break;
    case ROW_0_BOTTOM_COL0_LEFT:
        printf( "Row 0 on bottom, Col 0 on left" ); break;
    case ROW_0_LEFT_COL_0_TOP:
        printf( "Row 0 on left, Col 0 on Top" ); break;
    case ROW_0_RIGHT_COL_0_TOP:
        printf( "Row 0 on right, Col 0 on Top" ); break;
    case ROW_0_RIGHT_COL_0_BOTTOM:
        printf( "Row 0 on right, Col 0 on bottom" ); break;
    case ROW_0_LEFT_COL_0_BOTTOM:
        printf( "Row 0 on left, Col 0 on bottom" ); break;
    }
}

static void format_resolution_uint( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf("Unknown (%d)", num); break;
    case DOTS_PER_ARBITRATY_UNIT:
        printf("Dots per arbitraty unit"); break;
    case DOTS_PER_INCH:
        printf("Dots per inch"); break;
    case DOTS_PER_CM:
        printf("Dots per cm"); break;
    }
}

static void format_ycbcr_positioning( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf("Unknown (%d)", num); break;
    case YCBCR_CENTERED:    printf("Centered"); break;
    case YCBCR_COSITED:     printf("Cosited"); break;
    }
}

static void format_exposure_time( uint32_t num, uint32_t den )
{
    printf( "%f seconds (%u/%u)", (float)(num)/(float)(den), num, den );
}

static void format_exposure_program( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Unknown (%u)", num ); break;
    case UNDEFINED:         printf( "Undefined" ); break;
    case MANUAL:            printf( "Manual" ); break;
    case NORMAL_PROGRAM:    printf( "Normal program" ); break;
    case APERTURE_PRIORITY: printf( "Aperture priority" ); break;
    case SHUTTER_PRIORITY:  printf( "Shutter priority" ); break;
    case CREATIVE_PROGRAM:  printf( "Creative program" ); break;
    case ACTION_PROGRAM:    printf( "Action program" ); break;
    case PORTRAIT_MODE:     printf( "Portrait mode" ); break;
    case LANDSCAPE_MODE:    printf( "Landscape mode" ); break;
    }
}

static void format_sensitivity_type( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                    printf( "Non-listed (%u)", num ); break;
    case UNKNOWN_SENSITIVITY:   printf( "UNknown" ); break;

    case STANDARD_OUTPUT_SENSITIVITY:
        printf( "Standard output sensitivity" );
        break;
    case RECOMMENDED_EXPOSURE_INDEX:
        printf( "Recommended exposure index" );
        break;
    case ISO_SPEED:             printf( "ISO speed" ); break;

    case STANDARD_OUTPUT_SENSITIVITY_RECOMMENDED_EXPOSURE_INDEX:
        printf( "Standard output sensitivity, Recommended exposure index" );
        break;
    case STANDARD_OUTPUT_SENSITIVITY_ISO_SPEED:
        printf( "Standard output sensitivity, ISO speed" );
        break;
    case RECOMMENDED_EXPOSURE_INDEX_ISO_SPEED:
        printf( "Recommended exposure index, ISO speed" );
        break;
    case STANDARD_OUTPUT_SENSITIVITY_RECOMMENDED_EXPOSURE_INDEX_ISO_SPEED:
        printf( "Standard output sensitivity, Recommended exposure index, ISO speed" );
        break;
    }
}

static void format_metering_mode( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf( "Not listed (%u)", num ); break;
    case UNKNOWN_MODE:
        printf( "Unknown mode" ); break;
    case AVERAGE:
        printf( "Average" ); break;
    case CENTER_WEIGHTED_AVERAGE_PROGRAM:
        printf( "Center Weighted Average" ); break;
    case SPOT:
        printf( "Spot" ); break;
    case MULTISPOT:
        printf( "Multispot" ); break;
    case PATTERN:
        printf( "Pattern" ); break;
    case PARTIAL:
        printf( "Partial" ); break;
    case OTHER:
        printf( "Other" ); break;
    }
}

static void format_ligth_source( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf( "Not listed (%u)", num ); break;
    case UNKNOWN_SOURCE:
        printf( "Unknown source" ); break;
    case DAYLIGHT:
        printf( "Daylight" ); break;
    case FLUORESCENT:
        printf( "Fluorescent" ); break;
    case TUNGSTEN_INCANDESCENT_LIGHT:
        printf( "Tungsten Incandescent" ); break;
    case FLASH:
        printf( "Flash" ); break;
    case FINE_WEATHER:
        printf( "Fine weather" ); break;
    case CLOUDY_WEATHER:
        printf( "Cloudy weather" ); break;
    case SHADE:
        printf( "Shade" ); break;
    case DAYLIGHT_FLUORESCENT_D_5700_7100K:
        printf( "Daylight fluorescent D 5700-7400K" ); break;
    case DAY_WHITE_FLUORESCENT_N_4600_5400K:
        printf( "Day white fluorescent D 4600-5400K" ); break;
    case COOL_WHITE_FLUORESCENT_W_3900_4500K:
        printf( "Cool white fluorescent D 3900-4500K" ); break;
    case WHITE_FLUORESCENT_WW_3200_3700K:
        printf( "White fluorescent D 3200-3700K" ); break;
    case STANDARD_LIGHT_A:
        printf( "Standard light A" ); break;
    case STANDARD_LIGHT_B:
        printf( "Standard light B" ); break;
    case STANDARD_LIGHT_C:
        printf( "Standard light C" ); break;
    case D55:
        printf( "D55" ); break;
    case D65:
        printf( "D65" ); break;
    case D75:
        printf( "D75" ); break;
    case D50:
        printf( "D50" ); break;
    case ISO_STUDIO_TUNGSTEN:
        printf( "ISO studio tungsten" ); break;
    case OTHER_LIGHT_SOURCE:
        printf( "Other light source" ); break;
    }
}

static void format_flash( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf( "Not listed (%u)", num ); break;
    case NO_FLASH_FUNCTION:
        printf("No flash function"); break;
    case FLASH_DID_NOT_FIRE:
        printf("Flash did not fire"); break;
    case FLASH_DID_NOT_FIRE_COMPULSORY_FLASH_MODE:
        printf("Flash did not fire, compulsory mode"); break;
    case FLASH_DID_NOT_FIRE_AUTO_MODE:
        printf("Flash did not fire, auto mode"); break;
    case FLASH_FIRED:
        printf("Flash fired"); break;
    case FLASH_FIRED_STROBE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, return light not detected"); break;
    case FLASH_FIRED_STROBE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, return light detected"); break;
    case FLASH_FIRED_COMPULSORY_FLASH_MODE:
        printf("Flash fired, compulsory mode"); break;
    case FLASH_FIRED_COMPULSORY_FLASH_MODE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, compulsory mode, return light not detected"); break;
    case FLASH_FIRED_COMPULSORY_FLASH_MODE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, compulsory mode, return light detected"); break;
    case FLASH_FIRED_RED_EYE_REDUCTION_MODE:
        printf("Flash fired, red eye reduction"); break;
    case FLASH_FIRED_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, red eye reduction, return light not detected"); break;
    case FLASH_FIRED_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, red eye reduction, return light detected"); break;
    case FLASH_FIRED_COMPULSORY_FLASH_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, compulsory mode, red eye reduction, return light not detected"); break;
    case FLASH_FIRED_COMPULSORY_FLASH_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, compulsory mode, red eye reduction, return light detected"); break;
    case FLASH_FIRED_AUTO_MODE:
        printf("Flash fired, auto mode"); break;
    case FLASH_FIRED_AUTO_MODE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, auto mode, return light not detected"); break;
    case FLASH_FIRED_AUTO_MODE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, auto mode, return light detected"); break;
    case FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE:
        printf("Flash fired, auto mode, red eye reduction"); break;
    case FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED:
        printf("Flash fired, auto mode, red eye reduction, return light not detected"); break;
    case FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED:
        printf("Flash fired, auto mode, red eye reduction, return light detected"); break;
    }
}

static void format_color_space( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:            printf("Not listed (%u)", num); break;
    case SRGB:          printf("SRGB"); break;
    case UNCALIBRATED:  printf("Uncalibrated"); break;
    }
}

static void format_planar_configuration( uint32_t num, uint32_t den )
{
    switch ( num ) {
    default:            printf("Not listed (%u)", num); break;
    case CHUNKY_FORMAT: printf("Chunky"); break;
    case PLANAR_FORMAT: printf("Planar"); break;
    }
}

static void format_resolution_unit( uint32_t num, uint32_t den )
{
    switch ( num ) {
    default:                    printf("Not listed (%u)", num); break;
    case RESOLUTION_UNIT_NONE:  printf("No Unit"); break;
    case RESOLUTION_UNIT_INCHES:printf("Inches"); break;
    case RESOLUTION_UNIT_CM:    printf("Centimeter"); break;
    case RESOLUTION_UNIT_MM:    printf("Millimeter"); break;
    case RESOLUTION_UNIT_UM:    printf("Micrometer"); break;
    }
}

static void format_sensing_method( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:
        printf("Not listed (%u)", num); break;
    case UNDEFINED_SENSOR:
        printf("Not defined"); break;
    case ONE_CHIP_COLOR_AREA_SENSOR:
        printf("One-chip color area"); break;
    case TWO_CHIP_COLOR_AREA_SENSOR:
        printf("Two-chip color area"); break;
    case THREE_CHIP_COLOR_AREA_SENSOR:
        printf("Three-chip color area"); break;
    case COLOR_SEQUENTIAL_AREA_SENSOR:
        printf("Color sequential area"); break;
    case TRILINEAR_SENSOR:
        printf("Trilinear"); break;
    case COLOR_SEQUENTIAL_LINEAR_SENSOR:
        printf("Color sequential linear"); break;
    }
}

static void format_file_source( uint32_t num, uint32_t den )
{
    if ( num == 3 ) {
        printf( "DSC");
    } else {
        printf( "Unknown (%d)", num );
    }
}

static void format_scene_type( uint32_t num, uint32_t den )
{
    if ( num == 1 ) {
        printf( "DSC");
    } else {
        printf( "Unknown (%d)", num );
    }
}

static void format_custom_rendered( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf("Not listed (%u)", num); break;
    case NORMAL_PROCESS:    printf("Normal process"); break;
    case CUSTOM_PROCESS:    printf("Custom process"); break;
    }
}

static void format_exposure_mode( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf("Not listed (%u)", num); break;
    case AUTO_EXPOSURE:     printf("Auto exposure"); break;
    case MANUAL_EXPOSURE:   printf("Manual exposure"); break;
    case AUTO_BRACKET:      printf("Auto bracket"); break;
    }
}

static void format_white_balance( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                    printf("Not listed (%u)", num); break;
    case AUTO_WHITE_BALANCE:    printf("Auto balance"); break;
    case MANUAL_WHITE_BALANCE:  printf("Manual balance"); break;
    }
}

static void format_digital_zoom_ratio( uint32_t num, uint32_t den )
{
    if ( num != 0 && den != 0 ) {
        printf( "%f", (float)num / (float)den );
    } else {
        printf( "No digital zoom" );
    }
}

static void format_35mm_focal_length( uint32_t num, uint32_t den )
{
    if ( num != 0 ) {
        printf("%u", num);
    } else {
        printf("Unknown length");
    }
}

static void format_scene_capture_type( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case STANDARD_SCENE:    printf( "Standard" ); break;
    case LANDSCAPE:         printf( "Landscape" ); break;
    case PORTRAIT:          printf( "Portrait" ); break;
    case NIGHT_SCENE:       printf( "Night Scene" ); break;
    }
}

static void format_gain_control( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case NO_GAIN:           printf( "No gain" ); break;
    case LOW_GAIN_UP:       printf( "Low gain up" ); break;
    case HIGH_GAIN_UP:      printf( "High gain up" ); break;
    case LOW_GAIN_DOWN:     printf( "Low gain down" ); break;
    case HIGH_GAIN_DOWN:    printf( "High gain down" ); break;
    }
}

static void format_contrast( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case NORMAL_CONTRAST:   printf( "Normal contrast" ); break;
    case SOFT_CONTRAST:     printf( "Soft contrast" ); break;
    case HARD_CONTRAST:     printf( "Hard contrast" ); break;
    }
}

static void format_saturation( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case NORMAL_SATURATION: printf( "Normal saturation" ); break;
    case LOW_SATURATION:    printf( "Low saturation" ); break;
    case HIGH_SATURATION:   printf( "High saturation" ); break;
    }
}

static void format_sharpness( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case NORMAL_SHARPNESS:  printf( "Normal sharpness" ); break;
    case SOFT_SHARPNESS:    printf( "Low sharpness" ); break;
    case HARD_SHARNESS:     printf( "High saturation" ); break;
    }
}

static void format_subject_distance_range( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                printf( "Not listed (%u)", num ); break;
    case UNKNOWN_DISTANCE:  printf( "Unknown" ); break;
    case MACRO:             printf( "Macro" ); break;
    case CLOSE_VIEW:        printf( "Close View" ); break;
    case DISTANT_VIEW:      printf( "Distant view" ); break;
    }
}

static void format_composite_image( uint32_t num, uint32_t den )
{
    switch( num ) {
    default:                        printf( "Not listed (%u)", num ); break;
    case UNKNOWN_COMPOSITION:       printf( "Unknown" ); break;
    case NON_COMPOSITE_IMAGE:       printf( "Image is not composite" ); break;
    case GENERAL_COMPOSITE_IMAGE:   printf( "General composite image" ); break;
    case COMPOSITE_IMAGE_CAPTURED_WHEN_SHOOTING:
                                    printf( "Images captured when shooting" ); break;
    }
}

typedef void (*print_fct)( exif_desc_t *desc, ifd_id_t id, uint16_t tag,
                           char *indent, char *header,
                           formated_print_fct format );

typedef struct {
    uint16_t            tag;
    char *              header;
    print_fct           print;
    formated_print_fct  format;
} ifd_tag_print_t;

static ifd_tag_print_t ifd_01_tag_print[] = {

    { PROCESSING_SOFTWARE, "Processing Software", print_string_tag, NULL },
    { IMAGE_WIDTH_TAG, "Image Width", print_uint_tag_array, NULL },
    { IMAGE_LENGTH_TAG, "Image Length", print_uint_tag_array, NULL },
    { BITS_PER_SAMPLE_TAG, "Bits per Sample", print_uint_tag_array, NULL },
    { COMPRESSION_TAG, "Compression", print_uint_tag_array, format_compression },
    { PHOTOMETRIC_INTERPRETATION_TAG, "Photographic Interpretation",
        print_uint_tag_array, format_photographic_interpretation },

    { IMAGE_DESCRIPTION_TAG, "Image Description", print_string_tag, NULL },
    { MAKE_TAG, "Manufacturer", print_string_tag, NULL },
    { MODEL_TAG, "Model", print_string_tag, NULL },

    { STRIP_OFFSETS_TAG, "Strip offsets", print_uint_tag_array, NULL },

    { ORIENTATION_TAG, "Image Orientation",
        print_uint_tag_array, format_orientation },

    { SAMPLES_PER_PIXEL_TAG, "Samples per Pixel", print_uint_tag_array, NULL },
    { ROWS_PER_STRIP_TAG, "Rows per strip", print_uint_tag_array, NULL },
    { STRIP_BYTE_COUNTS_TAG, "Strip byte counts", print_uint_tag_array, NULL },


    // should not be in IFD0, but it happens...
    { CUSTOM_RENDERED_TAG, "Data Processing",
        print_uint_tag_array, format_custom_rendered },
    { EXPOSURE_MODE_TAG, "Exposure Mode",
        print_uint_tag_array, format_exposure_mode },
    { WHITE_BALANCE_TAG, "White Balance",
        print_uint_tag_array, format_white_balance },
    { FOCAL_LENGTH_IN_35MM_FILM_TAG, "Focal length in 35mm film",
        print_uint_tag_array, format_35mm_focal_length },

    { GAIN_CONTROL_TAG, "Gain Control",
        print_uint_tag_array, format_gain_control },
    { CONTRAST_TAG, "Contrast", print_uint_tag_array, format_contrast },
    { SATURATION_TAG, "Saturation", print_uint_tag_array, format_saturation },
    { SHARPNESS_TAG, "Sharpness", print_uint_tag_array, format_sharpness },
    { SUBJECT_DISTANCE_RANGE_TAG, "Subject Distance Range",
        print_uint_tag_array, format_subject_distance_range },
    // end of special tags

    { X_RESOLUTION_TAG, "X Resolution", print_uint_tag_array, NULL },
    { Y_RESOLUTION_TAG, "Y Resolution", print_uint_tag_array, NULL },
    { PLANAR_CONFIGURATION_TAG, "Planar Configuration",
        print_uint_tag_array, format_planar_configuration },

    { RESOLUTION_UNIT_TAG, "Resolution unit",
        print_uint_tag_array, format_resolution_unit },

    { SOFTWARE_TAG, "Software", print_string_tag, NULL },
    { DATE_TIME_TAG, "Date", print_string_tag, NULL },

    { ARTIST_TAG, "Artist", print_string_tag, NULL },
    { HOST_COMPUTER_TAG, "HostComputer", print_string_tag, NULL },

    { WHITE_POINT_TAG, "White point", print_uint_tag_array, NULL },
    { PRIMARY_CHROMACITIES_TAG, "Primary Chromacities",
      print_uint_tag_array, NULL },

    { TILE_WIDTH_TAG, "Tile width", print_uint_tag_array, NULL },
    { TILE_LENGTH_TAG, "Tile length", print_uint_tag_array, NULL },
    { TILE_BYTE_COUNTS_TAG, "Tiles size in bytes", print_uint_tag_array, NULL },
    { TILE_OFFSETS_TAG, "Tiles offset in TIFF", print_uint_tag_array, NULL },

    { YCBCR_COEFFICIENTS_TAG, "YCbCr Coefficients", print_uint_tag_array, NULL },
    { YCBCR_SUBSAMPLING_TAG, "YCbCr Subsampling", print_uint_tag_array, NULL },
    { YCBCR_POSITIONING_TAG, "YCbCr Positioning",
        print_uint_tag_array, format_ycbcr_positioning },

    { REFERENCE_BLACK_WHITE_TAG, "Reference Black, White",
        print_uint_tag_array, NULL },

    { COPYRIGHT_TAG, "Copyright", print_string_tag, NULL },

    { PRINT_IM_TAG, "Print IM", print_uint_tag_array, format_hex_bytes }
};

static ifd_tag_print_t ifd_2_tag_print[] = {
    { EXPOSURE_TIME_TAG, "Exposure Time",
        print_uint_tag_array, format_exposure_time },
    { FNUMBER_TAG, "FNumber", print_uint_tag_array, NULL },
    { EXPOSURE_PROGRAM_TAG, "Exposure Program",
        print_uint_tag_array, format_exposure_program },
    { ISO_SPEED_RATINGS_TAG, "ISO Speed Ratings", print_uint_tag_array, NULL },

    { SENSITIVITY_TYPE_TAG, "Sensitivity type",
      print_uint_tag_array, format_sensitivity_type  },
    { STANDARD_OUTPUT_SENSITIVITY_TAG, "Standard output sensitivity",
      print_uint_tag_array, NULL },
    { RECOMMENDED_EXPOSURE_INDEX_TAG, "Recommended exposure index",
      print_uint_tag_array, NULL },

    { EXIF_VERSION_TAG,"Exif version", print_string_tag, NULL },

    { DATE_TIME_ORIGINAL_TAG, "Original Picture Date", print_string_tag, NULL },
    { DATE_TIME_DIGITIZED_TAG, "Digitized Picture Date",
        print_string_tag, NULL },
    { OFFSET_TIME_TAG, "Time offset from UTC", print_string_tag, NULL },
    { OFFSET_TIME_ORIGINAL_TAG, "Original Picture Time offset from UTC",
        print_string_tag, NULL },
    { OFFSET_TIME_DIGITIZED_TAG, "Digitized Picture Time offset from UTC",
        print_string_tag, NULL },

    { COMPONENTS_CONFIGURATION_TAG, "Components configuration",
        print_string_tag, NULL },

    { COMPRESSED_BITS_PER_PIXEL_TAG, "Compressed bits per pixel",
        print_uint_tag_array, NULL },

    { SHUTTER_SPEED_VALUE_TAG, "Shutter-speed value",
        print_uint_tag_array, format_srational },

    { APERTURE_VALUE_TAG, "Aperture Value", print_uint_tag_array, NULL },
    { BRIGHTNESS_VALUE_TAG, "Brightness Value",
        print_uint_tag_array, format_srational },

    { EXPOSURE_BIAS_VALUE_TAG, "Exposure-Bias value",
        print_uint_tag_array, format_srational },

    { MAX_APERTURE_VALUE_TAG, "Max Aperture Value",
         print_uint_tag_array, NULL },
    { SUBJECT_DISTANCE_TAG, "Subject Distance", print_uint_tag_array, NULL },

    { METERING_MODE_TAG, "Metering mode",
        print_uint_tag_array, format_metering_mode },
    { LIGHT_SOURCE_TAG, "Light Source",
        print_uint_tag_array, format_ligth_source },

    { FLASH_TAG, "Flash", print_uint_tag_array, format_flash },
    { FOCAL_LENGTH_TAG, "Focal length", print_uint_tag_array, NULL },

    { SUBJECT_AREA_TAG, "Subject Area", print_uint_tag_array, NULL },

    { USER_COMMENT_TAG, "User Comment", print_user_comment, NULL },

    { SUBSEC_TIME_TAG, "Sub-second time", print_string_tag, NULL },
    { SUBSEC_TIME_ORIGINAL_TAG, "Sub-second original time",
        print_string_tag, NULL },
    { SUBSEC_TIME_DIGITIZED_TAG, "Sub-second digitized time",
        print_string_tag, NULL },

    { FLASHPIX_VERSION_TAG, "Flashpix version", print_string_tag, NULL },
    { COLOR_SPACE_TAG, "Color Space", print_uint_tag_array,
        format_color_space },

    { PIXEL_X_DIMENSION_TAG, "Image X dimension in pixels",
        print_uint_tag_array, NULL },
    { PIXEL_Y_DIMENSION_TAG, "Image Y dimension in pixels",
        print_uint_tag_array, NULL },
    { RELATED_SOUND_FILE_TAG, "Related Sound File", print_string_tag, NULL },

    { SPATIAL_FREQUENCY_RESPONSE_TAG, "Spatial frequency response",
        print_uint_tag_array, format_hex_bytes },

    { FOCAL_PLANE_X_RESOLUTION_TAG, "Focal Plane X resolution",
        print_uint_tag_array, NULL },
    { FOCAL_PLANE_Y_RESOLUTION_TAG, "Focal Plane Y resolution",
        print_uint_tag_array, NULL },
    { FOCAL_PLANE_RESOLUTION_UNIT_TAG, "Focal Plane Resolution Unit",
        print_uint_tag_array, format_resolution_uint },

    { SUBJECT_LOCATION_TAG, "Subject Location (X,Y)",
        print_uint_tag_array, NULL },

    { EXPOSURE_INDEX_TAG, "Exposure Index", print_uint_tag_array, NULL },
    { SENSING_METHOD_TAG, "Sensing Method",
        print_uint_tag_array, format_sensing_method },

    { FILE_SOURCE_TAG, "File Source",
        print_uint_tag_array, format_file_source },
    { SCENE_TYPE_TAG, "Scene Type",
        print_uint_tag_array, format_scene_type },

    // CFS pattern hs been replaced with an ascii string during parsing
    { CFA_PATTERN_TAG, "Color Filter Array", print_string_tag, NULL },
    { CUSTOM_RENDERED_TAG, "Data Processing",
        print_uint_tag_array, format_custom_rendered },

    { EXPOSURE_MODE_TAG, "Exposure Mode",
        print_uint_tag_array, format_exposure_mode },
    { WHITE_BALANCE_TAG, "White Balance",
        print_uint_tag_array, format_white_balance },
    { DIGITAL_ZOOM_RATIO_TAG, "Digital Zoom Ration",
        print_uint_tag_array, format_digital_zoom_ratio },
    { FOCAL_LENGTH_IN_35MM_FILM_TAG, "Focal length in 35mm film",
        print_uint_tag_array, format_35mm_focal_length },

    { SCENE_CAPTURE_TYPE_TAG, "Scene Capture Type",
        print_uint_tag_array, format_scene_capture_type },
    { GAIN_CONTROL_TAG, "Gain Control",
        print_uint_tag_array, format_gain_control },

    { CONTRAST_TAG, "Contrast", print_uint_tag_array, format_contrast },
    { SATURATION_TAG, "Saturation", print_uint_tag_array, format_saturation },
    { SHARPNESS_TAG, "Sharpness", print_uint_tag_array, format_sharpness },
    { SUBJECT_DISTANCE_RANGE_TAG, "Subject Distance Range",
        print_uint_tag_array, format_subject_distance_range },
    { IMAGE_UNIQUE_ID_TAG, "Image Unique ID", print_string_tag, NULL },

    { OWNER_NAME_TAG, "Owner name", print_string_tag, NULL },
    { BODY_SERIAL_NUMBER_TAG, "Serial Numner", print_string_tag, NULL },

    { LENS_SPECIFICATION_TAG, "Lens Specification",
        print_uint_tag_array, NULL },
    { LENS_MAKE_TAG, "Lens Manufacturer", print_string_tag, NULL },
    { LENS_MODEL_TAG, "Lens Model", print_string_tag, NULL },
    { LENS_SERIAL_NUMBER_TAG, "Lens serial number", print_string_tag, NULL },

    { COMPOSITE_IMAGE_TAG, "Composition",
        print_uint_tag_array, format_composite_image },
    { COMPOSITE_IMAGE_COUNT_TAG, "Composite image count",
        print_uint_tag_array, NULL },
    { COMPOSITE_IMAGE_EXPOSURE_TIME_TAG, "Composite image exposure time",
        print_uint_tag_array, NULL }
};

static ifd_tag_print_t ifd_3_tag_print[] = {

    { GPS_VERSION_ID_TAG, "GPS version", print_uint_tag_array, NULL },
    { GPS_LATITUDE_REF_TAG, "GPS Latitude reference", print_string_tag, NULL },
    { GPS_LATITUDE_TAG, "GPS latitude", print_uint_tag_array, NULL },
    { GPS_LONGITUDE_REF_TAG, "GPS Longitude reference", print_string_tag, NULL },
    { GPS_LONGITUDE_TAG, "GPS Longitude", print_uint_tag_array, NULL },
    { GPS_ALTITUDE_REF_TAG, "GPS Altitude reference", print_string_tag, NULL },
    { GPS_ALTITUDE_TAG, "GPS Altitude", print_uint_tag_array, NULL },
    { GPS_TIME_STAMP_TAG, "GPS time", print_uint_tag_array, NULL },
    { GPS_SATELLITES_TAG, "GPS Satellites", print_string_tag, NULL },
    { GPS_STATUS_TAG, "GPS Status", print_string_tag, NULL },
    { GPS_MEASURE_MODE_TAG, "GPS Measure mode", print_string_tag, NULL },
    { GPS_DOP_TAG, "GPS DOP", print_uint_tag_array, NULL },
    { GPS_SPEED_REF_TAG, "GPS Speed reference", print_string_tag, NULL },
    { GPS_SPEED_TAG, "GPS Speed", print_uint_tag_array, NULL },
    { GPS_TRACK_REF_TAG, "GPS version", print_string_tag, NULL },
    { GPS_TRACK_TAG, "GPS version", print_uint_tag_array, NULL },
    { GPS_IMG_DIRECTION_REF_TAG, "GPS version", print_string_tag, NULL },
    { GPS_IMG_DIRECTION_TAG, "GPS version", print_uint_tag_array, NULL },

};

extern void print_ifd_tags( exif_desc_t *desc, ifd_id_t id, slice_t *tags,
                            char *indent_string )
{
    if ( NULL == tags ) {
        return;
    }

    ifd_tag_print_t *ptr;
    int n_tags;
    switch( id ) {
    case PRIMARY: case THUMBNAIL:
        ptr = ifd_01_tag_print;
        n_tags = sizeof(ifd_01_tag_print)/sizeof(ifd_tag_print_t);
        break;
    case EXIF:
        ptr = ifd_2_tag_print;
        n_tags = sizeof(ifd_2_tag_print)/sizeof(ifd_tag_print_t);
        break;
    case GPS:
        ptr = ifd_3_tag_print;
        n_tags = sizeof(ifd_2_tag_print)/sizeof(ifd_tag_print_t);
        break;
    case IOP:
        return;
    }

    int start = -1;
    uint32_t len = slice_len( tags );
    for ( uint32_t i = 0; i < len; ++i ) {
        uint16_t tag = *(uint16_t *)slice_item_at( tags, i );

        int index = start;
        while ( true ) {
            ++index;
            if ( index == n_tags ) {
                break;
            }

            if ( ptr[index].tag == tag ) {
                ptr[index].print( desc, id, tag, indent_string,
                                  ptr[index].header, ptr[index].format );
                start = index;
                break;
            }
        }
    }
}

