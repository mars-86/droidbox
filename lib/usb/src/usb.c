#include "../usb.h"
#include <asm/byteorder.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define TIMEOUT 1000
#define BUFF_MAX_SIZE 4096
#define STRING_MAX_SIZE 128

#define __ENDPOINT_TRANSFER_MASK 0x03
#define __ENDPOINT_SYNC_MASK 0x0C
#define __ENDPOINT_USAGE_MASK 0x30
#define __ENDPOINT_MAX_PACKET_SIZE_MASK 0x7FF
#define __ENDPOINT_TRANSAC_PER_MF_MASK 0x1800

#define __ENDPOINT_ADDRESS(ued) ued->bEndpointAddress & 0x07
#define __ENDPOINT_DIRECTION(ued) ued->bEndpointAddress & 0x80 ? "IN" : "OUT"
#define __ENDPOINT_MAX_PACKET_SIZE(ued) ued->wMaxPacketSize& __ENDPOINT_MAX_PACKET_SIZE_MASK

typedef struct idescriptor idt_t;

struct idescriptor {
    struct usb_interface_descriptor uid;
    struct usb_endpoint_descriptor* ued;
};

struct usb_parsed_device_descriptor {
    struct usb_device_descriptor udd;
};

struct usb_parsed_config_descriptor {
    struct usb_config_descriptor ucd;
    idt_t* ifaced;
};

static int __usb_bulk_msg(int fd, struct usbdevfs_urb* uurb)
{
    printf("SUBMIT URB\n");
    int res;
    do {
        res = ioctl(fd, USBDEVFS_SUBMITURB, uurb);
    } while (res == -1 && errno == EINTR);

    if (res < 0) {
        return 1;
    }

    printf("SUBMIT REAPURB\n");
    unsigned char uurb_id[8];
    do {
        res = ioctl(fd, USBDEVFS_REAPURB, uurb_id);
    } while (res == -1 && errno == EINTR);

    if (res < 0)
        return 1;

    return 0;
}

static void __parse_stream(void* dest, unsigned char* src, size_t n);
static void __print_device_descriptor(FILE* output, int fd, unsigned short langid, struct usb_device_descriptor* udd);
static void __print_config_descriptor(FILE* output, int fd, unsigned short langid, struct usb_config_descriptor* ucd);
static void __print_interface_descriptor(FILE* output, int fd, unsigned short langid, struct usb_interface_descriptor* uid);
static void __print_endpoint_descriptor(FILE* output, struct usb_endpoint_descriptor* ued);
static void __print_interface_association_descriptor(FILE* output, struct usb_interface_assoc_descriptor* uiad);
static const char* __get_descriptor_type(__u8 dt);
static const char* __endpoint_transfer_type(__u8 eattr);
static const char* __endpoint_sync_type(__u8 eattr);
static const char* __endpoint_usage_type(__u8 eattr);
static const char* __endpoint_transac_per_mf(__le16 epacksz);
static const char* __get_class(__u8 cattr);

int usb_clear_feature(int fd, unsigned short feature_selector, unsigned short w_index)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD;
    usbct.bRequest = USB_REQ_CLEAR_FEATURE;
    usbct.wValue = feature_selector;
    usbct.wIndex = w_index;
    usbct.wLength = 0;
    usbct.timeout = TIMEOUT;

    switch (feature_selector) {
    case USB_DEVICE_REMOTE_WAKEUP:
        usbct.bRequestType |= USB_RECIP_DEVICE;
        break;
    case USB_ENDPOINT_HALT:
        usbct.bRequestType |= USB_RECIP_ENDPOINT;
        break;
    case USB_DEVICE_TEST_MODE:
        usbct.bRequestType |= USB_RECIP_DEVICE;
        break;
    default:;
    }

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_get_configuration(int fd, unsigned char* buff, int* wlen)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_IN;
    usbct.bRequest = USB_REQ_GET_CONFIGURATION;
    usbct.wValue = 0;
    usbct.wIndex = 0;
    usbct.wLength = 1;
    usbct.timeout = TIMEOUT;
    usbct.data = buff;

    *wlen = usbct.wLength;
    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_get_descriptor(int fd, unsigned char type, unsigned char index, unsigned short langid, unsigned char* buff, int* wlen)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_IN;
    usbct.bRequest = USB_REQ_GET_DESCRIPTOR;
    usbct.wValue = (unsigned short)((type << 8) | index);
    usbct.wIndex = langid;
    usbct.timeout = TIMEOUT;
    usbct.data = buff;

    switch (type) {
    case USB_DT_DEVICE:
        usbct.wLength = USB_DT_DEVICE_SIZE;
        break;
    case USB_DT_CONFIG:
        usbct.wLength = USB_DT_CONFIG_SIZE;
        break;
    case USB_DT_STRING:
        usbct.wLength = USB_MAX_STRING_LEN;
        break;
    case USB_DT_INTERFACE:
        usbct.wLength = USB_DT_INTERFACE_SIZE;
        break;
    case USB_DT_ENDPOINT:
        usbct.wLength = USB_DT_ENDPOINT_SIZE;
        break;
    case USB_DT_DEVICE_QUALIFIER:
        usbct.wLength = 32;
        break;
    case USB_DT_OTHER_SPEED_CONFIG:
        usbct.wLength = 32;
        break;
    }

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;

    // we need to ask for the complete configuration (byte 2 and 3 holds the total config size)
    if (type == USB_DT_CONFIG) {
        usbct.wLength = (buff[3] << 8) | buff[2];
        if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
            return 1;
    }
    if (wlen)
        *wlen = buff[0];
    return 0;
}

