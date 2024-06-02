#ifndef __PICTURE_TRANSFER_PROTOCOL_INCLUDED_H__
#define __PICTURE_TRANSFER_PROTOCOL_INCLUDED_H__

#include "../usb/usb.h"
#include "./src/association.h"
#include "./src/dataset.h"
#include "./src/object.h"
#include "./src/storage.h"
#include "error.h"
#include <stdint.h>

struct ptp_res_params {
    uint32_t Parameter1;
    uint32_t Parameter2;
    uint32_t Parameter3;
};

typedef struct ptp_res_params ptp_res_params_t;

struct ptp_dev {
    usb_dev_t fd;
    uint8_t endp_in;
    uint16_t endp_in_max_pack_size;
    uint8_t endp_out;
    uint16_t endp_out_max_pack_size;
    uint8_t endp_int;
    uint16_t endp_int_max_pack_size;
};

typedef struct ptp_dev ptp_dev_t;

struct ptp_res {
    uint32_t length;
    uint16_t code;
} __attribute__((packed));

typedef struct ptp_res ptp_res_t;

/* PTP Generic container types  */
#define PTP_CONTAINER_TYPE_UNDEFINED 0
#define PTP_CONTAINER_TYPE_COMMAND_BLOCK 1
#define PTP_CONTAINER_TYPE_DATA_BLOCK 2
#define PTP_CONTAINER_TYPE_RESPONSE_BLOCK 3
#define PTP_CONTAINER_TYPE_EVENT_BLOCK 4

/* PTP Requests */
#define PTP_REQUEST_UNDEFINED 0x1000
#define PTP_REQUEST_GET_DEVICE_INFO 0x1001
#define PTP_REQUEST_OPEN_SESSION 0x1002
#define PTP_REQUEST_CLOSE_SESSION 0x1003
#define PTP_REQUEST_GET_STORAGE_ID 0x1004
#define PTP_REQUEST_GET_STORAGE_INFO 0x1005
#define PTP_REQUEST_GET_NUM_OBJECTS 0x1006
#define PTP_REQUEST_GET_OBJECT_HANDLES 0x1007
#define PTP_REQUEST_GET_OBJECT_INFO 0x1008
#define PTP_REQUEST_GET_OBJECT 0x1009
#define PTP_REQUEST_GET_THUMB 0x100A
#define PTP_REQUEST_DELETE_OBJECT 0x100B
#define PTP_REQUEST_SEND_OBJECT_INFO 0x100C
#define PTP_REQUEST_SEND_OBJECT 0x100D
#define PTP_REQUEST_INITIATE_CAPTURE 0x100E
#define PTP_REQUEST_FORMAT_STORE 0x100F
#define PTP_REQUEST_RESET_DEVICE 0x1010
#define PTP_REQUEST_SELF_TEST 0x1011
#define PTP_REQUEST_SET_OBJECT_PROTECTION 0x1012
#define PTP_REQUEST_POWER_DOWN 0x1013
#define PTP_REQUEST_GET_DEVICE_PROP_DESC 0x1014
#define PTP_REQUEST_GET_DEVICE_PROP_VALUE 0x1015
#define PTP_REQUEST_SET_DEVICE_PROP_VALUE 0x1016
#define PTP_REQUEST_RESET_DEVICE_PROP_VALUE 0x1017
#define PTP_REQUEST_TERMINATE_OPEN_CAPTURE 0x1018
#define PTP_REQUEST_MOVE_OBJECT 0x1019
#define PTP_REQUEST_COPY_OBJECT 0x101A
#define PTP_REQUEST_GET_PARTIAL_OBJECT 0x101B
#define PTP_REQUEST_INITIAL_OPEN_CAPTURE 0x101C

