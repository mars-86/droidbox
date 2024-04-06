#include "ptp.h"
#include "object.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define __RESPONSE_TYPE_MASK(res) ((res[5] << 8) | (res[4]))
#define __RESPONSE_BUFFER_LEN 2048
#define __HEADER_LEN 0x0C
#define __PARAM_LEN 0x04

#define PTP_REQUEST_LEN(nparams) (__HEADER_LEN + (nparams * __PARAM_LEN))
#define PTP_DATA_REQUEST_LEN(payload_len) (__HEADER_LEN + payload_len)

/* length: bytes 0, 1, 2, 3 */
#define RESPONSE_LENGTH_MASK(stream) ((stream[3] << 24) | (stream[2] << 16) | (stream[1] << 8) | (stream[0]))

/* code: bytes 6, 7 */
#define RESPONSE_CODE_MASK(stream) ((stream[7] << 8) | (stream[6]))

struct ptp_header {
    uint32_t ContaierLength;
    uint16_t ContainerType;
    uint16_t Code;
    uint32_t TransacionID;
} __attribute__((packed));

struct ptp_cmd_payload {
    uint32_t Parameter1;
    uint32_t Parameter2;
    uint32_t Parameter3;
    uint32_t Parameter4;
    uint32_t Parameter5;
} __attribute__((packed));

typedef struct ptp_cmd_payload ptp_cmd_payload_t;
typedef uint8_t* ptp_data_payload_t;

struct ptp_container {
    struct ptp_header header;
    union {
        ptp_cmd_payload_t cmd_payload;
        ptp_data_payload_t data_payload;
    };
} __attribute__((packed));

static ptp_res_t __send_request(int fd, int endpoint, const uint8_t* req, uint32_t len, uint8_t* res, ptp_res_params_t* rparams)
{
    printf("BEFORE SEND\n");
    int sent_bytes = usb_bulk_send(fd, endpoint, (void*)req, len);
    printf("AFTER SEND\n");

    int res_block = 0, offset = 0;
    uint8_t __rbuffer[512];
    ptp_res_t __res = { 0 };
    while (!res_block) {
        int recv_bytes = usb_bulk_recv(fd, endpoint, __rbuffer, sizeof(__rbuffer));
        int data_len = RESPONSE_LENGTH_MASK(__rbuffer);

        switch (__RESPONSE_TYPE_MASK(__rbuffer)) {
        case PTP_CONTAINER_TYPE_DATA_BLOCK:
            while (1) {
                memcpy(res + offset, __rbuffer, recv_bytes);

                data_len -= recv_bytes;
                offset += recv_bytes;
                if (!data_len) {
                    __res.length = RESPONSE_LENGTH_MASK(res);
#ifdef __DEBUG
                    printf("\nEND\n");
#endif
                    break;
                }
                recv_bytes = usb_bulk_recv(fd, endpoint, __rbuffer, sizeof(__rbuffer));
            }
            break;
        case PTP_CONTAINER_TYPE_RESPONSE_BLOCK:
#ifdef __DEBUG
            printf("RESPONSE BLOCK RECEIVED\n");
#endif
            __res.code = RESPONSE_CODE_MASK(__rbuffer);
            if (rparams)
                memcpy(rparams, __rbuffer + __HEADER_LEN, RESPONSE_LENGTH_MASK(__rbuffer) - __HEADER_LEN);
            res_block = 1;
            break;
        default:
            printf("DEF\n");
        }
    }

    return __res;
}

static ptp_res_t __handle_request(usb_dev_t fd, int endpoint, void* container, uint8_t* data, uint32_t len, ptp_res_params_t* rparams)
{
    struct ptp_header* header = (struct ptp_header*)container;
    int __bytes_len = header->ContaierLength;
    unsigned char __rstream[__RESPONSE_BUFFER_LEN];
    unsigned char* __ptpstream = container;

    ptp_res_t __res = __send_request(fd, endpoint, __ptpstream, __bytes_len, __rstream, rparams);

    if (data) {
        /* check if provided buffer is large enougth to copy the response */
        uint32_t __bytes_to_copy = __res.length > len ? len : __res.length;
        memcpy(data, __rstream, __bytes_to_copy);
    }

#ifdef __DEBUG
    printf("\n%.2X\n%.2X\n", __res.length, __res.code);
#endif

    return __res;
}

static inline int __attribute__((always_inline)) __get_transaction_id(void)
{
    return 1 + (rand() % (RAND_MAX - 1));
}

