
#ifndef __PARSE_H__
#define __PARSE_H__

#include <stdint.h>
#include <stdbool.h>

#include "map.h"

/*
    EXIF metadata layout:

    Exif header:
      "Exif\x00\x00"            Fixed 6-byte header

    TIFF header:    Note this is the origin of following offsets
      "II" | "MM"               2-byte endianess (Intel LE/Motorola BE)
                                All following multi-byte values depend on endianess
      0x002a                    2-byte Magic Number
      0x00000008                4-byte offset of immediately following primary IFD

    IFD0:           Primary Image Data
    IFD1:           Thumbnail Image Data (optional)
*/

// TIFF header offsets and sizes
#define ORIGIN_OFFSET   6   // TIFF header offset in EXIF file
#define HEADER_SIZE     8   // TIFF header size
#define VAL_OFF_SIZE    4   // value fits in if <= 4 bytes, otherwise offset
#define IFD_ENTRY_SIZE  ((SHORT_SIZE+LONG_SIZE)*2)

/*
    An IFD has the following layout
      <n>                       2-byte (uint16_t) count of following entries
      { IFD entry } * n         12-byte entry
      <offset next IFD>         4-byte offset of Thumbnail IFD
      <IFD data>                variable length data area for values pointed to
                                by entries, for embedded IFDs (EXIF and/or GPS),
                                or for an embedded JPEG thumbnail image.

    Each IFD entry is:
        entry Tag               2-byte unique tag (namespace according to IFDn)
        entry type              2-byte TIFF type
        value count             4-byte count of values:
                                    number of items in an array, or number of
                                    bytes in an ASCII string.
        value or value offset   4-byte data:
                                    if the value fits in 4 bytes, the value is
                                    directly in the entry, otherwise the entry
                                    value contains the offset in the IFD data
                                    where the value is located.
*/

// TIFF IFD entry type code
#define TIFF_UINT8      1
#define TIFF_STRING     2
#define TIFF_UINT16     3
#define TIFF_UINT32     4
#define TIFF_URATIONAL  5

#define TIFF_INT8       6
#define TIFF_UNDEFINED  7
#define TIFF_INT16      8
#define TIFF_INT32      9
#define TIFF_RATIONAL   10

#define TIFF_FLOAT      11
#define TIFF_DOUBLE     12

// TIFF Type sizes (signed or unsigned)
#define BYTE_SIZE       1
#define SHORT_SIZE      2
#define LONG_SIZE       4
#define RATIONAL_SIZE   8
#define FLOAT_SIZE      4
#define DOUBLE_SIZE     8

/*
    A complete IFD is made of:
         (_ShortSize + ( _IfdEntrySize * n ) + _LongSize) bytes
    plus the variable size data area

    Typically IFD0 is a TIFF IFD that embeds up to 4 other IFDs:

        GPS IFD     for specific Global Positioning System tags
        EXIF IFD    for specific Exif tags, which may include 2 other IFDs:

            IOP pointer         IOP IFD for specific Interoperability tags
            MN pointer          MakerNote (format similar to IFD but proprietary)

    Note that embedded IFDs are pointed to from a parent IFD, and do not use
    the next IFD pointer at the end. They are located in their parent IFD
    variable size data area. Their exact location in the data area does not
    matter.

    IFD0 is usually followed by IFD1, which is dedicated to embedded thumbnail
    images. In case of a JPEG thumbnails, the whole JPEG file without APPn
    marker is embedded in the IFD1 data area.

    Baseline TIFF requires only IFD0. EXIF requires that an EXIF IFD be embedded
    in IFD0, and if a thumnail image is included in addition to the main image,
    that IFD1 follows IFD0. IFD1 embeds the thumnail image.

    Metadata:

      IFD0 (Primary) ===================================
        n    (2 byte count)                            ^
        ...  (12-byte entries)                         |
        _ExifIFD ----------                            |
        ...                |         fixed size = (n * 12) + 2 + 4
        _GpsIFD ---------  |                           |
        ...              | |                           |
        ...              | |                           v
   -- next IFD (4 bytes) | | ===========================
   |  < IFD0 data        | |
   |     ...             | |
   |     GPS IFD <-------  | (optional)
   |       ...             |
   |       ...             |
   |     < GPS IFD data    |
   |       ...             |
   |     >                 |
   |     ...               |
   |     EXIF IFD <--------
   |       ...
   |       _MN -----------------
   |       ...                  |
   |       _IOP ----------      |
   |       ...            |     |
   |     < EXIF IFD data  |     |
   |       ...            |     |
   |       IOP IFD <------      |
   |         ...                |
   |       next IFD = 0         |
   |       < IOP IFD data >     |
   |       ...                  |
   |       MN IFD <-------------
   |         ...
   |       next IFD = 0
   |       < MN IFD data >
   |       ...
   |     > (end EXIF IFD)
   |     ...
   |  > (end IFD0)
   -> IFD1 (Thumbnail, optional)
        n    (2 byte count)
        ...  (12-byte entries)
        ...
      < IFD1 data
        Thumbnail JPEG image
      >
      next IFD = 0
*/

