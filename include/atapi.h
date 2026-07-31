#include <libusb-1.0/libusb.h>
#include <string.h>

// definitions for SCSI opcodes
#define REQUEST_SENSE_OPCODE 0x03

#define RETRY_MAX 5

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
int scsi_start_stop_unit(libusb_device_handle *handle, uint8_t immed, uint8_t LoEj, uint8_t start);