/* PTP Responses */
#define PTP_RESPONSE_UNDEFINED 0x2000
#define PTP_RESPONSE_OK 0x2001
#define PTP_RESPONSE_GENERAL_ERROR 0x2002
#define PTP_RESPONSE_SESSION_NOT_OPEN 0x2003
#define PTP_RESPONSE_INVALID_TRANSACTION_ID 0x2004
#define PTP_RESPONSE_OPERATION_NOT_SUPPORTED 0x2005
#define PTP_RESPONSE_PARAMETER_NOT_SUPPORTED 0x2006
#define PTP_RESPONSE_INCOMPLETE_TRANSFER 0x2007
#define PTP_RESPONSE_INVALID_STORAGE_ID 0x2008
#define PTP_RESPONSE_INVALID_OBJECT_HANDLE 0x2009
#define PTP_RESPONSE_DEVICEPROP_NOT_SUPPORTED 0x200A
#define PTP_RESPONSE_INVALID_OBJECT_FORMAT_CODE 0x200B
#define PTP_RESPONSE_STORE_FULL 0x200C
#define PTP_RESPONSE_OBJECT_WRITE_PROTECTED 0x200D
#define PTP_RESPONSE_STORE_READ_ONLY 0x200E
#define PTP_RESPONSE_ACCESS_DENIED 0x200F
#define PTP_RESPONSE_NO_THUMBNAIL_PRESENT 0x2010
#define PTP_RESPONSE_SELF_TEST_FAILED 0x2011
#define PTP_RESPONSE_PARTIAL_DELETION 0x2012
#define PTP_RESPONSE_STORE_NOT_AVAILABLE 0x2013
#define PTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED 0x2014
#define PTP_RESPONSE_NO_VALID_OBJECT_INFO 0x2015
#define PTP_RESPONSE_INVALID_CODE_FORMAT 0x2016
#define PTP_RESPONSE_UNKNOWN_VENDOR_CODE 0x2017
#define PTP_RESPONSE_CAPTURE_ALREADY_TERMINATED 0x2018
#define PTP_RESPONSE_DEVICE_BUSY 0x2019
#define PTP_RESPONSE_INVALID_PARENT_OBJECT 0x201A
#define PTP_RESPONSE_INVALID_DEVICEPROP_FORMAT 0x201B
#define PTP_RESPONSE_INVALID_DEVICEPROP_VALUE 0x201C
#define PTP_RESPONSE_INVALID_PARAMETER 0x201D
#define PTP_RESPONSE_SESSION_ALREADY_OPEN 0x201E
#define PTP_RESPONSE_TRANSACTION_CANCELLED 0x201F
#define PTP_RESPONSE_SPECIFICATION_OF_DESTINATION_UNSUPPORTED 0x2020

/*
 * GetDeviceInfo
 * Data Direction: R -> I
 * ResponseCode Options: OK, Parameter_Not_Supported
 * Description: This operation returns information and capabilities about the Responder device
 * by returning a DeviceInfo dataset. This dataset is described in Clause 5.5.1. This
 * operation is the only operation that may be issued inside or outside of a session. When
 * used outside a session, both the SessionID and the TransactionID in the
 * OperationRequest dataset shall be set to 0x00000000.
 */

