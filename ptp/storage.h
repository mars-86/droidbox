#ifndef __PICTURE_TRANSFER_PROTOCOL_STORAGE_INCLUDED_H__
#define __PICTURE_TRANSFER_PROTOCOL_STORAGE_INCLUDED_H__

#define PTP_STORAGE_PHYSICAL_ID_MASK(storage_id) ((storage_id | 0xFFFF0000) >> 16)
#define PTP_STORAGE_LOGICAL_ID_MASK(storage_id) (storage_id | 0x0000FFFF)

#define PTP_STORAGE_ID_UNUSED 0x00000000
#define PTP_STORAGE_HANDLE_ROOT 0xFFFFFFFF
#define PTP_STORAGE_HANDLE_UNUSED 0x00000000

#endif // __PICTURE_TRANSFER_PROTOCOL_STORAGE_INCLUDED_H__
