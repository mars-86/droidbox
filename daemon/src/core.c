#include "core.h"
#include "../../lib/ptp/ptp.h"
#include "product.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_MAX_SIZE 4096
#define STRING_MAX_SIZE 128
#define LANG_ID 0x0409

struct __device_info {
    unsigned short product_id;
    unsigned short vendor_id;
    char manufacturer[STRING_MAX_SIZE];
    char product[STRING_MAX_SIZE];
    char serial_number[STRING_MAX_SIZE];
};

struct __ptp_iface {
    unsigned short number;
    unsigned short endp_in;
    unsigned short endp_in_pack_size;
    unsigned short endp_out;
    unsigned short endp_out_pack_size;
};

static void __parse_stream(void* dest, unsigned char* src, size_t n)
{
    memcpy(dest, src, n);
}

static void __usb_interface(int fd, struct usb_interface_descriptor* uid)
{
    unsigned char buff[BUFF_MAX_SIZE] = { 0 };
    int rlen, i;

    if (usb_get_descriptor(fd, USB_DT_INTERFACE, 0x00, 0x00, buff, &rlen) != 0) {
        perror("ioctl");
        return;
    }
}

static unsigned short __usb_device(int fd, struct usb_device_descriptor* udd)
{
    unsigned char buff[BUFF_MAX_SIZE] = { 0 };
    int rlen, i;

    if (usb_get_descriptor(fd, USB_DT_DEVICE, 0x00, 0x00, buff, &rlen) != 0) {
        perror("ioctl");
        return 0;
    }

    struct usb_device_descriptor __udd;

    __parse_stream(&__udd, buff, sizeof(struct usb_device_descriptor));

    if (udd)
        memcpy(udd, &__udd, sizeof(struct usb_device_descriptor));

    int __n_config = __udd.bNumConfigurations;

    ptp_dev_t __ptp = { .fd = fd };
    struct usb_config_descriptor __ucd;
    struct usb_interface_descriptor __uid;
    struct usb_endpoint_descriptor __ued;
    unsigned char __ucd_size = USB_DT_CONFIG_SIZE, __uid_size = USB_DT_INTERFACE_SIZE, __ued_size = USB_DT_ENDPOINT_SIZE;
    int __is_ptp = 0, __is_parsed = 0;
    for (i = 0; i < __n_config || (__is_ptp && __is_parsed); ++i) {
        memset(buff, 0, BUFF_MAX_SIZE);
        if (usb_get_descriptor(fd, USB_DT_CONFIG, i, 0x00, buff, &rlen) != 0) {
            perror("ioctl");
            return 0;
        }

        __parse_stream(&__ucd, buff, __ucd_size);
        unsigned short __data_size = __ucd.wTotalLength, offset;
        for (offset = USB_DT_CONFIG_SIZE; offset < __data_size; offset += *(buff + offset)) {
            unsigned char dt = *(buff + offset + 1);
            switch (dt) {
            case USB_DT_INTERFACE:
                __parse_stream(&__uid, buff + offset, __uid_size);
                if (__is_ptp)
                    __is_parsed = 1;
                if (__uid.bInterfaceClass == 0x06)
                    __is_ptp = 1;
                break;
            case USB_DT_ENDPOINT:
                __parse_stream(&__ued, buff + offset, __ued_size);
                if (__is_ptp) {
                    if (__ued.bEndpointAddress & USB_DIR_IN && __ued.bmAttributes != 0x03) {
                        __ptp.endp_in = __ued.bEndpointAddress;
                        __ptp.endp_in_max_pack_size = __ued.wMaxPacketSize;
                    } else if (__ued.bEndpointAddress & USB_DIR_OUT && __ued.bmAttributes != 0x03) {
                        __ptp.endp_out = __ued.bEndpointAddress;
                        __ptp.endp_out_max_pack_size = __ued.wMaxPacketSize;
                    } else {
                        __ptp.endp_int = __ued.bEndpointAddress;
                        __ptp.endp_int_max_pack_size = __ued.wMaxPacketSize;
                    }
                }
                break;
            case USB_DT_INTERFACE_ASSOCIATION:
                // __parse_interface_association(output, buff + offset);
                break;
            }
        }
    }

    return __udd.idProduct;
}

static void __get_device_info(int fd, struct usb_device_descriptor* udd, struct __device_info* dinfo)
{
    dinfo->vendor_id = udd->idVendor;
    usb_get_string(fd, udd->iManufacturer, LANG_ID, dinfo->manufacturer);
    usb_get_string(fd, udd->iProduct, LANG_ID, dinfo->product);
    usb_get_string(fd, udd->iSerialNumber, LANG_ID, dinfo->serial_number);
}

int scan_ports(const char* path)
{
    struct dirent** filenames;
    int nfiles;

    if ((nfiles = scandir(path, &filenames, NULL, NULL)) < 0)
        return -1;

    const char* __fname = NULL;
    unsigned char __ftype = 0;
    printf("\r%s ->\n\t", path);
    while (nfiles--) {
        __fname = filenames[nfiles]->d_name;
        __ftype = filenames[nfiles]->d_type;
        if (__ftype == DT_DIR) {
            if (strcmp(__fname, "..") && strcmp(__fname, ".")) {
                char __folder[257];
                sprintf(__folder, "%s/%s", path, __fname);
                scan_ports(__folder);
            } else {
                printf("%s\n\t", __fname);
            }
        } else {
            char __file[257] = { 0 };
            sprintf(__file, "%s/%s", path, __fname);
            printf("FULL PATH: %s\n\t", __file);
            int fd = open(__file, O_RDWR);

            if (fd == -1) {
                if (errno != EACCES)
                    perror("open");
            } else {
                struct usb_device_descriptor udd;
                unsigned short __pid = __usb_device(fd, &udd);
                puts("HERE");
                if (is_android_device(__pid)) {
                    struct __device_info dev_info = { 0 };
                    usb_dump_device(fd, stdout);
                    __get_device_info(fd, &udd, &dev_info);
                    printf("Vendor ID: %.4X\n", dev_info.vendor_id);
                    printf("Manufacturer: %s\n", dev_info.manufacturer);
                    printf("Product: %s\n", dev_info.product);
                    printf("Serial Number: %s\n", dev_info.serial_number);
                    // add it to memory
                }
                close(fd);
            }
        }

        free(filenames[nfiles]);
    }
    free(filenames);

    putc('\r', stdout);

    return 0;
}
