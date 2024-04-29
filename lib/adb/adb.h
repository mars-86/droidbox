#ifndef __ANDROID_DEBUG_BRIDGE_INCLUDED_H__
#define __ANDROID_DEBUG_BRIDGE_INCLUDED_H__

#include "../usb/usb.h"
#include <stdint.h>

struct message {
    uint32_t command; /* command identifier constant (A_CNXN, ...) */
    uint32_t arg0; /* first argument */
    uint32_t arg1; /* second argument */
    uint32_t data_length; /* length of payload (0 is allowed) */
    uint32_t data_crc32; /* crc32 of data payload */
    uint32_t magic; /* command ^ 0xffffffff */
} __attribute__((packed));

struct adb_dev {
    usb_dev_t fd;
    int in_endp;
    int out_endp;
} __attribute__((packed));

typedef struct adb_dev adb_dev_t;

struct adb_res {
    uint32_t length;
    uint32_t code;
} __attribute__((packed));

typedef struct adb_res adb_res_t;

#define ADB_PROTO_VERSION 0x01000000
#define ADB_MAX_DATA_SIZE 1024 * 256
#define ADB_MAX_PACKET_SIZE 4096 // Older versions of adbhard-coded maxdata=4096

#define ADB_COMMAND_A_SYNC 0x434e5953
#define ADB_COMMAND_A_CNXN 0x4e584e43
#define ADB_COMMAND_A_AUTH 0x48545541
#define ADB_COMMAND_A_OPEN 0x4e45504f
#define ADB_COMMAND_A_OKAY 0x59414b4f
#define ADB_COMMAND_A_CLSE 0x45534c43
#define ADB_COMMAND_A_WRTE 0x45545257
#define ADB_COMMAND_A_STLS 0x534C5453

#define ADB_AUTH_TYPE_TOKEN 1
#define ADB_AUTH_TYPE_SIGNATURE 2
#define ADB_AUTH_TYPE_RSAPUBLICKEY 3

#define ADB_STLS_VERSION_MIN 0x01000000
#define ADB_STLS_VERSION 0x01000000

#define ADB_DESTINATION(service) (service##" ")
#define ADB_SERVICE_SHELL_RAW(cmd) "shell:"##cmd
#define ADB_SERVICE_SHELL_IT "shell:"
#define ADB_SERVICE_SHELL_V2 "shell,v2:"
#define ADB_SERVICE_EXEC "exec:"
#define ADB_SERVICE_ABB(cmd) "abb:"##cmd
#define ADB_SERVICE_ABB_PTY(cmd) "abb_exec:"##cmd
#define ADB_SERVICE_REMOUNT "remount:"
#define ADB_SERVICE_DEV(path) "dev:"##path
#define ADB_SERVICE_DEV_RAW(path) "dev-raw:"##path
#define ADB_SERVICE_TCP(port) "tcp:"##port
#define ADB_SERVICE_TCP_SNAME(port, server_name) "tcp:"##port##":"##server_name
// ...
#define ADB_SERVICE_FRAMEBUFFER "framebuffer:"
// ...
#define ADB_SERVICE_TRACK_APP "track-app:"
#define ADB_SERVICE_SYNC "sync:"
#define ADB_SERVICE_REVERSE(cmd) "reverse:"##cmd

int adb_connect(adb_dev_t* dev, uint32_t version, uint32_t maxdata, const char* system_id, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_stls(adb_dev_t* dev, uint32_t type, uint32_t version, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_auth(adb_dev_t* dev, uint32_t type, const char* auth_data, uint32_t auth_len, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_open(adb_dev_t* dev, uint32_t local_id, const char* destination, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_ready(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_write(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, const char* payload, uint8_t* data, uint32_t len, adb_res_t* res);

int adb_close(adb_dev_t* dev, uint32_t local_id, uint32_t remote_id, uint8_t* data, uint32_t len, adb_res_t* res);

#endif // __ANDROID_DEBUG_BRIDGE_INCLUDED_H__