int usb_get_interface(int fd, unsigned short interface, unsigned char* buff, int* wlen)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE;
    usbct.bRequest = USB_REQ_GET_INTERFACE;
    usbct.wValue = 0;
    usbct.wIndex = interface;
    usbct.wLength = 1;
    usbct.timeout = TIMEOUT;
    usbct.data = buff;

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;

    if (wlen)
        *wlen = usbct.wLength;
    return 0;
}

int usb_get_status(int fd, unsigned char recipient, unsigned char w_index, void* buff, int* wlen)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | recipient;
    usbct.bRequest = USB_REQ_GET_STATUS;
    usbct.wValue = 0;
    usbct.wIndex = w_index;
    usbct.wLength = 2;
    usbct.timeout = TIMEOUT;
    usbct.data = buff;

    *wlen = usbct.wLength;
    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_set_address(int fd, unsigned short address)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    usbct.bRequest = USB_REQ_SET_ADDRESS;
    usbct.wValue = address;
    usbct.wIndex = 0;
    usbct.wLength = 0;
    usbct.timeout = TIMEOUT;

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_set_configuration(int fd, unsigned short config)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_OUT;
    usbct.bRequest = USB_REQ_SET_CONFIGURATION;
    usbct.wValue = config;
    usbct.wIndex = 0;
    usbct.wLength = 0;
    usbct.timeout = TIMEOUT;

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_set_descriptor(int fd, unsigned char type, unsigned char index, unsigned short langid, unsigned char* buff, int* wlen)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD;
    usbct.bRequest = USB_REQ_SET_DESCRIPTOR;
    usbct.wValue = (unsigned short)((type << 8) | index);
    usbct.wIndex = langid;
    usbct.wLength = 126;
    usbct.timeout = TIMEOUT;

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_set_interface(int fd, unsigned short interface, unsigned short alternate_setting)
{
    /*
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE;
    usbct.bRequest = USB_REQ_SET_INTERFACE;
    usbct.wValue = alternate_setting;
    usbct.wIndex = interface;
    usbct.wLength = 0;
    usbct.timeout = TIMEOUT;

    int res;
    if ((res = ioctl(fd, USBDEVFS_CONTROL, &usbct)) == -1)
        return 1;
    */
    /* using USBDEVFS_INTERFACE request generate logs */
    struct usbdevfs_setinterface __setif;
    __setif.altsetting = alternate_setting;
    __setif.interface = interface;
    if (ioctl(fd, USBDEVFS_SETINTERFACE, &__setif) == -1)
        return 1;
    return 0;
}

int usb_synch_frame(int fd, unsigned char endpoint, void* buff)
{
    struct usbdevfs_ctrltransfer usbct;
    usbct.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_ENDPOINT;
    usbct.bRequest = USB_REQ_SYNCH_FRAME;
    usbct.wValue = 0;
    usbct.wIndex = endpoint;
    usbct.wLength = 2;
    usbct.timeout = TIMEOUT;
    usbct.data = buff;

    if (ioctl(fd, USBDEVFS_CONTROL, &usbct) == -1)
        return 1;
    return 0;
}

