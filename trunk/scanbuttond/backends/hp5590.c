/* hp5590.c: HP4570/5550/5590/7650 backend
 * This file is part of scanbuttond.
 * Copyleft )c( 2008 by Ilia Sotnikov <hostcc@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <netinet/in.h> /* For htons() */
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "hp5590.h"

static char* backend_name = "HP5590 USB";

#define NUM_SUPPORTED_USB_DEVICES 4

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] =
{
       /* vendor, product, num_buttons */
       { 0x03f0, 0x1305, 5 },  /* HP Scanjet 4570 */
       { 0x03f0, 0x1305, 5 },  /* HP Scanjet 5550 */
       { 0x03f0, 0x1705, 5 },  /* HP Scanjet 5590 */
       { 0x03f0, 0x1805, 5 },  /* HP Scanjet 7650 */
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] =
{
       { "Hewlett-Packard", "ScanJet 4570" },
       { "Hewlett-Packard", "ScanJet 5550" },
       { "Hewlett-Packard", "Scanjet 5590" },
       { "Hewlett-Packard", "Scanjet 7650" },
};

static libusb_handle_t* libusb_handle;
static scanner_t* hp5590_scanners = NULL;

/* returns -1 if the scanner is unsupported, or the index of the
 * corresponding vendor-product pair in the supported_usb_devices array.
 */
static int
hp5590_match_libusb_scanner(libusb_device_t* device)
{
       int index;

       for (index = 0; index < NUM_SUPPORTED_USB_DEVICES; index++)
       {
               if (supported_usb_devices[index][0] == device->vendorID &&
                       supported_usb_devices[index][1] == device->productID)
                       break;
       }

       if (index >= NUM_SUPPORTED_USB_DEVICES)
               return -1;

       return index;
}

static void
hp5590_attach_libusb_scanner (libusb_device_t* device)
{
       const char* descriptor_prefix = "hp5590:libusb:";
       int             index;

       index = hp5590_match_libusb_scanner(device);
       /* Unsupported */
       if (index < 0)
               return;

       scanner_t* scanner = (scanner_t*) malloc (sizeof(scanner_t));
       scanner->vendor = usb_device_descriptions[index][0];
       scanner->product = usb_device_descriptions[index][1];
       scanner->connection = CONNECTION_LIBUSB;
       scanner->internal_dev_ptr = (void*) device;
       scanner->lastbutton = 0;
       scanner->sane_device = (char*) malloc (strlen (device->location) +
                                                  strlen (descriptor_prefix) + 1);
       strcpy (scanner->sane_device, descriptor_prefix);
       strcat (scanner->sane_device, device->location);
       scanner->num_buttons = supported_usb_devices[index][2];
       scanner->is_open = 0;
       scanner->next = hp5590_scanners;
       hp5590_scanners = scanner;
}

static void
hp5590_detach_scanners (void)
{
       scanner_t* next;
       while (hp5590_scanners != NULL)
       {
               next = hp5590_scanners->next;
               free (hp5590_scanners->sane_device);
               free (hp5590_scanners);
               hp5590_scanners = next;
       }
}

static void
hp5590_scan_devices (libusb_device_t* devices)
{
       int index;
       libusb_device_t* device = devices;
       while (device != NULL)
       {
               index = hp5590_match_libusb_scanner (device);
               if (index >= 0)
                       hp5590_attach_libusb_scanner (device);
               device = device->next;
       }
}

static int
hp5590_init_libusb (void)
{
       libusb_device_t* devices;

       libusb_handle = libusb_init ();
       devices = libusb_get_devices (libusb_handle);
       hp5590_scan_devices (devices);
       return 0;
}

static void
hp5590_flush (scanner_t* scanner)
{
       switch (scanner->connection)
       {
               case CONNECTION_LIBUSB:
                       libusb_flush ((libusb_device_t*) scanner->internal_dev_ptr);
                       break;
       }
}

const char*
scanbtnd_get_backend_name (void)
{
       return backend_name;
}

