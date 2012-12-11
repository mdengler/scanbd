// hp3500.c: RTS8801C2 chipset based devices backend
// This file is part of scanbuttond.
// Copyleft )c( 2008 by Jan Michaelis
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include "scanbuttond/scanbuttond.h"
#include "scanbuttond/libusbi.h"
#include "hp3500.h"

static char* backend_name = "HP3500 USB";

#define NUM_SUPPORTED_USB_DEVICES 1

static int supported_usb_devices[NUM_SUPPORTED_USB_DEVICES][3] = {
	// vendor, product, num_buttons
	{ 0x03f0, 0x2205, 3 }  // HP ScanJet 3500C
	//{ 0x03f0, 0x2005, 4 }	// HP ScanJet 3530C
	//{ 0x03f0, 0x2005, 4 }	// HP ScanJet 3570C
};

static char* usb_device_descriptions[NUM_SUPPORTED_USB_DEVICES][2] = {
	{ "Hewlett-Packard", "ScanJet 3500C" }
	//{ "Hewlett-Packard", "ScanJet 3530C" }
	//{ "Hewlett-Packard", "Scanjet 3570C" }
};

/* 
Notes for HP ScanJet 3530C and 3570C:
Since i've no information about the button status register on these devices,
i cannot implement them now, but i think that their structure of the status
register is mostly the same. Its address is 0xD0 too and there is just one
button more, which i'm expecting at bit 5. If you got one of this devices,
feel free to add some lines of code to finally support them.
*/


libusb_handle_t* libusb_handle;
scanner_t* hp3500_scanners = NULL;


// returns -1 if the scanner is unsupported, or the index of the
// corresponding vendor-product pair in the supported_usb_devices array.
int hp3500_match_libusb_scanner(libusb_device_t* device)
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


void hp3500_attach_libusb_scanner(libusb_device_t* device)
{
	const char* descriptor_prefix = "hp3500:libusb:";
	int index = hp3500_match_libusb_scanner(device);
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
	scanner->next = hp3500_scanners;
	hp3500_scanners = scanner;
}


void hp3500_detach_scanners(void)
{
	scanner_t* next;
	while (hp3500_scanners != NULL) {
		next = hp3500_scanners->next;
		free(hp3500_scanners->sane_device);
		free(hp3500_scanners);
		hp3500_scanners = next;
	}
}


void hp3500_scan_devices(libusb_device_t* devices)
{
	int index;
	libusb_device_t* device = devices;
	while (device != NULL) {
		index = hp3500_match_libusb_scanner(device);
		if (index >= 0) 
			hp3500_attach_libusb_scanner(device);
		device = device->next;
	}
}


int hp3500_init_libusb(void)
{
	libusb_device_t* devices;

	libusb_handle = libusb_init();
	devices = libusb_get_devices(libusb_handle);
	hp3500_scan_devices(devices);
	return 0;
}


const char* scanbtnd_get_backend_name(void)
{
	return backend_name;
}


int scanbtnd_init(void)
{
	hp3500_scanners = NULL;

	syslog(LOG_INFO, "hp3500-backend: init");
	return hp3500_init_libusb();
}


int scanbtnd_rescan(void)
{
	libusb_device_t* devices;

	hp3500_detach_scanners();
	hp3500_scanners = NULL;
	libusb_rescan(libusb_handle);
	devices = libusb_get_devices(libusb_handle);
	hp3500_scan_devices(devices);
	return 0;
}


const scanner_t* scanbtnd_get_supported_devices(void)
{
	return hp3500_scanners;
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

int hp3500_read(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_read((libusb_device_t*)scanner->internal_dev_ptr, 
				buffer, bytecount);
			break;
	}
	return -1;
}


int hp3500_write(scanner_t* scanner, void* buffer, int bytecount)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			return libusb_write((libusb_device_t*)scanner->internal_dev_ptr, 
				buffer, bytecount);
			break;
	}
	return -1;
}


void hp3500_flush(scanner_t* scanner)
{
	switch (scanner->connection) {
		case CONNECTION_LIBUSB:
			libusb_flush((libusb_device_t*)scanner->internal_dev_ptr);
			break;
	}
}


int scanbtnd_get_button(scanner_t* scanner)
{
	#define BUFFER_SIZE 4
	unsigned char bytes[BUFFER_SIZE];
	int num_bytes;
	
	bytes[0] = 0x80; //Command: "Read one or more registers"
	bytes[1] = 0xD0; //The Register containing button status
	bytes[2] = 0x00; //Count HH
	bytes[3] = 0x01; //Count LL
	
	/*
	Very detailled informations about the "RTS8801C2" chip can be found at:
	http://projects.troy.rollo.name/rt-scanners/chip.html
	*/

	if (!scanner->is_open)
		return -EINVAL;

	num_bytes = hp3500_write(scanner, (void*)bytes, 4);
	if (num_bytes != 4) {
		syslog(LOG_WARNING, "hp3500-backend: communication error: "
			"write length:%d (expected:%d)", num_bytes, 4);
		hp3500_flush(scanner); 
		return 0;
	}
	num_bytes = hp3500_read(scanner, (void*)bytes, 1);
	if (num_bytes != 1) {
		syslog(LOG_WARNING, "hp3500-backend: communication error: "
			"read length:%d (expected:%d)", num_bytes, 1);
		hp3500_flush(scanner);
		return 0;
	}
	
	/*
	If none button is pressed, register 0xD0 contains 0xFF.
	One button is pressed when its bit is 0 in that register.

	Current button state register 0xD0
	Bit	Bit value	Button
	0	0x01		Clear if transparency adapter (TA) is present
	2	0x04		Copy
	3	0x08		Scan
	4	0x10		Mail
	
	I decided to implement the following behaviour:
	You can only press one button at one time (even if it would be possible to press more simultaneously).
	If more than one button is pressed, this case its treated as if no button is pressed at all.
	*/
	
	bytes[0] = bytes[0] & 0x1C; //Ignore other bits that are not 2,3,4
	
	if(bytes[0] == 0x18) //Copy pressed
		return 1;
	else if(bytes[0] == 0x14) //Scan pressed
		return 2;
	else if(bytes[0] == 0x0C) //Mail pressed
		return 3;
	
	return 0;
	
}


const char* scanbtnd_get_sane_device_descriptor(scanner_t* scanner)
{
	return scanner->sane_device;
}


int scanbtnd_exit(void)
{
	syslog(LOG_INFO, "hp3500-backend: exit");
	hp3500_detach_scanners();
	libusb_exit(libusb_handle);
	return 0;
}