int usb_detach_interface(int fd, unsigned short interface)
{
    struct usbdevfs_ioctl __uioctl;
    __uioctl.ifno = interface;
    __uioctl.ioctl_code = USBDEVFS_DISCONNECT;
    __uioctl.data = NULL;

    if (ioctl(fd, USBDEVFS_IOCTL, &__uioctl) == -1)
        return 1;
    return 0;
}

int usb_claim_interface(int fd, unsigned short interface)
{
    int __iface = interface;
    return (ioctl(fd, USBDEVFS_CLAIMINTERFACE, &__iface) == -1) ? 1 : 0;
}

int usb_release_interface(int fd, unsigned short interface)
{
    int __iface = interface;
    return (ioctl(fd, USBDEVFS_RELEASEINTERFACE, &__iface) == -1) ? 1 : 0;
}

int usb_get_driver(int fd, unsigned short interface, char* driver, size_t len)
{
    struct usbdevfs_getdriver __ugd;
    __ugd.interface = interface;

    if (ioctl(fd, USBDEVFS_GETDRIVER, &__ugd) == -1)
        return 1;

    // FIX
    size_t __dlen = strlen(__ugd.driver) + 1, __len = len < __dlen ? len : __dlen;
    memcpy(driver, __ugd.driver, __len);

    return 0;
}

int usb_bulk_send(int fd, uint16_t endpoint, void* data, uint32_t len)
{
    unsigned char* __data = (unsigned char*)data;
    struct usbdevfs_urb uurb = { 0 };
    uurb.type = USBDEVFS_URB_TYPE_BULK;
    uurb.endpoint = USB_DIR_OUT | endpoint;
    uurb.status = -1;
    uurb.buffer = __data;
    uurb.buffer_length = len;

    if (__usb_bulk_msg(fd, &uurb))
        return -1;

    return uurb.actual_length;
}

int usb_bulk_recv(int fd, uint16_t endpoint, void* data, uint32_t len)
{
    struct usbdevfs_urb uurb = { 0 };
    uurb.type = USBDEVFS_URB_TYPE_BULK;
    uurb.endpoint = USB_DIR_IN | endpoint;
    uurb.status = -1;
    uurb.buffer = data;
    uurb.buffer_length = len;

    if (__usb_bulk_msg(fd, &uurb))
        return -1;

    return uurb.actual_length;
}

static void __parse_interface_association(FILE* output, const unsigned char* buff)
{
    struct usb_interface_assoc_descriptor uiad;
    unsigned char __uiad_size = USB_DT_INTERFACE_ASSOCIATION_SIZE;

    __parse_stream(&uiad, (unsigned char*)buff, __uiad_size);
    __print_interface_association_descriptor(output, &uiad);
}

void usb_dump_device(int fd, FILE* output)
{
    unsigned char buff[BUFF_MAX_SIZE] = { 0 };
    int rlen, i;

    if (usb_get_descriptor(fd, USB_DT_STRING, 0x00, 0x00, buff, &rlen) != 0) {
        perror("ioctl");
        return;
    }

    if (usb_get_descriptor(fd, USB_DT_DEVICE, 0x00, 0x00, buff, &rlen) != 0) {
        perror("ioctl");
        return;
    }

    struct usb_device_descriptor __udd;

    __parse_stream(&__udd, buff, sizeof(struct usb_device_descriptor));
    __print_device_descriptor(output, fd, 0x0409, &__udd);

    int __n_config = __udd.bNumConfigurations;

    struct usb_config_descriptor __ucd;
    struct usb_interface_descriptor __uid;
    struct usb_endpoint_descriptor __ued;
    unsigned char __ucd_size = USB_DT_CONFIG_SIZE, __uid_size = USB_DT_INTERFACE_SIZE, __ued_size = USB_DT_ENDPOINT_SIZE;
    for (i = 0; i < __n_config; ++i) {
        memset(buff, 0, BUFF_MAX_SIZE);
        if (usb_get_descriptor(fd, USB_DT_CONFIG, i, 0x00, buff, &rlen) != 0) {
            perror("ioctl");
            return;
        }
        __parse_stream(&__ucd, buff, __ucd_size);
        __print_config_descriptor(output, fd, 0x0409, &__ucd);
        unsigned short __data_size = __ucd.wTotalLength, offset;
        for (offset = USB_DT_CONFIG_SIZE; offset < __data_size; offset += *(buff + offset)) {
            unsigned char dt = *(buff + offset + 1);
            switch (dt) {
            case USB_DT_INTERFACE:
                __parse_stream(&__uid, buff + offset, __uid_size);
                __print_interface_descriptor(output, fd, 0x0409, &__uid);
                break;
            case USB_DT_ENDPOINT:
                __parse_stream(&__ued, buff + offset, __ued_size);
                __print_endpoint_descriptor(output, &__ued);
                break;
            case USB_DT_INTERFACE_ASSOCIATION:
                __parse_interface_association(output, buff + offset);
                break;
            }
        }
    }
}

