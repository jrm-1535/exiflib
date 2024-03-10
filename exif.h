
#ifndef __EXIF_H__
#define  __EXIF_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <assert.h>

#include "slice.h"

// Internal data structures map TIFF/EXIF types to their corresponding native
// C types, uint(8|16|32)_t or float | double, except the TIFF rationals which
// map to the following structures.
typedef struct {
    uint32_t    numerator, denominator;
} urational_t;

typedef struct {
    int32_t     numerator, denominator;
} rational_t;

// subset of supported TIFF tags
typedef enum {                               // PRIMARY & THUMBNAIL IFD tags

    PROCESSING_SOFTWARE             = 0x000b,   // TIFF, ascii string

    IMAGE_WIDTH_TAG                 = 0x0100,   // Baseline TIFF, uint(16|32)_t
    IMAGE_LENGTH_TAG                = 0x0101,   // Baseline TIFF, uint(16|32)_t
    BITS_PER_SAMPLE_TAG             = 0x0102,   // Baseline TIFF, uint16_t array
    COMPRESSION_TAG                 = 0x0103,   // Baseline TIFF, uint16_t

    PHOTOMETRIC_INTERPRETATION_TAG  = 0x0106,   // Baseline TIFF, 1 uint16_t

    IMAGE_DESCRIPTION_TAG           = 0x010e,   // Baseline TIFF, ascii string
    MAKE_TAG                        = 0x010f,   // Baseline TIFF, ascii string
    MODEL_TAG                       = 0x0110,   // Baseline TIFF, ascii string

    STRIP_OFFSETS_TAG               = 0x0111,   // Baseline TIFF, n uint(16|32)_t

    ORIENTATION_TAG                 = 0x0112,   // Baseline TIFF, 1 uint16_t

    SAMPLES_PER_PIXEL_TAG           = 0x0115,   // Baseline TIFF, 1 uint16_t
    ROWS_PER_STRIP_TAG              = 0x0116,   // Baseline TIFF, uint(16|32)_t
    STRIP_BYTE_COUNTS_TAG           = 0x0117,   // Baseline TIFF, n uint(16|32)_t

    X_RESOLUTION_TAG                = 0x011a,   // Baseline TIFF, 1 urational_t
    Y_RESOLUTION_TAG                = 0x011b,   // Baseline TIFF, 1 urational_t
    PLANAR_CONFIGURATION_TAG        = 0x011c,   // Baseline TIFF, 1 planar_conf_t

    RESOLUTION_UNIT_TAG             = 0x0128,   // Baseline TIFF, 1 uint16_t

    SOFTWARE_TAG                    = 0x0131,   // Baseline TIFF, ascii string
    DATE_TIME_TAG                   = 0x0132,   // Baseline TIFF, ascii string

    ARTIST_TAG                      = 0x013b,   // Baseline TIFF, ascii string
    HOST_COMPUTER_TAG               = 0x013c,   // Baseline TIFF, ascii string

    WHITE_POINT_TAG                 = 0x013e,   // Baseline TIFF, 2 urational_t
    PRIMARY_CHROMACITIES_TAG        = 0x013f,   // Baseline TIFF, 6 urational_t

    TILE_WIDTH_TAG                  = 0x0142,   // Tiled image, uint(16|32)_t
    TILE_LENGTH_TAG                 = 0x0143,   // Tiled image, uint(16|32)_t
    TILE_OFFSETS_TAG                = 0x0144,   // Tiled image, uint32_t
    TILE_BYTE_COUNTS_TAG            = 0x0145,   // Tiled image, uint(16|32)_t

    YCBCR_COEFFICIENTS_TAG          = 0x0211,   // YCbCr extension, 3 uint16_t
    YCBCR_SUBSAMPLING_TAG           = 0x0212,   // YCbCr extension, 2 uint16_t
    YCBCR_POSITIONING_TAG           = 0x0213,   // YCbCr extension, 1 uint16_t

    REFERENCE_BLACK_WHITE_TAG       = 0x0214,   // Bassline TIFF, 6 urational_t

    COPYRIGHT_TAG                   = 0x8298,   // Baseline TIFF, ascii string

    PRINT_IM_TAG                    = 0xc4a5    // undefined, array of bytes
} primary_ifd_tag_t;

