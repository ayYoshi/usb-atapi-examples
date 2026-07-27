#include "../include/usb.h"
#include "../include/atapi.h"
#include <libusb-1.0/libusb.h>

int usb_bulk_storage_reset(libusb_device_handle *handle) {
  int rc = 0;
  // data is unused for this command as we request back a length of 0
  unsigned char *data;

  // initiate bulk-only reset
  rc = libusb_control_transfer(handle, 0b00100001, 0xff, 0, 0, data, 0, 5000);
  if (rc != 0) {
    printf("mass storage reset failed with %s\n", libusb_error_name(rc));
    exit(1);
  }
  rc = libusb_clear_halt(handle, ENDPOINT_IN);
  if (rc != 0) {
    printf("Clear halt on EP_IN failed with %s\n", libusb_error_name(rc));
  }
  rc = libusb_clear_halt(handle, ENDPOINT_OUT);
  if (rc != 0) {
    printf("Clear halt on EP_OUT failed with %s\n", libusb_error_name(rc));
  }
  return 0;
}
int usb_get_csw(libusb_device_handle *handle,
                struct usb_cmd_status_wrapper *csw) {
  int bytes_transferred;
  int retry = 0;
  int rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)csw,
                                CSW_SIZE, &bytes_transferred, 5000);
  // LIBUSB_ERROR_PIPE indicates the bytes did not transfer properly and the
  // endpoint halted.  We try to copy the bytes 5 more times before giving up
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)csw,
                              CSW_SIZE, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  printf("Bulk Transfer IN Succeeded with %d bytes transferred\n", bytes_transferred);
  printf("CSW SIGNATURE: 0x%08x\n", csw->dCSWSignature);
  printf("CSW TAG: 0x%08x\n", csw->dCSWTag);
  printf("CSW RESIDUE: 0x%08x\n", csw->dCSWDataResidue);
  printf("CSW STATUS: 0x%02x\n", csw->bCSWStatus);

  return 0;
}