int usb_get_string(int fd, unsigned char index, unsigned short langid, char* buff)
{
    unsigned char __buff[USB_MAX_STRING_LEN];
    char* __buffp = buff;
    int __rlen;
    if (usb_get_descriptor(fd, USB_DT_STRING, index, langid, __buff, &__rlen) != 0) {
        *__buffp = '\0';
        return 1;
    }

    int i;
    // i = 2 skips length and descriptor type
    for (i = 2; i < __rlen; ++i)
        if (__buff[i])
            *__buffp++ = __buff[i];
    *__buffp = '\0';

    return 0;
}

static void __parse_stream(void* dest, unsigned char* src, size_t n)
{
    memcpy(dest, src, n);
}

static updd_t* __parse_device_descriptor(unsigned char* src)
{
    updd_t* __updd = (updd_t*)malloc(sizeof(updd_t));
    size_t __udd_size = sizeof(struct usb_device_descriptor);

    __parse_stream(&__updd->udd, src, __udd_size);

    return __updd;
}

static upcd_t* __parse_config_descriptor(unsigned char* src)
{
    upcd_t* __upcd = (upcd_t*)malloc(sizeof(upcd_t));
    size_t __ucd_size = USB_DT_CONFIG_SIZE;
    size_t __uid_size = USB_DT_INTERFACE_SIZE;
    size_t __ued_size = USB_DT_ENDPOINT_SIZE;

    __parse_stream(&__upcd->ucd, src, __ucd_size);
    unsigned short __data_size = __upcd->ucd.wTotalLength, __n_iface = __upcd->ucd.bNumInterfaces, offset = __ucd_size,
                   __iface_offset = 0, __endp_offset = 0;

    int i;
    struct idescriptor* __idtp;
    struct usb_endpoint_descriptor* __uedp;
    __upcd->ifaced = (idt_t*)malloc(__n_iface * sizeof(idt_t));

    for (i = offset; i < __data_size; i += offset) {
        unsigned char __dt = *(src + i + 1);
        switch (__dt) {
        case USB_DT_INTERFACE:
            __idtp = (__upcd->ifaced + __iface_offset);
            __parse_stream(&__idtp->uid, src + i, __uid_size);
            unsigned short __n_endp = __upcd->ifaced->uid.bNumEndpoints;
            __idtp->ued = (struct usb_endpoint_descriptor*)malloc(__n_endp * sizeof(__ued_size));
            __iface_offset += __uid_size, __endp_offset = 0;
            break;
        case USB_DT_ENDPOINT:
            __uedp = (__upcd->ifaced + (__iface_offset - __uid_size))->ued;
            __parse_stream(__uedp + __endp_offset, src + i, __ued_size);
            __endp_offset += __ued_size;
            break;
        }
        offset = *(src + i);
    }
    return __upcd;
}

void* parse_descriptor(unsigned char* src)
{
    unsigned char type = src[1];

    switch (type) {
    case USB_DT_DEVICE:
        return __parse_device_descriptor(src);
    case USB_DT_CONFIG:
        return __parse_config_descriptor(src);
    default:;
    }
    return NULL;
}

static void __release_device_descriptor(updd_t* updd)
{
    free(updd);
}

static void __release_config_descriptor(upcd_t* upcd)
{
    free(upcd->ifaced->ued);
    free(upcd->ifaced);
    free(upcd);
}

void release_descriptor(void* descriptor)
{
    unsigned char* type = (unsigned char*)(descriptor + 1);

    switch (*type) {
    case USB_DT_DEVICE:
        __release_device_descriptor((updd_t*)descriptor);
        break;
    case USB_DT_CONFIG:
        __release_config_descriptor((upcd_t*)descriptor);
        break;
    default:;
    }
}

