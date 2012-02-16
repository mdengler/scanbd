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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "scanbd.h"
#include "scanbd_dbus.h"
#include "scanbuttond_wrapper.h"

static DBusConnection* conn = NULL;
static pthread_t dbus_tid = 0;

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
// this is non-portable
static pthread_mutex_t dbus_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
static pthread_mutex_t dbus_mutex;
#endif


void dbus_send_signal_argv(const char* signal_name, char** argv) {
    // TODO: we need a better dbus mainloop integrations, so that we
    // can wakeup the main thread to sendout the messages we generate here

    DBusMessage* signal = NULL;

    if (conn == NULL) {
        if (!dbus_init()) {
            return;
        }
    }
    assert(conn);

    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return;
    }

    assert(signal_name != NULL);
    if ((signal = dbus_message_new_signal(SCANBD_DBUS_OBJECTPATH,
                                          SCANBD_DBUS_INTERFACE,
                                          signal_name)) == NULL) {
        slog(SLOG_ERROR, "Can't create signal");
        return;
    }

    if (argv != NULL) {
        DBusMessageIter args;
        DBusMessageIter sub;
        dbus_message_iter_init_append(signal, &args);
        if (dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY,
                                             DBUS_TYPE_STRING_AS_STRING,
                                             &sub) != TRUE) {
            slog(SLOG_ERROR, "Can't initialize dbus container");
        }
        while(*argv != NULL) {
            slog(SLOG_DEBUG, "append string %s to signal %s", *argv, signal_name);
            if (dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, argv) != TRUE) {
                slog(SLOG_ERROR, "Can't append signal argument");
            }
            argv++;
        }
        if (dbus_message_iter_close_container(&args, &sub) != TRUE) {
            slog(SLOG_ERROR, "Can't close dbus container");
        }
    }

    slog(SLOG_DEBUG, "now sending signal %s", signal_name);
    dbus_uint32_t serial;
    if (dbus_connection_send(conn, signal, &serial) != TRUE) {
        slog(SLOG_ERROR, "Can't send signal");
    }
    slog(SLOG_DEBUG, "now flushing the dbus");
    // TODO: with the standard-main loop this would block without
    // using a timeout
    dbus_connection_flush(conn);
    slog(SLOG_DEBUG, "unref the signal");
    dbus_message_unref(signal);
}

void dbus_send_signal(const char* signal_name, const char* arg) {
    DBusMessage* signal = NULL;

    if (conn == NULL) {
        if (!dbus_init()) {
            return;
        }
    }
    assert(conn);

    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return;
    }

    assert(signal_name != NULL);
    if ((signal = dbus_message_new_signal(SCANBD_DBUS_OBJECTPATH,
                                          SCANBD_DBUS_INTERFACE,
                                          signal_name)) == NULL) {
        slog(SLOG_ERROR, "Can't create signal");
        return;
    }

    if (arg != NULL) {
        DBusMessageIter args;
        dbus_message_iter_init_append(signal, &args);
        slog(SLOG_DEBUG, "append string %s to signal %s", arg, signal_name);
        if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &arg) != TRUE) {
            slog(SLOG_ERROR, "Can't append signal argument");
        }
    }

    dbus_uint32_t serial;
    slog(SLOG_DEBUG, "now sending signal %s", signal_name);
    if (dbus_connection_send(conn, signal, &serial) != TRUE) {
        slog(SLOG_ERROR, "Can't send signal");
    }
    dbus_connection_flush(conn);
    dbus_message_unref(signal);
}

#ifdef USE_HAL
static void hal_device_added(LibHalContext* ctx, const char *udi) {
    (void)ctx;
    slog(SLOG_DEBUG, "New Device: %s", udi);

    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    if (libhal_device_query_capability(ctx, udi,
                                       HAL_SCANNER_CAPABILITY, &dbus_error) == TRUE) {
        // found new scanner
        slog(SLOG_INFO, "New Scanner: %s", udi);
#ifdef USE_SANE
        stop_sane_threads();
#else
        stop_scbtn_threads();
#endif
        slog(SLOG_DEBUG, "sane_exit");
#ifdef USE_SANE
        sane_exit();
#else
        scbtn_shutdown();
#endif

#ifdef SANE_REINIT_TIMEOUT
        sleep(SANE_REINIT_TIMEOUT); // TODO: don't know if this is
        // really neccessary
#endif
        slog(SLOG_DEBUG, "sane_init");
#ifdef USE_SANE
        sane_init(NULL, NULL);
        get_sane_devices();
        start_sane_threads();
#else
        if (scanbtnd_init() < 0) {
            slog(SLOG_INFO, "Could not initialize scanbuttond modules!\n");
            exit(EXIT_FAILURE);
        }
        assert(backend);

        get_scbtn_devices();
        start_scbtn_threads();
#endif
    }
}
#endif

