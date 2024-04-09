#ifndef __ANDROID_DEBUG_BRIDGE_INCLUDED_H__
#define __ANDROID_DEBUG_BRIDGE_INCLUDED_H__

#include "../../usb.h"
#include <stdint.h>

struct adb_dev {
    usb_dev_t fd;
    int endp;
};

typedef struct adb_dev adb_dev_t;

#define ADB_COMMAND_A_SYNC 0x434e5953
#define ADB_COMMAND_A_CNXN 0x4e584e43
#define ADB_COMMAND_A_AUTH 0x48545541
#define ADB_COMMAND_A_OPEN 0x4e45504f
#define ADB_COMMAND_A_OKAY 0x59414b4f
#define ADB_COMMAND_A_CLSE 0x45534c43
#define ADB_COMMAND_A_WRTE 0x45545257
#define ADB_COMMAND_A_STLS 0x534C5453

int adb_connect(adb_dev_t* dev, uint32_t version, uint32_t maxdata, const char* system_id, uint8_t* data, uint32_t len);

#endif // __ANDROID_DEBUG_BRIDGE_INCLUDED_H__
