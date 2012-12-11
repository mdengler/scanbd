#ifndef UDEV_H
#define UDEV_H

//#define USE_LIBUDEV

#include "scanbd.h"

#ifdef USE_LIBUDEV
#include <libudev.h>
#define UDEV_ADD_ACTION "add"
#define UDEV_REMOVE_ACTION "remove"
#define UDEV_DEVICE_TYPE "usb_device"
#define UDEV_SLEEP_IF_NO_DEVICE 1000

extern void udev_start_udev_thread(void);
extern void udev_stop_udev_thread(void);

#endif
#endif // UDEV_H
