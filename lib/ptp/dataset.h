#ifndef __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__
#define __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__

#include <stdint.h>

#define PTP_DEVICE_FUNCTIONAL_MODE_STANDARD 0x0000
#define PTP_DEVICE_FUNCTIONAL_MODE_SLEEP_STATE 0x0001
#define PTP_DEVICE_RESET_PROP_VALUES_ALL 0xFFFFFFFF

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

#endif // __PICTURE_TRANSFER_PROTOCOL_DATASET_INCLUDED_H__