#ifdef USE_HAL
static void hal_device_removed(LibHalContext* ctx, const char *udi) {
    (void)ctx;
    slog(SLOG_DEBUG, "Removed Device: %s", udi);

    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    // if the device is removed, the device informations isn't
    // available anymore, so we unconditionally stop/start the sane threads
    if (libhal_device_query_capability(ctx, udi,
                                       HAL_SCANNER_CAPABILITY, &dbus_error) == TRUE) {
        slog(SLOG_INFO, "Removed Scanner: %s", udi);
    }
#ifdef USE_SANE
    stop_sane_threads();
#else
    stop_scbtn_threads();
#endif

    slog(SLOG_DEBUG, "sane_exit");
#ifdef USE_SANE
    sane_exit();
#else
    scbtn_shutdown();
#endif

#ifdef SANE_REINIT_TIMEOUT
    sleep(SANE_REINIT_TIMEOUT);// TODO: don't know if this is really neccessary
#endif
    slog(SLOG_DEBUG, "sane_init");
#ifdef USE_SANE
    sane_init(NULL, NULL);
    get_sane_devices();
    start_sane_threads();
#else
    if (scanbtnd_init() < 0) {
        slog(SLOG_INFO, "Could not initialize scanbuttond modules!\n");
        exit(EXIT_FAILURE);
    }
    assert(backend);

    get_scbtn_devices();
    start_scbtn_threads();
#endif
}
#endif

// this function is used from dbus_thread and udev_thread
void dbus_signal_device_added(void) {
    if (pthread_mutex_lock(&dbus_mutex)) {
        slog(SLOG_ERROR, "Can't lock mutex");
    }
#ifndef USE_HAL
    slog(SLOG_DEBUG, "dbus_signal_device_added");
    // look for new scanner
#ifdef USE_SANE
    stop_sane_threads();
#else
    stop_scbtn_threads();
#endif // USE_SANE
    slog(SLOG_DEBUG, "sane_exit");
#ifdef USE_SANE
    sane_exit();
#else
    scbtn_shutdown();
#endif // USE_SANE

#ifdef SANE_REINIT_TIMEOUT
    sleep(SANE_REINIT_TIMEOUT); // TODO: don't know if this is
    // really neccessary
#endif

#ifdef USE_SANE
    slog(SLOG_DEBUG, "sane_init");
    sane_init(NULL, NULL);
    get_sane_devices();
    start_sane_threads();
#else
    if (scanbtnd_init() < 0) {
        slog(SLOG_INFO, "Could not initialize scanbuttond modules!\n");
        exit(EXIT_FAILURE);
    }
    assert(backend);

    get_scbtn_devices();
    start_scbtn_threads();
#endif // USE_SANE
#endif // USE_HAL
    if (pthread_mutex_unlock(&dbus_mutex)) {
        slog(SLOG_ERROR, "Can't unlock mutex");
    }
}

void dbus_signal_device_removed(void) {
    if (pthread_mutex_lock(&dbus_mutex)) {
        slog(SLOG_ERROR, "Can't lock mutex");
    }
#ifndef USE_HAL
    slog(SLOG_DEBUG, "dbus_signal_device_removed");
    // look for removed scanner
#ifdef USE_SANE
    stop_sane_threads();
#else
    stop_scbtn_threads();
#endif

    slog(SLOG_DEBUG, "sane_exit");
#ifdef USE_SANE
    sane_exit();
#else
    scbtn_shutdown();
#endif

#ifdef SANE_REINIT_TIMEOUT
    sleep(SANE_REINIT_TIMEOUT); // TODO: don't know if this is
    // really neccessary
#endif
    slog(SLOG_DEBUG, "sane_init");
#ifdef USE_SANE
    sane_init(NULL, NULL);
    get_sane_devices();
    start_sane_threads();
#else
    if (scanbtnd_init() < 0) {
        slog(SLOG_INFO, "Could not initialize scanbuttond modules!\n");
        exit(EXIT_FAILURE);
    }
    assert(backend);

#endif
#endif
    if (pthread_mutex_unlock(&dbus_mutex)) {
        slog(SLOG_ERROR, "Can't unlock mutex");
    }
}

