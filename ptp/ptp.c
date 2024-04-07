#include "ptp.h"
#include "object.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define __RESPONSE_TYPE_MASK(res) ((res[5] << 8) | (res[4]))
#define __RESPONSE_BUFFER_LEN 2048
#define __HEADER_LEN 0x0C
#define __PARAM_LEN 0x04

#define __RESPONSE_PHASE 0x00
#define __DATA_PHASE 0x01
#define __EVENT_PHASE 0x02

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

static int __send_request(int fd, int endpoint, const uint8_t* req, uint32_t len, uint8_t* data, ptp_res_t* res, ptp_res_params_t* rparams, int next_phase)
{
    int res_block = 0, offset = 0, sent_bytes = 0;
    if ((sent_bytes = usb_bulk_send(fd, endpoint, (void*)req, len)) < 0)
        return -1;

    uint8_t __rbuffer[512];
    if (next_phase != __DATA_PHASE) {
        while (!res_block) {
            int recv_bytes = usb_bulk_recv(fd, endpoint, __rbuffer, sizeof(__rbuffer));
            int data_len = RESPONSE_LENGTH_MASK(__rbuffer);

            switch (__RESPONSE_TYPE_MASK(__rbuffer)) {
            case PTP_CONTAINER_TYPE_DATA_BLOCK:
                while (1) {
                    memcpy(data + offset, __rbuffer, recv_bytes);

                    data_len -= recv_bytes;
                    offset += recv_bytes;
                    if (!data_len) {
                        if (res)
                            res->length = RESPONSE_LENGTH_MASK(data);
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
                if (res)
                    res->code = RESPONSE_CODE_MASK(__rbuffer);
                if (rparams)
                    memcpy(rparams, __rbuffer + __HEADER_LEN, RESPONSE_LENGTH_MASK(__rbuffer) - __HEADER_LEN);
                res_block = 1;
                break;
            default:;
            }
        }
    }

    return 0;
}

static int __handle_request(usb_dev_t fd, int endpoint, void* container, uint8_t* data, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams, int next_phase)
{
    struct ptp_header* header = (struct ptp_header*)container;
    int __bytes_len = header->ContaierLength;
    uint8_t __rstream[__RESPONSE_BUFFER_LEN];

    int __ret = __send_request(fd, endpoint, (const uint8_t*)container, __bytes_len, __rstream, res, rparams, next_phase);

    if (data) {
        /* check if provided buffer is large enougth to copy the response */
        uint32_t __bytes_to_copy = res->length > len ? len : res->length;
        memcpy(data, __rstream, __bytes_to_copy);
    }

#ifdef __DEBUG
    printf("\n%.2X\n%.2X\n", __res.length, __res.code);
#endif

    return __ret;
}

static inline int __attribute__((always_inline)) __get_transaction_id(void)
{
    return 1 + (rand() % (RAND_MAX - 1));
}

int ptp_get_device_info(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_open_session(ptp_dev_t* dev, uint32_t session_id, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, res, NULL, __RESPONSE_PHASE);
}

int ptp_close_session(ptp_dev_t* dev, ptp_res_t* res)
{
    struct ptp_header __ptpreq = { 0 };

    __ptpreq.ContaierLength = PTP_REQUEST_LEN(0);
    __ptpreq.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.Code = PTP_REQUEST_CLOSE_SESSION;
    __ptpreq.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("CLOSE SESSION\n");
#endif

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_storage_id(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_storage_info(ptp_dev_t* dev, uint32_t storage_id, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_num_objects(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, ptp_res_t* res, ptp_res_params_t* rparams)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, res, rparams, __RESPONSE_PHASE);
}

int ptp_get_object_handles(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_object_info(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_object(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_get_thumb(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_delete_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t object_format_code, uint8_t* data, uint32_t len, ptp_res_t* res)
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

    return __handle_request(dev->fd, dev->endp, &__ptpreq, data, len, res, NULL, __RESPONSE_PHASE);
}

int ptp_send_object_info(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, struct object_info* obj_info, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams)
{
    int nparams = 2;
    struct ptp_container __ptpreq = { 0 };
    uint32_t transaction_id = __get_transaction_id();

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpreq.header.TransacionID = transaction_id;

    __ptpreq.cmd_payload.Parameter1 = storage_id;
    __ptpreq.cmd_payload.Parameter2 = object_handle;

#ifdef __DEBUG
    printf("SEND OBJECT INFO\n");
#endif

    __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, NULL, rparams, __DATA_PHASE);

    __ptpreq.header.ContaierLength = PTP_DATA_REQUEST_LEN(sizeof(struct object_info2));
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_DATA_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpreq.header.TransacionID = transaction_id;

    // TODO: fix this to accept object_info parameter
    struct object_info2 oi2 = { 0 };

    __ptpreq.data_payload = (uint8_t*)&oi2;

    return __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, res, rparams, __RESPONSE_PHASE);
}

int ptp_send_object(ptp_dev_t* dev, void* object, uint32_t len, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_container __ptpreq = { 0 };
    uint32_t transaction_id = __get_transaction_id();

    __ptpreq.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpreq.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpreq.header.Code = PTP_REQUEST_SEND_OBJECT;
    __ptpreq.header.TransacionID = transaction_id;

#ifdef __DEBUG
    printf("SEND OBJECT\n");
#endif

    __handle_request(dev->fd, dev->endp, &__ptpreq, NULL, 0, NULL, NULL, __DATA_PHASE);

    struct ptp_container __ptpreq2 = { 0 };
    uint8_t* __payload;

    __ptpreq2.header.ContaierLength = PTP_DATA_REQUEST_LEN(len);
    __ptpreq2.header.ContainerType = PTP_CONTAINER_TYPE_DATA_BLOCK;
    __ptpreq2.header.Code = PTP_REQUEST_SEND_OBJECT;
    __ptpreq2.header.TransacionID = transaction_id;

    // TODO: fix this to accept object parameter
    struct object_info2 oi2 = { 0 };

    __ptpreq2.data_payload = (uint8_t*)&oi2;

    return __handle_request(dev->fd, dev->endp, &__ptpreq2, NULL, 0, res, NULL, __RESPONSE_PHASE);
}
