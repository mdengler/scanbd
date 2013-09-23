/*
 * $Id$
 *
 *  scanbd - KMUX scanner button daemon
 *
 *  Copyright (C) 2008 - 2013  Wilhelm Meier (wilhelm.meier@fh-kl.de)
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
#include <libgen.h>

// parsing the config-file via libconfuse
void cfg_do_parse(const char *config_file_name) {
    slog(SLOG_INFO, "reading config file %s", config_file_name);

    cfg_opt_t cfg_numtrigger[] = {
        CFG_INT(C_FROM_VALUE, C_FROM_VALUE_DEF_INT, CFGF_NONE),
        CFG_INT(C_TO_VALUE, C_TO_VALUE_DEF_INT, CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t cfg_strtrigger[] = {
        CFG_STR(C_FROM_VALUE, C_FROM_VALUE_DEF_STR, CFGF_NONE),
        CFG_STR(C_TO_VALUE, C_TO_VALUE_DEF_STR, CFGF_NONE),
        CFG_END()
    };
    
    cfg_opt_t cfg_action[] = {
        CFG_STR(C_FILTER, C_ACTION_DEF, CFGF_NONE),
        CFG_SEC(C_NUMERICAL_TRIGGER, cfg_numtrigger, CFGF_NONE),
        CFG_SEC(C_STRING_TRIGGER, cfg_strtrigger, CFGF_NONE),
        CFG_STR(C_DESC, C_DESC_DEF, CFGF_NONE),
        CFG_STR(C_SCRIPT, C_SCRIPT_DEF, CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t cfg_function[] = {
        CFG_STR(C_FILTER, C_FUNCTION_DEF, CFGF_NONE),
        CFG_STR(C_DESC, C_DESC_DEF, CFGF_NONE),
        CFG_STR(C_ENV, C_ENV_FUNCTION_DEF, CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t cfg_environment[] = {
        CFG_STR(C_ENV_DEVICE, C_ENV_DEVICE_DEF, CFGF_NONE),
        CFG_STR(C_ENV_ACTION, C_ENV_ACTION_DEF, CFGF_NONE),
        CFG_END()
    };

    cfg_opt_t cfg_global[] = {
        CFG_BOOL(C_DEBUG, C_DEBUG_DEF, CFGF_NONE),
        CFG_BOOL(C_MULTIPLE_ACTIONS, C_MULTIPLE_ACTIONS_DEF, CFGF_NONE),
        CFG_INT(C_DEBUG_LEVEL, C_DEBUG_LEVEL_DEF, CFGF_NONE),
        CFG_STR(C_USER, C_USER_DEF, CFGF_NONE),
        CFG_STR(C_GROUP, C_GROUP_DEF, CFGF_NONE),
        CFG_STR(C_SANED, C_SANED_DEF, CFGF_NONE),
        CFG_STR_LIST(C_SANED_OPTS, C_SANED_OPTS_DEF, CFGF_NONE),
        CFG_STR_LIST(C_SANED_ENVS, C_SANED_ENVS_DEF, CFGF_NONE),
        CFG_STR(C_SCRIPTDIR, C_SCRIPTDIR_DEF, CFGF_NONE),
        CFG_STR(C_DEVICE_INSERT_SCRIPT, C_DEVICE_INSERT_SCRIPT_DEF, CFGF_NONE),
        CFG_STR(C_DEVICE_REMOVE_SCRIPT, C_DEVICE_REMOVE_SCRIPT_DEF, CFGF_NONE),
        CFG_STR(C_SCANBUTTONS_BACKENDS_DIR, C_SCANBUTTONS_BACKENDS_DIR_DEF, CFGF_NONE),
        CFG_INT(C_TIMEOUT, C_TIMEOUT_DEF, CFGF_NONE),
        CFG_STR(C_PIDFILE, C_PIDFILE_DEF, CFGF_NONE),
        CFG_SEC(C_ENVIRONMENT, cfg_environment, CFGF_NONE),
        CFG_SEC(C_FUNCTION, cfg_function, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC(C_ACTION, cfg_action, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    cfg_opt_t cfg_device[] = {
        CFG_STR(C_FILTER, "^fujitsu.*", CFGF_NONE),
        CFG_STR(C_DESC, C_DESC_DEF, CFGF_NONE),
        CFG_SEC(C_FUNCTION, cfg_function, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC(C_ACTION, cfg_action, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    cfg_opt_t cfg_options[] = {
        CFG_SEC(C_GLOBAL, cfg_global, CFGF_NONE),
        CFG_SEC(C_DEVICE, cfg_device, CFGF_MULTI | CFGF_TITLE),
        CFG_FUNC(C_INCLUDE, cfg_include),
        CFG_END()
    };

    if (cfg) {
        cfg_free(cfg);
        cfg = NULL;
    }

    char wd[PATH_MAX] = {};
    char config_file[PATH_MAX] = {};
    char* scanbd_conf_dir = NULL;

    // get current directory
    if (getcwd(wd, PATH_MAX) == NULL) {
        slog(SLOG_ERROR, "can't get working directory");
        exit(EXIT_FAILURE);
    }

    // cd into directory where scanbd.conf lives
    
    strncpy(config_file, config_file_name, PATH_MAX);

    scanbd_conf_dir = dirname(config_file);
    assert(scanbd_conf_dir);

    if (chdir(scanbd_conf_dir) != 0) {
        slog(SLOG_ERROR, "can't access the directory for: %s", config_file_name);
        exit(EXIT_FAILURE);
    }

    cfg = cfg_init(cfg_options, CFGF_NONE);

    int ret = 0;
    if ((ret = cfg_parse(cfg, config_file_name)) != CFG_SUCCESS) {
        if (CFG_FILE_ERROR == ret) {
            slog(SLOG_ERROR, "can't open config file: %s", config_file_name);
            exit(EXIT_FAILURE);
        }
        else {
            slog(SLOG_ERROR, "parse error in config file");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE); // not reached
    }

    // cd back to original
    if (chdir(wd) < 0) {
        slog(SLOG_ERROR, "can't cd back to: %s", wd);
        exit(EXIT_FAILURE);
    }

}

char *make_script_path_abs(const char *script) {

    char* script_abs = malloc(PATH_MAX);
    assert(script_abs);
    strncpy(script_abs, SCANBD_NULL_STRING, PATH_MAX);

    assert(script);

    if ((script[0] == '/') || (strcmp(script, SCANBD_NULL_STRING) == 0)) {
        // Script has already an absolute path or is an empty string
        strncpy(script_abs, script, PATH_MAX);
        slog(SLOG_DEBUG, "using absolute script path: %s", script_abs);
    } else {
        // script has a relative path, determine the directory
        // get the scriptdir from the global config

        cfg_t* cfg_sec_global = NULL;
    	cfg_sec_global = cfg_getsec(cfg, C_GLOBAL);
    	assert(cfg_sec_global);

        const char* scriptdir =  cfg_getstr(cfg_sec_global, C_SCRIPTDIR);

        if(!scriptdir || (strlen(scriptdir) == 0)) {
            // scriptdir is not set, script is relative to SCANBD_CFG_DIR
            snprintf(script_abs, PATH_MAX, "%s/%s", SCANBD_CFG_DIR, script);
        } else if (scriptdir[0] == '/') {
            // scriptdir is an absolute path
            snprintf(script_abs, PATH_MAX, "%s/%s", scriptdir, script);
        } else {
            // scriptdir is relative to config directory
            snprintf(script_abs, PATH_MAX, "%s/%s/%s", SCANBD_CFG_DIR, scriptdir, script);
        }
        slog(SLOG_DEBUG, "using relative script path: %s, expanded to: %s", script, script_abs);
    } 
    return script_abs;
}
