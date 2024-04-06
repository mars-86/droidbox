#include "ptp/error.h"
#include "ptp/object.h"
#include "ptp/ptp.h"
#include "usb.h"
#include <asm-generic/errno-base.h>
#include <asm/byteorder.h>
#include <bits/types/clockid_t.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEV_USB_DIR "/dev/bus/usb"
#define DEVICE "025"

int main(int argc, char* argv[])
{
    FILE* _fd;
    if (!(_fd = fopen(DEV_USB_DIR "/003/" DEVICE, "rb"))) {
        perror("fopen");
        return 1;
    }

    int c;
    while ((c = getc(_fd)) != EOF) {
        printf("%.2X ", c);
    }
    putc('\n', stdout);

    fclose(_fd);

    int fd = open(DEV_USB_DIR "/003/" DEVICE, O_RDWR);

    if (fd == -1) {
        perror("open");
        return 1;
    }

    // usb_dump_device(fd, stdout);

    unsigned short iface = 0x00;
    char dname[256];

    if (usb_get_driver(fd, iface, dname, 256) != 0)
        perror("ioctl get driver");
    else {
        printf("%s\n", dname);

        if (usb_detach_interface(fd, iface) != 0)
            perror("ioctl");
        /*
        if (usb_detach_interface(fd, 1) != 0)
            perror("ioctl");
        if (usb_detach_interface(fd, 2) != 0)
            perror("ioctl");
        if (usb_detach_interface(fd, 3) != 0)
            perror("ioctl");
        */
    }

    if (usb_claim_interface(fd, iface) != 0)
        perror("ioctl");

    struct timeval tv;

    gettimeofday(&tv, 0);
    tv.tv_sec = tv.tv_sec + 3;

    srand(time(NULL));

    ptp_dev_t ptpdev = {
        .fd = fd,
        .endp = 0x01
    };

    unsigned char response_buffer[2048];
    ptp_res_params_t rparams = { 0 };

    ptp_res_t devres = ptp_open_session(&ptpdev, 0x1234);
    printf("%s\n", ptp_get_error(devres.code));

    devres = ptp_get_device_info(&ptpdev, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);

    devres = ptp_get_storage_id(&ptpdev, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);
    printf("\n");

    devres = ptp_get_storage_info(&ptpdev, 0x00010001, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);
    printf("\n");

    devres = ptp_get_num_objects(&ptpdev, 0x00010001, PTP_OBJECT_FORMAT_CODE_UNUSED, PTP_OBJECT_ASSOCIATION_ROOT, &rparams);
    printf("%s\n", ptp_get_error(devres.code));

    printf("%d\n", rparams.Parameter1);
    printf("%d\n", rparams.Parameter2);
    printf("%d\n", rparams.Parameter3);

    memset(response_buffer, 0, 2048);
    devres = ptp_get_object_handles(&ptpdev, 0x00010001, PTP_OBJECT_FORMAT_CODE_HANDLES_UNUSED, PTP_OBJECT_ASSOCIATION_HANDLES_ROOT, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);
    printf("\n");

    devres = ptp_get_object_info(&ptpdev, 0x0000000D, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%c", response_buffer[i]);
    printf("\n");

    devres = ptp_get_object(&ptpdev, 0x00000019, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);
    printf("\n");

    devres = ptp_get_thumb(&ptpdev, 0x00000001, response_buffer, 2048);
    printf("%s\n", ptp_get_error(devres.code));

    for (int i = 0; i < devres.length; ++i)
        printf("%.2X", response_buffer[i]);
    printf("\n");

    devres = ptp_send_object_info(&ptpdev, 0x00010001, PTP_OBJECT_ASSOCIATION_HANDLES_ROOT, NULL, 66, &rparams);
    printf("%s\n", ptp_get_error(devres.code));

    printf("%d\n", rparams.Parameter1);
    printf("%d\n", rparams.Parameter2);
    printf("%d\n", rparams.Parameter3);

    devres = ptp_close_session(&ptpdev);
    printf("%s\n", ptp_get_error(devres.code));

    if (usb_release_interface(fd, iface) != 0)
        perror("ioctl");

    close(fd);

    return 0;
}
