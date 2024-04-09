#include "adb.h"

struct message {
    uint32_t command; /* command identifier constant (A_CNXN, ...) */
    uint32_t arg0; /* first argument */
    uint32_t arg1; /* second argument */
    uint32_t data_length; /* length of payload (0 is allowed) */
    uint32_t data_crc32; /* crc32 of data payload */
    uint32_t magic; /* command ^ 0xffffffff */
};
