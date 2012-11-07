// hp3900.c: RTS8822 & RTS8823 chipset based devices backend
// This file is part of scanbuttond.
// Copyleft )c( 2007 by Jonathan Bravo Lopez
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "hp3900.h"

static char* backend_name = "HP3900 USB";

#define NUM_SUPPORTED_USB_DEVICES 9

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x03f0, 0x2605, 3 },  // HP Scanjet 3800
	{ 0x03f0, 0x2305, 4 },	// HP Scanjet 3970
	{ 0x03f0, 0x2405, 4 },	// HP Scanjet 4070
	{ 0x03f0, 0x4105, 4 },	// HP Scanjet 4370
	{ 0x03f0, 0x2805, 3 },  // HP Scanjet G2710
	{ 0x03f0, 0x4205, 4 },	// HP Scanjet G3010
	{ 0x03f0, 0x4305, 4 },	// HP Scanjet G3110
	{ 0x06dc, 0x0020, 4 },	// Umax Astra 4900/4950
	{ 0x04a5, 0x2211, 3 }	// BenQ 5550T
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Hewlett-Packard", "ScanJet 3800" },
	{ "Hewlett-Packard", "ScanJet 3970" },
	{ "Hewlett-Packard", "Scanjet 4070 Photosmart" },
	{ "Hewlett-Packard", "Scanjet 4370" },
	{ "Hewlett-Packard", "ScanJet G2710" },
	{ "Hewlett-Packard", "Scanjet G3010" },
        { "Hewlett-Packard", "Scanjet G3110" },
	{ "UMAX", "Astra 4900/4950" },
	{ "BenQ", "5550T" }
};


libusb_handle_t* libusb_handle;
scanner_t* hp3900_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int hp3900_match_libusb_scanner(libusb_device_t* device)
{
	int index;
	for (index = 0; index < NUM_SUPPORTED_USB_DEVICES; index++) {
		if (supported_usb_devices[index][0] == device->vendorID &&
				  supported_usb_devices[index][1] == device->productID) {
			break;
		}
	}
	if (index >= NUM_SUPPORTED_USB_DEVICES) return -1;
	return index;
}


void hp3900_attach_libusb_scanner(libusb_device_t* device)
{
	const char* descriptor_prefix = "hp3900:libusb:";
	int index = hp3900_match_libusb_scanner(device);
	if (index < 0) return; // unsupported
	scanner_t* scanner = (scanner_t*)malloc(sizeof(scanner_t));
	scanner->vendor = usb_device_descriptions[index][0];
	scanner->product = usb_device_descriptions[index][1];
	scanner->connection = CONNECTION_LIBUSB;
	scanner->internal_dev_ptr = (void*)device;
	scanner->lastbutton = 0;
	scanner->sane_device = (char*)malloc(strlen(device->location) +
		strlen(descriptor_prefix) + 1);
	strcpy(scanner->sane_device, descriptor_prefix);
	strcat(scanner->sane_device, device->location);
	scanner->num_buttons = supported_usb_devices[index][2];
	scanner->is_open = 0;
	scanner->next = hp3900_scanners;
	hp3900_scanners = scanner;
}


void hp3900_detach_scanners(void)
{
	scanner_t* next;
	while (hp3900_scanners != NULL) {
		next = hp3900_scanners->next;
		free(hp3900_scanners->sane_device);
		free(hp3900_scanners);
		hp3900_scanners = next;
	}
}


void hp3900_scan_devices(libusb_device_t* devices)
{
	int index;
	libusb_device_t* device = devices;
	while (device != NULL) {
		index = hp3900_match_libusb_scanner(device);
		if (index >= 0) 
			hp3900_attach_libusb_scanner(device);
		device = device->next;
	}
}


int hp3900_init_libusb(void)
{
	libusb_device_t* devices;

	libusb_handle = libusb_init();
	devices = libusb_get_devices(libusb_handle);
	hp3900_scan_devices(devices);
	return 0;
}


const char* scanbtnd_get_backend_name(void)
{
	return backend_name;
}


int scanbtnd_init(void)
{
	hp3900_scanners = NULL;

	syslog(LOG_INFO, "hp3900-backend: init");
	return hp3900_init_libusb();
}


int scanbtnd_rescan(void)
{
	libusb_device_t* devices;

	hp3900_detach_scanners();
	hp3900_scanners = NULL;
	libusb_rescan(libusb_handle);
	devices = libusb_get_devices(libusb_handle);
	hp3900_scan_devices(devices);
	return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
	return hp3900_scanners;
}


int scanbtnd_open(scanner_t* scanner)
{
	int result = -ENOSYS;
	if (scanner->is_open)
		return -EINVAL;
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			// if devices have been added/removed, return -ENODEV to
			// make scanbuttond update its device list
			if (libusb_get_changed_device_count() != 0)
				return -ENODEV;
			result = libusb_open((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
	if (result == 0)
		scanner->is_open = 1;
	return result;
}


int scanbtnd_close(scanner_t* scanner)
{
	int result = -ENOSYS;
	if (!scanner->is_open)
		return -EINVAL;
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			result = libusb_close((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
	if (result == 0)
		scanner->is_open = 0;
	return result;
}


int hp3900_read(scanner_t* scanner, void* buffer)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
	                        0xc0, 0x04, 0xe968, 0x0100, (void *)buffer, 0x0002);
			break;
	}
	return -1;
}


void hp3900_flush(scanner_t* scanner)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			libusb_flush((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
}


int scanbtnd_get_button(scanner_t* scanner)
{
    unsigned char bytes[2] = {};
    int num_bytes = 0;
	int button = 0;
    int c = 0;
    int mask = 0;
		
	if (!scanner->is_open)
		return -EINVAL;

	/* Lets read RTS8822 register 0xe968 */
	num_bytes = hp3900_read(scanner, bytes);

	if (num_bytes != 2) {
		syslog(LOG_WARNING, "hp3900-backend: communication error: "
			"read length:%d (expected:%d)", num_bytes, 2);
		hp3900_flush(scanner);
		return 0;
	}

	/* If none button is pressed, register 0xe968 contains 0x3F.
	   RTS8822 seems to support 6 buttons and more than one button can be pressed
	   at the same time. One button is pressed when its bit is 0 in that register. */

	mask = 1;
	for (c = 0; c < scanner->num_buttons; c++) {
		if ((bytes[0] & mask) == 0) {
			button = c + 1;
			break;
		}
		mask = mask << 1;
	}

	return button;
}


const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
	return scanner->sane_device;
}


int scanbtnd_exit(void)
{
	syslog(LOG_INFO, "hp3900-backend: exit");
	hp3900_detach_scanners();
	libusb_exit(libusb_handle);
	return 0;
}

