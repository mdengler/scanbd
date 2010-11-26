/*
 * $Id$
 *
 *  scanbd - KMUX scanner button daemon
 *
 *  Copyright (C) 2008  Wilhelm Meier (wilhelm.meier@fh-kl.de)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef SCANBD_DBUS_H
#define SCANBD_DBUS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "common.h"
#include "slog.h"

#include <dbus-1.0/dbus/dbus.h>
//#include <dbus-1.0/dbus/dbus-bus.h>

#ifdef USE_HAL
#include <hal/libhal.h>
#endif

// dbus interface
#define SCANBD_DBUS_ADDRESS         "de.kmux.scanbd.server"
#define SCANBD_DBUS_OBJECTPATH      "/de/kmux/scanbd/server"
#define SCANBD_DBUS_INTERFACE       "de.kmux.scanbd.server"

// dbus methods handled by scandb 
#define SCANBD_DBUS_METHOD_ACQUIRE  "aquire"
#define SCANBD_DBUS_METHOD_RELEASE  "release"
#define SCANBD_DBUS_METHOD_TRIGGER  "trigger"

// dbus signals send out 
#define SCANBD_DBUS_SIGNAL_TRIGGER	"trigger"
#define SCANBD_DBUS_SIGNAL_SANED_BEGIN	"saned_begin"
#define SCANBD_DBUS_SIGNAL_SANED_END	"saned_end"
#define SCANBD_DBUS_SIGNAL_SCAN_BEGIN	"scan_begin"
#define SCANBD_DBUS_SIGNAL_SCAN_END	"scan_end"

// dbus signals we receive
#define DBUS_HAL_INTERFACE          "org.freedesktop.Hal.Manager"
#define DBUS_HAL_SIGNAL_DEV_ADDED   "DeviceAdded"
#define DBUS_HAL_SIGNAL_DEV_REMOVED "DeviceRemoved"

#define HAL_SCANNER_CAPABILITY      "scanner"

extern void dbus_send_signal(const char*, const char*);
extern void dbus_send_signal_argv(const char*, char**);
extern void dbus_send_signal_argv_async(const char*, char**);

extern bool dbus_init(void);
extern void dbus_send(void);

extern void dbus_call_method(const char*, const char*);
extern void dbus_call_trigger(unsigned int, unsigned int);

extern void dbus_start_dbus_thread(void);
extern void dbus_stop_dbus_thread(void);

#endif
