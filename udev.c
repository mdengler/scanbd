/*
 * $Id$
 *
 *  scanbd - KMUX scanner button daemon
 *
 *  Copyright (C) 2008 - 2012  Wilhelm Meier (wilhelm.meier@fh-kl.de)
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


#ifndef USE_LIBUDEV
//#define USE_LIBUDEV
#endif

#include "udev.h"

#ifdef USE_LIBUDEV

static pthread_t udev_tid = 0;
static struct udev* udev = NULL;
static struct udev_monitor* mon = NULL;

static int udev_init() {
    slog(SLOG_DEBUG, "udev init");
    udev = udev_new();
    if (!udev) {
        slog(SLOG_DEBUG, "can't create udev");
        return -1;
    }
    slog(SLOG_DEBUG, "get udev monitor");
    mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        slog(SLOG_DEBUG, "Can't contact udev");
        return -1;
    }

    if (udev_monitor_enable_receiving(mon) < 0) {
        slog(SLOG_DEBUG, "Can't enable udev receiving");
        return -1;
    }

    // check to see if the udev file-descriptor is blocking. This is necessary
    // for a blocking udev_monitor_receive_device() later on!
    // The behaviour of libudev changed in this resprect some times ago. And changed
    // again back... So be explicit here!

    int fd = -1;
    if ((fd = udev_monitor_get_fd(mon)) < 0) {
        slog(SLOG_DEBUG, "Can't get monitor fd");
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        slog(SLOG_DEBUG, "Can't get flags of udev fd: %s", strerror(errno));
        return -1;
    }
    if (flags & O_NONBLOCK) {
        slog(SLOG_INFO, "udev fd is non-blocking, now setting to blocking mode");
        flags &= ~O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) < 0) {
            slog(SLOG_DEBUG, "Can't set udev fd to blocking mode: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

static int udev_close() {
    if (mon) {
        slog(SLOG_DEBUG, "close udev monitor");
        udev_monitor_unref(mon);
    }
    mon = NULL;
    if (udev) {
        slog(SLOG_DEBUG, "close udev");
        udev_unref(udev);
    }
    udev = NULL;
    return 0;
}

static void udev_thread_cleanup(void* arg) {
    slog(SLOG_DEBUG, "cleanup device handler");
    struct udev_device* device = *((struct udev_device**)arg);
    if (device) {
        slog(SLOG_DEBUG, "cleanup device");
        udev_device_unref(device);
    }
}

static void* udev_thread(void* arg)
{
    slog(SLOG_DEBUG, "udev thread started");
    (void)arg;
    // we only expect the main thread to handle signals
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    while(true) {
        assert(mon);

        struct udev_device* device = 0;
        pthread_cleanup_push(udev_thread_cleanup, &device);
        device = udev_monitor_receive_device(mon);

        pthread_testcancel();

        if (!device) {
            slog(SLOG_WARN, "no device from udev, sleeping 1000ms");
            usleep(1000 * UDEV_SLEEP_IF_NO_DEVICE);
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
            device = 0;
        }
        pthread_cleanup_pop(0);
    }
    pthread_exit(NULL);
}

void udev_start_udev_thread(void)
{
    if (udev_init() < 0) {
        slog(SLOG_ERROR, "Can't init udev");
        return;
    }
    slog(SLOG_DEBUG, "start udev thread");
    if (udev_tid != 0) {
        slog(SLOG_DEBUG, "udev thread already running");
        return;
    }
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
    slog(SLOG_DEBUG, "join udev thread");
    if (pthread_join(udev_tid, NULL) < 0) {
        slog(SLOG_ERROR, "pthread_join: %s", strerror(errno));
    }
    udev_tid = 0;
    if (udev_close() < 0) {
        slog(SLOG_ERROR, "Can't close udev");
    }
}

#endif