// Image Compression is a uint16_t corresponding to the following enum
typedef enum {
    NOT_COMPRESSED = 1,
    CCITT_1D,
    CCITT_Group3,
    CCITT_Group4,
    LZW,
    JPEG,
    JPEG_Technote2,
    Deflate,
    RFC_2301_BW_JBIG,
    RFC_2301_Color_JBIG,
    PACKBITS = 32773
} compression_t;

// Photometric Interpretration is a uint16_t corresponding to the following enum
typedef enum {
    BILEVEL_OR_GRAYSCALE_0_WHITE = 0,
    BILEVEL_OR_GRAYSCALE_0_BLACK,
    RGB,
    PALETTE
} photometric_interpretation_t;

// tiff orientation is a uint16_t corresponding to the following enum
typedef enum {
    ROW_0_TOP_COL_0_LEFT = 1,
    ROW_0_TOP_COL_0_RIGHT,
    ROW_0_BOTTOM_COL0_RIGHT,
    ROW_0_BOTTOM_COL0_LEFT,
    ROW_0_LEFT_COL_0_TOP,
    ROW_0_RIGHT_COL_0_TOP,
    ROW_0_RIGHT_COL_0_BOTTOM,
    ROW_0_LEFT_COL_0_BOTTOM,
} tiff_orientation_t;

// tiff planar configuration is a uint16_t correspnding to the follwoing enum
typedef enum {
    CHUNKY_FORMAT = 1,
    PLANAR_FORMAT
} planar_conf_t;

// tiff resolution unit is a uint16_t corresponding to the following enum
typedef enum {
    DOTS_PER_ARBITRATY_UNIT = 1,
    DOTS_PER_INCH,
    DOTS_PER_CM
} tiff_resolution_unit_t;

// YCbCr positioning is a uint16_t corresponding to the following enum
typedef enum {
    YCBCR_CENTERED = 1,
    YCBCR_COSITED
} YCbCr_positioning_t;

