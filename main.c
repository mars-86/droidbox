#include "lib/adb/adb.h"
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
#include <openssl/bio.h>
#include <openssl/cms.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEV_USB_DIR "/dev/bus/usb"
#define DEVICE "094"

extern const unsigned char rsakey_ex[];

static EVP_PKEY* load_private_key(const char* file)
{
    BIO* keybio;
    if ((keybio = BIO_new_file(file, "r")) == NULL) {
        ERR_print_errors_fp(stderr);
        exit(0);
    }
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(keybio, NULL, NULL, NULL);
    if (pkey == NULL) {
        ERR_print_errors_fp(stderr);
        exit(0);
    }
    return pkey;
}

static int sign_msg(const char* prk_path, unsigned char* msg, int len, unsigned char* sign, size_t* signlen)
{
    int ret = 0;
    EVP_PKEY_CTX* ctx = NULL;
    /* md is a SHA-256 digest in this example. */
    unsigned char *md = msg, *sig = NULL;
    size_t mdlen = 32, siglen;
    EVP_PKEY* signing_key = NULL;

    /*
     * NB: assumes signing_key and md are set up before the next
     * step. signing_key must be an RSA private key and md must
     * point to the SHA-256 digest to be signed.
     */

    signing_key = load_private_key(prk_path);

    if (!signing_key) {
        ERR_print_errors_fp(stderr);
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE CONTEXT\n");

    ctx = EVP_PKEY_CTX_new(signing_key, NULL /* no engine */);
    // ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL /* no engine */);
    if (!ctx) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE INIT SIGN\n");

    if (EVP_PKEY_sign_init(ctx) <= 0) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE RSA PADDING\n");

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE SIGN MD\n");

    if (EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) <= 0) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE SIGN\n");

    /* Determine buffer length */
    if (EVP_PKEY_sign(ctx, NULL, &siglen, md, mdlen) <= 0) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE MALLOC SIGN: %ld\n", siglen);

    sig = OPENSSL_malloc(siglen);

    if (!sig) {
        ret = ERR_get_error();
        goto done;
    }

    printf("CREATE SIGN\n");

    if (EVP_PKEY_sign(ctx, sig, &siglen, md, mdlen) <= 0) {
        ret = ERR_get_error();
        goto done;
    }

    if (sign)
        memcpy(sign, sig, siglen);
    if (signlen)
        *signlen = siglen;

done:
    if (ctx)
        EVP_PKEY_CTX_free(ctx);
    if (sig)
        OPENSSL_free(sig);
    if (signing_key)
        EVP_PKEY_free(signing_key);

    return ret;
}

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

    FILE* fpub = fopen("~/adbkey.pub", "r");
    char pubk[2048];

    while (!feof(fpub))
        fread(pubk, 2048, 1, fpub);

    fclose(fpub);

    adb_res_t adbres = { 0 };
    unsigned char adb_res_buff[2048];
    int ret, i = 0;
    unsigned char signature[2048];
    size_t signature_len = 0;

    char token[20];

    uint8_t n64[8];

    printf("%ld\n", sizeof(uint64_t));
    uint32_t num[2] = { (1 << 31), (1 << 31) };
    uint32_t num32 = (1 << 31);
    uint64_t num64 = 0xffffffffffffffff;

    memcpy(n64, &num64, 8);

    printf("NUM64: %lu\n", num64);
    long n;
    printf("NUM: ");
    for (int i = 0; i < 8; ++i)
        printf("%d", n64[i]);
    printf("%s\n", n64);
    printf("\n");

    printf("%u\n", (uint32_t)(2 << 30));
    printf("%X\n", (uint32_t)(2 << 30));

    printf("N0INV: %d\n", ((uint32_t)(0x77c12ca7 % ((unsigned long)2 << 32))));
    printf("N0INV: %X\n", ((uint32_t)(0x77c12ca7 % ((unsigned long)2 << 32))));

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

    // printf("RSA KEY LEN: %ld\n", strlen(rsakey));
    // ret = adb_auth(&adbdev, 3, rsakey, adb_res_buff, 2048, &adbres);
    ++i;

    ret = adb_auth(&adbdev, ADB_AUTH_TYPE_RSAPUBLICKEY, (const char*)rsakey_ex, 524, adb_res_buff, 2048, &adbres);

    if (ret == 0) {
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    FILE* stream = fopen("~/adbstream", "wb+");

    if (!stream)
        perror("fopen");

    int tty = open("/dev/tty1", O_RDWR);

    if (tty == -1) {
        perror("open");
    }

    ret = adb_open(&adbdev, tty, "jdwp:1234", adb_res_buff, 2048, &adbres);
    // ret = adb_sync(&adbdev, 0xF, "shell", adb_res_buff, 2048, &adbres);

    // ret = adb_stls(&adbdev, 1, ADB_STLS_VERSION, adb_res_buff, 2048, &adbres);

    // ret = adb_write(&adbdev, 1, 1, "sync:", adb_res_buff, 2048, &adbres);

    if (ret == 0) {
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%.2X", adb_res_buff[i]);
        printf("\n");
        for (int i = sizeof(struct message); i < adbres.length; ++i)
            printf("%c", adb_res_buff[i]);
        printf("\n");
    }

    if (usb_release_interface(fd, iface) != 0)
        perror("ioctl");

    fclose(stream);
    close(tty);
    close(fd);

    return 0;
}