void __print_device_descriptor(FILE* output, int fd, unsigned short langid, struct usb_device_descriptor* udd)
{
    char __manufacturer[STRING_MAX_SIZE], __product[STRING_MAX_SIZE], __serial_number[STRING_MAX_SIZE];
    usb_get_string(fd, udd->iManufacturer, langid, __manufacturer);
    usb_get_string(fd, udd->iProduct, langid, __product);
    usb_get_string(fd, udd->iSerialNumber, langid, __serial_number);

    fprintf(output,
        "Length: %d\n"
        "Descriptor Type (%.2X): Device\n"
        "BCD USB: %.4X\n"
        "Device Class: %.2X\n"
        "Device Sub Class: %.2X\n"
        "Device Protocol: %.2X\n"
        "Max Packet Size: %.2X\n"
        "Vendor ID: %.4X\n"
        "Product ID: %.4X\n"
        "BCD Device: %.4X\n"
        "Manufacturer (%.2X): %s\n"
        "Product (%.2X): %s\n"
        "Serial Number (%.2X): %s\n"
        "Configurations Number: %.2X\n",
        udd->bLength,
        udd->bDescriptorType,
        udd->bcdUSB,
        udd->bDeviceClass,
        udd->bDeviceSubClass,
        udd->bDeviceProtocol,
        udd->bMaxPacketSize0,
        udd->idVendor,
        udd->idProduct,
        udd->bcdDevice,
        udd->iManufacturer,
        __manufacturer,
        udd->iProduct,
        __product,
        udd->iSerialNumber,
        __serial_number,
        udd->bNumConfigurations);
}

void __print_config_descriptor(FILE* output, int fd, unsigned short langid, struct usb_config_descriptor* ucd)
{
    char __configuration[STRING_MAX_SIZE];
    usb_get_string(fd, ucd->iConfiguration, langid, __configuration);

    fprintf(output,
        "Length: %d\n"
        "Descriptor Type (%.2X): Config\n"
        "Total Data Length: %.4X\n"
        "Interfaces: %.2X\n"
        "Configuration Value: %.2X\n"
        "Configuration Description (%.2X): %s\n"
        "Configuration Characteristics: %.2X\n"
        "Maximum Power Consumption: %.2X\n",
        ucd->bLength,
        ucd->bDescriptorType,
        ucd->wTotalLength,
        ucd->bNumInterfaces,
        ucd->bConfigurationValue,
        ucd->iConfiguration,
        __configuration,
        ucd->bmAttributes,
        ucd->bMaxPower);
}

void __print_interface_descriptor(FILE* output, int fd, unsigned short langid, struct usb_interface_descriptor* uid)
{
    char __interface[STRING_MAX_SIZE];
    usb_get_string(fd, uid->iInterface, langid, __interface);

    fprintf(output,
        "Length: %d\n"
        "Descriptor Type (%.2X): Interface\n"
        "Interface Number: %.2X\n"
        "Alternate Setting: %.2X\n"
        "Number of Endpoints (%.2X): %d\n"
        "Interface Class (%.2X): %s\n"
        "Interface SubClass (%.2X): %s\n"
        "Interface Protocol: %.2X\n"
        "Interface Description (%.2X): %s\n",
        uid->bLength,
        uid->bDescriptorType,
        uid->bInterfaceNumber,
        uid->bAlternateSetting,
        uid->bNumEndpoints,
        uid->bNumEndpoints,
        uid->bInterfaceClass,
        __get_class(uid->bInterfaceClass),
        uid->bInterfaceSubClass,
        __get_class(uid->bInterfaceSubClass),
        uid->bInterfaceProtocol,
        uid->iInterface,
        __interface);
}

void __print_endpoint_descriptor(FILE* output, struct usb_endpoint_descriptor* ued)
{
    fprintf(output,
        "Length: %d\n"
        "Descriptor Type (%.2X): Endpoint\n"
        "Endpoint Address (%.2X): %.2X - %s\n"
        "Endpoint Attributes (%.2X): %s - %s - %s\n"
        "Maximum Packet Size (%.4X): %d - %s\n"
        "Polling Interval: %.2X\n",
        ued->bLength,
        ued->bDescriptorType,
        ued->bEndpointAddress,
        __ENDPOINT_ADDRESS(ued),
        __ENDPOINT_DIRECTION(ued),
        ued->bmAttributes,
        __endpoint_transfer_type(ued->bmAttributes),
        __endpoint_sync_type(ued->bmAttributes),
        __endpoint_usage_type(ued->bmAttributes),
        ued->wMaxPacketSize,
        __ENDPOINT_MAX_PACKET_SIZE(ued),
        __endpoint_transac_per_mf(ued->wMaxPacketSize),
        ued->bInterval);
}