int ptp_get_device_info(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * OpenSession
 * Data Direction: N/A
 * ResponseCode Options: OK, Parameter_Not_Supported, Invalid_Parameter,
 * Session_Already_Open, Device_Busy
 * Description: Causes device to allocate resources, assigns handles to data objects if necessary,
 * and performs any connection-specific initialization. The SessionID will then be used by
 * all other operations during the session. Unless otherwise specified, an open session is
 * required to invoke an operation. If the first parameter is 0x00000000, the operation
 * should fail with a response of Invalid_Parameter. If a session is already open, and the
 * device does not support multiple sessions, the response Session_Already_Open should
 * be returned, with the SessionID of the already open session as the first response
 * parameter. The response Session_Already_Open should also be used if the device
 * supports multiple sessions, but a session with that ID is already open. If the device
 * supports multiple sessions, and the maximum number of sessions are open, the device
 * should respond with Device_Busy.
 * The SessionID and TransactionID fields of the operation dataset should both be set to
 * 0x00000000 for this operation.
 */

int ptp_open_session(ptp_dev_t* dev, uint32_t session_id, ptp_res_t* res);

/*
 * CloseSession
 * Data Direction: N/A
 * ResponseCode Options: OK, Session_Not_Open, Invalid_TransactionID,
 * Parameter_Not_Supported
 * Description: Closes the session. Causes device to perform any session-specific cleanup.
 */

int ptp_close_session(ptp_dev_t* dev, ptp_res_t* res);

/*
 * GetStorageIDs
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Parameter_Not_Supported
 * Description: This operation returns a list of the currently valid StorageIDs. This array shall
 * contain one StorageID for each valid logical store. One StorageID should also be
 * present for each removable media that is not inserted, which would contain a non-zero
 * PhysicalStorageID and a LogicalStorageID with the value 0x0000.
 */

int ptp_get_storage_id(ptp_dev_t* dev, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetStorageInfo
 * Data Direction: R -> I
 * ResponseCode Options: OK, Session_Not_Open, Invalid_TransactionID, Access_Denied,
 * Invalid_StorageID, Store_Not_Available, Parameter_Not_Supported
 * Description: Returns a StorageInfo dataset for the particular storage area indicated in the first
 * parameter. This dataset is defined in Clause 5.5.3.
 */

int ptp_get_storage_info(ptp_dev_t* dev, uint32_t storage_id, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetNumObjects
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_StorageID, Store_Not_Available,
 * Specification_By_Format_Unsupported, Invalid_Code_Format,
 * Parameter_Not_Supported, Invalid_ParentObject, Invalid_ObjectHandle,
 * Invalid_Parameter
 * Description: Returns the total number of objects present in the store indicated by the first
 * parameter. If the number of objects aggregated across all stores is desired, a StorageID
 * of 0xFFFFFFFF may be used. If a single store is specified, and the store is unavailable
 * because of media removal, this operation should return Store_Not_Available.
 * By default, this operation returns the total number of objects, which includes both image
 * and non-image objects of all types.
 * The second parameter, ObjectFormatCode, is optional, and may not be supported. This
 * parameter is used to identify a particular ObjectFormatCode, so that only objects of the
 * particular type will be counted towards NumObjects. If the number of objects of all
 * formats that are images is desired, the value 0xFFFFFFFF may be used. If this
 * parameter is not used, it shall be set to 0x00000000. If the value is non-zero, and the
 * Responder does not support specification by ObjectFormatCode, it should fail the
 * operation by returning a ResponseCode with the value of
 * Specification_By_Format_Unsupported.
 * The third parameter is optional, and may be used to request only the number of objects
 * that belong directly to a particular association. If the third parameter is a valid
 * ObjectHandle for an Association, this operation should only return the number of
 * ObjectHandles that exist for objects that are direct children of the Association, and
 * therefore only the number of ObjectHandles which refer to objects that possess an
 * ObjectInfo dataset with the ParentObject field set to the value indicated in the third
 * parameter. If the number of only those ObjectHandles corresponding to objects in the
 * “root” of a store is desired, this parameter may be set to 0xFFFFFFFF. If the
 * ObjectHandle referred to is not a valid ObjectHandle, the appropriate response is
 * Invalid_ObjectHandle. If this parameter is specified, is a valid ObjectHandle, but the
 * object referred to is not an association, the response Invalid_ParentObject should be
 * returned. If unused, this operation returns the number of ObjectHandles aggregated
 * across the entire device (modified by the second parameter), and the third parameter
 * should be set to 0x00000000.
 */

int ptp_get_num_objects(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, ptp_res_t* res, ptp_res_params_t* rparams);

/*
 * GetObjectHandles
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_StorageID, Store_Not_Available,
 * Invalid_ObjectFormatCode, Specification_By_Format_Unsupported,
 * Invalid_Code_Format, Invalid_ObjectHandle, Invalid_Parameter,
 * Parameter_Not_Supported, Invalid_ParentObject, Invalid_ObjectHandle
 * Description: Returns an array of ObjectHandles present in the store indicated by the
 * StorageID in the first parameter. If an aggregated list across all stores is desired, this
 * value shall be set to 0xFFFFFFFF. Arrays are described in Clause 5.4.
 * The second parameter is optional, and may or may not be supported. This parameter
 * allows the Initiator to ask for only the handles that represent data objects that possess a
 * format specified by the ObjectFormatCode. If a list of handles that represent only image
 * objects is desired, this second parameter may be set to 0xFFFFFFFF. If it is not used, it
 * shall be set to 0x00000000. If the value is non-zero, and the Responder does not support
 * specification by ObjectFormatCode, it should fail the operation by returning a
 * ResponseCode with the value of Specification_By_Format_Unsupported. If a single
 * store is specified, and the store is unavailable because of media removal, this operation
 * should return Store_Not_Available.
 * The third parameter is optional, and may be used to request only a list of the handles of
 * objects that belong to a particular association. If the third parameter is a valid
 * ObjectHandle for an Association, this operation should return only a list of
 * ObjectHandles of objects that are direct children of the Association, and therefore only
 * ObjectHandles who refer to objects that possess an ObjectInfo dataset with the
 * ParentObject field set to the value indicated in the third parameter. If a list of only those
 * ObjectHandles corresponding to objects in the “root” of a store is desired, this parameter
 * may be set to 0xFFFFFFFF. If the ObjectHandle referred to is not a valid ObjectHandle,
 * the appropriate response is Invalid_ObjectHandle. If this parameter is specified, is a
 * valid ObjectHandle, but the object referred to is not an association, the response
 * Invalid_ParentObject should be returned. If the third parameter is unused, this operation
 * returns ObjectHandles aggregated across the entire device (modified by the second
 * parameter), and the third parameter should be set to 0x00000000.
 */

int ptp_get_object_handles(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetObjectInfo
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_ObjectHandle, Store_Not_Available,
 * Parameter_Not_Supported
 * Description: Returns the ObjectInfo dataset. The primary
 * purpose of this operation is to obtain information about a data object present on the
 * device before deciding whether to retrieve that object or its thumbnail with a succeeding
 * GetThumb or GetObject operation. This information may also be used by the caller to
 * allocate memory before receiving the object. Objects that possess an ObjectFormat of
 * type Association do not require a GetObject operation, as these objects are fully
 * qualified by their ObjectInfo dataset.
 */

int ptp_get_object_info(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetObject
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_ObjectHandle, Invalid_Parameter, Store_Not_Available,
 * Parameter_Not_Supported, Incomplete_Transfer
 * Description: Retrieves one object from the device. This operation is used for all types of
 * data objects present on the device, including both images and non-image data objects,
 * and should be preceded (although not necessarily immediately) by a GetObjectInfo
 * operation that uses the same ObjectHandle. This operation is not necessary for objects
 * of type Association, as these objects are fully qualified by their ObjectInfo dataset. If the
 * store that contains the object being sent is removed during the object transfer, the
 * Incomplete_Transfer response should be used, along with the Store_Removed event.
 */

int ptp_get_object(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetThumb
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_ObjectHandle, Thumbnail_Not_Present,
 * Invalid_ObjectFormatCode, Store_Not_Available, Parameter_Not_Supported
 * Description: Retrieves the thumbnail from the device that is associated with the
 * ObjectHandle that is indicated in the first parameter.
 */

int ptp_get_thumb(ptp_dev_t* dev, uint32_t object_handle, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * DeleteObject
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_ObjectHandle, Object_WriteProtected,
 * Store_Read_Only, Partial_Deletion, Store_Not_Available,
 * Specification_By_Format_Unsupported, Invalid_Code_Format, Device_Busy,
 * Parameter_Not_Supported
 * Description: Deletes the data object specified by the ObjectHandle from the device if it is not
 * protected. If the ObjectHandle passed has the value of 0xFFFFFFFF, then all objects on
 * the device shall be deleted. Any write-protected objects shall also not be deleted by this
 * operation. If one object is indicated for deletion and it is write-protected, the response
 * code Object_WriteProtected shall be returned. If all objects are indicated for deletion
 * and a subset of the objects are write-protected, only the objects that are not protected
 * shall be deleted, and the response code of Partial_Deletion shall be returned. If the store
 * is read-only without object deletion, the response Store_Read_Only should be returned.
 * If the store is read-only with object deletion, this operation should succeed unless other
 * factors prevent it from succeeding.
 * The second parameter is optional, and may not be supported. This parameter may only
 * be used if the first parameter is set to 0xFFFFFFFF. This parameter is used to indicate
 * that objects only of the type specified are to be deleted. If this second parameter is also
 * set to 0xFFFFFFFF, then only objects that are images shall be deleted. If it is not used, it
 * shall be set to 0x00000000. If the value is non-zero, and the Responder does not support
 * specification by ObjectFormatCode, it should fail the operation by returning a
 * ResponseCode with the value of Specification_By_Format_Unsupported.
 * If the ObjectHandle indicated in the first parameter is an Association, then all objects
 * that are a part of that association (and all descendants of descendants) shall be deleted as
 * well. If only individual items within an association are to be deleted, then individual
 * DeleteObject operations should be issued on each object or sub-association individually.
 */

int ptp_delete_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t object_format_code, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * SendObjectInfo
 * Data Direction: I -> R
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Access_Denied, Invalid_StorageID, Store_Read_Only,
 * Store_Full, Invalid_ObjectFormatCode, Store_Not_Available,
 * Parameter_Not_Supported, Invalid_ParentObject, Invalid_Dataset
 * Description: This operation is used as the first operation when the Initiator wishes to send an
 * object to the Responder. This operation sends an ObjectInfo dataset from the Initiator to
 * the Responder. All the fields in this ObjectInfo dataset are from the perspective of the
 * Initiator, meaning that the StorageID, for example, would be interpreted as the StorageID
 * of the store in which the object resides on the Initiator before being sent to the
 * Responder. This operation is sent prior to the SendObject operation, described in Clause
 * 10.4.13, in order to inform the Responder about the properties of the object that it
 * intends to send later, and to effectively ask permission whether the object can be sent to
 * the Responder. A response of OK infers that the receiver can accept the object, and
 * serves to inform the sender that it may now issue a SendObject operation for the object.
 * The first parameter is optionally used to indicate the store on the Responder into which
 * the object should be stored. If this parameter is specified, and the Responder will not be
 * able to store the object in the indicated store, the operation should fail, and the
 * appropriate response, such as Specification_Of_Destination_Unsupported,
 * Store_Not_Available, Store_Read_Only, or Store_Full should be used. If this parameter
 * is unused, it should be set to 0x00000000, and the Responder shall decide in which store
 * to place the object, be that a Responder-determined default location, or the location with
 * the most room (or possibly the only location with enough room).
 * The second parameter is optionally used to indicate where on the indicated store the
 * object should be placed (i.e. the association/folder that the object should become a child
 * of.) If this parameter is used, the first parameter must also be used. If the receiver is
 * unable to place the object as a child of the indicated second parameter, the operation
 * should fail. If the problem with the attempted specification is the general inability of the
 * receiving device to allow the specification of destination, the response
 * Specification_of_Destination_Unsupported should be sent. This response infers that the
 * Initiator should not try to specify a destination location in future invocations of
 * SendObjectInfo, as all attempts at such specification will fail. If the problem is only
 * with the particular destination specified, the Invalid_ObjectHandle or
 * Invalid_ParentObject response should be used, depending on whether the ObjectHandle
 * did not refer to a valid object, or whether the indicated object is a valid object but is not
 * an association. If the root directory of the indicated store is desired, the second
 * parameter should be set to 0xFFFFFFFF. If this parameter is unused, it should be set to
 * 0x00000000, and the Responder shall decide where in the indicated store the object is to
 * be placed. If neither the first nor the second parameter is used, the Responder shall
 * decide both which store to place the object in as well as where to place it within that
 * store.
 * If the Responder agrees that the object may be sent, it is required to retain this
 * ObjectInfo dataset until the next SendObject or SendObjectInfo operation is performed
 * subsequently within the session. If the SendObjectInfo operation succeeds, and the next
 * occurring SendObject operation does not return a successful response, the
 * SendObjectInfo held by the Responder shall be retained in case the Initiator wishes to re-
 * attempt the SendObject operation for that previously successful SendObjectInfo
 * operation. If the Initiator wishes to resend the ObjectInfo dataset before attempting to
 * resend the object it may do so. Successful completion of the SendObjectInfo operation
 * conveys that the Responder possesses a copy of the ObjectInfo and that the Responder
 * has allocated space for the incoming data object. Any other response code other than
 * OK indicates that the Responder has not retained the ObjectInfo dataset, and that the
 * object should not attempt to be sent.
 * For a particular session, the receiving device shall only retain one ObjectInfo that is the
 * result of a SendObjectInfo operation in memory at a time. If another SendObjectInfo
 * operation occurs before a SendObject operation, the new ObjectInfo shall replace the
 * previously held one. If this occurs, any storage space or memory space reserved for the
 * object described in the overwritten ObjectInfo dataset should be freed before overwriting
 * and allocation of the resources for the new ObjectInfo dataset.
 * The first response parameter of this operation should be set to the StorageID that the
 * Responder will store the object into if it sent. The second response parameter of this
 * operation should be set to the Parent ObjectHandle of the association that the object
 * becomes a child of. If the object is stored in the root of the store, this parameter should
 * be set to 0xFFFFFFFF.
 * If the Initiator wishes to retain associations and/or hierarchies on the Responder for the
 * objects it is sending, then the objects should be sent top down, starting with the highest
 * level of the hierarchy, proceeding in either a depth-first or breadth-first fashion down the
 * hierarchy tree. The Initiator shall use the Responder’s newly assigned ObjectHandle in
 * the third response parameter for the ParentObject that is returned in the SendObjectInfo
 * response as the second operation parameter for a child’s SendObjectInfo operation.
 */

int ptp_send_object_info(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, struct object_info* obj_info, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams);

/*
 * SendObject
 * Data Direction: I -> R
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Store_Full, Store_Not_Available, No_Valid_ObjectInfo,
 * Device_Busy, Parameter_Not_Supported, Incomplete_Transfer
 * Description: This operation is used as the second operation when the Initiator wishes to send
 * an object to the Responder, following the SendObjectInfo operation.
 * This operation sends a data object to the device to be written to the
 * Responder’s store, according to the information in the ObjectInfo dataset as transmitted
 * during the most recent SendObjectInfo operation in the same session, and the
 * information indicated by the responder in the response parameters of the
 * SendObjectInfo.
 * Upon successful completion of this operation, the Responder should discard and/or
 * invalidate the Initiator’s ObjectInfo that the Responder held while waiting for that
 * object. If there is no valid ObjectInfo held by the Responder, the response
 * No_Valid_ObjectInfo should be returned. Any response other than OK indicates that the
 * SendObject failed, for the reason indicated by the response code. In this case, the
 * unassigned ObjectInfo should be retained by the Responder in case the Initiator wishes
 * to attempt to resend the object, for at most the duration of the session. If the destination
 * store is removed during object transmission, the Incomplete_Transfer response should be
 * issued along with the StoreRemoved event.
 */

int ptp_send_object(ptp_dev_t* dev, void* object, uint32_t len, ptp_res_t* res);

/*
 * InitiateCapture
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_StorageID, Store_Full, Invalid_ObjectFormatCode,
 * Invalid_Parameter, Store_Not_Available, Invalid_Code_Format, Device_Busy,
 * Parameter_Not_Supported
 * Description: Causes the device to initiate the capture of one or more new data objects
 * according to its current device properties, storing the data into the store indicated by the
 * first parameter. If the StorageID is 0x00000000, the object(s) will be stored in a store
 * that is determined by the capturing device. If the particular store specified is not
 * available, or no store is specified and there are no stores available, this operation should
 * return Store_Not_Available.
 * The capturing of new data objects is an asynchronous operation. This operation may be
 * used to capture images or any type of data that can be fully captured using a single
 * operation trigger. For these types of captures, the length of the capture and the number
 * of objects to capture is known apriori by the Responder, as opposed to being
 * dynamically terminable after capture initiation by the Initiator. A separate operation,
 * InitiateOpenCapture, described in Clause 10.4.28, can be used to support dynamically
 * controlled captures that are terminable by the Initiator.
 * If the ObjectFormatCode in the second operation parameter is 0x00000000, the device
 * shall capture an image in the format that is the default for the device. A successful
 * response to an InitiateCapture operation indicates the Responder’s acceptance of the
 * InitiateCapture operation, and not the completion status of the actual object capture,
 * which is indicated using the CaptureComplete event.
 * As the capture is executed, one or more new data objects should be created on the
 * device. The number of objects to be captured is not specified as part of the
 * InitiateCapture operation, but is determined by the state of the capturing device, and may
 * optionally be set by the Initiator using an appropriate DeviceProperty. As each of the
 * newly captured objects becomes available, the Responder is required to send an
 * ObjectAdded event to the Initiator, indicating the ObjectHandle that is assigned to each
 * as described in Clause 12.5.2. This ObjectAdded event shall contain the TransactionID
 * of the InitiateCapture operation with which it is associated. If, at any time, the store
 * becomes full, the device shall invoke a Store_Full event, which shall contain the
 * TransactionID of the InitiateCapture operation that failed to cause a new object to be
 * stored. In the case of multiple objects being captured, each object shall be handled
 * separately, so any object captured before the store becomes full should be retained.
 * When all objects have been captured, the Responder shall send a CaptureComplete event
 * to the Initiator. If the Store_Full event was issued, the CaptureComplete event should
 * not be issued. If another capture is occurring when this operation is invoked, the
 * Device_Busy response should be used.
 */

int ptp_initiate_capture(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, ptp_res_t* res);

/*
 * FormatStore
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_StorageID, Store_Not_Available, Device_Busy,
 * Parameter_Not_Supported, Invalid_Parameter, Store_Read_Only
 * Description: Formats the media specified by the StorageID. The second parameter is
 * optional and may be used to indicate the format that the store should be formatted in,
 * according to the FilesystemFormat codes described in Clause 5.5.3. If a given format is
 * not supported, the response Invalid_Parameter should be returned. If the device is
 * currently capturing objects to the store, or is otherwise unable to format due to
 * concurrent access, the Device_Busy operation should be returned.
 */

int ptp_format_store(ptp_dev_t* dev, uint32_t storage_id, uint32_t fs_format, ptp_res_t* res);

/*
 * ResetDevice
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Device_Busy
 * Description: Resets the device to its device-dependent default state. This does not include
 * resetting any device properties, which is performed using ResetDeviceProp. This does
 * include closing the current session, and any other open sessions. If this operation is
 * supported and the device supports multiple concurrent sessions, the device is responsible
 * for supporting the DeviceReset event, which should be sent to all open sessions
 * excluding the one within which the ResetDevice operation was initiated prior to closing
 * the sessions.
 */

int ptp_reset_device(ptp_dev_t* dev, ptp_res_t* res);

/*
 * SelfTest
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, SelfTest_Failed, Device_Busy, Parameter_Not_Supported
 * Description: Causes the device to initiate a device-dependent self-test. The first parameter is
 * used to indicate the type of self-test that should be performed.
 */

int ptp_self_test(ptp_dev_t* dev, uint32_t self_test_type, ptp_res_t* res);

/*
 * SetObjectProtection
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Access_Denied, Invalid_ObjectHandle, Invalid_Parameter,
 * Store_Not_Available, Parameter_Not_Supported, Store_Read_Only
 * Description: Sets the write-protection status for the data object referred to in the first
 * parameter to the value indicated in the second parameter. For a description of the
 * ProtectionStatus field, refer to the ObjectInfo dataset described in Clause 5.5.2. If the
 * ProtectionStatus field does not hold a legal value, the ResponseCode should be
 * Invalid_Parameter.
 */

int ptp_set_object_protection(ptp_dev_t* dev, uint32_t object_handle, uint32_t protection_status, ptp_res_t* res);

/*
 * PowerDown
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Device_Busy, Parameter_Not_Supported
 * Description: Causes the device to power down. This will cause all currently open sessions to
 * close.
 */

int ptp_power_down(ptp_dev_t* dev, ptp_res_t* res);

/*
 * GetDevicePropDesc
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Access_Denied, DeviceProp_Not_Supported, Device_Busy,
 * Parameter_Not_Supported
 * Description: Returns the appropriate Property Describing Dataset as indicated by the first
 * parameter
 */

int ptp_get_device_prop_desc(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * GetDevicePropValue
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, DeviceProp_Not_Supported, Device_Busy,
 * Parameter_Not_Supported
 * Description: Returns the current value of a property. The size and format of the data
 * returned from this operation should be determined from the corresponding
 * DevicePropDesc dataset returned from the GetDevicePropDesc operation. The current
 * value of a property can also be retrieved directly from the DevicePropDesc, so this
 * operation is not typically required unless a DevicePropChanged event occurs.
 */

int ptp_get_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * SetDevicePropValue
 * Data Direction: I -> R
 * ResponseCode Options: OK, Session_Not_Open, Invalid_TransactionID, Access_Denied,
 * DeviceProp_Not_Supported, Property_Not_Supported, Invalid_DeviceProp_Format,
 * Invalid_DeviceProp_Value, Device_Busy, Parameter_Not_Supported
 * Description: Sets the current value of the device property indicated by parameter 1 to the
 * value indicated in the data phase of this operation. The format of the property value
 * object sent in the data phase can be determined from the DatatypeCode field of the
 * property’s DevicePropDesc dataset. If the property is not settable, the response
 * Access_Denied should be returned. If the value is not allowed by the device,
 * Invalid_DeviceProp_Value should be returned. If the format or size of the property
 * value is incorrect, Invalid_DeviceProp_Format should be returned.
 */

int ptp_set_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, uint8_t* data, uint32_t len, ptp_res_t* res);

/*
 * ResetDevicePropValue
 * Data Direction: None
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, DeviceProp_Not_Supported, Device_Busy,
 * Parameter_Not_Supported
 * Description: Sets the value of the indicated device property to the factory default setting.
 * The first parameter may be set to 0xFFFFFFFF to indicate that all properties should be
 * reset to their factory default settings.
 */

int ptp_reset_device_prop_value(ptp_dev_t* dev, uint32_t device_prop_code, ptp_res_t* res);

/*
 * TerminateOpenCapture
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Parameter_Not_Supported, Invalid_Parameter,
 * Capture_Already_Terminated
 * Description: This operation is used after an InitiateOpenCapture operation for situations
 * where the capture operation length is open-ended, and determined by the Initiator. This
 * operation is not used for trigger captures, which are invoked using a separate operation,
 * InitiateCapture, described in Clause 10.4.14. This operation allows the termination of
 * one capture operation that is being used to capture many objects over some period of
 * time, such as a burst, or for long single objects such as manually-controlled image
 * exposures, audio captures, or video clips. The first parameter of this operation indicates
 * the TransactionID of the InitiateOpenCapture operation that is being terminated. If the
 * capture has already terminated for some other reason, this operation should return
 * Capture_Already_Terminated. If the TransactionID parameter does not refer to
 * transaction that was an InitiateOpenCapture, this operation should return
 * Invalid_TransactionID.
 */

int ptp_terminate_open_capture(ptp_dev_t* dev, uint32_t transaction_id, ptp_res_t* res);

/*
 * MoveObject
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Store_Read_Only, Store_Not_Available, Invalid_ObjectHandle,
 * Invalid_ParentObject, Device_Busy, Parameter_Not_Supported, Invalid_StorageHandle
 * Description: This operation causes the object to be moved from its location within the
 * hierarchy to a new location indicated by the second and third parameters. If the root of
 * the store is desired, the third parameter may be set to 0x00000000. If the third parameter
 * does not refer to a valid object of type Association, the response Invalid_ParentObject
 * should be returned. If a store is read-only (with or without deletion) the response
 * Store_Read_Only should be returned. This operation does not cause the ObjectHandle
 * of the object that is being moved to change. If the object is to be moved
 */

int ptp_move_object(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, uint32_t object_handle_parent, ptp_res_t* res);

/*
 * CopyObject
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Store_Read_Only, Invalid_ObjectHandle, Invalid_ParentObject,
 * Device_Busy, Store_Full, Parameter_Not_Supported, Invalid_StorageID
 * Description: This operation causes the object to be replicated within the Responder. The
 * first parameter refers to the ObjectHandle of the object that is to be copied. The second
 * parameter refers to the StorageID into which the newly copied object should be placed.
 * The third parameter refers to the ParentObject of where the newly replicated copy should
 * be placed. If the object is to be copied into the root of the store, this value should be set
 * to 0x00000000.
 */

int ptp_copy_object(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_handle, uint32_t object_handle_parent, ptp_res_t* res, ptp_res_params_t* rparams);

/*
 * GetPartialObject
 * Data Direction: R -> I
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_ObjectHandle, Invalid_ObjectFormatCode,
 * Invalid_Parameter, Store_Not_Available, Device_Busy, Parameter_Not_Supported
 * Description: Retrieves a partial object from the device. This operation is optional, and may
 * be used in place of the GetObject operation for devices that support this alternative. If
 * supported, this operation should be generic, and therefore useable with all types of data
 * objects present on the device, including both images and non-image data objects, and
 * should be preceded (although not necessarily immediately) by a GetObjectInfo operation
 * that uses the same ObjectHandle. For this operation, the size fields in the ObjectInfo
 * represent maximum size as opposed to actual size. This operation is not necessary for
 * objects of type Association, as objects of this type are fully qualified by their ObjectInfo
 * dataset.
 * The operation behaves exactly like GetObject, except that the second and third
 * parameters hold the offset in bytes and the number of bytes to obtain starting from the
 * offset, respectively. If the portion of the object that is desired is from the offset to the
 * end, the third parameter may be set to 0xFFFFFFFF. The first response parameter
 * should contain the actual number of bytes of the object sent, not including any wrappers
 * or overhead structures.
 */

int ptp_get_partial_object(ptp_dev_t* dev, uint32_t object_handle, uint32_t offset, uint32_t max_bytes, uint8_t* data, uint32_t len, ptp_res_t* res, ptp_res_params_t* rparams);

/*
 * InitiateOpenCapture
 * Data Direction: N/A
 * ResponseCode Options: OK, Operation_Not_Supported, Session_Not_Open,
 * Invalid_TransactionID, Invalid_StorageID, Store_Full, Invalid_ObjectFormatCode,
 * Invalid_Parameter, Store_Not_Available, Invalid_Code_Format, Device_Busy,
 * Description: Causes the device to initiate the capture of one or more new data objects
 * according to its current device properties, storing the data into the store indicated by the
 * StorageID. If the StorageID is 0x00000000, the object(s) will be stored in a store that is
 * determined by the capturing device. If the particular store specified is not available, or
 * no store is specified and there are no stores available, this operation should return
 * Store_Not_Available.
 * The capturing of new data objects is an asynchronous operation. This operation may be
 * used to implement an Initiate/Terminate mechanism to capture one or more objects over
 * an Initiator-controlled time period, such as a single long still exposure, a series of stills,
 * audio capture, etc. Whether the time period controls the time of capture for a single
 * object or the number of fixed-time objects that are captured is determined by the
 * Responder, and may be a function of the ObjectFormat as well as any appropriate
 * DeviceProperties.
 * A separate operation, InitiateCapture, can be used to
 * support captures that do not require the Initiator to indicate when the capture should
 * terminate.
 * If the ObjectFormatCode in the second operation parameter is 0x00000000, the device
 * shall capture an image in the format that is the default for the device. A successful
 * response to an InitiateOpenCapture operation indicates the Responder’s acceptance of
 * the InitiateOpenCapture operation, and not the completion status of the capture
 * operation.
 * A successful response to the InitiateOpenCapture operation implies that the Responder
 * has started to capture one or more objects. When the Initiator wishes to terminate the
 * capture, it is required to send a TerminateOpenCapture operation. The CaptureComplete
 * event is not used for this operation, as the end of the capture period is determined by the
 * Initiator. As each of the newly captured objects becomes available, the Responder is
 * required to send an ObjectAdded event to the Initiator, indicating the ObjectHandle that
 * is assigned to each. The ObjectAdded event shall contain
 * the TransactionID of the InitiateOpenCapture operation with which it is associated. If, at
 * any time, the store becomes full, the device shall issue a Store_Full event, which shall
 * contain the TransactionID of the InitiateOpenCapture operation that failed to cause a
 * new object to be stored. In the case of multiple objects being captured, each object shall
 * be treated separately, so any object captured before the store becomes full should be
 * retained. Whether or not an object that was partially captured can be retained and used is
 * a function of the device’s behavior and object format. For example, if the device runs
 * out of room while capturing a video clip, it may be able to save the portion that it had
 * room to store. Any object that is retained in these situations should cause an
 * ObjectAdded event to be issued, while any object that is not retained should cause no
 * event to be issued. A Store_Full event effectively terminates the capture, and in these
 * cases, issuing the TerminateOpenCapture operation is not used. If another object capture
 * is occurring when this operation is invoked, the Device_Busy response should be used.
 */

int ptp_initiate_open_capture(ptp_dev_t* dev, uint32_t storage_id, uint32_t object_format_code, ptp_res_t* res);

#endif // __PICTURE_TRANSFER_PROTOCOL_INCLUDED_H__