ptp_res_t ptp_get_device_info(ptp_dev_t* dev, uint8_t* data, uint32_t len)
{
    int nparams = 0;
    struct ptp_header __ptpreq = { 0 };

    __ptpreq.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.Code = PTP_REQUEST_GET_DEVICE_INFO;
    __ptpreq.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("GET DEVICE INFO\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_open_session(ptp_dev_t* dev, uint32_t session_id)
{
    int nparams = 1;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_OPEN_SESSION;
    __ptpreq.header.TransacionID = 0x00000000;

    __ptpreq.cmd_payload.Parameter1 = session_id;

#ifdef __DEBUG
    printf("OPEN SESSION\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, NULL);
}

ptp_res_t ptp_close_session(ptp_dev_t* dev)
{
    struct ptp_header __ptpreq = { 0 };

    __ptpreq.ContaierLength = PTP_REQUEST_LEN(0);
    __ptpreq.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.Code = PTP_REQUEST_CLOSE_SESSION;
    __ptpreq.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("CLOSE SESSION\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, NULL);
}

ptp_res_t ptp_get_storage_id(ptp_dev_t* dev, uint8_t* data, uint32_t len)
{
    int nparams = 0;
    struct ptp_header __ptpreq = { 0 };

    __ptpreq.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.Code = PTP_REQUEST_GET_STORAGE_ID;
    __ptpreq.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("GET STORAGE ID\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_get_storage_info(ptp_dev_t* dev, uint32_t storage_id, uint8_t* data, uint32_t len)
{
    int nparams = 1;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_STORAGE_INFO;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = storage_id;

#ifdef __DEBUG
    printf("GET STORAGE INFO\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_get_num_objects(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, ptp_res_params_t* rparams)
{
    int nparams = 3;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_NUM_OBJECTS;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = storage_id;
    __ptpreq.cmd_payload.Parameter2 = object_format_code;
    __ptpreq.cmd_payload.Parameter3 = object_handle;

#ifdef __DEBUG
    printf("GET NUM OBJECTS\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, rparams);
}

ptp_res_t ptp_get_object_handles(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, uint8_t* data, uint32_t len)
{
    int nparams = 3;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_OBJECT_HANDLES;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = storage_id;
    __ptpreq.cmd_payload.Parameter2 = object_format_code;
    __ptpreq.cmd_payload.Parameter3 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT HANDLES\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_get_object_info(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len)
{
    int nparams = 1;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_OBJECT_INFO;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT INFO\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, NULL);
}

ptp_res_t ptp_get_object(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len)
{
    int nparams = 1;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_OBJECT;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_get_thumb(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len)
{
    int nparams = 1;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_GET_THUMB;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t ptp_delete_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t object_format_code, uint8_t* data, uint32_t len)
{
    int nparams = 2;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_DELETE_OBJECT;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = object_handle;
    __ptpreq.cmd_payload.Parameter2 = object_format_code;

#ifdef __DEBUG
    printf("GET DELETE OBJECT\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, NULL);
}

ptp_res_t
ptp_send_object_info(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, struct object_info* obj_info, uint32_t len, ptp_res_params_t* rparams)
{
    int nparams = 2;
    struct ptp_container __ptpreq = { 0 };

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpreq.header.TransacionID = __get_transaction_id();

    __ptpreq.cmd_payload.Parameter1 = storage_id;
    __ptpreq.cmd_payload.Parameter2 = object_handle;

    unsigned char* __ptpstream = (unsigned char*)&__ptpreq;
#ifdef __DEBUG
    printf("SEND OBJECT INFO\n");
#endif

    int sent_bytes = usb_bulk_send(dev->fd, dev->endp, (void*)__ptpstream, __ptpreq.header.ContaierLength);

    struct ptp_container __ptpreq2 = { 0 };
    uint8_t* __payload;

    __ptpreq2.header.ContaierLength = PTP_DATA_REQUEST_LEN(sizeof(struct object_info2));
    __ptpreq2.header.ContainerType = PTP_CONTAINER_TYPE_DATA_BLOCK;
    __ptpreq2.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpreq2.header.TransacionID = __get_transaction_id();

    struct object_info2 oi2 = { 0 };

    __ptpreq2.data_payload = (uint8_t*)&oi2;
    // memcpy(__ptpreq2.data_payload + 56, obj_info->Filename, 8);

    // for (int i = 0; i < len; ++i)
    //    printf("%.2X", __ptpreq2.data_payload[i]);
    // printf("\n");

    ptp_res_t __cmd_res = __handle_request(dev->fd, dev->endp, &__ptpreq2, NULL, 0, rparams);

    return __cmd_res;

    ptp_res_t r = {
        .code = 0x2001,
        .length = 0
    };

    return r;
}
