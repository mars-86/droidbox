#ifndef __PICTURE_TRANSFER_PROTOCOL_OBJECT_INCLUDED_H__
#define __PICTURE_TRANSFER_PROTOCOL_OBJECT_INCLUDED_H__

#include <stdint.h>

struct object_info {
    uint32_t StorageID;
    uint16_t ObjectFormat;
    uint16_t ProtectionStatus;
    uint32_t ObjectCompressedSize;
    uint16_t ThumbFormat;
    uint32_t ThumbCompressedSize;
    uint32_t ThumbPixWidth;
    uint32_t ThumbPixHeight;
    uint32_t ImagePixWidth;
    uint32_t ImagePixHeight;
    uint32_t ImageBitDepth;
    uint32_t ParentObject;
    uint16_t AssociationType;
    uint32_t AssociationDesc;
    uint32_t SequenceNumber;
    char* Filename;
    char* CaptureDate;
    char* ModificationDate;
    char* Keywords;
} __attribute__((packed));

struct object_info2 {
    uint32_t StorageID;
    uint16_t ObjectFormat;
    uint16_t ProtectionStatus;
    uint32_t ObjectCompressedSize;
    uint16_t ThumbFormat;
    uint32_t ThumbCompressedSize;
    uint32_t ThumbPixWidth;
    uint32_t ThumbPixHeight;
    uint32_t ImagePixWidth;
    uint32_t ImagePixHeight;
    uint32_t ImageBitDepth;
    uint32_t ParentObject;
    uint16_t AssociationType;
    uint32_t AssociationDesc;
    uint32_t SequenceNumber;
    uint8_t flen;
    uint16_t f;
    uint8_t clen;
    uint16_t c;
    uint8_t mlen;
    uint16_t m;
    uint8_t klen;
    uint16_t k;
} __attribute__((packed));

#define PTP_OBJECT_PROTECTION_NO_PROTECTION 0x0000
#define PTP_OBJECT_PROTECTION_READ_ONLY 0x0001

#define PTP_OBJECT_AGGREGATED_STORES 0xFFFFFFFF
#define PTP_OBJECT_FORMAT_CODE_IMAGE 0xFFFFFFFF
#define PTP_OBJECT_FORMAT_CODE_UNUSED 0x00000000
#define PTP_OBJECT_ASSOCIATION_ROOT 0xFFFFFFFF
#define PTP_OBJECT_ASSOCIATION_UNUSED 0x00000000
#define PTP_OBJECT_AGGREGATED_HANDLES 0xFFFFFFFF
#define PTP_OBJECT_FORMAT_CODE_HANDLES_IMAGE 0xFFFFFFFF
#define PTP_OBJECT_FORMAT_CODE_HANDLES_UNUSED 0x00000000
#define PTP_OBJECT_ASSOCIATION_HANDLES_ROOT 0xFFFFFFFF
#define PTP_OBJECT_ASSOCIATION_HANDLES_UNUSED 0x00000000

#define PTP_OBJECT_DELETE_ALL_HANDLES 0xFFFFFFFF
#define PTP_OBJECT_FORMAT_CODE_IMAGE_DELETE 0xFFFFFFFF
#define PTP_OBJECT_NO_PARENT 0x00000000 // Object exists in root
#define PTP_OBJECT_NO_SEQ_NUMBER 0x00000000

#define PTP_OBJECT_FORMAT_UNDEFINED_NON_IMAGE 0x3000 // A Undefined Undefined non-image object
#define PTP_OBJECT_FORMAT_ASSOCIATION 0x3001 // A Association Association (e.g. folder)
#define PTP_OBJECT_FORMAT_SCRIPT 0x3002 // A Script Device-model-specific script
#define PTP_OBJECT_FORMAT_EXECUTABLE 0x3003 // A Executable Device - model - specific binary executable
#define PTP_OBJECT_FORMAT_TEXT 0x3004 // A Text Text file
#define PTP_OBJECT_FORMAT_HTML 0x3005 // A HTML HyperText Markup Language file (text)
#define PTP_OBJECT_FORMAT_DPOF 0x3006 // A DPOF Digital Print Order Format file (text)
#define PTP_OBJECT_FORMAT_AIFF 0x3007 // A AIFF Audio clip
#define PTP_OBJECT_FORMAT_WAV 0x3008 // A WAV Audio clip
#define PTP_OBJECT_FORMAT_MP3 0x3009 // A MP3 Audio clip
#define PTP_OBJECT_FORMAT_AVI 0x300A // A AVI Video clip
#define PTP_OBJECT_FORMAT_MPEG 0x300B // A MPEG Video clip
#define PTP_OBJECT_FORMAT_ASF 0x300C // A ASF Microsoft Advanced Streaming Format (video)
#define PTP_OBJECT_FORMAT_UNKNOWN_IMAGE 0x3800 // I Undefined Unknown image object
#define PTP_OBJECT_FORMAT_EXIF_JPEG 0x3801 // I EXIF/JPEG Exchangeable File Format, JEIDA standard
#define PTP_OBJECT_FORMAT_TIFF_EP 0x3802 // I TIFF/EP Tag Image File Format for Electronic Photography
#define PTP_OBJECT_FORMAT_FLASHPIX 0x3803 // I FlashPix Structured Storage Image Format
#define PTP_OBJECT_FORMAT_BMP 0x3804 // I BMP Microsoft Windows Bitmap file
#define PTP_OBJECT_FORMAT_CIFF 0x3805 // I CIFF Canon Camera Image File Format
#define PTP_OBJECT_FORMAT_UNDEFINED_RESERVED 0x3806 // I Undefined Reserved
#define PTP_OBJECT_FORMAT_GIF 0x3807 // I GIF Graphics Interchange Format
#define PTP_OBJECT_FORMAT_JFIF 0x3808 // I JFIF JPEG File Interchange Format
#define PTP_OBJECT_FORMAT_PCD 0x3809 // I PCD PhotoCD Image Pac
#define PTP_OBJECT_FORMAT_PICT 0x380A // I PICT Quickdraw Image Format
#define PTP_OBJECT_FORMAT_PNG 0x380B // I PNG Portable Network Graphics
#define PTP_OBJECT_FORMAT_UNDEFINED_RESERVED2 0x380C // I Undefined Reserved
#define PTP_OBJECT_FORMAT_TIFF 0x380D // I TIFF Tag Image File Format
#define PTP_OBJECT_FORMAT_TIFF_IT 0x380E // I TIFF/IT Tag Image File Format for Information Technology (graphic arts)
#define PTP_OBJECT_FORMAT_JP2 0x380F // I JP2 JPEG2000 Baseline File Format
#define PTP_OBJECT_FORMAT_JPX 0x3810 // I JPX JPEG2000 Extended File Format

#endif // __PICTURE_TRANSFER_PROTOCOL_OBJECT_INCLUDED_H__
