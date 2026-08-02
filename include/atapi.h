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
struct TOC_track_descriptor {
  uint8_t sessionNumber;
  uint8_t cntrl;
  uint8_t track;
  uint8_t MSF[3];
  uint8_t zeroField; //TODO: learn what a ZERO field is
  uint8_t zeroField2[3]; // TODO: idk what this is
};

/* SCSI COMMANDS */

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
// Read TOC of passed-in Track Number. 
int scsi_read_toc(libusb_device_handle *handle, uint8_t format, uint8_t track_number, unsigned char *data);

// Prints inquiry data in a readable format. inquiry_data must be at least 95 bytes long
void scsi_inquiry_pprint(unsigned char* inquiry_data);
// Prints TOC Data in a readable format. Expected format is Format Field 0b00 and MSF bit enabled. Invalid/shortened TOC data may result in a segfault
void scsi_TOC_pprint(unsigned char* toc_data);

