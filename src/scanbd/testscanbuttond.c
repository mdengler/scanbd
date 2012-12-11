/*
 * $Id: slog.h 6 2008-12-15 10:56:05Z wimalopaan $
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

#include "scanbd.h"
#ifdef USE_SCANBUTTOND
#include "scanbuttond_loader.h"
#include "scanbuttond_wrapper.h"
#endif

#define DEF_POLL_DELAY			333000L
#define MIN_POLL_DELAY			1000L
#define DEF_RETRY_DELAY			2000000L
#define MIN_RETRY_DELAY			10000L
#define BUF_SIZE			256

cfg_t* cfg = NULL;

static char* connection_names[NUM_CONNECTIONS] =
{ "none", "libusb" };

backend_t* backend;
static long poll_delay;
static long retry_delay;
static int killed = 0;

char* scanbtnd_get_connection_name(int connection)
{
    return connection_names[connection];
}

// Ensures a graceful exit on SIGHUP/SIGTERM/SIGINT/SIGSEGV
void sighandler(int i)
{
    killed = 1;
    slog(SLOG_INFO, "received signal %d", i);
    scbtn_shutdown();
    exit(i == SIGTERM ? EXIT_SUCCESS : EXIT_FAILURE);
}

void list_devices(scanner_t* devices)
{
    scanner_t* dev = devices;
    while (dev != NULL) {
        slog(SLOG_INFO, "found scanner: vendor=\"%s\", product=\"%s\", connection=\"%s\", sane_name=\"%s\"",
             dev->vendor, dev->product, scanbtnd_get_connection_name(dev->connection),
             backend->scanbtnd_get_sane_device_descriptor(dev));
        dev = dev->next;
    }
}

int main()
{
    int button;
    int result;
    scanner_t* scanners;
    scanner_t* scanner;

    slog_init("test");

    debug = true;
    debug_level = 7;

    scanbtnd_set_libdir("./scanbuttond/backends");

    if (scanbtnd_loader_init() != 0) {
        slog(SLOG_INFO, "Could not initialize module loader!\n");
        exit(EXIT_FAILURE);
    }

    backend = scanbtnd_load_backend("meta");
    if (!backend) {
        slog(SLOG_INFO, "Unable to load backend library\n");
        scanbtnd_loader_exit();
        exit(EXIT_FAILURE);
    }

    if (backend->scanbtnd_init() != 0) {
        slog(SLOG_ERROR, "Error initializing backend. Terminating.");
        exit(EXIT_FAILURE);
    }

    scanners = backend->scanbtnd_get_supported_devices();

    if (scanners == NULL) {
        slog(SLOG_WARN, "no known scanner found yet, " \
             "waiting for device to be attached");
    }

    list_devices(scanners);

    signal(SIGTERM, &sighandler);
    signal(SIGHUP, &sighandler);
    signal(SIGINT, &sighandler);
    signal(SIGSEGV, &sighandler);
    signal(SIGCHLD, SIG_IGN);

    slog(SLOG_INFO, "scanbuttond started");

    // main loop
    while (killed == 0) {
        if (scanners == NULL) {
            slog(SLOG_DEBUG, "rescanning devices...");
            backend->scanbtnd_rescan();
            scanners = backend->scanbtnd_get_supported_devices();
            if (scanners == NULL) {
                slog(SLOG_DEBUG, "no supported devices found. rescanning in a few seconds...");
                usleep(retry_delay);
                continue;
            }
            scanners = backend->scanbtnd_get_supported_devices();
            continue;
        }

        scanner = scanners;
        while (scanner != NULL) {
            result = backend->scanbtnd_open(scanner);
            if (result != 0) {
                slog(SLOG_WARN, "scanbtnd_open failed, error code: %d", result);
                if (result == -ENODEV) {
                    // device has been disconnected, force re-scan
                    slog(SLOG_INFO, "scanbtnd_open returned -ENODEV, device rescan will be performed");
                    scanners = NULL;
                    usleep(retry_delay);
                    break;
                }
                usleep(retry_delay);
                break;
            }

            slog(SLOG_INFO, "get");
            button = backend->scanbtnd_get_button(scanner);
            slog(SLOG_INFO, "value: %d", button);
            backend->scanbtnd_close(scanner);

            if ((button > 0) && (button != scanner->lastbutton)) {
                slog(SLOG_INFO, "button %d has been pressed.", button);
                scanner->lastbutton = button;
            }
            if ((button == 0) && (scanner->lastbutton > 0)) {
                slog(SLOG_INFO, "button %d has been released.", scanner->lastbutton);
                scanner->lastbutton = button;
            }
            scanner = scanner->next;
        }

        usleep(poll_delay);

    }

    slog(SLOG_WARN, "exited main loop!?!");

    scbtn_shutdown();
    exit(EXIT_SUCCESS);
}

