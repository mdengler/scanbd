/*
 * $Id: scanbd.c 35 2010-11-26 14:15:44Z wimalopaan $
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
#include "scanbuttond_loader.h"
#include "scanbuttond_wrapper.h"

#include <dlfcn.h>

// this file is basicly the same as loader.c from the scanbuttond-project,
// but modified to meet the needs of scanbd

static char lib_dir[PATH_MAX] = SCANBUTTOND_LIB_DIR;

int scanbtnd_init() {
    const char* backends_dir = NULL;
    char backends_dir_abs[PATH_MAX] = SCANBD_NULL_STRING;

    backends_dir = cfg_getstr(cfg_getsec(cfg, C_GLOBAL), C_SCANBUTTONS_BACKENDS_DIR);
    if ( backends_dir && (backends_dir[0] != '/')) {
        // Relative path, expand 
        assert(backends_dir_abs);
        snprintf(backends_dir_abs, PATH_MAX, "%s/%s", SCANBD_CFG_DIR, backends_dir);
        backends_dir = backends_dir_abs;
    }

    assert(backends_dir);
    scanbtnd_set_libdir(backends_dir);

    if (scanbtnd_loader_init() != 0) {
        slog(SLOG_INFO, "Could not initialize module loader!\n");
        return -1;
    }
    backend = scanbtnd_load_backend("meta");
    if (!backend) {
        slog(SLOG_INFO, "Unable to load backend library\n");
        scanbtnd_loader_exit();
        return -1;
    }
    if (backend->scanbtnd_init() != 0) {
        slog(SLOG_ERROR, "Error initializing backend. Terminating.");
        return -1;
    }
    return 0;
}

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

