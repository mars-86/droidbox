#include "core.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_MAX_SIZE 4096
#define STRING_MAX_SIZE 128

static void __parse_stream(void* dest, unsigned char* src, size_t n)
{
    memcpy(dest, src, n);
}

static void __print_device_descriptor(FILE* output, int fd, unsigned short langid, struct usb_device_descriptor* udd)
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

static void __usb_dump_device(int fd, FILE* output)
{
    unsigned char buff[BUFF_MAX_SIZE] = { 0 };
    int rlen;

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
                __usb_dump_device(fd, stdout);
                close(fd);
            }
        }

        free(filenames[nfiles]);
    }
    free(filenames);

    putc('\r', stdout);

    return 0;
}