// is called when saned exited
static void dbus_method_release(void) {
    slog(SLOG_DEBUG, "dbus_method_release");
    // start all threads
#ifdef USE_SANE
    start_sane_threads();
#else
    start_scbtn_threads();
#endif
}

// is called before saned started
static void dbus_method_acquire(void) {
    slog(SLOG_DEBUG, "dbus_method_acquire");
    // stop all threads
#ifdef USE_SANE
    stop_sane_threads();
#else
    stop_scbtn_threads();
#endif
}

struct sane_trigger_arg {
    void (*f)(void*);
    int device;
    int action;
};
typedef struct sane_trigger_arg sane_trigger_arg_t;

void* dbus_call_sane_trigger_action_thread(void* arg) {
    sane_trigger_arg_t* a = (sane_trigger_arg_t*) arg;
    assert(a != NULL);
#ifdef USE_SANE
    sane_trigger_action(a->device, a->action);
#else
    scbtn_trigger_action(a->device, a->action);
#endif
    return NULL;
}

void sane_trigger_action_async(int device, int action) {
    sane_trigger_arg_t* arg = calloc(1, sizeof(sane_trigger_arg_t));
    assert(arg != NULL);

    arg->device = device;
    arg->action = action;

    pthread_t tid;
    if (pthread_create(&tid, NULL, dbus_call_sane_trigger_action_thread, arg) < 0) {
        slog(SLOG_WARN, "pthread_create: %s", strerror(errno));
    }
}

static void dbus_method_trigger(DBusMessage *message) {
    DBusMessageIter args;

    dbus_uint32_t device = -1;
    dbus_uint32_t action = -1;
    
    if (!dbus_message_iter_init(message, &args)) {
        slog(SLOG_WARN, "trigger has no arguments");
        return;
    }
    if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32) {
        dbus_message_iter_get_basic(&args, &device);
        slog(SLOG_INFO, "trigger device %d", device);
    }
    else {
        slog(SLOG_WARN, "trigger has wrong argument type");
        return;
    }
    if (!dbus_message_iter_next(&args)) {
        slog(SLOG_WARN, "trigger has too few arguments");
        return;
    }
    if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32) {
        dbus_message_iter_get_basic(&args, &action);
        slog(SLOG_INFO, "trigger action %d", action);
    }
    else {
        slog(SLOG_WARN, "trigger has wrong argument type");
        return;
    }
    sane_trigger_action_async(device, action);
}

static void unregister_func(DBusConnection* connection, void* user_data) {
    (void)connection;
    (void)user_data;
    slog(SLOG_DEBUG, "unregister_func_called");
}

static DBusHandlerResult message_func(DBusConnection *connection, DBusMessage *message,
                                      void *user_data) {
    (void)connection;
    (void)user_data;
    
    slog(SLOG_DEBUG, "message_func");
    DBusMessage* reply = NULL;
    if (dbus_message_is_method_call(message,
                                    SCANBD_DBUS_INTERFACE,
                                    SCANBD_DBUS_METHOD_ACQUIRE)) {
        dbus_method_acquire();
    }
    else if (dbus_message_is_method_call(message,
                                         SCANBD_DBUS_INTERFACE,
                                         SCANBD_DBUS_METHOD_RELEASE)) {
        dbus_method_release();
    }
    else if (dbus_message_is_method_call(message,
                                         SCANBD_DBUS_INTERFACE,
                                         SCANBD_DBUS_METHOD_TRIGGER)) {
        dbus_method_trigger(message);
    }
    else if (dbus_message_is_signal(message,
                                    DBUS_HAL_INTERFACE,
                                    DBUS_HAL_SIGNAL_DEV_ADDED)) {
        dbus_signal_device_added();
    }
    else if (dbus_message_is_signal(message,
                                    DBUS_HAL_INTERFACE,
                                    DBUS_HAL_SIGNAL_DEV_REMOVED)) {
        dbus_signal_device_removed();
    }

    /* If the message was handled, send back the reply */
    if (reply != NULL) {
        dbus_connection_send(conn, reply, NULL);
        dbus_message_unref(reply);
    }
    return reply ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusObjectPathVTable dbus_vtable = {
    unregister_func,
    message_func,
    NULL,
    NULL,
    NULL,
    NULL
};

void dbus_thread_cleanup(void* arg) {
    assert(arg);
    slog(SLOG_DEBUG, "dbus_thread_cleanup");
    (void)arg;
}

static void* dbus_thread(void* arg) {
    (void)arg;
    // we only expect the main thread to handle signals
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    
    cfg_t* cfg_sec_global = NULL;
    cfg_sec_global = cfg_getsec(cfg, C_GLOBAL);
    assert(cfg_sec_global);

    int timeout = cfg_getint(cfg_sec_global, C_TIMEOUT);
    if (timeout <= 0) {
        timeout = C_TIMEOUT_DEF;
    }
    slog(SLOG_DEBUG, "timeout: %d ms", timeout);

    while(dbus_connection_read_write_dispatch(conn, timeout)) {
        slog(SLOG_DEBUG, "Iteration on dbus call");
        usleep(timeout * 1000);
    }
    return NULL;
}

bool dbus_init(void) {
    slog(SLOG_DEBUG, "dbus_init");

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
    slog(SLOG_INFO, "dbus init mutex");
    pthread_mutexattr_t mutexattr;
    if (pthread_mutexattr_init(&mutexattr) < 0) {
        slog(SLOG_ERROR, "Can't initialize mutex attr");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE) < 0) {
        slog(SLOG_ERROR, "Can't set mutex attr");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&dbus_mutex, &mutexattr) < 0) {
        slog(SLOG_ERROR, "Can't init mutex");
        exit(EXIT_FAILURE);
    }
