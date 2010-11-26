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

#ifndef SCANBD_H
#define SCANBD_H

#define _GNU_SOURCE

#include "common.h"

#include <getopt.h>
#include <confuse.h>
#include <sane/sane.h>

#include "slog.h"
#include "scanbd_dbus.h"

#define SANE_REINIT_TIMEOUT 3 // TODO: don't know if this is really neccessary

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
extern void get_sane_devices(void);
extern void sane_trigger_action(int, int);
extern void stop_sane_threads(void);
extern void start_sane_threads(void);

extern void daemonize(void);

#endif
