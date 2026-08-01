#include "../include/atapi.h"
#include "../include/usb.h"
#include <stdint.h>

int scsi_request_sense(libusb_device_handle *handle, struct scsi_sense_data *sense_data) {
  uint32_t tag;
  int rc;
  int retry = 0;
  int bytes_transferred;
  unsigned char cdb[12];
  // sense_data is for debugging purposes
  unsigned char data_buf[20];
  memset(cdb, 0, 12);
  memset(data_buf, 0, 20);
  // set up the CDB with the correct opcode and allocation length
  cdb[0] = REQUEST_SENSE_OPCODE;
  cdb[4] = 0x14;
  // set up cbw
  retry = 0;
  rc = usb_send_cbw(handle, cdb, 0x14, &tag);
  if (rc != 0) {
    printf("couldnt send SENSE command (%d)\n", rc);
    return -127;
  }
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data_buf,
                            20, &bytes_transferred, 5000);
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    printf("clearing halt...\n");
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -127;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data_buf,
                              20, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -127;
  }
  if (bytes_transferred != 20) {
    // We log the error, but do not stop execution as different drives may send
    // different SENSE lengths
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred, 20);
  }
  printf("Bulk Transfer IN succeeded with %d bytes transferred\n",
         bytes_transferred);
  rc = usb_get_csw(handle, &tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -127;
  }
  // First, we ensure that the CSW returned 0
  if (rc != 0) {
    printf("bCSWStatus returned %d\n", rc);
    return rc * -1;;
  }
  // Since the CSW is valid, we can now prepare and return the SENSE Key
  // by erasing all bits except the ones containing the SENSE key
  sense_data->senseKey = (data_buf[2] & 0b00001111);
  sense_data->ASC = data_buf[12];
  sense_data->ASCQ = data_buf[13];
  // TODO: Allow the function to optionally return the raw SENSE data via a
  // pointer or smth
  return 0;
}
int scsi_prevent_allow_medium_removal(libusb_device_handle *handle,
                                      uint8_t prevent_flag) {
  int rc;
  uint32_t expected_tag;
  unsigned char cdb[12];
  if ((prevent_flag != 1) && (prevent_flag != 0)) {
    printf("invalid prevent_flag\n");
    return -1;
  }
  memset(cdb, 0, 12);
  // Prevent/Allow Medium Removal Opcode: ox1E
  cdb[0] = 0x1E;
  cdb[4] = prevent_flag;

  if (usb_send_cbw(handle, cdb, 0, &expected_tag) != 0) {
    printf("couldn't send command to USB device");
    return -1;
  }
  rc = usb_get_csw(handle, &expected_tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -127;
  }
  return rc;
}
int scsi_start_stop_unit(libusb_device_handle *handle, uint8_t immed, uint8_t LoEj, uint8_t start) {
  int rc;
  uint32_t expected_tag;
  unsigned char cdb[12];
  if ((immed != 1) && (immed != 0)) {
    printf("invalid immed\n");
    return -1;
  }
  if ((LoEj != 1) && (LoEj != 0)) {
    printf("invalid LoEj\n");
    return -1;
  }
  if ((start != 1) && (start != 0)) {
    printf("invalid start\n");
    return -1;
  }
  memset(cdb, 0, 12);
  // Start/Stop Unit opcode: 0x1B
  cdb[0] = 0x1B;
  cdb[1] = immed;
  // set bit 0 to START and bit 1 to LoEj
  cdb[4] = ((start) | (LoEj << 1));
  if (usb_send_cbw(handle, cdb, 0, &expected_tag) != 0) {
    printf("couldn't send command to USB device");
    return -1;
  }
  rc = usb_get_csw(handle, &expected_tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -1;
  }
  return rc;

}
int scsi_inquiry(libusb_device_handle *handle, unsigned char* data) {
  int rc;
  int bytes_transferred;
  int retry = 0;
  uint32_t expected_tag;
  unsigned char cdb[12];

  memset(cdb, 0, 12);
  // Inquiry Opcode: 0x12
  cdb[0] = 0x12;
  // Set allocation length to INQUIRY_DATA_LENGTH
  cdb[4] = INQUIRY_DATA_LENGTH;
  if (usb_send_cbw(handle, cdb, INQUIRY_DATA_LENGTH, &expected_tag) != 0) {
    printf("couldn't send command to USB device");
    return -1;
  }
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data,
                            INQUIRY_DATA_LENGTH, &bytes_transferred, 5000);
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    printf("clearing halt...\n");
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data,
                              INQUIRY_DATA_LENGTH, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  if (bytes_transferred != INQUIRY_DATA_LENGTH) {
    // We log the error, but do not stop execution as different drives may send
    // different INQUIRY lengths
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred, INQUIRY_DATA_LENGTH);
  }
  printf("Bulk Transfer IN succeeded with %d bytes transferred\n",
         bytes_transferred);
  rc = usb_get_csw(handle, &expected_tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -1;
  }
  return rc;
}