// subset of supported EXIF tags
typedef enum {

    EXPOSURE_TIME_TAG               = 0x829a,   // EXIF, 1 urational_t
    FNUMBER_TAG                     = 0x829d,   // EXIF, 1 urational_t

    EXPOSURE_PROGRAM_TAG            = 0x8822,   // EXIF, exif_exposure_program_t

    ISO_SPEED_RATINGS_TAG           = 0x8827,   // EXIF, uint16_t array

    SENSITIVITY_TYPE_TAG            = 0x8830,   // EXIF, exif_sensibility_type_t
    STANDARD_OUTPUT_SENSITIVITY_TAG = 0x8831,   // EXIF, 1 uint32_t
    RECOMMENDED_EXPOSURE_INDEX_TAG  = 0x8832,   // EXIF, 1 uint32_t

    EXIF_VERSION_TAG                = 0x9000,   // EXIF, 4 uint8_t

    DATE_TIME_ORIGINAL_TAG          = 0x9003,   // EXIF, ascii string
    DATE_TIME_DIGITIZED_TAG         = 0x9004,   // EXIF, ascii string

    OFFSET_TIME_TAG                 = 0x9010,   // EXIF, ascii string
    OFFSET_TIME_ORIGINAL_TAG        = 0x9011,   // EXIF, ascii string
    OFFSET_TIME_DIGITIZED_TAG       = 0x9012,   // EXIF, ascii string

    COMPONENTS_CONFIGURATION_TAG    = 0x9101,   // EXIF, ascii string
    COMPRESSED_BITS_PER_PIXEL_TAG   = 0x9102,   // EXIF, 1 urational_t

    SHUTTER_SPEED_VALUE_TAG         = 0x9201,   // EXIF, 1 (signed) rational_t
    APERTURE_VALUE_TAG              = 0x9202,   // EXIF, 1 urational_t
    BRIGHTNESS_VALUE_TAG            = 0x9203,   // EXIF, 1 (signed) rational_t
    EXPOSURE_BIAS_VALUE_TAG         = 0x9204,   // EXIF, 1 (signed) rational_t
    MAX_APERTURE_VALUE_TAG          = 0x9205,   // EXIF, 1 urational_t
    SUBJECT_DISTANCE_TAG            = 0x9206,   // EXIF, 1 urational_t
    METERING_MODE_TAG               = 0x9207,   // EXIF, exif_metering_mode_t
    LIGHT_SOURCE_TAG                = 0x9208,   // EXIF, exif_ligth_source_t
    FLASH_TAG                       = 0x9209,   // EXIF, exif_flash_t
    FOCAL_LENGTH_TAG                = 0x920a,   // EXIF, 1 urational_t

    SUBJECT_AREA_TAG                = 0x9214,   // EXIF, 2, 3 or 4 uint16_t

    USER_COMMENT_TAG                = 0x9286,   // EXIF, uint8_t array

    SUBSEC_TIME_TAG                 = 0x9290,   // EXIF, ascii string
    SUBSEC_TIME_ORIGINAL_TAG        = 0x9291,   // EXIF, ascii string
    SUBSEC_TIME_DIGITIZED_TAG       = 0x9292,   // EXIF, ascii string

    FLASHPIX_VERSION_TAG            = 0xa000,   // EXIF, 4 uint8_t
    COLOR_SPACE_TAG                 = 0xa001,   // EXIF, color_space_t
    PIXEL_X_DIMENSION_TAG           = 0xa002,   // EXIF, 1 uint(16|32)_t
    PIXEL_Y_DIMENSION_TAG           = 0xa003,   // EXIF, 1 uint(16|32)_t
    RELATED_SOUND_FILE_TAG          = 0xa004,   // EXIF, ascii string

    SPATIAL_FREQUENCY_RESPONSE_TAG  = 0xa20c,   // EXIF, spatial_frequency_response_t
    FOCAL_PLANE_X_RESOLUTION_TAG    = 0xa20e,   // EXIF, 1 urational_t
    FOCAL_PLANE_Y_RESOLUTION_TAG    = 0xa20f,   // EXIF, 1 urational_t
    FOCAL_PLANE_RESOLUTION_UNIT_TAG = 0xa210,   // EXIF, focal_resolution_unit_t

    SUBJECT_LOCATION_TAG            = 0xa214,   // EXIF, 2 uint16_t
    EXPOSURE_INDEX_TAG              = 0xa215,   // EXIF, 1 urational_t
    SENSING_METHOD_TAG              = 0xa217,   // EXIF, sensing_method_t

    FILE_SOURCE_TAG                 = 0xa300,   // EXIF, 1 uint8_t
    SCENE_TYPE_TAG                  = 0xa301,   // EXIF, 1 uint8_t
    CFA_PATTERN_TAG                 = 0xa302,   // EXIF, uint8_t array

    CUSTOM_RENDERED_TAG             = 0xa401,   // EXIF, custom_rendered_t
    EXPOSURE_MODE_TAG               = 0xa402,   // EXIF, exposure_mode_t
    WHITE_BALANCE_TAG               = 0xa403,   // EXIF, white_balance_t
    DIGITAL_ZOOM_RATIO_TAG          = 0xa404,   // EXIF, 1 urational_t (0=no zoom)
    FOCAL_LENGTH_IN_35MM_FILM_TAG   = 0xa405,   // EXIF, 1 uint16_t (0=unknown)
    SCENE_CAPTURE_TYPE_TAG          = 0xa406,   // EXIF, scene_capture_type_t
    GAIN_CONTROL_TAG                = 0xa407,   // EXIF, gain_control_t
    CONTRAST_TAG                    = 0xa408,   // EXIF, contrast_t
    SATURATION_TAG                  = 0xa409,   // EXIF, saturation_t
    SHARPNESS_TAG                   = 0xa40a,   // EXIF, sharpness_t

    SUBJECT_DISTANCE_RANGE_TAG      = 0xa40c,   // EXIF, subject_distance_range_t

    IMAGE_UNIQUE_ID_TAG             = 0xa420,   // EXIF, ascii string

    OWNER_NAME_TAG                  = 0xa430,   // EXIF, ascii string
    BODY_SERIAL_NUMBER_TAG          = 0xa431,   // EXIF, ascii string
    LENS_SPECIFICATION_TAG          = 0xa432,   // EXIF, 4 urational_t
    LENS_MAKE_TAG                   = 0xa433,   // EXIF, ascii string
    LENS_MODEL_TAG                  = 0xa434,   // EXIF, ascii string
    LENS_SERIAL_NUMBER_TAG          = 0xa435,   // EXIF, ascii string

    COMPOSITE_IMAGE_TAG             = 0xa460,   // EXIF, exif_composite_image_t
    COMPOSITE_IMAGE_COUNT_TAG       = 0xa461,   // EXIF, 2 uint16_t (total/used)
    COMPOSITE_IMAGE_EXPOSURE_TIME_TAG = 0xa462    // EXIF, array of binary data
} exif_ifd_tag_t;

