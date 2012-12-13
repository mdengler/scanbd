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

#ifndef SCANBD_H
#define SCANBD_H

#ifndef USE_SANE
#ifndef USE_SCANBUTTOND
#define USE_SCANBUTTOND
#endif
#endif

#define _GNU_SOURCE

#include "common.h"

#include <getopt.h>
#include <confuse.h>

#ifdef USE_SANE
#include <sane/sane.h>
#else
#include <scanbuttond/libusbi.h>
#endif

#include "slog.h"
#include "scanbd_dbus.h"
#include "udev.h"

#define SANE_REINIT_TIMEOUT 3 // TODO: don't know if this is really neccessary

#define SCANBUTTOND_ALARM_TIMEOUT 5 // reconfigure after this amount of seconds if
// device was busy

#define C_FROM_VALUE "from-value"
#define C_FROM_VALUE_DEF_INT 0
#define C_FROM_VALUE_DEF_STR ""

#define C_TO_VALUE "to-value"
#define C_TO_VALUE_DEF_INT 1
#define C_TO_VALUE_DEF_STR ".+"

#define C_FILTER "filter"
#define C_ACTION_DEF "^scan.*"
#define C_FUNCTION_DEF "^function.*"

#define C_NUMERICAL_TRIGGER "numerical-trigger"

#define C_STRING_TRIGGER "string-trigger"

#define C_DESC "desc"
#define C_DESC_DEF "The description goes here"

#define C_SCRIPT "script"
#define C_SCRIPT_DEF ""

#define C_ENV "env"
#define C_ENV_FUNCTION "SCANBD_FUNCTION"
#define C_ENV_FUNCTION_DEF "SCANBD_FUNCTION"

#define C_ENV_DEVICE "device"
#define C_ENV_DEVICE_DEF "SCANBD_DEVIVCE"

#define C_ENV_ACTION "action"
#define C_ENV_ACTION_DEF "SCANBD_ACTION"

#define C_DEBUG "debug"
#define C_DEBUG_DEF false

#define C_MULTIPLE_ACTIONS "multiple_actions"
#define C_MULTIPLE_ACTIONS_DEF true

#define C_DEBUG_LEVEL "debug-level"
#define C_DEBUG_LEVEL_DEF 1

#define C_USER "user"
#define C_USER_DEF "saned"

#define C_GROUP "group"
#define C_GROUP_DEF "scanner"

#define C_SANED "saned"
#define C_SANED_DEF "/usr/sbin/saned"

#define C_SANED_OPTS "saned_opt"
#define C_SANED_OPTS_DEF "{}"

#define C_SCRIPTDIR "scriptdir"
#define C_SCRIPTDIR_DEF ""

#define C_SCANBUTTONS_BACKENDS_DIR "scanbuttond_backends_dir"
#ifdef SCANBUTTOND_CFG_DIR
#define C_SCANBUTTONS_BACKENDS_DIR_DEF SCANBUTTOND_CFG_DIR
#else
#ifndef USE_SANE
#warning "Using predefined directory: /usr/local/etc/scanbd/scanbuttond/backends"
#endif
#define C_SCANBUTTONS_BACKENDS_DIR_DEF "/usr/local/etc/scanbd/scanbuttond/backends"
#endif

#define C_SANED_ENVS "saned_env"
#define C_SANED_ENVS_DEF "{}"

#define C_TIMEOUT "timeout"
#define C_TIMEOUT_DEF 500

#define C_PIDFILE "pidfile"
#define C_PIDFILE_DEF "/var/run/scanbd.pid"

#define C_ENVIRONMENT "environment"

#define C_FUNCTION "function"

#define C_ACTION "action"

#define C_GLOBAL "global"
#define C_DEVICE "device"

#define SCANBD_NULL_STRING "(null)"

#ifdef SCANBD_CFG_DIR
#define SCANBD_CONF  SCANBD_CFG_DIR "/scanbd.conf"
#else
#define SCANBD_CONF "scanbd.conf"
#endif


struct scanbdOptions {
    bool        managerMode;
    bool        foreground;
    bool        signal;
    const char* config_file_name;
};

// command-line options
extern struct scanbdOptions scanbd_options;

// config from the config-file 
extern cfg_t* cfg;

// functions
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
extern void sane_init_mutex();
#endif
extern void get_sane_devices(void);
extern void sane_trigger_action(int, int);
extern void stop_sane_threads(void);
extern void start_sane_threads(void);

extern void daemonize(void);

#endif
