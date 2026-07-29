#include "../include/usb.h"
#include "../include/atapi.h"
#include <libusb-1.0/libusb.h>

uint32_t tag = 0;

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
int usb_get_csw(libusb_device_handle *handle, uint32_t *expected_tag) {
  struct usb_cmd_status_wrapper csw;
  int bytes_transferred;
  int retry = 0;
  int rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)&csw,
                                CSW_SIZE, &bytes_transferred, 5000);
  // LIBUSB_ERROR_PIPE indicates the bytes did not transfer properly and the
  // endpoint halted.  We try to copy the bytes 5 more times before giving up
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)&csw,
                              CSW_SIZE, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  if (bytes_transferred != CSW_SIZE) {
    printf("Host read %d bytes (expected %d)\n", bytes_transferred, CSW_SIZE);
    return -1;
  }
  // All CSW packets must have the signature 0x53425355
  if (csw.dCSWSignature != DCSW_SIGNATURE) {
    printf("Invalid dCSWSignature of 0x%08x\n", csw.dCSWSignature);
    return -1;
  }
  if (csw.dCSWTag != *expected_tag) {
    printf("Invalid Tag (is %d, expected %d\n", csw.dCSWTag, *expected_tag);
    return -1;
  }
  // we ignore dCSWDataResidue as many devices set it incorrectly

  /*
  printf("CSW SIGNATURE: 0x%08x\n", csw.dCSWSignature);
  printf("CSW TAG: 0x%08x\n", csw.dCSWTag);
  printf("CSW RESIDUE: 0x%08x\n", csw.dCSWDataResidue);
  printf("CSW STATUS: 0x%02x\n", csw.bCSWStatus);
  */

  return csw.bCSWStatus;
}
int usb_send_cbw(libusb_device_handle *handle, unsigned char *cbwcb, uint32_t dCBWDataTransferLength, uint32_t *returned_tag) {
  struct usb_cmd_block_wrapper cbw;
  struct usb_cmd_status_wrapper csw;
  int rc;
  int retry = 0;
  int bytes_transferred;
  // sense_data is for debugging purposes
  unsigned char sense_data[20];
  memset(sense_data, 0, 20);
  cbw.dCBWSignature = DCBW_SIGNATURE;
  cbw.dCBWTag = tag;
  *returned_tag = tag;
  // increment tag to avoid duplicate tags
  tag++;
  // length of CSW + length of SCSI sense data
  cbw.dCBWDataTransferLength = dCBWDataTransferLength;
  // bit 7 is set to 1, indicating a device-to-host transfer
  cbw.bCBWFlags = 0b10000000;
  cbw.bCBWLUN = 0;
  // length of CDB is 12
  cbw.CBWCBLength = 12;
  memset(cbw.CBWCB, 0, 16);
  // copy CDB into CBWCB
  memcpy(cbw.CBWCB, cbwcb, 12);

  rc = libusb_bulk_transfer(handle, ENDPOINT_OUT, (unsigned char *)&cbw,
                            CBW_SIZE, &bytes_transferred, 5000);
  // LIBUSB_ERROR_PIPE indicates the bytes did not transfer properly and the
  // endpoint halted.  We try to copy the bytes 5 more times before giving up
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    int rc2 = libusb_clear_halt(handle, ENDPOINT_OUT);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -127;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_OUT, (unsigned char *)&cbw,
                              CBW_SIZE, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer OUT failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -127;
  }
  if (bytes_transferred != CBW_SIZE) {
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred, CBW_SIZE);
    return -127;
  }
  printf("Bulk Transfer OUT succeeded with %d bytes transferred\n", bytes_transferred);
  return 0;
}