// exif exposure program is a uint16_t corresponding to the following enum
typedef enum {

    UNDEFINED = 0,
    MANUAL,
    NORMAL_PROGRAM,
    APERTURE_PRIORITY,
    SHUTTER_PRIORITY,
    CREATIVE_PROGRAM,
    ACTION_PROGRAM,
    PORTRAIT_MODE,
    LANDSCAPE_MODE

} exif_exposure_program_t;

// exif sensibility type is a uint16_t corresponding to the following enum
typedef enum {
    UNKNOWN_SENSITIVITY = 0,
    STANDARD_OUTPUT_SENSITIVITY,
    RECOMMENDED_EXPOSURE_INDEX,
    ISO_SPEED,
    STANDARD_OUTPUT_SENSITIVITY_RECOMMENDED_EXPOSURE_INDEX,
    STANDARD_OUTPUT_SENSITIVITY_ISO_SPEED,
    RECOMMENDED_EXPOSURE_INDEX_ISO_SPEED,
    STANDARD_OUTPUT_SENSITIVITY_RECOMMENDED_EXPOSURE_INDEX_ISO_SPEED

} exif_sensibility_type_t;

// exif component configurations are strings, giving for each component its
// type, i.e. either one of 'Y', 'Cb', 'Cr' or 'R', 'G', 'B' for non-transformed
// RGB pixels. A '?' indicate an unknown component type (i.e. not YCbCr or RGB).

// exif metering mode is a uint16_t corresponding to the following enum
typedef enum {

    UNKNOWN_MODE = 0,
    AVERAGE,
    CENTER_WEIGHTED_AVERAGE_PROGRAM,
    SPOT,
    MULTISPOT,
    PATTERN,
    PARTIAL,

    OTHER = 255

} exif_metering_mode_t;

// exif light source us a uint16_t corresponding to the following enum
typedef enum {

    UNKNOWN_SOURCE = 0,
    DAYLIGHT,
    FLUORESCENT,
    TUNGSTEN_INCANDESCENT_LIGHT,
    FLASH,

    FINE_WEATHER = 9,
    CLOUDY_WEATHER,
    SHADE,
    DAYLIGHT_FLUORESCENT_D_5700_7100K,
    DAY_WHITE_FLUORESCENT_N_4600_5400K,
    COOL_WHITE_FLUORESCENT_W_3900_4500K,
    WHITE_FLUORESCENT_WW_3200_3700K,

    STANDARD_LIGHT_A = 17,
    STANDARD_LIGHT_B,
    STANDARD_LIGHT_C,
    D55,
    D65,
    D75,
    D50,
    ISO_STUDIO_TUNGSTEN,

    OTHER_LIGHT_SOURCE = 255

} exif_ligth_source_t;

