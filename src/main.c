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

  printf("sending sense command...\n");
  int rc = scsi_request_sense(discreader);
  if (rc != 0) {
    printf("command failed with %d\n", rc);
    return -1;
  }
  printf("command passed\n");

  return EXIT_SUCCESS;
}
