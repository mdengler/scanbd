// loader.h: dynamic backend library loader
// This file is part of scanbuttond.
// Copyleft )c( 2005-2006 by Bernhard Stiftner
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

#ifndef __LOADER_H_INCLUDED
#define __LOADER_H_INCLUDED

#include "scanbuttond/scanbuttond.h"

#ifndef MODULE_PATH_ENV
#  define MODULE_PATH_ENV        "SCANBUTTOND_MODULE_PATH"
#endif

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

int scanbtnd_loader_init();

void scanbtnd_loader_exit(void);

backend_t* scanbtnd_load_backend(const char* filename);

void scanbtnd_unload_backend(backend_t* backend);

#endif