// exif flash value is a uint16_t with only 6 meaningful bits
// bit 0 indicates whether the flash was fired
// bit 1 indicates whether a strobe return dectection was done
// bit 2 codew the light status: 10 no light detected, 11 light detected
// bits 3 and 4 code the flah mode: 00 unknown, 01 compulsory flash firing,
//                                  10 compulsory flash suppression, 11 auto
// bit 5 indocates the presence of flash function: 0 flash present, 1 no flash
// bit 6 indicates the re-eye mode: 0 no re-eye reduction, 1 red-eye reduction
//
// The valid combinations correspond to the following enum
typedef enum {

    NO_FLASH_FUNCTION = 0x20,

    FLASH_DID_NOT_FIRE = 0x00,
    FLASH_DID_NOT_FIRE_COMPULSORY_FLASH_MODE = 0x10,    // compulsory suppressed
    FLASH_DID_NOT_FIRE_AUTO_MODE = 0x18,

    FLASH_FIRED = 0x01,
    FLASH_FIRED_STROBE_RETURN_LIGHT_NOT_DETECTED = 0x05,
    FLASH_FIRED_STROBE_RETURN_LIGHT_DETECTED = 0x07,

    FLASH_FIRED_COMPULSORY_FLASH_MODE = 0x09,           // compulsory fired
    FLASH_FIRED_COMPULSORY_FLASH_MODE_RETURN_LIGHT_NOT_DETECTED = 0x0d,
    FLASH_FIRED_COMPULSORY_FLASH_MODE_RETURN_LIGHT_DETECTED = 0x0f,

    FLASH_FIRED_RED_EYE_REDUCTION_MODE = 0x41,
    FLASH_FIRED_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED = 0x45,
    FLASH_FIRED_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED = 0x47,

    FLASH_FIRED_COMPULSORY_FLASH_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED = 0x49,
    FLASH_FIRED_COMPULSORY_FLASH_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED = 0x4f,

    FLASH_FIRED_AUTO_MODE = 0x19,
    FLASH_FIRED_AUTO_MODE_RETURN_LIGHT_NOT_DETECTED = 0x1d,
    FLASH_FIRED_AUTO_MODE_RETURN_LIGHT_DETECTED = 0x1f,

    FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE = 0x59,
    FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_NOT_DETECTED = 0x5d,
    FLASH_FIRED_AUTO_MODE_RED_EYE_REDUCTION_MODE_RETURN_LIGHT_DETECTED = 0x5f

} exif_flash_t;

// exif user comments are an array of uint8_t values, starting with 8 bytes
// indicating the encoding:
//  'A', 'S', 'C', 'I', 'I',  0,   0,  0 for ITU-T T.50 IA5 (ASCII)
//  'J', 'I', 'S',  0,   0,   0,   0,  0 for JIS X208-1990 (JIS)
//  'U', 'N', 'I', 'C', 'O', 'D', 'E', 0 for Unicode Standard
//   0,   0,   0,   0,   0,   0,   0,  0 for unspecified encoding
// The flowwing bytes contain the user comments according to that encoding,
// without requiring a terminaring 0 (which may still be present).

// exif color space is a uint16_t corresponding to the following enum
typedef enum {

    SRGB = 0x0001,
    UNCALIBRATED = 0xffff

} color_space_t;


// exif focal length is a urational_t corresponding to the focal length of the
// lens in mmm.

// exif subject area is a series of 2, 3 or 4 uint16_t with the following
// meaning:
// if series of 2 values, the first value is the X coordinate, the second is
//                        the Y coordinate
// if series of 3 values, the area is a circle. The first value is the circle
//                        center X coordinate, the second is the center Y
//                        coordinate and the last is the circle diameter.
// if series of 4 values, the area is a rectangle. The first value is the
//                        rectangle center X coordinate, the second is the
//                        center Y coordinate, the third value is the width of
//                        the rectangle and tje last is its height.

// exif spatial frequency response is stored as a byte array corresponding to
// the following pseudo structure
typedef struct {
    uint16_t n_col, n_rows; // number of columns and rows
                            // for ( i=0; i < n_cols; ++i ) {
    char     col_name[2];   //   column[i] name // actually a variable length,
                            // }                // \0 terminated string.
    urational_t array[2];   // replace 2 with (n_cols * n_rows)
} spatial_frequency_response_t;

