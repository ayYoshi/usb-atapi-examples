
#include "../include/atapi.h"
#include "../include/usb.h"

int main(int argc, char *argv[]) {
  libusb_device_handle *discreader = NULL;
  if (libusb_init(NULL) != 0) {
    printf("libusb init error\n");
    return 1;
  }

  printf("attempting to open device with VID 0x%04x and PID 0x%04x\n",
         VENDOR_ID, PRODUCT_ID);
  discreader = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
  if (discreader == NULL) {
    printf("could not open device\n");
    return 1;
  }
  int kernal = libusb_kernel_driver_active(discreader, 0);
  if (kernal) {
    printf("USB device is being used by kernal. attempting to detach...\n");
    if (libusb_detach_kernel_driver(discreader, 0) != 0) {
      printf("driver failed to detach\n");
      exit(1);
    }
    printf("detached without issue\n");
  }
  printf("all basic checks passed\n");

  // Prepare data buffer for INQUIRY data
  unsigned char inq_data[INQUIRY_DATA_LENGTH];
  memset(inq_data, 0, INQUIRY_DATA_LENGTH);

  printf("Sending Inquiry Command...\n");
  int rc = scsi_inquiry(discreader, inq_data);
  if (rc != 0) {
    printf("Get Inquiry FAILED with %d. Getting sense...\n", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("could not get sense data\n");
      return -1;
    }
    printf("Got SENSE data:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n", sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }
  scsi_inquiry_pprint(inq_data);
  // Start spinning the disc and read TOC
  rc = scsi_start_stop_unit(discreader, 0, 0, 1);
  if (rc != 0) {
    printf("command failed with %d\n. getting sense...", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("command failed with:\nSENSE KEY: %d\nASC: %d\nASCQ: %d\n", sense.senseKey, sense.ASC, sense.ASCQ);
      return -1;
    }
    printf("command success with:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n", sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }
  printf("command succeeded\n");
  unsigned char toc_data[805];
  memset(toc_data, 0, 805);
  int toc_format = 0b00;
  printf("\nAttempting to read TOC with format %d...\n", toc_format);
  rc = scsi_read_toc(discreader, toc_format, 1, toc_data);
  if (rc != 0) {
    printf("Get TOC FAILED with %d. Getting sense...\n", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("could not get sense data\n");
      return -1;
    }
    printf("Got SENSE data:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n", sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }

  printf("Dumping TOC Data...\r\n\r\n");
  for (int i = 0; i < sizeof(toc_data); i++) {
    //printf("%d : %02x : %c\n", i, toc_data[i], toc_data[i]);
    printf("%c", toc_data[i]);
  }
  printf("\r\n\r\nDone.\n");
  scsi_TOC_pprint(toc_data);
  unsigned char cdtext_data[1024 * 3];
  memset(cdtext_data, 0, 1024 * 3);
  char toc_str[1024];
  toc_format = 0b0101;
  printf("\nAttempting to read CD_TEXT with format %d...\n", toc_format);
  rc = scsi_read_toc(discreader, toc_format, 1, cdtext_data);
  if (rc != 0) {
    printf("Get TOC FAILED with %d. Getting sense...\n", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("could not get sense data\n");
      return -1;
    }
    printf("Got SENSE data:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n", sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }

  printf("Dumping CD_TEXT Data...\r\n\r\n");
  for (int i = 0; i < sizeof(cdtext_data); i++) {
    //printf("%d : %02x : %c\n", i, toc_data[i], toc_data[i]);
    printf("%c", cdtext_data[i]);
  }
  printf("\r\n\r\nDone.\n");
  return 0;
}
