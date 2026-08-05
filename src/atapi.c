#include "../include/atapi.h"
#include "../include/usb.h"
#include <stdint.h>

int scsi_request_sense(libusb_device_handle *handle,
                       struct scsi_sense_data *sense_data) {
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
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data_buf, 20,
                            &bytes_transferred, 5000);
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
    return rc * -1;
    ;
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
int scsi_start_stop_unit(libusb_device_handle *handle, uint8_t immed,
                         uint8_t LoEj, uint8_t start) {
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
int scsi_inquiry(libusb_device_handle *handle, unsigned char *data) {
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
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred,
           INQUIRY_DATA_LENGTH);
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
int scsi_read_toc(libusb_device_handle *handle, uint8_t format,
                  uint8_t track_number, unsigned char *data) {
  int rc;
  int bytes_transferred;
  int retry = 0;
  uint32_t expected_tag;
  unsigned char cdb[12];
  // Make sure format is not greater than 0b1010
  if (format > 0b1010) {
    printf("Invalid Format\n");
    return -1;
  }
  // There can only be 100 tracks, so any track request above 99 is invalid
  if (track_number > 100) {
    printf("Track Number too large\n");
    return -1;
  }
  memset(cdb, 0, 12);
  // Read TOC Opcode: 0x12
  cdb[0] = 0x43;
  // Set MSF to 0
  cdb[1] = 0b00000000;
  cdb[2] = format;
  cdb[6] = track_number;
  // To allocate 804 bytes, we must put the LSB in cdb[8] and MSB in cdb[7]
  cdb[8] = MAX_TOC_DATA_LENGTH & 0x00FF;
  cdb[7] = (MAX_TOC_DATA_LENGTH >> 8);
  if (usb_send_cbw(handle, cdb, MAX_TOC_DATA_LENGTH, &expected_tag) != 0) {
    printf("couldn't send command to USB device");
    return -1;
  }
  rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data,
                            MAX_TOC_DATA_LENGTH, &bytes_transferred, 5000);
  while ((rc == LIBUSB_ERROR_PIPE) && (retry < RETRY_MAX)) {
    printf("clearing halt...\n");
    int rc2 = libusb_clear_halt(handle, ENDPOINT_IN);
    if (rc2 != 0) {
      printf("stall clear failed with %d\n", rc2);
      return -1;
    }
    rc = libusb_bulk_transfer(handle, ENDPOINT_IN, (unsigned char *)data,
                              MAX_TOC_DATA_LENGTH, &bytes_transferred, 5000);
    retry++;
  }
  if (rc != 0) {
    printf("Bulk Transfer IN failed with %s and %d bytes transferred\n",
           libusb_error_name(rc), bytes_transferred);
    return -1;
  }
  if (bytes_transferred != MAX_TOC_DATA_LENGTH) {
    // We log the error, but do not stop execution as different drives may send
    // different INQUIRY lengths
    printf("Host only send %d bytes (expected %d)\n", bytes_transferred,
           MAX_TOC_DATA_LENGTH);
  }
  printf("Bulk Transfer IN succeeded with %d bytes transferred\n",
         bytes_transferred);
  rc = usb_get_csw(handle, &expected_tag);
  if (rc < 0) {
    printf("invalid command status wrapper\n");
    return -1;
  }
  return rc;

  return rc;
}

void scsi_inquiry_pprint(unsigned char *inquiry_data) {
  printf("\n*** INQUIRY DATA ***\n");
  int peripheral_device_type = inquiry_data[0] & 0b00011111;
  printf("Peripheral Device Type: 0x%02x: ", peripheral_device_type);
  switch (peripheral_device_type) {
  case 0x00:
    printf("DIRECT ACCESS DEVICE");
    break;
  case 0x05:
    printf("CD-ROM DEVICE");
    break;
  case 0x07:
    printf("OPTICAL MEMORY DEVICE");
    break;
  default:
    printf("UNKNOWN DEVICE TYPE");
    break;
  }
  printf("\n");

  (inquiry_data[1] >> 7) ? printf("Device can remove media\n")
                         : printf("Device cannot remove media\n");

  printf("ISO Version: %d\n", (inquiry_data[2] >> 6));
  printf("ECMA Version: %d\n", ((inquiry_data[2] & 0b00111000) >> 3));
  printf("ANSI Version: %d\n", (inquiry_data[2] & 0b00000111));

  printf("ATAPI Version: %d\n", (inquiry_data[3] >> 4));
  printf("Response Data Format: %d\n", ((inquiry_data[3] & 0b00001111)));

  char vendor_id[9];
  memcpy(vendor_id, inquiry_data + 8, 8);
  printf("Vendor ID: %s\n", vendor_id);
  char product_id[17];
  memcpy(product_id, inquiry_data + 16, 16);
  printf("Product ID: %s\n", product_id);
  char product_rev[5];
  memcpy(product_rev, inquiry_data + 32, 4);
  printf("Product Revision: %s\n", product_rev);
}
void scsi_TOC_pprint(unsigned char *toc_data) {
  printf("\n*** TOC DATA ***\n");
  uint16_t data_length;
  data_length = (toc_data[1] | (toc_data[0] << 8));
  printf("Data Length: %d (0x%04x)\n", data_length, data_length);
  printf("Starting Track: %d\n", toc_data[2]);
  printf("Ending Track: %d\n\n", toc_data[3]);
  // This number includes the leadout track, which is why we add 2 instead of 1
  int total_track_number = toc_data[3] - toc_data[2] + 2;
  unsigned char *track_ptr = toc_data + 4;
  for (int i = 0; i < total_track_number; i++) {
    if (track_ptr[2] == 0xAA) {
      printf("Track Number: 0xAA (leadout)\n");
    } else {
      printf("Track Number: %d\n", track_ptr[2]);
    }
    printf("ADDR: 0x%02x\n", track_ptr[1] >> 4);
    printf("CONTROL: 0x%02x\n", track_ptr[1] & 0b00001111);
    // Track Address has the LSB at Byte 7 and MSB at Byte 4
    uint32_t track_address = track_ptr[7] | (track_ptr[6] << 8) |
                             (track_ptr[5] << 16) | (track_ptr[4] << 24);
    printf("Track Address (LBA): %d\n\n", track_address);
    track_ptr += 8;
  }
}
void scsi_TOC_CDText_parse(unsigned char *toc_data, int num_tracks, char *cdtext_string) {
  uint16_t data_length;
  data_length = (toc_data[1] | (toc_data[0] << 8));
  // text_p is set to the start of the first cd_text pack
  unsigned char* text_pointer = toc_data + 4;
  int track_count = 0;
  while (track_count < num_tracks + 1) {
    // for now we will not parse the first 4 bytes
    text_pointer += 4;
    for (int i = 0; i < 12; i++) {
      char s = *(text_pointer);
      if (s == '\0') {
        track_count++;
      }
      printf("%p: %c (%d)\n", (text_pointer+i), s, s);
      *cdtext_string = s;
      cdtext_string++;
      text_pointer++;
    }
    // skip the last 2 bytes
    text_pointer += 2;
  }
}