// exif focal_resolution_unit_t is a uint16_t corresponding to the following enum
typedef enum {
    RESOLUTION_UNIT_NONE = 1,
    RESOLUTION_UNIT_INCHES,
    RESOLUTION_UNIT_CM,
    RESOLUTION_UNIT_MM,
    RESOLUTION_UNIT_UM,
} focal_resolution_unit_t;

// exif subject location is a series of 2 uint16_t, the first one giving the
// X coordinate  (column number )of the main subject center and the second one
// giving the Y cordinate (row number).

// exif sensing method is a uint16_t corresponding to the following enum
typedef enum {

    UNDEFINED_SENSOR = 1,
    ONE_CHIP_COLOR_AREA_SENSOR,
    TWO_CHIP_COLOR_AREA_SENSOR,
    THREE_CHIP_COLOR_AREA_SENSOR,
    COLOR_SEQUENTIAL_AREA_SENSOR,
    TRILINEAR_SENSOR = 7,
    COLOR_SEQUENTIAL_LINEAR_SENSOR

} sensing_method_t;

// exif file source is a uint8_t, which if present must be equal to 3 with the
// meaning of DSC (image recorded on a DSC).

// exif scene type is a uint8_t, which if present must be equal to 1 with the
// meaning of directly photographed image (image recorded by a DSC).

// exif custom rendered is a uint16_t corresponding to the following enum
typedef enum {

    NORMAL_PROCESS = 0,
    CUSTOM_PROCESS,

    HDR_PROCESS,
    HDR_PROCESS_ORIGINAL_SAVED,
    ORIGINAL,

    PANORAMA_PROCESS = 6,
    HDR_PORTRAIT_PROCESS,
    PORTRAIT_PROCESS

} custom_rendered_t;

// exif exposure mode is a uint16_t correspnding to the following enum
typedef enum {

    AUTO_EXPOSURE = 0,
    MANUAL_EXPOSURE,
    AUTO_BRACKET

} exposure_mode_t;

// exif white balance is a uint16_t coresponding to the following enum
typedef enum {

    AUTO_WHITE_BALANCE = 0,
    MANUAL_WHITE_BALANCE

} white_balance_t;

// exif scene capture type is a uint16 corresponding to the following enum
typedef enum {

    STANDARD_SCENE = 0,
    LANDSCAPE,
    PORTRAIT,
    NIGHT_SCENE

} scene_capture_type_t;

// exif CFA (Color Filter Array) pattern is a 0 terminated string of ascii
// characters, one for each pixel in each row of the sensor array, plus a
// comma delimiter at the end of each row except the last one.
//
// Each character encodes the filtered color:
//    'R' for red, 'G' for green, 'B' for blue, 'C' for cyan,
//    'M' for magenta and 'w' for white.
//
// For example, a CFA string such as "GB,RG\0" indicates a 4 pixel array, a
// matrix of 2 rows of 2 pixels, with the first row being made of a green
// filter for the first column and blue filter for the second column, and the
// second row being made of a red filter for the first column and a green
// filter for the second column.

// exif gain control is a uint16_t corresponding to the following enum
typedef enum {

    NO_GAIN = 0,
    LOW_GAIN_UP,
    HIGH_GAIN_UP,
    LOW_GAIN_DOWN,
    HIGH_GAIN_DOWN

} gain_control_t;

// exif contrast is a uint16_t corresponding to the follwing enum
typedef enum {

    NORMAL_CONTRAST = 0,
    SOFT_CONTRAST,
    HARD_CONTRAST

} contrast_t;

// exif saturation is a uint16_t corresponding to the following enum
typedef enum {

    NORMAL_SATURATION = 0,
    LOW_SATURATION,
    HIGH_SATURATION

} saturation_t;

// exif sharpness is a uint16_t corresponding to the following enum
typedef enum {

    NORMAL_SHARPNESS = 0,
    SOFT_SHARPNESS,
    HARD_SHARNESS

} sharpness_t;

// exif subject distance range is a uint16_t corresponding to the following enum
typedef enum {

    UNKNOWN_DISTANCE = 0,
    MACRO,
    CLOSE_VIEW,
    DISTANT_VIEW

} subject_distance_range_t;


