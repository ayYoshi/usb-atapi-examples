#include "../include/usb.h"

int bulk_storage_reset(libusb_device_handle *handle, int endpoint_in,
                       int endpoint_out) {
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