void __print_interface_association_descriptor(FILE* output, struct usb_interface_assoc_descriptor* uiad)
{
    fprintf(output,
        "Length: %d\n"
        "Descriptor Type (%.2X): Interface Association\n"
        "First Interface: %.2X\n"
        "Interface Count: %.2X\n"
        "Function Class: %.2X\n"
        "Function SubClass: %.2X\n"
        "Function Protocol: %.2X\n"
        "iFunction: %.2X\n",
        uiad->bLength,
        uiad->bDescriptorType,
        uiad->bFirstInterface,
        uiad->bInterfaceCount,
        uiad->bFunctionClass,
        uiad->bFunctionSubClass,
        uiad->bFunctionProtocol,
        uiad->iFunction);
}

const char* __get_descriptor_type(__u8 dt)
{
    switch (dt) {
    case USB_DT_DEVICE:
        return "Device";
    case USB_DT_INTERFACE:
        return "Interface";
    case USB_DT_ENDPOINT:
        return "Endpoint";
    default:
        return "Unknown";
    }
}

const char* __endpoint_transfer_type(__u8 eattr)
{
    switch (eattr & __ENDPOINT_TRANSFER_MASK) {
    case 0:
        return "Control";
    case 1:
        return "Isochronous";
    case 2:
        return "Bulk";
    default:
        return "Interrupt";
    }
}

const char* __endpoint_sync_type(__u8 eattr)
{
    switch (eattr & __ENDPOINT_SYNC_MASK) {
    case 0:
        return "No Synchronization";
    case 1:
        return "Asynchronous";
    case 2:
        return "Adaptive";
    default:
        return "Synchronous";
    }
}

const char* __endpoint_usage_type(__u8 eattr)
{
    switch (eattr & __ENDPOINT_USAGE_MASK) {
    case 0:
        return "Data endpoint";
    case 1:
        return "Feedback endpoint";
    case 2:
        return "Implicit feedback Data endpoint";
    default:
        return "Reserved";
    }
}

const char* __endpoint_transac_per_mf(__le16 epacksz)
{
    switch (epacksz & __ENDPOINT_TRANSAC_PER_MF_MASK) {
    case 0:
        return "None (1 p/mf)";
    case 1:
        return "1 additional (2 p/mf)";
    case 2:
        return "1 additional (3 p/mf)";
    default:
        return "Reserved";
    }
}

const char* __get_class(__u8 cattr)
{
    switch (cattr) {
    case USB_CLASS_PER_INTERFACE:
        return "Device";
    case USB_CLASS_AUDIO:
        return "Audio";
    case USB_CLASS_COMM:
        return "Communications Device Class (CDC)";
    case USB_CLASS_HID:
        return "Human Interface Device (HID)";
    case USB_CLASS_PHYSICAL:
        return "Physical";
    case USB_CLASS_STILL_IMAGE:
        return "Still Image";
    case USB_CLASS_PRINTER:
        return "Printer";
    case USB_CLASS_MASS_STORAGE:
        return "Mass Storage (MSD)";
    case USB_CLASS_HUB:
        return "HUB";
    case USB_CLASS_CDC_DATA:
        return "CDC DATA";
    case USB_CLASS_CSCID:
        return "Chip+ Smart Card"; /* chip+ smart card */
    case USB_CLASS_CONTENT_SEC:
        return "Content Security"; /* content security */
    case USB_CLASS_VIDEO:
        return "Video";
    case USB_CLASS_WIRELESS_CONTROLLER:
        return "Wireless Controller";
    case USB_CLASS_PERSONAL_HEALTHCARE:
        return "Personal Healhcare";
    case USB_CLASS_AUDIO_VIDEO:
        return "Audio/Video";
    case USB_CLASS_BILLBOARD:
        return "Billboard";
    case USB_CLASS_USB_TYPE_C_BRIDGE:
        return "USB Type C Bridge";
    case USB_CLASS_MISC:
        return "Miscellaneous";
    case USB_CLASS_APP_SPEC:
        return "Application Specific";
    case USB_CLASS_VENDOR_SPEC:
        return "Vendor Specific";
    default:
        return "Vendor Specific";
    }
}
