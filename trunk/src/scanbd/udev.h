/*
 * $Id$
 *
 *  scanbd - KMUX scanner button daemon
 *
 *  Copyright (C) 2008 - 2013  Wilhelm Meier (wilhelm.meier@fh-kl.de)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef UDEV_H
#define UDEV_H

#include "scanbd.h"

#ifdef USE_LIBUDEV
# include <libudev.h>
# define UDEV_ADD_ACTION "add"
# define UDEV_REMOVE_ACTION "remove"
# define UDEV_DEVICE_TYPE "usb_device"
# define UDEV_SLEEP_IF_NO_DEVICE 1000

extern void udev_start_udev_thread(void);
extern void udev_stop_udev_thread(void);

#endif
#endif // UDEV_H
