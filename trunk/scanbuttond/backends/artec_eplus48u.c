// artec_eplus48u.c : scanbuttond backend for Artec E+48U scanners
// This file is part of scanbuttond.
// Copyleft )c( 2006 by Joel Fuster
//
// Built primarily from "niash" scanbuttond backend.  
// Scanner button interface details from SANE backend "artec_eplus48u"
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "artec_eplus48u.h"

static char* backend_name = "Artec E+48U USB";

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x05d8, 0x4003, 4 } // Artec E+48u
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Artec", "E+48U"}
};


static libusb_handle_t* libusb_handle;
static scanner_t* artec_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int artec_match_libusb_scanner(libusb_device_t* device)
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


void artec_attach_libusb_scanner(libusb_device_t* device)
{
	const char* descriptor_prefix = "artec_eplus48u:libusb:";
	int index = artec_match_libusb_scanner(device);
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
	scanner->next = artec_scanners;
	artec_scanners = scanner;
}


void artec_detach_scanners(void)
{
	scanner_t* next;
	while (artec_scanners != NULL) {
		next = artec_scanners->next;
		free(artec_scanners->sane_device);
		free(artec_scanners);
		artec_scanners = next;
	}
}


void artec_scan_devices(libusb_device_t* devices)
{
	int index;
	libusb_device_t* device = devices;
	while (device != NULL) {
		index = artec_match_libusb_scanner(device);
		if (index >= 0)
			artec_attach_libusb_scanner(device);
		device = device->next;
	}
}


int artec_init_libusb(void)
{
	libusb_device_t* devices;

	libusb_handle = libusb_init();
	devices = libusb_get_devices(libusb_handle);
	artec_scan_devices(devices);
	return 0;
}

int artec_control_msg(scanner_t* scanner, int requesttype, int request,
					  int value, int index, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
									   requesttype, request, value, index, buffer,
									   bytecount);
			break;
	}
	return -1;
}


const char* scanbtnd_get_backend_name(void)
{
	return backend_name;
}


int scanbtnd_init(void)
{
	artec_scanners = NULL;

	syslog(LOG_INFO, "artec_eplus48u-backend: init");
	return artec_init_libusb();
}


int scanbtnd_rescan(void)
{
	libusb_device_t *devices;

	artec_detach_scanners();
	artec_scanners = NULL;
	libusb_rescan(libusb_handle);
	devices = libusb_get_devices(libusb_handle);
	artec_scan_devices(devices);
	return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
	return artec_scanners;
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


int scanbtnd_get_button(scanner_t* scanner)
{

  unsigned char bytes[64];
	int num_bytes;

	const unsigned char request_packet[64] = { 
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x74, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};   

	if (!scanner->is_open)
		return -EINVAL;

	// Send request
	num_bytes = artec_control_msg(scanner,
		0x40, 0x01, 0x2012, 0x3f40, (void *)request_packet, 64);
	if (num_bytes < 0) return 0;

	// Get response
	num_bytes = artec_control_msg(scanner,
		0xc0, 0x01, 0x2013, 0x3f00, (void *)bytes, 64);
	if (num_bytes < 0) return 0;

	switch (bytes[2]) {
	case 0x08: // scan 
		return 1;
	case 0x02: // copy
		return 2;
	case 0x04: // ocr
		return 3;
	case 0x01: // email
		return 4;
	}
	return 0;
}

const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
	return scanner->sane_device;
}


int scanbtnd_exit(void)
{
	syslog(LOG_INFO, "artec_eplus48u-backend: exit");
	artec_detach_scanners();
	libusb_exit(libusb_handle);
	return 0;
}