int
scanbtnd_init (void)
{
       hp5590_scanners = NULL;

       syslog (LOG_INFO, "hp5590-backend: init");
       return hp5590_init_libusb ();
}

int
scanbtnd_rescan (void)
{
       libusb_device_t* devices;

       hp5590_detach_scanners ();
       hp5590_scanners = NULL;
       libusb_rescan (libusb_handle);
       devices = libusb_get_devices (libusb_handle);
       hp5590_scan_devices (devices);

       return 0;
}

const scanner_t*
scanbtnd_get_supported_devices (void)
{
       return hp5590_scanners;
}

int
scanbtnd_open (scanner_t* scanner)
{
       int result = -ENOSYS;

       if (scanner->is_open)
               return -EINVAL;

       switch (scanner->connection)
       {
               case CONNECTION_LIBUSB:
                       /* if devices have been added/removed, return -ENODEV to
                        * make scanbuttond update its device list
                        */
                       if (libusb_get_changed_device_count () != 0)
                               return -ENODEV;
                       result = libusb_open ((libusb_device_t*) scanner->internal_dev_ptr);
                       break;
       }

       if (result == 0)
               scanner->is_open = 1;

       return result;
}

int
scanbtnd_close(scanner_t* scanner)
{
       int result = -ENOSYS;

       if (!scanner->is_open)
               return -EINVAL;

       switch (scanner->connection)
       {
               case CONNECTION_LIBUSB:
                       result = libusb_close ((libusb_device_t*) scanner->internal_dev_ptr);
                       break;
       }

       if (result == 0)
               scanner->is_open = 0;

       return result;
}

/* Taken from sane-backends/include/sane/sanei_usb.h */

#define USB_DIR_OUT                    0x00
#define USB_DIR_IN                     0x80

/* Taken from sane-backends/backends/hp5590_cmds.c */

/* Button flags */
#define BUTTON_FLAG_EMAIL      1 << 15
#define BUTTON_FLAG_COPY       1 << 14
#define BUTTON_FLAG_DOWN       1 << 13
#define BUTTON_FLAG_MODE       1 << 12
#define BUTTON_FLAG_UP         1 << 11
#define BUTTON_FLAG_FILE       1 << 9
#define BUTTON_FLAG_POWER      1 << 5
#define BUTTON_FLAG_SCAN       1 << 2
#define BUTTON_FLAG_COLLECT    1 << 1
#define BUTTON_FLAG_CANCEL     1 << 0

#define CMD_BUTTON_STATUS      0x0020

/* Taken from sane-backends/backends/hp5590_low.c */

/* Flags for hp5590_cmd() */
#define CMD_IN                         1 << 0  /* Indicates IN direction, otherwise - OUT */
#define CMD_VERIFY                     1 << 1  /* Requests last command verification */

/* Core flags for hp5590_cmd() - they indicate so called CORE commands */
#define CORE_NONE                       0      /* No CORE operation */
#define CORE_DATA                      1 << 0  /* Operate on CORE data */

/* CORE status flag - ready or not */
#define CORE_FLAG_NOT_READY    1 << 1

/* Structure describing control URB */
struct usb_in_usb_ctrl_setup
{
       u_int8_t  bRequestType;
       u_int8_t  bRequest;
       u_int16_t wValue;
       u_int16_t wIndex;
       u_int16_t wLength;
} __attribute__ ((packed));

static int
hp5590_get_ack (scanner_t *scanner)
{
       u_int8_t        status;
       int                     ret;

       /* Check if USB-in-USB operation was accepted */
       ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                         USB_DIR_IN | USB_TYPE_VENDOR,
                                                         0x0c, 0x8e, 0x20,
                                                         &status, sizeof (status));
       if (ret <= 0)
       {
      syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error getting acknowledge");
      return -1;
    }

       /* Check if we received correct acknowledgement */
       if (status != 0x01)
       {
               syslog (LOG_ERR, "hp5590-backend: USB-in-USB: not accepted (status %u)", status);
               return -1;
       }

       return 0;
}