// exif composite image is a uint16_t corresponding to the following enum
typedef enum {

    UNKNOWN_COMPOSITION = 0,
    NON_COMPOSITE_IMAGE,
    GENERAL_COMPOSITE_IMAGE,
    COMPOSITE_IMAGE_CAPTURED_WHEN_SHOOTING

} exif_composite_image_t;

// exif lens specification is given as an array of 4 rationals, in the
// following order: minimum, maximum focal length, minimum, maximum F number
// When a F number is not known its notation is 0/0

typedef enum {                              // GPS IFD specific tags
    GPS_VERSION_ID_TAG          = 0x00,         // GPS, 4-byte version number
    GPS_LATITUDE_REF_TAG        = 0x01,
    GPS_LATITUDE_TAG            = 0x02,
    GPS_LONGITUDE_REF_TAG       = 0x03,
    GPS_LONGITUDE_TAG           = 0x04,
    GPS_ALTITUDE_REF_TAG        = 0x05,
    GPS_ALTITUDE_TAG            = 0x06,
    GPS_TIME_STAMP_TAG          = 0x07,
    GPS_SATELLITES_TAG          = 0x08,
    GPS_STATUS_TAG              = 0x09,
    GPS_MEASURE_MODE_TAG        = 0x0a,
    GPS_DOP_TAG                 = 0x0b,
    GPS_SPEED_REF_TAG           = 0x0c,
    GPS_SPEED_TAG               = 0x0d,
    GPS_TRACK_REF_TAG           = 0x0e,
    GPS_TRACK_TAG               = 0x0f,
    GPS_IMG_DIRECTION_REF_TAG   = 0x10,
    GPS_IMG_DIRECTION_TAG       = 0x11,
    GPS_MAP_DATUM_TAG           = 0x12,
    GPS_DEST_LATITUDE_REF_TAG   = 0x13,
    GPS_DEST_LATITUDE_TAG       = 0x14,
    GPS_DEST_LONGITUDE_REF_TAG  = 0x15,
    GPS_DEST_LONGITUDE_TAG      = 0x16,
    GPS_DEST_BEARING_REF_TAG    = 0x17,
    GPS_DEST_BEARING_TAG        = 0x18,
    GPS_DEST_DISTANCE_REF_TAG   = 0x19,
    GPS_DEST_DISTANCE_TAG       = 0x1a,
    GPS_PROCESSING_METHOD_TAG   = 0x1b,
    GPS_AREA_INFORMATION_TAG    = 0x1c,
    GPS_DATE_STAMP_TAG          = 0x1d,
    GPS_DIFFERENTIAL_TAG        = 0x1e,
    GPS_H_POSITIONING_ERROR_TAG = 0x1f

} gps_ifd_tag_t;

typedef enum {                              // IOP IFD tags
    INTEROPERABILITY_INDEX_TAG      = 0x01,
    INTEROPERABILITY_VERSION_TAG    = 0x02,

    RELATED_IMAGE_FILE_FORMAT_TAG   = 0x1000, // ascii string
    RELATED_IMAGE_WIDTH_TAG         = 0x1001, // uint16_t
    RELATED_IMAGE_HEIGHT_TAG        = 0x1002, // uint16_t

} iop_ifd_t;

// IFD ID, used as a namespace for IFD tags
typedef enum {
    PRIMARY,                // namespace for IFD0, first (TIFF) IFD
    THUMBNAIL,              // namespace for IFD1 (Thumbnail) pointed to by IFD0
    EXIF,                   // EXIF namespace, embedded in IFD0
    GPS,                    // GPS namespace, embedded in IFD0
    IOP,                    // Interoperability namespace, embedded in EXIF IFD
// the following IFDs are possibe but not processed here
//    MAKER,                  // non-standard IFD for each maker, embedded in EXIF IFD
//    EMBEDDED                // possible non-standard IFD embedded in MAKER
} ifd_id_t;

typedef struct {
    ifd_id_t        origin; // either THUMBNAIL or EMBEDDED
    compression_t   comp;   // type of image compression
    uint32_t        size;   // image size
} thumbnail_info_t;

