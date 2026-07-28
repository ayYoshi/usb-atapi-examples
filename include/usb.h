#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>

// harcoded specific values for the CDROM device used for testing
#define VENDOR_ID 0x13fd
#define PRODUCT_ID 0x0840
#define ENDPOINT_IN 0x81
#define ENDPOINT_OUT 0x02

// struct for the USB COMMAND BLOCK WRAPPER
struct usb_cmd_block_wrapper {
  uint32_t dCBWSignature;
  uint32_t dCBWTag;
  uint32_t dCBWDataTransferLength;
  char bCBWFlags;
  char bCBWLUN;
  char CBWCBLength;
  char CBWCB[16];
};

// struct for the USB COMMAND STATUS WRAPPER
struct usb_cmd_status_wrapper {
  uint32_t dCSWSignature;
  uint32_t dCSWTag;
  uint32_t dCSWDataResidue;
  uint8_t bCSWStatus;
};

#define DCBW_SIGNATURE 0x43425355
#define DCSW_SIGNATURE 0x53425355

// packet lengths for USB commands
#define CBW_SIZE 31
#define CSW_SIZE 13

// send USB BULK-ONLY RESET. takes device handle and the IN/OUT endpoints
int usb_bulk_storage_reset(libusb_device_handle *handle);
/* get csw from usb device. returns -1 if the csw is invalid, otherwise returns
 * bCSWStatus. expected_tag is the tag that the csw should contain
 */
int usb_get_csw(libusb_device_handle *handle, int *expected_tag);
