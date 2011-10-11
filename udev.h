#ifndef UDEV_H
#define UDEV_H

//#define USE_LIBUDEV

#include "scanbd.h"
#ifdef USE_LIBUDEV
#include <libudev.h>
#endif

#define UDEV_ADD_ACTION "add"
#define UDEV_REMOVE_ACTION "remove"
#define UDEV_DEVICE_TYPE "usb_device"

extern void udev_start_udev_thread(void);
extern void udev_stop_udev_thread(void);

#endif // UDEV_H
