#ifndef __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__
#define __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__

#include <stdint.h>

#define PTP_DEVICE_FUNCTIONAL_MODE_STANDARD 0x0000
#define PTP_DEVICE_FUNCTIONAL_MODE_SLEEP_STATE 0x0001

struct device_info {
    uint16_t StandardVersion;
    uint32_t VendorExtensionID;
    uint16_t VendorExtensionVersion;
    char* VendorExtensionDesc;
    uint16_t FunctionalMode;
    uint8_t* OperationsSupported;
    uint8_t* EventsSupported;
    uint8_t* DevicePropertiesSupported;
    uint8_t* CaptureFormats;
    uint8_t* ImageFormats;
    char* Manufacturer;
    char* Model;
    char* DeviceVersion;
    char* SerialNumber;
} __attribute__((packed));

#define PTP_STORAGE_TYPE_UNDEFINED 0x0000
#define PTP_STORAGE_TYPE_FIXED_ROM 0x0001
#define PTP_STORAGE_TYPE_REMOVABLE_ROM 0x0002
#define PTP_STORAGE_TYPE_FIXED_RAM 0x0003
#define PTP_STORAGE_TYPE_REMOVABLE_RAM 0x0004

#define PTP_STORAGE_FILESYSTEM_UNDEFINED 0x0000
#define PTP_STORAGE_FILESYSTEM_GENERIC_FLAT 0x0001
#define PTP_STORAGE_FILESYSTEM_GENERIC_HIERARCHICAL 0x0002
#define PTP_STORAGE_FILESYSTEM_DCF 0x0003

#define PTP_STORAGE_ACCESS_CAPABILITY_RW 0x0000
#define PTP_STORAGE_ACCESS_CAPABILITY_RO_WITHOUT_OD 0x0001
#define PTP_STORAGE_ACCESS_CAPABILITY_RO_WITH_OD 0x0002

#define PTP_STORAGE_MAX_CAPACITY_UNUSED 0xFFFFFFFFFFFFFFFF
#define PTP_STORAGE_FREE_SPACE_IN_BYTES_UNUSED 0xFFFFFFFFFFFFFFFF
#define PTP_STORAGE_FREE_SPACE_IN_IMAGES_UNUSED 0xFFFFFFFF

struct storage_info {
    uint16_t StorageType;
    uint16_t FilesystemType;
    uint16_t AccessCapability;
    uint64_t MaxCapacity;
    uint64_t FreeSpaceInBytes;
    uint32_t FreeSpaceInImages;
    char* StorageDescription;
    char* VolumeLabel;
} __attribute__((packed));

#endif // __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__
