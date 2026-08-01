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

  printf("sending prevent/allow medium removal command...\n");
  int rc = scsi_prevent_allow_medium_removal(discreader, 0);
  if (rc != 0) {
    printf("command failed with %d\n. getting sense...", rc);
    struct scsi_sense_data sense;
    int rc = scsi_request_sense(discreader, &sense);
    if (rc != 0) {
      printf("could not get sense data\n");
      return -1;
    }
    printf("Got SENSE data:\nSENSE KEY: 0x%02x\nASC: 0x%02x\nASCQ: 0x%02x\n", sense.senseKey, sense.ASC, sense.ASCQ);
    return -1;
  }
  printf("sending start/stop unit command (disc ejection)...\n");
  // to eject a disc, LoEj must be set to 1 and START must be set to 0
  rc = scsi_start_stop_unit(discreader, 0, 1, 0);
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
  libusb_close(discreader);

  return EXIT_SUCCESS;
}