typedef struct {
    bool                    skip_unknown_tags;
    bool                    warnings;
    bool                    parse_debug;
} exif_control_t;

typedef struct _exif_desc exif_desc_t;

// parse_exif looks up for the EXIF header at the offset corresponding to the
// given start value. If an exif header is found the following file content is
// parsed, otherwise it backs up to the beginning of the file and looks for a
// TIFF header (which may or may not include an exif IFD) and if found it
// starts parsing the following file content. If no EXIF or TIFF header was
// found it returns a NULL pointer, otherwise it returns a non-NULL exif
// descriptor pointer that can be used to get the content of all IFDs that
// have been sucessfully parsed,
//
// It implements the bitap (or shift-Or) algorithm to quickly find the exif
// header. Exif header is 6-byte long ("Exif\x0\x0") and requires only a 6-bit
// position mask. It uses a 256-byte mask array, which is is likely to stay in
// cache, and a bitmask that fits in a register. The time complexity is O(n).
extern exif_desc_t *parse_exif( FILE *f, uint32_t start,
                                exif_control_t *control );

// read_exif opens the file associated with the given path and calls parse_exif.
// If no EXIF or TIFF header was found it returns a NULL pointer, otherwise it
// returns a non-NULL exif descriptor pointer that can be used to get the
// content of all IFDs that have been sucessfully parsed,
extern exif_desc_t *read_exif( char *path, uint32_t start,
                               exif_control_t *control );

// exif_get_ifd_ids returns the slice of available IFD Ids from the given exif
// descriptor, or NULL in case of failure. IFD ids are returned as type ifd_id_t
// inside the slice. After use, the returned slice must be freed by the caller.
extern slice_t *exif_get_ifd_ids( exif_desc_t *desc );

// exif_get_ifd_tags returns a slice with all tags available from the requested
// IFD id or NULL in case of failure. If the argument cmp is not NULL, the
// returned slice is sorted according the the given comparison function cmp.
// After use, the returned slice must be freed by the caller.
extern slice_t *exif_get_ifd_tags( exif_desc_t *desc, ifd_id_t id,
                                   comp_fct cmp );

typedef enum {
    NOT_A_TYPE = 0,     // an error return

    UBYTE_TYPE = 1,     // each value in array is uint8_t
    ASCII_TYPE = 2,     // value is char[], zero terminated array
    USHORT_TYPE = 3,    // each value in array is uint16_t
    ULONG_TYPE = 4,     // each value in array is uint32_t
    URATIONAL_TYPE = 5, // each value is array is urational_t

    SBYTE_TYPE = 6,     // each value in array is int8_t
    UNDEFINED_TYPE = 7, // each value in array depends on tag semantics
    SSHORT_TYPE = 8,    // each value in array is int16_t
    SLONG_TYPE = 9,     // each value in array is int32_t
    SRATIONAL_TYPE = 10 // each value in array is rational_t

} exif_type_t;
// if the requested tag is found in the IFD specified by id then the exif type
// of its values is returned, otherwise the exif_type_t value of NOT_A_TYPE
// is returned
extern exif_type_t exif_get_ifd_tag_type( exif_desc_t *desc, ifd_id_t id,
                                          uint16_t tag );
// if the requested tag is found in the IFD specified by id then the vector
// of associated values is returned as a side effect and the function returns
// true, otherwise it returns false.
//
// WARNING: The vector obtained with this call is the internal vector and it
// should NEVER be modified or freed by the caller.
extern bool exif_get_ifd_tag_values( exif_desc_t *desc, ifd_id_t id,
                                     uint16_t tag, vector_t **values );

// exif_print_ifd_entries prints all metadata found in the ifd specified by id.
// The argument indent_string gives the optional text that is prepended to each
// entry.
extern bool exif_print_ifd_entries( exif_desc_t *desc, ifd_id_t id,
                                    char *indent_string );

// exif_free frees all internal exif data structures.
extern bool exif_free( exif_desc_t *desc );

#endif /* __EXIF_H__ */
