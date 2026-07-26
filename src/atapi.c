#include "../include/atapi.h"
#include "../include/usb.h"
#include <libusb-1.0/libusb.h>
#define RETRY_MAX 5

uint32_t tag;

int scsi_request_sense(libusb_device_handle *handle) {
  struct usb_cmd_block_wrapper cbw;
  struct usb_cmd_status_wrapper csw;
  int rc;
  int retry = 0;
  int bytes_transferred;
  unsigned char cdb[12];
  // data_buf is for debugging purposes
  unsigned char data_buf[128];
  memset(cdb, 0, 12);
  memset(data_buf, 0, 128);
  // set up the CDB with the correct opcode and allocation length
  cdb[0] = REQUEST_SENSE_OPCODE;
  cdb[4] = 0x14;
  // set up cbw
  cbw.dCBWSignature = DCBW_SIGNATURE;
  cbw.dCBWTag = tag;
  // increment tag to avoid duplicate tags
  tag++;
  // length of CSW + length of SCSI sense data
  cbw.dCBWDataTransferLength = 13 + 0x14;
  // bit 7 is set to 1, indicating a device-to-host transfer
  cbw.bCBWFlags = 0b10000000;
  cbw.bCBWLUN = 0;
  // length of CDB is 12
  cbw.CBWCBLength = 12;
  memset(&cbw.CBWCB, 0, 16);
  // copy CDB into CBWCB
  memcpy(cbw.CBWCB, cdb, 12);

  rc = libusb_bulk_transfer(handle, ENDPOINT_OUT, (unsigned char *)&cbw,
                            CBW_SIZE, &bytes_transferred, 5000);
  // LIBUSB_ERROR_PIPE indicates the bytes did not transfer properly and the
  // endpoint halted.  We try to copy the bytes 5 more times before giving up
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    int rc2 = libusb_clear_halt(handle, ENDPOINT_OUT);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_OUT, (unsigned char *)&cbw,
                              CBW_SIZE, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer OUT failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  // TODO: Verify device recieved the correct amount of bytes
  // since the transfer of the command worked, we now try to read the sense data
  // from the drive

  retry = 0;
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)&data_buf,
                            128, &bytes_transferred, 5000);
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    printf("clearing halt...\n");
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)&data_buf,
                              128, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  // TODO: Parse the DATA sent by Sense cmd
  for (int i = 0; i < 128; i++) {
    printf("%02x", data_buf[i]);
  }
  return 0;
}
