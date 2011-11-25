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

#include "udev.h"

#define USE_LIBUDEV

#ifdef USE_LIBUDEV

static pthread_t udev_tid = 0;

static void* udev_thread(void* arg)
{
    slog(SLOG_DEBUG, "udev thread started");
    (void)arg;
    // we only expect the main thread to handle signals
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    struct udev* udev = udev_new();
    if (!udev) {
        slog(SLOG_DEBUG, "can't create udev");
        return NULL;
    }
    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        slog(SLOG_DEBUG, "Can't contact udev");
        return NULL;
    }

    //	udev_monitor_filter_add_match_subsystem_devtype(mon, "hidraw", NULL);

    if (udev_monitor_enable_receiving(mon) < 0) {
        slog(SLOG_DEBUG, "Can't enable udev receiving");
        return NULL;
    }

    // check to see if the udev file-descriptor is blocking. This is necessary
    // for a blocking udev_monitor_receive_device() later on!
    // The behaviour of libudev changed in this resprect some times ago.

    int fd = -1;
    if ((fd = udev_monitor_get_fd(mon)) < 0) {
        slog(SLOG_DEBUG, "Can't get monitor fd");
        return NULL;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        slog(SLOG_DEBUG, "Can't get flags of udev fd");
        return NULL;
    }
    if (flags & O_NONBLOCK) {
        slog(SLOG_INFO, "udev fd is non-blocking");
        flags &= ~O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) < 0) {
            slog(SLOG_DEBUG, "Can't set udev fd to blocking mode");
            return NULL;
        }
    }

    while(true) {
        struct udev_device* device = udev_monitor_receive_device(mon);
        if (!device) {
            slog(SLOG_WARN, "no device from udev, sleeping 1000ms");
            usleep(1000 * 1000);
        }
        else {
            assert(device);
            slog(SLOG_INFO, "new devive");
            const char* s = 0;
            s = udev_device_get_devtype(device);
            if (s) {
                slog(SLOG_INFO, "udev device type: %s", s);
                if (strcmp(s, UDEV_DEVICE_TYPE) == 0) {
                    s = udev_device_get_action(device);
                    if (s) {
                        slog(SLOG_INFO, "udev device action: %s", s);
                        if (strcmp(s, UDEV_ADD_ACTION) == 0) {
                            dbus_signal_device_added();
                        }
                        if (strcmp(s, UDEV_REMOVE_ACTION) == 0) {
                            dbus_signal_device_removed();
                        }
                    }
                }
            }
            udev_device_unref(device);
        }
    }
    return NULL;
}

void udev_start_udev_thread(void)
{
    slog(SLOG_DEBUG, "start udev thread");
    if (pthread_create(&udev_tid, NULL, udev_thread, NULL) < 0){
        slog(SLOG_ERROR, "Can't create udev thread: %s", strerror(errno));
        return;
    }

}

void udev_stop_udev_thread(void) {
    slog(SLOG_DEBUG, "stop udev thread");
    if (udev_tid == 0) {
        slog(SLOG_DEBUG, "no udev thread to stop");
        return;
    }
    if (pthread_cancel(udev_tid) < 0) {
        if (errno == ESRCH) {
            slog(SLOG_WARN, "udev thread was already cancelled");
        }
        else {
            slog(SLOG_WARN, "unknown error from pthread_cancel: %s", strerror(errno));
        }
    }
    if (pthread_join(udev_tid, NULL) < 0) {
        slog(SLOG_ERROR, "pthread_join: %s", strerror(errno));
    }
    udev_tid = 0;
}

#endif