#endif

    if (!dbus_threads_init_default()) {
        slog(SLOG_ERROR, "DBus thread initialization failure");
    }
    
    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    if (conn == NULL) {
        conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
        if (dbus_error_is_set(&dbus_error)) {
            conn = NULL;
            slog(SLOG_ERROR, "DBus connection error: %s", dbus_error.message);
            dbus_error_free(&dbus_error);
            return false;
        }
    }
    else {
        slog(SLOG_WARN, "dbus connection already established");
    }
    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return false;
    }

    assert(conn);

#ifdef USE_HAL
    LibHalContext *hal_ctx = NULL;

    hal_ctx = (LibHalContext*)libhal_ctx_new();
    if (!hal_ctx) {
        slog(SLOG_WARN, "Failed to create HAL context!");
        return false;
    }

    libhal_ctx_set_dbus_connection(hal_ctx, conn);
    
    libhal_ctx_set_device_added(hal_ctx, hal_device_added);
    libhal_ctx_set_device_removed(hal_ctx, hal_device_removed);

    if (!libhal_ctx_init(hal_ctx, &dbus_error)) {
        slog(SLOG_WARN, "Couldn't initialise HAL!");
        return false;
    }
#else
    char match[PATH_MAX];
    snprintf(match, PATH_MAX, "type='signal',interface='%s'", DBUS_HAL_INTERFACE);
    slog(SLOG_ERROR, "dbus match %s", match);
    dbus_bus_add_match(conn, match, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) {
        slog(SLOG_ERROR, "DBus match error: %s", dbus_error.message);
        dbus_error_free(&dbus_error);
        return false;
    }
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&dbus_error)) {
        slog(SLOG_ERROR, "DBus match error: %s", dbus_error.message);
        dbus_error_free(&dbus_error);
        return false;
    }
#endif
    return true;
}

void dbus_start_dbus_thread(void) {
    slog(SLOG_DEBUG, "start dbus thread");
    if (conn == NULL) {
        if (!dbus_init()) {
            return;
        }
    }
    assert(conn);
    
    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return;
    }

    DBusError dbus_error;
    
    dbus_error_init(&dbus_error);
    if (dbus_connection_register_object_path(conn, SCANBD_DBUS_OBJECTPATH,
                                             &dbus_vtable, NULL) == FALSE) {
        slog(SLOG_ERROR, "Can't register object path: %s", SCANBD_DBUS_OBJECTPATH);
        return;
    }

    dbus_error_init(&dbus_error);
    int ret = dbus_bus_request_name(conn, SCANBD_DBUS_ADDRESS,
                                    0, &dbus_error);

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        slog(SLOG_WARN, "Not Primary Owner (%d)", ret);
        if (dbus_error_is_set(&dbus_error)) {
            slog(SLOG_WARN, "Name Error (%s)", dbus_error.message);
            dbus_error_free(&dbus_error);
            return;
        }
    }
    if (dbus_tid != 0) {
        slog(SLOG_DEBUG, "dbus thread already running");
        dbus_stop_dbus_thread();
    }
    if (pthread_create(&dbus_tid, NULL, dbus_thread, NULL) < 0){
        slog(SLOG_ERROR, "Can't create dbus thread: %s", strerror(errno));
        return;
    }
    return;
}

