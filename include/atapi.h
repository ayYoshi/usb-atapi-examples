#include <libusb-1.0/libusb.h>
#include <string.h>


// definitions for SCSI opcodes
#define REQUEST_SENSE_OPCODE 0x03

#define RETRY_MAX 5

/* sends scsi sense command to cdrom. 
 * returns -1 for USB related errors, otherwise returns SCSI sense key
 */
int scsi_request_sense(libusb_device_handle *handle);
