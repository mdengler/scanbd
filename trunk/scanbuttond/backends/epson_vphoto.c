// epson_vphoto.c: Epson ESC/V device backend
// This file is part of scanbuttond.
// Copyleft )c( 2011 by Philipp Klaus, based on code from Bernhard Stiftner
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
#include "epson_vphoto.h"

#define	ESC        	0x1B		/* ASCII value for ESC */

static char* backend_name = "Epson VX00 Photo USB";

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x04B8, 0x012E, 4 }	// Epson Perfection V200
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Epson", "Perfection V200"}
};


static libusb_handle_t* libusb_handle;
static scanner_t* epsonvp_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int epsonvp_match_libusb_scanner(libusb_device_t* device)
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


void epsonvp_attach_libusb_scanner(libusb_device_t* device)
{
	const char* descriptor_prefix = "epkowa:interpreter:";
	int index = epsonvp_match_libusb_scanner(device);
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
	scanner->next = epsonvp_scanners;
	epsonvp_scanners = scanner;
}


void epsonvp_detach_scanners(void)
{
	scanner_t* next;
	while (epsonvp_scanners != NULL) {
		next = epsonvp_scanners->next;
		free(epsonvp_scanners->sane_device);
		free(epsonvp_scanners);
		epsonvp_scanners = next;
	}
}


void epsonvp_scan_devices(libusb_device_t* devices)
{
	int index;
	libusb_device_t* device = devices;
	while (device != NULL) {
		index = epsonvp_match_libusb_scanner(device);
		if (index >= 0)
			epsonvp_attach_libusb_scanner(device);
		device = device->next;
	}
}


int epsonvp_init_libusb(void)
{
	libusb_device_t* devices;

	libusb_handle = libusb_init();
	devices = libusb_get_devices(libusb_handle);
	epsonvp_scan_devices(devices);
	return 0;
}


const char* scanbtnd_get_backend_name(void)
{
	return backend_name;
}


int scanbtnd_init(void)
{
	epsonvp_scanners = NULL;

	syslog(LOG_INFO, "epson-vphoto-backend: init");
	return epsonvp_init_libusb();
}


int scanbtnd_rescan(void)
{
	libusb_device_t *devices;

	epsonvp_detach_scanners();
	epsonvp_scanners = NULL;
	libusb_rescan(libusb_handle);
	devices = libusb_get_devices(libusb_handle);
	epsonvp_scan_devices(devices);
	return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
	return epsonvp_scanners;
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


int epsonvp_read(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_read((libusb_device_t*)scanner->internal_dev_ptr, 
				buffer, bytecount);
			break;
	}
	return -1;
}


int epsonvp_write(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_write((libusb_device_t*)scanner->internal_dev_ptr, 
				buffer, bytecount);
			break;
	}
	return -1;
}


void epsonvp_flush(scanner_t* scanner)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			libusb_flush((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
}


int scanbtnd_get_button(scanner_t* scanner)
{
	#define BUFFER_SIZE 16
	unsigned char bytes[BUFFER_SIZE];
	//int rcv_len;
	int num_bytes;
	if (!scanner->is_open)
		return -EINVAL;

	bytes[0] = 0x1E;
	bytes[1] = 0x85;
	bytes[2] = '\0';
	num_bytes = epsonvp_write(scanner, (void*)bytes, 2);
	if (num_bytes != 2) {
		syslog(LOG_WARNING, "epson-vphoto-backend: communication error: "
			"write length:%d (expected:%d)", num_bytes, 2);
		epsonvp_flush(scanner); 
		return 0;
	}

	num_bytes = epsonvp_read(scanner, (void*)bytes, 1);
	if (num_bytes != 1) {
		syslog(LOG_WARNING, "epson-vphoto-backend: communication error: "
			"read length:%d (expected:%d)", num_bytes, 1);
		epsonvp_flush(scanner);
		return 0;
	}
	
	return bytes[0];
}


const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
	return scanner->sane_device;
}


int scanbtnd_exit(void)
{
	syslog(LOG_INFO, "epson-vphoto-backend: exit");
	epsonvp_detach_scanners();
	libusb_exit(libusb_handle);
	return 0;
}
