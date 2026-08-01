#include <libusb-1.0/libusb.h>
#include <string.h>

// definitions for SCSI opcodes
#define REQUEST_SENSE_OPCODE 0x03

#define RETRY_MAX 5
#define MAX_TOC_DATA_LENGTH 804
#define INQUIRY_DATA_LENGTH 95

struct scsi_sense_data {
  uint8_t senseKey;
  uint8_t ASC;
  uint8_t ASCQ;
};



/* sends scsi sense command to cdrom. Pass a sense_data struct to retrieve SENSE data
 * returns -127 for USB transfer related errors, returns 1 or 2 for csw status.
 * SENSE DATA only valid if return code is 0
 */
int scsi_request_sense(libusb_device_handle *handle, struct scsi_sense_data *sense_data);
int scsi_test_unit_ready(libusb_device_handle *handle);
// prevent_flag should be set to 0 for ALLOW REMOVAL and 1 to PREVENT REMOVAL
int scsi_prevent_allow_medium_removal(libusb_device_handle *handle, uint8_t prevent_flag);
// all flags must be set to 0 or 1
int scsi_start_stop_unit(libusb_device_handle *handle, uint8_t immed, uint8_t LoEj, uint8_t start);
// requests 95 bytes of Inquiry data. The array of data passed in is expected to be 95 bytes of length. Inquiry valid if RC is 0
int scsi_inquiry(libusb_device_handle *handle, unsigned char* data);



//int scsi_read_TOC(libusb_device_handle *handle, )
