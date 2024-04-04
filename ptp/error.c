#include "error.h"
#define ERROR_CODE_MASK 0x00FF

static const char* error_msg[] = {
    "Undefined",
    "OK",
    "General Error",
    "Session Not Open",
    "Invalid TransactionID",
    "Operation Not Supported",
    "Parameter Not Supported",
    "Incomplete Transfer",
    "Invalid StorageID",
    "Invalid ObjectHandle",
    "DeviceProp Not Supported",
    "Invalid ObjectFormatCode",
    "Store Full",
    "Object WriteProtected",
    "Store Read-Only",
    "Access Denied",
    "No Thumbnail Present",
    "SelfTest Failed",
    "Partial Deletion",
    "Store Not Available",
    "Specification By Format Unsupported",
    "No Valid ObjectInfo",
    "Invalid Code Format",
    "Unknown Vendor Code",
    "Capture Already Terminated",
    "Device Busy",
    "Invalid ParentObject",
    "Invalid DeviceProp Format",
    "Invalid DeviceProp Value",
    "Invalid Parameter",
    "Session Already Open",
    "Transaction Cancelled",
    "Specification of Destination Unsupported"
};

const char* ptp_get_error(short ecode)
{
    return error_msg[ecode & ERROR_CODE_MASK];
}