static int
hp5590_control_msg (scanner_t *scanner,
                               int requesttype, int request,
                                       int value, int index, unsigned char *bytes,
                               int size, int core_flags)
{
       struct usb_in_usb_ctrl_setup    ctrl;
       int                                                             ret;
       unsigned int                                    len;
       unsigned char                                   *ptr;
       u_int8_t                                                ack;
       u_int8_t                                                response;

       /* IN (read) operation will be performed */
       if (requesttype & USB_DIR_IN)
       {
               /* Prepare USB-in-USB control message */
               memset (&ctrl, 0, sizeof (ctrl));
               ctrl.bRequestType = 0xc0;
               ctrl.bRequest = request;
               ctrl.wValue = htons (value);
               ctrl.wIndex = htons (index);
               ctrl.wLength = size;

               /* Send USB-in-USB control message */
               ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                 USB_DIR_OUT | USB_TYPE_VENDOR,
                                                                 0x04, 0x8f, 0x00,
                                                                 (unsigned char *) &ctrl, sizeof (ctrl));
               if (ret <= 0)
               {
                       syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error sending control message");
                       return -1;
               }

               ret = hp5590_get_ack (scanner);
               if (ret != 0)
                       return ret;

               len = size;
               ptr = bytes;
               /* Data is read in 8 byte portions */
               while (len)
               {
                       unsigned int next_packet_size;
                       next_packet_size = 8;
                       if (len < 8)
                               next_packet_size = len;

                       /* Read USB-in-USB data */
                       ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                         USB_DIR_IN | USB_TYPE_VENDOR,
                                                                         core_flags & CORE_DATA ? 0x0c : 0x04,
                                                                         0x90, 0x00,
                                                                         ptr, next_packet_size);
                       if (ret <= 0)
                       {
                               syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error reading data");
                               return -1;
                       }

                       ptr += next_packet_size;
                       len -= next_packet_size;
               }

               /* Confirm data reception */
               ack = 0;
               ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                 USB_DIR_OUT | USB_TYPE_VENDOR,
                                                                 0x0c, 0x8f, 0x00,
                                                                 &ack, sizeof (ack));
               if (ret <= 0)
               {
                       syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error confirming data reception");
                       return -1;
               }

               ret = hp5590_get_ack (scanner);
               if (ret != 0)
                       return ret;
       }

       /* OUT (write) operation will be performed */
       if (!(requesttype & USB_DIR_IN))
       {
               /* Prepare USB-in-USB control message */
               memset (&ctrl, 0, sizeof (ctrl));
               ctrl.bRequestType = 0x40;
               ctrl.bRequest = request;
               ctrl.wValue = htons (value);
               ctrl.wIndex = htons (index);
               ctrl.wLength = size;

               /* Send USB-in-USB control message */
               ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                 USB_DIR_OUT | USB_TYPE_VENDOR,
                                                                 0x04, 0x8f, 0x00,
                                                                 (unsigned char *) &ctrl, sizeof (ctrl));
               if (ret <= 0)
               {
                       syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error sending control message");
                       return -1;
               }

               ret = hp5590_get_ack (scanner);
               if (ret != 0)
                       return ret;

               len = size;
               ptr = bytes;
               /* Data is sent in 8 byte portions */
               while (len)
               {
                       unsigned int next_packet_size;
                       next_packet_size = 8;
                       if (len < 8)
                               next_packet_size = len;

                       /* Send USB-in-USB data */
                       ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                         USB_DIR_OUT | USB_TYPE_VENDOR,
                                                                         core_flags & CORE_DATA ? 0x04 : 0x0c,
                                                                         0x8f, 0x00, ptr, next_packet_size);
                       if (ret <= 0)
                       {
                               syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error sending data");
                               return -1;
                       }

                       /* CORE data is acknowledged packet by packet */
                       if (core_flags & CORE_DATA)
                       {
                               ret = hp5590_get_ack (scanner);
                               if (ret != 0)
                                       return ret;
                       }

                       ptr += next_packet_size;
                       len -= next_packet_size;
               }

               /* Normal (non-CORE) data is acknowledged after its full transmission */
               if (!(core_flags & CORE_DATA))
               {
                       ret = hp5590_get_ack (scanner);
                       if (ret != 0)
                               return ret;
               }

               /* Getting  response after data transmission */
               ret = libusb_control_msg ((libusb_device_t *) scanner->internal_dev_ptr,
                                                                 USB_DIR_IN | USB_TYPE_VENDOR,
                                                                 0x0c, 0x90, 0x00,
                                                                 &response, sizeof (response));
               if (ret <= 0)
               {
                       syslog (LOG_ERR, "hp5590-backend: USB-in-USB: error getting response");
                       return -1;
               }

               /* Necessary response after normal (non-CORE) data is 0x00 */
               if (response != 0x00)
               {
                       syslog (LOG_ERR, "hp5590-backend: USB-in-USB: invalid response received "
                               "(needed 0x00, got %04x)", response);

                       return -1;
               }
       }

       return 0;
}

