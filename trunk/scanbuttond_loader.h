/*
 * $Id: scanbd.c 35 2010-11-26 14:15:44Z wimalopaan $
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

#ifndef SCANBUTTOND_LOADER_H
#define SCANBUTTOND_LOADER_H

struct backend;
typedef struct backend backend_t;

struct backend {
    char* (*scanbtnd_get_backend_name)(void);
    int (*scanbtnd_init)(void);
    int (*scanbtnd_rescan)(void);
    scanner_t* (*scanbtnd_get_supported_devices)(void);
    int (*scanbtnd_open)(scanner_t* scanner);
    int (*scanbtnd_close)(scanner_t* scanner);
    int (*scanbtnd_get_button)(scanner_t* scanner);
    char* (*scanbtnd_get_sane_device_descriptor)(scanner_t* scanner);
    int (*scanbtnd_exit)(void);
    void* handle;  // handle for dlopen/dlsym/dlclose

    backend_t* next;
};

int scanbtnd_init();
int scanbtnd_loader_init(void);
void scanbtnd_loader_exit(void);
backend_t* scanbtnd_load_backend(const char* filename);
void scanbtnd_unload_backend(backend_t* backend);
void scanbtnd_set_libdir(const char* dir);

#endif

