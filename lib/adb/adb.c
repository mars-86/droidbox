#include "adb.h"
#include <stdint.h>
#include <string.h>

#define HEADER_LEN 0x18
#define ADB_REQUEST_LEN(payload_len) (HEADER_LEN + payload_len)

#define RESPONSE_BUFFER_LEN 2048
#define ADB_MAGIC(cmd) (cmd ^ 0xFFFFFFFF)

#define ADB_COMMAND_MASK(stream) ((uint32_t)((stream[3] << 24) | (stream[2] << 16) | (stream[1] << 8) | (stream[0])))
#define ADB_DATA_LEN_MASK(stream) ((uint32_t)((stream[15] << 24) | (stream[14] << 16) | (stream[13] << 8) | (stream[12])))

#define ADB_SET_MESSAGE(umsg, ucmd, uarg0, uarg1, udata_len, upayload)            \
    umsg.header.command = ucmd;                                                   \
    umsg.header.arg0 = uarg0;                                                     \
    umsg.header.arg1 = uarg1;                                                     \
    umsg.header.data_length = upayload ? udata_len : 0;                           \
    umsg.header.data_crc32 = upayload ? crc32((uint8_t*)upayload, udata_len) : 0; \
    umsg.header.magic = ADB_MAGIC(ucmd);
// umsg.payload = (char*)upayload;

#define CRC_CHECK_BITS(val)                                     \
    do {                                                        \
        int i;                                                  \
        for (i = 0; i < 8; i++)                                 \
            val = val & 1 ? (val >> 1) ^ 0xEDB88320 : val >> 1; \
    } while (0);

uint32_t crc32(uint8_t* buf, uint32_t len)
{
    uint32_t val, crc = 0xFFFFFFFF;
    while (len--) {
        val = (crc ^ *buf++) & 0xFF;
        CRC_CHECK_BITS(val);
        crc = val ^ crc >> 8;
    }
    return crc ^ 0xFFFFFFFF;
}

struct adb_message {
    struct message header;
    // char* payload;
} __attribute__((packed));

static int __send_request(adb_dev_t* dev, const uint8_t* req, uint32_t len, uint8_t* data, adb_res_t* res)
{
    int __offset = 0, __sent_bytes = 0, __incomming_data = 0;
#ifdef __DEBUG
    printf("SENT BYTES\n");
#endif
    if ((__sent_bytes = usb_bulk_send(dev->fd, dev->out_endp, (void*)req, len)) < 0)
        return -1;

    uint8_t __rbuffer[512];
    adb_res_t __res = { 0 };
    while (1) {
        int __recv_bytes = usb_bulk_recv(dev->fd, dev->in_endp, __rbuffer, sizeof(__rbuffer));
#ifdef __DEBUG
        printf("RECV BYTES: %d\n", recv_bytes);
#endif
        if (__incomming_data && __recv_bytes != -1) {
            memcpy(data + __offset, __rbuffer, __recv_bytes);
            __res.length += __recv_bytes;
            break;
        }

        if (__recv_bytes != -1) {
            memcpy(data + __offset, __rbuffer, __recv_bytes);
            __res.length += __recv_bytes;
            __res.code = ADB_COMMAND_MASK(__rbuffer);
            __offset += __recv_bytes;
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
    struct message* __msg = (struct message*)msg;
    int __bytes_len = ADB_REQUEST_LEN(__msg->data_length);
    uint8_t __rstream[RESPONSE_BUFFER_LEN];

    int __ret = __send_request(dev, (const uint8_t*)msg, __bytes_len, __rstream, res);

    if (data && __ret == 0) {
        /* check if provided buffer is large enougth to copy the response */
        uint32_t __bytes_to_copy = res->length > len ? len : res->length;
        memcpy(data, __rstream, __bytes_to_copy);
    }

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