static int
hp5590_verify_last_cmd (scanner_t *scanner, unsigned int cmd)
{
       u_int16_t               verify_cmd;
       unsigned int    last_cmd;
       unsigned int    core_status;
       int                             ret;

       /* Read last command along with CORE status */
       ret = hp5590_control_msg (scanner,
                                                         USB_DIR_IN,
                                                         0x04, 0xc5, 0x00,
                                                         (unsigned char *) &verify_cmd,
                                                         sizeof (verify_cmd), CORE_NONE);
       if (ret != 0)
               return ret;

       /* Last command - minor byte */
       last_cmd = verify_cmd & 0xff;
       /* CORE status - major byte */
       core_status = (verify_cmd & 0xff00) >> 8;

       /* Verify last command */
       if ((cmd & 0x00ff) != last_cmd)
       {
               syslog (LOG_ERR, "hp5590-backend: USB-in-USB: command verification failed: "
                               "expected 0x%04x, got 0x%04x", cmd, last_cmd);
               return -1;
       }

       /* Return value depends on CORE status */
       return core_status & CORE_FLAG_NOT_READY ? -1 : 0;
}

static int
hp5590_cmd (scanner_t *scanner, unsigned int flags,
                       unsigned int cmd, unsigned char *data, unsigned int size,
                       unsigned int core_flags)
{
       int ret;

       ret = hp5590_control_msg (scanner,
                                                         flags & CMD_IN ? USB_DIR_IN : USB_DIR_OUT,
                                                         0x04, cmd, 0x00, data, size, core_flags);
       if (ret != 0)
               return ret;

       ret = 0;
       /* Verify last command if requested */
       if (flags & CMD_VERIFY)
               ret = hp5590_verify_last_cmd (scanner, cmd);

       return ret;
}

int
scanbtnd_get_button(scanner_t* scanner)
{
       int             button = 0;
       u_int16_t       button_status;
       int                     ret;

       if (!scanner->is_open)
               return -EINVAL;
       
       ret = hp5590_cmd (scanner, CMD_IN | CMD_VERIFY,
                                         CMD_BUTTON_STATUS,
                                         (unsigned char *) &button_status,
                                         sizeof (button_status), CORE_NONE);
       if (ret != 0) {
               hp5590_flush (scanner);
               return 0;
       }

       /* Network order */
       button_status = ntohs (button_status);

       if (button_status & BUTTON_FLAG_SCAN)
               button = 1;

       if (button_status & BUTTON_FLAG_COLLECT)
               button = 2;

       if (button_status & BUTTON_FLAG_FILE)
               button = 3;

       if (button_status & BUTTON_FLAG_EMAIL)
               button = 4;

       if (button_status & BUTTON_FLAG_COPY)
               button = 5;

       return button;
}

const char*
scanbtnd_get_sane_device_descriptor (scanner_t* scanner)
{
       return scanner->sane_device;
}

int
scanbtnd_exit (void)
{
       syslog (LOG_INFO, "hp5590-backend: exit");
       hp5590_detach_scanners ();
       libusb_exit (libusb_handle);
       return 0;
}