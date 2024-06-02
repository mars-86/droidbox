#include "../ptp.h"
#include "object.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RESPONSE_TYPE_MASK(res) ((res[5] << 8) | (res[4]))
#define RESPONSE_BUFFER_LEN 2048
#define HEADER_LEN 0x0C
#define PARAM_LEN 0x04

#define RESPONSE_PHASE 0x00
#define DATA_PHASE 0x01
#define EVENT_PHASE 0x02

#define PTP_REQUEST_LEN(nparams) (HEADER_LEN + (nparams * PARAM_LEN))
#define PTP_DATA_REQUEST_LEN(payload_len) (HEADER_LEN + payload_len)

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

struct ptp_cmd_container {
    struct ptp_header header;
    ptp_cmd_payload_t payload;
} __attribute__((packed));

struct ptp_data_container {
    struct ptp_header header;
    ptp_data_payload_t payload;
} __attribute__((packed));

static int __send_request(int fd, uint8_t endp_in, uint8_t endp_out, const uint8_t* req, uint32_t len, uint8_t* data, ptp_res_t* res, ptp_res_params_t* rparams, int next_phase)
{
    int res_block = 0, offset = 0, sent_bytes = 0;
    if ((sent_bytes = usb_bulk_send(fd, endp_out, (void*)req, len)) < 0)
        return -1;

    uint8_t __rbuffer[512];
    if (next_phase != DATA_PHASE) {
        while (!res_block) {
            int recv_bytes = usb_bulk_recv(fd, endp_in, __rbuffer, sizeof(__rbuffer));
            int data_len = RESPONSE_LENGTH_MASK(__rbuffer);

            switch (RESPONSE_TYPE_MASK(__rbuffer)) {
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
                    recv_bytes = usb_bulk_recv(fd, endp_in, __rbuffer, sizeof(__rbuffer));
                }
                break;
            case PTP_CONTAINER_TYPE_RESPONSE_BLOCK:
#ifdef __DEBUG
                printf("RESPONSE BLOCK RECEIVED\n");
#endif
                if (res)
                    res->code = RESPONSE_CODE_MASK(__rbuffer);
                if (rparams)
                    memcpy(rparams, __rbuffer + HEADER_LEN, RESPONSE_LENGTH_MASK(__rbuffer) - HEADER_LEN);
                res_block = 1;
                break;
            default:;
            }
        }
    }

    return 0;
}

