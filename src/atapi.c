#include "../include/atapi.h"
#include "../include/usb.h"
#include <stdint.h>


int scsi_request_sense(libusb_device_handle *handle) {
  uint32_t tag;
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
  retry = 0;
  rc = usb_send_cbw(handle, cdb, 0x14, &tag);
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
  rc = usb_get_csw(handle, &tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -127;
  }
  // First, we ensure that the CSW returned 0
  if (rc != 0) {
    printf("bCSWStatus returned %d\n", csw.bCSWStatus);
    return csw.bCSWStatus * -1;
  }
  // Since the CSW is valid, we can now prepare and return the SENSE Key
  // by erasing all bits except the ones containing the SENSE key
  return (sense_data[2] & 0b00001111);
  // TODO: Allow the function to optionally return the raw SENSE data via a pointer or smth
}