#define _IFD_N (IOP+1)      // last supported ifd entry + 1 to size arrays

// internal tag definitions, not directly accessible

#define JPEG_INTERCHANGE_FORMAT_TAG         0x201   // in IFD0
#define JPEG_INTERCHANGE_FORMAT_LENGTH_TAG  0x202   // in IFD0

#define EXIF_IFD_TAG                        0x8769  // in IFD0
#define GPS_IFD_TAG                         0x8825  // in IFD0
#define INTEROPERABILITY_IFD_TAG            0xa005  // in EXIF

// tags to ignore
#define UNKNOWN_TAG1                        0x0220  // in IFD0 (in WIKO RAINBOW 4G)
#define UNKNOWN_TAG2                        0x0221  // in IFD0 (in WIKO RAINBOW 4G)
#define UNKNOWN_TAG3                        0x0222  // in IFD0 (in WIKO RAINBOW 4G)
#define UNKNOWN_TAG4                        0x0223  // in IFD0 (in WIKO RAINBOW 4G)
#define UNKNOWN_TAG5                        0x889a  // in IFD0 & EXIF
#define UNKNOWN_TAG6                        0x9a00  // in IFD0 & EXIF

#define MAKER_NOTE_TAG                      0x927c  // in EXIF
#define XP_COMMENT                          0x9c9c  // in IFD0 (Microsoft proprietary)
#define PANASONIC_TITLE                     0xc6d2  // in IFD0 (Panasonic proprietary)
#define PANASONIC_TITLE2                    0xc6d3  // in IFD0 (Panasocnic proprietary)
#define PADDING_TAG                         0xea1c  // May be in IFD0, IFD1 & Exif
#define OFFSET_SCHEMA_TAG                   0xea1d  // in EXIF (Microsoft proprietary)

// IFD generic support (conforming to TIFF, EXIF etc.)
typedef struct {
    ifd_id_t            id;         // namespace for each IFD
    struct _exif_desc   *desc;      // parent descriptor
    map_t               *content;   // all ifd { tag, values }
    long                saved_pos;  // temporary saved file position

                                    // current IFD field during parsing
    uint16_t            tag;        // field tag
    uint16_t            type;       // field type
    uint32_t            count;      // field count
    uint32_t            valoff;     // field value or offset in following data
} ifd_desc_t;

// exif descriptor with all required IFD metadata
struct _exif_desc {
    FILE                *file;
    long                header;
    bool                big_endian;
    exif_control_t      control;        // what to do when parsing

    uint32_t            thumb_offset;
    uint32_t            thumb_size;

//    map_t               *global;        // map for global information ?
    map_t               *ifds[_IFD_N];  // flat ifd content access by id

};

static inline size_t make_key_from_tag( uint16_t tag ) {
    return (size_t)0x10000 + (size_t)tag; // force key to be non-zero
}

extern uint16_t tiff_get_uint16( exif_desc_t *d );
extern uint32_t tiff_get_uint32( exif_desc_t *d );
extern uint32_t tiff_get_raw_uint32( exif_desc_t *d );
extern uint32_t tiff_endianize_uint32( exif_desc_t *d, uint32_t raw );
extern uint16_t tiff_endianize_uint16( exif_desc_t *d, uint16_t raw );

extern map_t *exif_parse_ifd( struct _exif_desc *desc,
                              ifd_id_t id, uint32_t *next );

#endif /* __PARSE_H__ */
