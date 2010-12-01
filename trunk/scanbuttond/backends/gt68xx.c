// gt68xx.c: gt68xx device backend
// This file is part of scanbuttond.
// Copyleft )c( 2005 by Hans Verkuil
// Copyleft )c( 2005-2006 by Bernhard Stiftner
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

// essentialy most of these files are just copies of each other apart of the
// device name. the only real changes are in the parsing routines.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "gt68xx.h"

static char* backend_name = "gt68xx USB";

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x0458, 0x2014, 5 }
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Genius", "Colorpage Vivid4" }
};


libusb_handle_t* libusb_handle;
scanner_t* gt68xx_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int gt68xx_match_libusb_scanner(libusb_device_t* device)
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


void gt68xx_attach_libusb_scanner(libusb_device_t* device)
{
	const char* descriptor_prefix = "gt68xx:libusb:";
	int index = gt68xx_match_libusb_scanner(device);
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
	scanner->next = gt68xx_scanners;
	gt68xx_scanners = scanner;
}


void gt68xx_detach_scanners(void)
{
	scanner_t* next;
	while (gt68xx_scanners != NULL) {
		next = gt68xx_scanners->next;
		free(gt68xx_scanners->sane_device);
		free(gt68xx_scanners);
		gt68xx_scanners = next;
	}
}


void gt68xx_scan_devices(libusb_device_t* devices)
{
	int index;
	libusb_device_t* device = devices;
	while (device != NULL) {
		index = gt68xx_match_libusb_scanner(device);
		if (index >= 0) 
			gt68xx_attach_libusb_scanner(device);
		device = device->next;
	}
}


int gt68xx_init_libusb(void)
{
	libusb_device_t* devices;

	libusb_handle = libusb_init();
	devices = libusb_get_devices(libusb_handle);
	gt68xx_scan_devices(devices);
	return 0;
}


const char* scanbtnd_get_backend_name(void)
{
	return backend_name;
}


int scanbtnd_init(void)
{
	gt68xx_scanners = NULL;

	syslog(LOG_INFO, "gt68xx-backend: init");
	return gt68xx_init_libusb();
}


int scanbtnd_rescan(void)
{
	libusb_device_t* devices;

	gt68xx_detach_scanners();
	gt68xx_scanners = NULL;
	libusb_rescan(libusb_handle);
	devices = libusb_get_devices(libusb_handle);
	gt68xx_scan_devices(devices);
	return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
	return gt68xx_scanners;
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


int gt68xx_read(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
				0xc0, 0x04, 0x2011, 0x3f00, buffer, bytecount);
			break;
	}
	return -1;
}


int gt68xx_write(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_control_msg((libusb_device_t*)scanner->internal_dev_ptr,
			    0x40, 0x04, 0x2010, 0x3f40, buffer, bytecount);
			break;
	}
	return -1;
}

void gt68xx_flush(scanner_t* scanner)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			libusb_flush((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
}


int scanbtnd_get_button(scanner_t* scanner)
{
	unsigned char bytes[255] = {};
	int num_bytes;
	int button = 0;
	
	bytes[0] = 0x29;
	bytes[1] = 0x01;
	bytes[2] = 0xa8;
	bytes[3] = 0x61;
	bytes[0x38] = 0x2e;
	
	if (!scanner->is_open)
		return -EINVAL;

	if (gt68xx_write(scanner, (void*)bytes, 0x40) < 0)
	    return 0;
	if ((num_bytes = gt68xx_read(scanner, (void*)bytes, 0x40)) < 0)
	    return 0;
	
	if (bytes[0] != 0x00 || bytes[1] != 0x29)
	    return 0;

	if (bytes[2] & 0x02)
	    button = 1; // scan
	if (bytes[2] & 0x01)
	    button = 2; // copy
	if (bytes[2] & 0x04)
	    button = 3; // ocr
	if (bytes[2] & 0x08)
	    button = 4; // mail
	if (bytes[2] & 0x10)
	    button = 5; // fax
	return button;
}


const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
	return scanner->sane_device;
}


int scanbtnd_exit(void)
{
	syslog(LOG_INFO, "gt68xx-backend: exit");
	gt68xx_detach_scanners();
	libusb_exit(libusb_handle);
	return 0;
}

