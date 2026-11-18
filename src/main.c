
#include "../include/atapi.h"
#include "../include/usb.h"
#include <libusb-1.0/libusb.h>

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
  // Start the disc
  printf("Spinning up disc...\n");
  if (scsi_start_stop_unit(discreader, 0, 0, 0, 1) != 0) {
    sense_handler(discreader);
    return -1;
  }
  printf("Done\n");
  // Read TOC with forms 0->2
  printf("Opening %s...\n", file_name);
  FILE* logfile = fopen(file_name, "w+");
  if (logfile == NULL) {
    printf("Could Not Open File\n");
    perror(file_name);
    return -1;
  }
  printf("Done\n");
  // Goal: Get number of tracks and read dump all Q-SUBCHANNEl data for the tracks
  size_t bytes_read = 0;
  unsigned char toc_header[4];
  printf("Getting TOC Header...\n");
  if (scsi_read_toc(discreader, 0b00, 0, 0, 4, &bytes_read, toc_header) != 0) {
    sense_handler(discreader);
    return -1;
  }
  uint8_t first_track = toc_header[2];
  uint8_t last_track = toc_header[3];

  printf("First Track: %d\nLast Track: %d\n", first_track, last_track);

  printf("Will attempt to dump Q-SUBCHANNEl data\n");
  for (uint8_t i = first_track; i <= last_track; i++) {
    printf("Getting data for Track %d... ", i);
    unsigned char q_subchannel_data[804] = {0};
    if (scsi_read_subchannel(discreader, i, 1u, 1u, 0x01, 804, &bytes_read, q_subchannel_data) != 0) {
      printf("Read operation failed for Track %d\n", i);
      return -1;
    }
    fprintf(logfile, "\r\nSTART TRACK %d\r\n", i);
    int log_bytes = fwrite(q_subchannel_data, sizeof(unsigned char), bytes_read, logfile);
    if (log_bytes != bytes_read) {
      printf("%s only had %d bytes written (expected %zu)", file_name, log_bytes, bytes_read);
      return -1;
    }
    fprintf(logfile, "\r\nEND TRACK %d\r\n", i);
    printf("Done.\n");
  }
  printf("Dumped all %d tracks without issue\n", (last_track - first_track));

  fflush(logfile);
  fclose(logfile);
  libusb_close(discreader);
  return 0;
}
