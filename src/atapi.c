#include "../include/atapi.h"
#include "../include/usb.h"

uint32_t tag = 0;

int scsi_request_sense(libusb_device_handle *handle) {
  struct usb_cmd_block_wrapper cbw;
  struct usb_cmd_status_wrapper csw;
  int rc;
  int retry = 0;
  int bytes_transferred;
  unsigned char cdb[12];
  // sense_data is for debugging purposes
  unsigned char sense_data[20];
  memset(cdb, 0, 12);
  memset(sense_data, 0, 20);
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

  retry = 0;
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)sense_data,
                            20, &bytes_transferred, 5000);
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    printf("clearing halt...\n");
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -127;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)sense_data,
                              20, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -127;
  }
  if (bytes_transferred != 20) {
    // We log the error, but do not stop execution as different drives may send different SENSE lengths
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred, 20);
  }
  printf("Bulk Transfer IN succeeded with %d bytes transferred\n", bytes_transferred);
  rc = usb_get_csw(handle, &csw);
  if (rc != 0) {
    printf("invalid command status wrapper\n");
    return -127;
  }
  // First, we ensure that the CSW returned 0
  if (csw.bCSWStatus != 0) {
    printf("bCSWStatus returned %d\n", csw.bCSWStatus);
    return csw.bCSWStatus * -1;
  }
  // Since the CSW is valid, we can now prepare and return the SENSE Key
  // by erasing all bits except the ones containing the SENSE key
  return (sense_data[2] & 0b00001111);
  // TODO: Allow the function to optionally return the raw SENSE data via a pointer or smth
}
