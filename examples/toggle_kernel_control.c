
#include <libusb-1.0/libusb.h>
#include "../include/atapi.h"
#include "../include/usb.h"

int main() {
  libusb_device_handle *discreader = NULL;
  if (libusb_init(NULL) != 0) {
    printf("libusb init error\n");
    return 1;
  }

  printf("attempting to open device with VID %x and PID %x\n", VENDOR_ID,
         PRODUCT_ID);
  discreader = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
  if (discreader == NULL) {
    printf("could not open device\n");
    return 1;
  }
  printf("opened device YIPEEE\n");
  libusb_device *mydevice = libusb_get_device(discreader);
  int kernal = libusb_kernel_driver_active(discreader, 0);
  printf("device is %s by the kernal\n", kernal ? "claimed" : "not claimed");
  if (!kernal) {
    printf("would you like to relinquish control of %0x:%0x? (y/n)   ", VENDOR_ID,
           PRODUCT_ID);
    char response;
    scanf("%c", &response);
    if (response == 'y' || response == 'Y') {
      if (libusb_attach_kernel_driver(discreader, 0) != 0) {
        printf("could not attach kernal driver\n");
        return 1;
      }
      printf("done\n");
      return 0;
    }
  } else {
    printf("would you like to gain control of %0x:%0x? (y/n)   ", VENDOR_ID,
           PRODUCT_ID);
    char response;
    scanf("%c", &response);
    if (response == 'y' || response == 'Y') {
      if (libusb_detach_kernel_driver(discreader, 0) != 0) {
        printf("could not detach kernal driver\n");
        return 1;
      }
      printf("done\n");
      return 0;
    }
  }
  printf("ok\n");
  return 0;
}