static int __handle_request(ptp_dev_t* dev, void* container, uint8_t* data, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams, int next_phase)
{
    struct ptp_header* header = (struct ptp_header*)container;
    int __bytes_len = header->ContaierLength;
    uint8_t __rstream[RESPONSE_BUFFER_LEN];

    int __ret = __send_request(dev->fd, dev->endp_in, dev->endp_out, (const uint8_t*)container, __bytes_len, __rstream, res, rparams, next_phase);

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

static inline int __attribute__((always_inline)) __get_transaction_id(void)
{
    return 1 + (rand() % (RAND_MAX - 1));
}

int ptp_get_device_info(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_header __ptpcmd = { 0 };

    __ptpcmd.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.Code = PTP_REQUEST_GET_DEVICE_INFO;
    __ptpcmd.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("GET DEVICE INFO\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_open_session(ptp_dev_t* dev, uint32_t session_id, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_OPEN_SESSION;
    __ptpcmd.header.TransacionID = 0x00000000;

    __ptpcmd.payload.Parameter1 = session_id;

#ifdef __DEBUG
    printf("OPEN SESSION\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_close_session(ptp_dev_t* dev, ptp_res_t* res)
{
    struct ptp_header __ptpcmd = { 0 };

    __ptpcmd.ContaierLength = PTP_REQUEST_LEN(0);
    __ptpcmd.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.Code = PTP_REQUEST_CLOSE_SESSION;
    __ptpcmd.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("CLOSE SESSION\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_get_storage_id(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_header __ptpcmd = { 0 };

    __ptpcmd.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.Code = PTP_REQUEST_GET_STORAGE_ID;
    __ptpcmd.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("GET STORAGE ID\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_storage_info(ptp_dev_t* dev, uint32_t storage_id, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_STORAGE_INFO;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = storage_id;

#ifdef __DEBUG
    printf("GET STORAGE INFO\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_num_objects(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, ptp_res_t* res, ptp_res_params_t* rparams)
{
    int nparams = 3;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_NUM_OBJECTS;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = storage_id;
    __ptpcmd.payload.Parameter2 = object_format_code;
    __ptpcmd.payload.Parameter3 = object_handle;

#ifdef __DEBUG
    printf("GET NUM OBJECTS\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, rparams, RESPONSE_PHASE);
}

int ptp_get_object_handles(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 3;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_OBJECT_HANDLES;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = storage_id;
    __ptpcmd.payload.Parameter2 = object_format_code;
    __ptpcmd.payload.Parameter3 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT HANDLES\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_object_info(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_OBJECT_INFO;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT INFO\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_object(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_thumb(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_THUMB;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;

#ifdef __DEBUG
    printf("GET OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_delete_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t object_format_code, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 2;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_DELETE_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;
    __ptpcmd.payload.Parameter2 = object_format_code;

#ifdef __DEBUG
    printf("GET DELETE OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_send_object_info(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, struct object_info* obj_info, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams)
{
    int nparams = 2;
    struct ptp_cmd_container __ptpcmd = { 0 };
    uint32_t transaction_id = __get_transaction_id();

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpcmd.header.TransacionID = transaction_id;

    __ptpcmd.payload.Parameter1 = storage_id;
    __ptpcmd.payload.Parameter2 = object_handle;

#ifdef __DEBUG
    printf("SEND OBJECT INFO\n");
#endif

    if (__handle_request(dev, &__ptpcmd, NULL, 0, NULL, rparams, DATA_PHASE) < 0)
        return -1;

    struct ptp_data_container __ptpdata = { 0 };

    __ptpdata.header.ContaierLength = PTP_DATA_REQUEST_LEN(sizeof(struct object_info2));
    __ptpdata.header.ContainerType = PTP_CONTAINER_TYPE_DATA_BLOCK;
    __ptpdata.header.Code = PTP_REQUEST_SEND_OBJECT_INFO;
    __ptpdata.header.TransacionID = transaction_id;

    // TODO: fix this to accept object_info parameter
    struct object_info2 oi2 = { 0 };

    __ptpdata.payload = (uint8_t*)&oi2;

    return __handle_request(dev, &__ptpdata, NULL, 0, res, rparams, RESPONSE_PHASE);
}

int ptp_send_object(ptp_dev_t* dev, void* object, uint32_t len, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_cmd_container __ptpcmd = { 0 };
    uint32_t transaction_id = __get_transaction_id();

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_SEND_OBJECT;
    __ptpcmd.header.TransacionID = transaction_id;

#ifdef __DEBUG
    printf("SEND OBJECT\n");
#endif

    if (__handle_request(dev, &__ptpcmd, NULL, 0, NULL, NULL, DATA_PHASE))
        return -1;

    struct ptp_data_container __ptpdata = { 0 };

    __ptpdata.header.ContaierLength = PTP_DATA_REQUEST_LEN(len);
    __ptpdata.header.ContainerType = PTP_CONTAINER_TYPE_DATA_BLOCK;
    __ptpdata.header.Code = PTP_REQUEST_SEND_OBJECT;
    __ptpdata.header.TransacionID = transaction_id;

    // TODO: fix this to accept object parameter
    struct object_info2 oi2 = { 0 };

    __ptpdata.payload = (uint8_t*)&oi2;

    return __handle_request(dev, &__ptpdata, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_initiate_capture(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, ptp_res_t* res)
{
// TODO: implement this function
#ifdef __DEBUG
    printf("INITIATE CAPTURE\n");
#endif
    return 0;
}

int ptp_format_store(ptp_dev_t* dev, uint32_t storage_id, uint32_t fs_format, ptp_res_t* res)
{
    int nparams = 2;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_DELETE_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = storage_id;
    __ptpcmd.payload.Parameter2 = fs_format;

#ifdef __DEBUG
    printf("FORMAT STORE\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_reset_device(ptp_dev_t* dev, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_header __ptpcmd = { 0 };

    __ptpcmd.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.Code = PTP_REQUEST_RESET_DEVICE;
    __ptpcmd.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("RESET DEVICE\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_self_test(ptp_dev_t* dev, uint32_t self_test_type, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_SELF_TEST;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = self_test_type;

#ifdef __DEBUG
    printf("SELF TEST\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_set_object_protection(ptp_dev_t* dev, uint32_t object_handle, uint32_t protection_status, ptp_res_t* res)
{
    int nparams = 2;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_SET_OBJECT_PROTECTION;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;
    __ptpcmd.payload.Parameter2 = protection_status;

#ifdef __DEBUG
    printf("SET OBJECT PROTECTION\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_power_down(ptp_dev_t* dev, ptp_res_t* res)
{
    int nparams = 0;
    struct ptp_header __ptpcmd = { 0 };

    __ptpcmd.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.Code = PTP_REQUEST_POWER_DOWN;
    __ptpcmd.TransacionID = __get_transaction_id();

#ifdef __DEBUG
    printf("POWER DOWN\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_get_device_prop_desc(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_DEVICE_PROP_DESC;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = device_prop_code;

#ifdef __DEBUG
    printf("GET DEVICE PROP DESC\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_get_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_DEVICE_PROP_VALUE;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = device_prop_code;

#ifdef __DEBUG
    printf("GET DEVICE PROP VALUE\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_set_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_SET_DEVICE_PROP_VALUE;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = device_prop_code;

#ifdef __DEBUG
    printf("SET DEVICE PROP VALUE\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, NULL, RESPONSE_PHASE);
}

int ptp_reset_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_RESET_DEVICE_PROP_VALUE;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = device_prop_code;

#ifdef __DEBUG
    printf("RESET DEVICE PROP VALUE\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_terminate_open_capture(ptp_dev_t* dev, uint32_t transaction_id, ptp_res_t* res)
{
    int nparams = 1;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_TERMINATE_OPEN_CAPTURE;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = transaction_id;

#ifdef __DEBUG
    printf("TERMINATE OPEN CAPTURE\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_move_object(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, uint32_t object_handle_parent, ptp_res_t* res)
{
    int nparams = 3;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_MOVE_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;
    __ptpcmd.payload.Parameter2 = storage_id;
    __ptpcmd.payload.Parameter3 = object_handle_parent;

#ifdef __DEBUG
    printf("MOVE OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}

int ptp_copy_object(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, uint32_t object_handle_parent, ptp_res_t* res, ptp_res_params_t* rparams)
{
    int nparams = 3;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_COPY_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;
    __ptpcmd.payload.Parameter2 = storage_id;
    __ptpcmd.payload.Parameter3 = object_handle_parent;

#ifdef __DEBUG
    printf("COPY OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, rparams, RESPONSE_PHASE);
}

int ptp_get_partial_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t offset, uint32_t max_bytes, uint8_t* data, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams)
{
    int nparams = 3;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_GET_PARTIAL_OBJECT;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = object_handle;
    __ptpcmd.payload.Parameter2 = offset;
    __ptpcmd.payload.Parameter3 = max_bytes;

#ifdef __DEBUG
    printf("GET PARTIAL OBJECT\n");
#endif

    return __handle_request(dev, &__ptpcmd, data, len, res, rparams, RESPONSE_PHASE);
}

int ptp_initiate_open_capture(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, ptp_res_t* res)
{
    int nparams = 2;
    struct ptp_cmd_container __ptpcmd = { 0 };

    __ptpcmd.header.ContaierLength = PTP_REQUEST_LEN(nparams);
    __ptpcmd.header.ContainerType = PTP_CONTAINER_TYPE_COMMAND_BLOCK;
    __ptpcmd.header.Code = PTP_REQUEST_INITIAL_OPEN_CAPTURE;
    __ptpcmd.header.TransacionID = __get_transaction_id();

    __ptpcmd.payload.Parameter1 = storage_id;
    __ptpcmd.payload.Parameter2 = object_format_code;

#ifdef __DEBUG
    printf("INITIATE OPEN CAPTURE\n");
#endif

    return __handle_request(dev, &__ptpcmd, NULL, 0, res, NULL, RESPONSE_PHASE);
}