void dbus_stop_dbus_thread(void) {
    slog(SLOG_DEBUG, "stop dbus thread");
    if (dbus_tid == 0) {
        return;
    }
    if (pthread_cancel(dbus_tid) < 0) {
        if (errno == ESRCH) {
            slog(SLOG_WARN, "dbus thread was already cancelled");
        }
        else {
            slog(SLOG_WARN, "unknown error from pthread_cancel: %s", strerror(errno));
        }
    }
    slog(SLOG_DEBUG, "join dbus thread");
    if (pthread_join(dbus_tid, NULL) < 0) {
        slog(SLOG_ERROR, "pthread_join: %s", strerror(errno));
    }
    dbus_tid = 0;
}

void dbus_call_method(const char* method, const char* value) {
    slog(SLOG_DEBUG, "dbus_call_method");
    if (conn == NULL) {
        if (!dbus_init()) {
            return;
        }
    }
    assert(conn);

    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return;
    }

    DBusMessage* msg = NULL;
    if ((msg = dbus_message_new_method_call(SCANBD_DBUS_ADDRESS,
                                            SCANBD_DBUS_OBJECTPATH,
                                            SCANBD_DBUS_INTERFACE,
                                            method)) == NULL) {
        slog(SLOG_ERROR, "Can't compose message");
        return;
    }
    assert(msg);

    if (value != NULL) {
        DBusMessageIter args;
        dbus_message_iter_init_append(msg, &args);
        if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, value) != TRUE) {
            slog(SLOG_ERROR, "Can't compose message");
            return;
        }
    }
    
    DBusPendingCall* pending = NULL;

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) {
        slog(SLOG_WARN, "Can't send message");
        return;
    }
    if (NULL == pending) {
        slog(SLOG_ERROR, "Disconnected from bus");
        return;
    }
    dbus_connection_flush(conn);

    assert(msg);
    dbus_message_unref(msg);

    slog(SLOG_DEBUG, "waiting for reply");
    assert(pending);
    dbus_pending_call_block(pending);

    // get the reply message
    DBusMessage* reply = NULL;
    if ((reply = dbus_pending_call_steal_reply(pending)) == NULL) {
        slog(SLOG_DEBUG, "Reply Null\n");
        return;
    }
    dbus_pending_call_unref(pending);

    DBusMessageIter args;
    if (!dbus_message_iter_init(reply, &args)) {
        slog(SLOG_INFO, "Reply has no arguments");
    }
    
    return;
}

void dbus_call_trigger(unsigned int device, unsigned int action) {
    slog(SLOG_DEBUG, "dbus_call_trigger for dev %d, action %d", device, action);
    if (conn == NULL) {
        if (!dbus_init()) {
            return;
        }
    }
    assert(conn);

    if (!conn) {
        slog(SLOG_DEBUG, "No dbus connection");
        return;
    }

    DBusMessage* msg = NULL;
    if ((msg = dbus_message_new_method_call(SCANBD_DBUS_ADDRESS,
                                            SCANBD_DBUS_OBJECTPATH,
                                            SCANBD_DBUS_INTERFACE,
                                            SCANBD_DBUS_METHOD_TRIGGER)) == NULL) {
        slog(SLOG_ERROR, "Can't compose message");
        return;
    }
    assert(msg);

    dbus_uint32_t dev = device;
    dbus_uint32_t act = action;
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    if (dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &dev) != TRUE) {
        slog(SLOG_ERROR, "Can't compose message");
        return;
    }
    if (dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &act) != TRUE) {
        slog(SLOG_ERROR, "Can't compose message");
        return;
    }
    
    DBusPendingCall* pending = NULL;

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) {
        slog(SLOG_WARN, "Can't send message");
        return;
    }
    if (NULL == pending) {
        slog(SLOG_ERROR, "Disconnected from bus");
        return;
    }
    dbus_connection_flush(conn);

    assert(msg);
    dbus_message_unref(msg);

    slog(SLOG_DEBUG, "waiting for reply");
    assert(pending);
    dbus_pending_call_block(pending);

    // get the reply message
    DBusMessage* reply = NULL;
    if ((reply = dbus_pending_call_steal_reply(pending)) == NULL) {
        slog(SLOG_DEBUG, "Reply Null\n");
        return;
    }
    dbus_pending_call_unref(pending);

    if (!dbus_message_iter_init(reply, &args)) {
        slog(SLOG_INFO, "Reply has no arguments");
    }
    
    return;
}
