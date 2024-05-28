#include "daemon.h"
#include "../../lib/adb/adb.h"
#include "../../lib/usb/usb.h"
#include <fcntl.h>
#include <stdio.h>

#define PATH "/tmp/droidbox.sock"
#define DEV_USB_DIR "/dev/bus/usb"
#define DEVICE "096"

extern const unsigned char rsakey_ex[];

int __loop()
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

    usb_dump_device(fd, stdout);

    unsigned short iface = 0x03;
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
    if (usb_detach_interface(fd, iface) != 0)
        perror("ioctl");

    if (usb_claim_interface(fd, iface) != 0)
        perror("ioctl");

    adb_dev_t adbdev = {
        .fd = fd,
        .in_endp = 0x05,
        .out_endp = 0x03
    };

    adb_res_t adbres = { 0 };
    unsigned char adb_res_buff[2048];
    int ret;

    char token[20];

    ret = adb_connect(&adbdev, ADB_PROTO_VERSION, 0x1000, "host::", adb_res_buff, 2048, &adbres);

    memcpy(token, adb_res_buff + sizeof(struct message), 20);

    printf("%d\n", ret);

    struct message* r = (struct message*)adb_res_buff;

    printf("CMD: %X\n", r->command);
    printf("ARG0: %X\n", r->arg0);
    printf("ARG1: %X\n", r->arg1);
    printf("DLEN: %X\n", r->data_length);
    printf("DCRC: %X\n", r->data_crc32);
    printf("MAGI: %X\n", r->magic);

    if (ret == 0) {
        for (int i = 0; i < 20; ++i)
            printf("%.2X", token[i]);
        printf("\n");
        for (int i = 0; i < 20; ++i)
            printf("%c", token[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    if (ret < 0)
        perror("ioctl");

    printf("%ld\n", strlen(token));

    ret = adb_auth(&adbdev, ADB_AUTH_TYPE_RSAPUBLICKEY, (const char*)rsakey_ex, 524, adb_res_buff, 2048, &adbres);

    if (ret == 0) {
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    if (ret == 0) {
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    ret = adb_open(&adbdev, 4, "framebuffer: ", adb_res_buff, 2048, &adbres);

    if (ret == 0) {
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    if (adbres.code == ADB_COMMAND_A_OKAY) {
        do {
            ret = adb_ready(&adbdev, 4, 3, adb_res_buff, 2048, &adbres);
            if (ret == 0) {
                for (int i = sizeof(struct message); i < adbres.length; ++i)
                    printf("%.2X", adb_res_buff[i]);
                printf("\n");
                for (int i = sizeof(struct message); i < adbres.length; ++i)
                    printf("%c", adb_res_buff[i]);
                printf("\n");
            }
        } while (adbres.code != ADB_COMMAND_A_CLSE);
    }

    if (usb_release_interface(fd, iface) != 0)
        perror("ioctl");

    close(fd);

    return 0;
}

void on_listen(void* usr)
{
    const char* path = (const char*)usr;

    printf("Daemon listening on %s\n", path);
}

int event_listener(int s, int e, struct sockaddr* addr)
{
    int sd;
    socket_t sock;

    sock.descriptor = s;
    if ((sd = connection_accept(&sock, addr)) < 0)
        return -1;

    printf("Connection accepted\n");

    return sd;
}

int pong(int s, int e, struct sockaddr* addr)
{
    socket_t sock = { .descriptor = s };
    char request[2048] = { 0 }, response[2048] = { 0 }, buffer[1024] = { 0 };
    int rbytes = connection_recv(&sock, request, 2048);
    const char* headers[] = {
        SET_HEADER("Content-Type", "text/plain"),
        NULL
    };

    // header_value2(buffer, request, "Content-Type");

    printf("%s\n", buffer);

    printf("on sock %d RECV %d\n", s, rbytes);
    printf("%s\n", request);
    if (rbytes > 0) {
        http_response(response, OK, headers, "world!\n");
        int sbytes = connection_send(&sock, response, strlen(response));

        printf("on sock %d SENT %d\n", s, sbytes);
        printf("%s\n", response);
        if (sbytes <= 0)
            printf("Error on send\n");
    }
    // connection_recv(s, buffer);
    // printf("ev %d recv in socket: %d\n", e, s);
    return rbytes;
}

int daemon_init(int argc, char* argv[])
{
    server_config_t sconf;
    sconf.backlog = 8;
    sconf.poll.nfds = 8;
    sconf.poll.fd0_events = POLLIN;
    sconf.poll.fd0_ev_handler = event_listener;
    sconf.poll.events = POLLIN;
    sconf.poll.ev_handler = pong;
    sconf.poll.timeout = 100;

    local_server_t* __local = server_local_init(&sconf);

    if (!__local)
        return 1;

    server_local_listen(__local, PATH, on_listen);

    server_local_destroy(__local);

    return 0;
}
