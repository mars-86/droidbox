#include "../adb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_LEN 0x18
#define ADB_REQUEST_LEN(payload_len) (HEADER_LEN + payload_len)

#define RESPONSE_BUFFER_LEN 2048
#define ADB_MAGIC(cmd) (cmd ^ 0xFFFFFFFF)

#define ADB_COMMAND_MASK(stream) ((uint32_t)((stream[3] << 24) | (stream[2] << 16) | (stream[1] << 8) | (stream[0])))
#define ADB_DATA_LEN_MASK(stream) ((uint32_t)((stream[15] << 24) | (stream[14] << 16) | (stream[13] << 8) | (stream[12])))

// upayload ? crc16((uint8_t*)upayload, udata_len) : 0;

#define ADB_SET_MESSAGE(umsg, ucmd, uarg0, uarg1, udata_len, upayload)                   \
    do {                                                                                 \
        umsg.header.command = ucmd;                                                      \
        umsg.header.arg0 = uarg0;                                                        \
        umsg.header.arg1 = uarg1;                                                        \
        umsg.header.data_length = upayload ? udata_len : 0;                              \
        umsg.header.data_crc32 = upayload ? checksum((uint8_t*)upayload, udata_len) : 0; \
        umsg.header.magic = ADB_MAGIC(ucmd);                                             \
        umsg.payload = (uint8_t*)upayload;                                               \
    } while (0)

#define CRC_CHECK_BITS(val, polynomial)                         \
    do {                                                        \
        int i;                                                  \
        for (i = 0; i < 8; i++)                                 \
            val = val & 1 ? (val >> 1) ^ polynomial : val >> 1; \
    } while (0);

uint32_t checksum(uint8_t* payload, uint32_t len)
{
    uint32_t sum = 0, i;
    for (i = 0; i < len; ++i)
        sum += payload[i];

    return sum;
}

uint16_t crc16(uint8_t* buf, uint32_t len)
{
    uint16_t val = 0, crc = 0x0000;
    while (len--) {
        val = (crc ^ *buf++) & 0xFF;
        CRC_CHECK_BITS(val, 0x93FC);
        crc = val ^ crc >> 8;
    }
    return (uint32_t)(crc ^ 0xFFFF);
}

uint32_t crc32(uint8_t* buf, uint32_t len)
{
    uint32_t val, crc = 0x00000000;
    while (len--) {
        val = (crc ^ *buf++) & 0xFF;
        CRC_CHECK_BITS(val, 0x93FC);
        crc = val ^ crc >> 8;
    }
    return crc ^ 0xFFFFFFFF;
}

uint32_t crc16_2(uint8_t* buf, uint32_t len)
{
    uint16_t poly = 0x0000;
    uint16_t val, crc;
    while (poly < 0xFFFF) {
        uint8_t* __buf = buf;
        uint32_t __len = len;
        val = 0, crc = 0x0000;
        while (__len--) {
            val = (crc ^ *__buf++) & 0xFF;
            CRC_CHECK_BITS(val, poly);
            crc = val ^ crc >> 8;
        }
        if ((crc ^ 0xFFFF) == (uint16_t)0x232) {
            printf("%X\n", crc);
            printf("%X\n", crc ^ 0xFFFF);
            printf("%X\n", poly);
        }
        poly++;
    }

    return (uint32_t)(crc ^ 0xFFFF);
}

uint32_t crc32_2(uint8_t* buf, uint32_t len)
{
    uint32_t poly = 0x00000000;
    uint32_t val, crc;
    while (poly < 0xFFFFFFFF) {
        uint8_t* __buf = buf;
        uint32_t __len = len;
        val = 0, crc = 0x00000000;
        while (__len--) {
            val = (crc ^ *__buf++) & 0xFF;
            CRC_CHECK_BITS(val, poly);
            crc = val ^ crc >> 8;
        }
        if ((crc ^ 0xFFFFFFFF) == (uint32_t)0x232) {
            printf("%X\n", poly);
            break;
        }
        poly++;
    }

    return crc ^ 0xFFFFFFFF;
}

struct adb_message {
    struct message header;
    uint8_t* payload;
} __attribute__((packed));

