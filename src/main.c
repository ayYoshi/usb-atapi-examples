
#include "../include/atapi.h"
#include "../include/usb.h"

struct scsi_msf addr = {
    .minute = 0,
    .second = 2,
    .frame = 0,
};

const char *file_name = "log.txt";

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
    sense_handler(discreader);
    return -1;
  }
  scsi_inquiry_pprint(inq_data);
  // Read TOC with forms 0->2
  FILE* logfile = fopen(file_name, "r");
  if (logfile == NULL) {
    printf("Could Not Open File\n");
    perror(file_name);
    return -1;
  }
  for (uint8_t toc_form = 0; toc_form < 3; toc_form++) {
    printf("Reading TOC Form %d... ", toc_form);
    fprintf(logfile, "\r\nSTART TOC FORM %d\r\n", toc_form);
    unsigned char data[804];
    if (scsi_read_toc(discreader, toc_form, 0, 0, data) != 0) {
      sense_handler(discreader);
      fclose(logfile);
      return -1;
    }
    fprintf(logfile, "%s", data);
    fprintf(logfile, "\r\nEND TOC FORM %d\r\n", toc_form);
    printf("Done.\n");
  }
  printf("Dumped TOC Data without issue\n");
  fflush(logfile);
  fclose(logfile);
  return 0;
}
