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
struct scsi_msf {
  uint8_t minute;
  uint8_t second;
  uint8_t frame;
};

/* SCSI COMMANDS */

/* sends scsi sense command to cdrom. Pass a sense_data struct to retrieve SENSE
 * data returns -127 for USB transfer related errors, returns 1 or 2 for csw
 * status. SENSE DATA only valid if return code is 0
 */
int scsi_request_sense(libusb_device_handle *handle,
                       struct scsi_sense_data *sense_data);
int scsi_test_unit_ready(libusb_device_handle *handle);
// prevent_flag should be set to 0 for ALLOW REMOVAL and 1 to PREVENT REMOVAL
int scsi_prevent_allow_medium_removal(libusb_device_handle *handle,
                                      uint8_t prevent_flag);
// all flags must be set to 0 or 1. If power_state is not 0, LoEj and start will
// be ignored
int scsi_start_stop_unit(libusb_device_handle *handle, uint8_t power_condition,
                         uint8_t immed, uint8_t LoEj, uint8_t start);
// requests 95 bytes of Inquiry data. The array of data passed in is expected
// to be 95 bytes of length. Inquiry valid if RC is 0
int scsi_inquiry(libusb_device_handle *handle, unsigned char *data);
// Read TOC of passed-in Track Number.
int scsi_read_toc(libusb_device_handle *handle, uint8_t format,
                  uint8_t track_number, uint8_t msf, unsigned char *data);
// sends the read_cd_msf command. Will only request one frame at a time.
// Sector type is hardcoded to 0b001 (CD-DA).
int scsi_read_cd_msf(libusb_device_handle *handle, struct scsi_msf addr,
                     uint8_t flag_bits, uint8_t subchannel_selection,
                     unsigned char *data);

// sends the scsi_get_event_status_notification command.
// data buffer will be assumed to have enough space to hold the data
int scsi_get_event_status_notification(libusb_device_handle *handle, uint8_t immed, uint8_t request_bits, uint16_t alloc_length, unsigned char *data);

// Prints inquiry data in a readable format. inquiry_data must be at least 95
// bytes long
void scsi_inquiry_pprint(unsigned char *inquiry_data);
// Prints TOC Data in a readable format. Expected format is Format Field 0b00.
// If TOC read with MSF format, set MSF flag. Invalid/shortened TOC data may
// result in a segfault
void scsi_TOC_pprint(unsigned char *toc_data, uint8_t msf);
// Prints event status headers in a readable way
void scsi_event_notif_pprint(unsigned char *event_data, int length);
// Load Album Info
void scsi_TOC_CDText_parse(unsigned char *toc_data, int num_tracks,
                           char *cdtext_string);

// Get sense if command fails
void sense_handler(libusb_device_handle *handle);
// Increment MSF frame
void increment_msf(struct scsi_msf *addr);
