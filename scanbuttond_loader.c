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

#include "scanbd.h"
#include "scanbuttond_loader.h"
#include <dlfcn.h>

// this file is basicly the same as loader.c from the scanbuttond-project,
// but modified to meet the needs of scanbd

static char lib_dir[PATH_MAX] = CFG_DIR;

void scanbtnd_set_libdir(const char* dir){
    strncpy(lib_dir, dir, PATH_MAX);
}


int scanbtnd_loader_init(){
    return 0;
}

void scanbtnd_loader_exit(void){
    return;
}

backend_t* scanbtnd_load_backend(const char* filename){
    const char* error;
    void* dll_handle;

    char dll_path[PATH_MAX];
    strncpy(dll_path, lib_dir, PATH_MAX);
    strncat(dll_path, "/", PATH_MAX - strlen(dll_path));
    strncat(dll_path, filename, PATH_MAX - strlen(dll_path));
    strncat(dll_path, ".so", PATH_MAX - strlen(dll_path));

    slog(SLOG_INFO, "Loading %s", dll_path);

    if (!(dll_handle = dlopen(dll_path, RTLD_NOW|RTLD_LOCAL))) {
	slog(SLOG_ERROR, "Can't load %s: %s", dll_path, dlerror());
	return NULL;
    }
    dlerror();  // Clear any existing error

    backend_t* backend = (backend_t*)malloc(sizeof(backend_t));
    assert(backend);

    backend->handle = (void*)dll_handle;

    backend->scanbtnd_get_backend_name = dlsym(dll_handle, "scanbtnd_get_backend_name");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_init = dlsym(dll_handle, "scanbtnd_init");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_rescan = dlsym(dll_handle, "scanbtnd_rescan");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_get_supported_devices = dlsym(dll_handle, "scanbtnd_get_supported_devices");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_open = dlsym(dll_handle, "scanbtnd_open");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_close = dlsym(dll_handle, "scanbtnd_close");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_get_button = dlsym(dll_handle, "scanbtnd_get_button");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_get_sane_device_descriptor = dlsym(dll_handle, "scanbtnd_get_sane_device_descriptor");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    backend->scanbtnd_exit = dlsym(dll_handle, "scanbtnd_exit");
    if ((error = dlerror()) != NULL) {
	slog(SLOG_ERROR, "Can't find symbol: %s", error);
	goto cleanup;
    }
    return backend;

    cleanup:
	assert(dll_handle);
	dlclose(dll_handle);
	free(backend);
	return NULL;
}

void scanbtnd_unload_backend(backend_t* backend){
    if (backend->handle != NULL) {
	dlclose(backend->handle);
	backend->handle = NULL;
	free(backend);
    }
}