static int __send_request(adb_dev_t* dev, const uint8_t* req, uint32_t len, uint8_t* data, adb_res_t* res)
{
    int __offset = 0, __sent_bytes = 0, __incomming_data = 0;
#ifdef __DEBUG
    printf("SENT BYTES\n");
#endif

    printf("SENDING HEADER\n");
    for (int i = 0; i < HEADER_LEN; ++i)
        printf("%.2X", req[i]);
    printf("\n");

    if ((__sent_bytes = usb_bulk_send(dev->fd, dev->out_endp, (void*)req, HEADER_LEN)) < 0)
        return -1;
    printf("SENT BYTES: %d\n", __sent_bytes);

    if (len > HEADER_LEN) {
        printf("SENDING PAYLOAD\n");
        for (int i = HEADER_LEN; i < len; ++i)
            printf("%.2X", req[i]);
        printf("\n");
        for (int i = HEADER_LEN; i < len; ++i)
            printf("%c", req[i]);
        printf("\n");
        if ((__sent_bytes = usb_bulk_send(dev->fd, dev->out_endp, (void*)(req + HEADER_LEN), len - HEADER_LEN)) < 0)
            return -1;
        // TODO: remove this
        printf("SENT BYTES: %d\n", __sent_bytes);
        // if (__sent_bytes == 451) {
        //    return 0;
        // }
    }

    uint8_t __rbuffer[512];
    adb_res_t __res = { 0 };
    while (1) {
        int __recv_bytes = usb_bulk_recv(dev->fd, dev->in_endp, __rbuffer, sizeof(__rbuffer));
        if (__recv_bytes < 0)
            return -1;
#ifdef __DEBUG
        printf("RECV BYTES: %d\n", __recv_bytes);
#endif
        printf("RECV BYTES: %d\n", __recv_bytes);
        if (__incomming_data && __recv_bytes != -1) {
            memcpy(data + __offset, __rbuffer, __recv_bytes);
            __res.length += __recv_bytes;
            for (int i = 0; i < __recv_bytes; ++i)
                printf("%.2X", __rbuffer[i]);
            printf("\n");
            for (int i = 0; i < __recv_bytes; ++i)
                printf("%c", __rbuffer[i]);
            printf("\n");
            break;
        }

        if (__recv_bytes != -1) {
            memcpy(data + __offset, __rbuffer, __recv_bytes);
            __res.length += __recv_bytes;
            __res.code = ADB_COMMAND_MASK(__rbuffer);
            __offset += __recv_bytes;
            for (int i = 0; i < __recv_bytes; ++i)
                printf("%.2X", __rbuffer[i]);
            printf("\n");
            for (int i = 0; i < __recv_bytes; ++i)
                printf("%c", __rbuffer[i]);
            printf("\n");

            if (ADB_DATA_LEN_MASK(__rbuffer))
                __incomming_data = 1;
            else
                break;
        }
    }
    memcpy(res, &__res, sizeof(adb_res_t));

    return 0;
}

static int __handle_request(adb_dev_t* dev, void* msg, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message* __msg = (struct adb_message*)msg;
    int __bytes_len = ADB_REQUEST_LEN(__msg->header.data_length);
    uint8_t __rstream[RESPONSE_BUFFER_LEN], *__mstream = (uint8_t*)malloc(__bytes_len * sizeof(uint8_t));

    if (!__mstream)
        return -1;

    memcpy(__mstream, __msg, HEADER_LEN);
    if (__msg->header.data_length) {
        // uint8_t* __p = __msg->payload + len;
        // uint32_t __len = __msg->header.data_length;
        // for (int i = 0; i < __len; i++)
        //    *(__mstream + HEADER_LEN + i) = *(__msg->payload + (__len - 1) - i);
        memcpy(__mstream + HEADER_LEN, __msg->payload, __msg->header.data_length);
    }

    int __ret = __send_request(dev, __mstream, __bytes_len, __rstream, res);

    if (data && __ret == 0) {
        /* check if provided buffer is large enougth to copy the response */
        uint32_t __bytes_to_copy = res->length > len ? len : res->length;
        memcpy(data, __rstream, __bytes_to_copy);
    }

    free(__mstream);

#ifdef __DEBUG
    printf("\n%.2X\n%.2X\n", __res.length, __res.code);
#endif

    return __ret;
}

int adb_connect(adb_dev_t* dev, uint32_t version, uint32_t maxdata, const char* system_id, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_CNXN, version, maxdata, strlen(system_id), system_id);

#ifdef __DEBUG
    printf("ADB CONNECT\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_stls(adb_dev_t* dev, uint32_t type, uint32_t version, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_STLS, type, version, 0, "");

#ifdef __DEBUG
    printf("ADB STLS\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_auth(adb_dev_t* dev, uint32_t type, const char* auth_data, uint32_t auth_len, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_AUTH, type, 0, auth_len, auth_data);
    // ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_AUTH, type, 0, 256, auth_data);

#ifdef __DEBUG
    printf("ADB AUTH\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_open(adb_dev_t* dev, uint32_t local_id, const char* destination, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };
    uint32_t __data_len = strlen(destination);

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_OPEN, local_id, 0, __data_len, destination);

#ifdef __DEBUG
    printf("ADB OPEN\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_ready(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_OKAY, local_id, remote_id, 0, "");

#ifdef __DEBUG
    printf("ADB READY\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_write(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, const char* payload, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_WRTE, local_id, remote_id, strlen(payload), payload);

#ifdef __DEBUG
    printf("ADB WRITE\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}

int adb_close(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, uint8_t* data, uint32_t len, adb_res_t* res)
{
    struct adb_message __msg = { 0 };

    ADB_SET_MESSAGE(__msg, ADB_COMMAND_A_CLSE, local_id, remote_id, 0, NULL);

#ifdef __DEBUG
    printf("ADB WRITE\n");
#endif

    return __handle_request(dev, &__msg, data, len, res);
}
