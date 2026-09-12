
#include "../include/atapi.h"
#include "../include/usb.h"

struct scsi_msf addr = {
    .minute = 0,
    .second = 2,
    .frame = 0,
};


int main(int argc, char *argv[]) {
  libusb_device_handle *discreader = NULL;
  if (libusb_init(NULL) != 0) {
    printf("libusb init error\n");
    return 1;
  }
  printf("Attempting to open device with VID 0x%04x and PID 0x%04x\n",
         VENDOR_ID, PRODUCT_ID);
  discreader = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
  if (discreader == NULL) {
    printf("Could not open device\n");
    return 1;
  }
  int kernal = libusb_kernel_driver_active(discreader, 0);
  if (kernal) {
    printf("USB device is being used by kernal. attempting to detach...\n");
    if (libusb_detach_kernel_driver(discreader, 0) != 0) {
      printf("Driver failed to detach\n");
      return 1;
    }
    printf("Detached without issue\n");
  }
  printf("All basic checks passed\n");

  usb_bulk_storage_reset(discreader);

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
    printf("Got SENSE data:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n",
           sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }
  scsi_inquiry_pprint(inq_data);
  // Start spinning the disc and read TOC
  rc = scsi_start_stop_unit(discreader, 0, 0, 1, 0);
  if (rc != 0) {
    printf("command failed with %d\n. getting sense...", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("command failed with:\nSENSE KEY: %d\nASC: %d\nASCQ: %d\n",
             sense.senseKey, sense.ASC, sense.ASCQ);
      return -1;
    }
    printf(
        "command success with:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n",
        sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }

  return 0;
}
