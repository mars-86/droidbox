#ifndef __USB_INCLUDED_H__
#define __USB_INCLUDED_H__

#include <linux/usb/ch9.h>
#include <stdint.h>
#include <stdio.h>

typedef int usb_dev_t;

typedef struct usb_parsed_device_descriptor updd_t;
typedef struct usb_parsed_config_descriptor upcd_t;

int usb_clear_feature(int fd, unsigned short feature_selector, unsigned short w_index);
int usb_get_configuration(int fd, unsigned char* buff, int* wlen);
int usb_get_descriptor(int fd, unsigned char type, unsigned char index, unsigned short langid, unsigned char* buff, int* wlen);
int usb_get_interface(int fd, unsigned short interface, unsigned char* buff, int* wlen);
int usb_get_status(int fd, unsigned char recipient, unsigned char w_index, void* buff, int* wlen);
int usb_set_address(int fd, unsigned short address);
int usb_set_configuration(int fd, unsigned short config);
int usb_set_descriptor(int fd, unsigned char type, unsigned char index, unsigned short langid, unsigned char* buff, int* wlen);
int usb_set_interface(int fd, unsigned short interface, unsigned short alternate_setting);
int usb_synch_frame(int fd, unsigned char endpoint, void* buff);
int usb_detach_interface(int fd, unsigned short interface);
int usb_claim_interface(int fd, unsigned short interface);
int usb_release_interface(int fd, unsigned short interface);
int usb_get_driver(int fd, unsigned short interface, char* driver, size_t len);
int usb_bulk_send(int fd, uint16_t endpoint, void* data, uint32_t len);
int usb_bulk_recv(int fd, uint16_t endpoint, void* data, uint32_t len);
void usb_dump_device(int fd, FILE* output);
int usb_get_string(int fd, unsigned char index, unsigned short langid, char* buff);
void* parse_descriptor(unsigned char* src);
void release_descriptor(void* descriptor);
void parse_config_descriptor(unsigned char* stream);

#endif // __USB_INCLUDED_H__
