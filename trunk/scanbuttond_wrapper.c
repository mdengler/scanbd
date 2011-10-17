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
#include "scanbuttond_wrapper.h"
#include "scanbuttond/include/scanbuttond/scanbuttond.h"

// all programm-global scbtn functions use this mutex to avoid races
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
// this is non-portable
pthread_mutex_t scbtn_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
pthread_mutex_t scbtn_mutex;
#endif

pthread_cond_t  scbtn_cv    = PTHREAD_COND_INITIALIZER;

// the following locking strategie must be obeyed:
// 1) lock the scbtn_mutex
// 2) lock the device specific mutex
// in this order to avoid deadlocks
// holding more than these two locks is not intended

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
void scbtn_init_mutex()
{
    slog(SLOG_INFO, "scbtn_init_mutex");
    pthread_mutexattr_t mutexattr;
    if (pthread_mutexattr_init(&mutexattr) < 0) {
        slog(SLOG_ERROR, "Can't initialize mutex attr");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE) < 0) {
        slog(SLOG_ERROR, "Can't set mutex attr");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&scbtn_mutex, &mutexattr) < 0) {
        slog(SLOG_ERROR, "Can't init mutex");
        exit(EXIT_FAILURE);
    }
}
#endif

struct scbtn_opt_value {
    unsigned long num_value; // before-value or after-value or actual-value (BOOL|INT|FIXED)
};
typedef struct scbtn_opt_value scbtn_opt_value_t;

struct scbtn_dev_option {
    int number;                  // the option-number of the device-option
    scbtn_opt_value_t from_value; // the before-value of the option
    scbtn_opt_value_t to_value;   // the after-value of the option (to
    // fire the trigger)
    scbtn_opt_value_t value;      // the option value (from the last
    //				 // polling cycle)
    const char* script;          // the found (matched) script to be called if
    // the option-valued changes
    const char* action_name;	 // the name of this action as
    // specified in the config file
};
typedef struct scbtn_dev_option scbtn_dev_option_t;

struct scbtn_dev_function {
    int number;			 // the option-number of the
    //				 // device-option
    const char* env;		 // the name of the environment-var to
    // pass to option value in
};
typedef struct scbtn_dev_function scbtn_dev_function_t;

// each polling thread is represented by struct scbtn_thread
// there is no locking, since this is "thread private data"
struct scbtn_thread {
    pthread_t tid;                   // the thread-id of the polling
    // thread
    pthread_mutex_t mutex;	     // mutex for this data-structure
    pthread_cond_t cv;		     // cv for this data-structure
    bool triggered;		     // a rule for this device has fired (triggered == true)
    int  triggered_option;           // the action number which triggered
    const scanner_t* dev;	     // the device
    int num_of_options;	             // the number of all options for
    // this device
    scbtn_dev_option_t *opts;        // the list of matched actions
    // for this device
    int num_of_options_with_scripts; // the number of elements in the
    // above list
    scbtn_dev_function_t *functions;  // the list of matched functions
    // for this device
    int num_of_options_with_functions;// the number of elements in the
    // above list
};
typedef struct scbtn_thread scbtn_thread_t;

// the list of all polling threads
static scbtn_thread_t* scbtn_poll_threads = NULL;

// the list of all devices locally connected to our system
static const scanner_t* scbtn_device_list = NULL;

// the number of devices = the number of polling threads
static int num_devices = 0;

void get_scbtn_devices(void) {
    // detect all the scanners we have
    slog(SLOG_INFO, "Scanning for local-only devices" );

    if (pthread_mutex_lock(&scbtn_mutex) < 0) {
        // if we can't get the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        return;
    }
    scbtn_device_list = NULL;
    num_devices = 0;
    if ((scbtn_device_list = backend->scanbtnd_get_supported_devices()) == NULL) {
        slog(SLOG_WARN, "Can't get the scbtn device list");
    }
    const scanner_t* dev = scbtn_device_list;
    if (dev == NULL) {
        slog(SLOG_DEBUG, "device list null");
        goto cleanup;
    }
    while(dev != NULL) {
        slog(SLOG_DEBUG, "found device: %s %s %s",
             dev->product, dev->vendor, dev->sane_device);
        num_devices += 1;
        dev = dev->next;
    }
    if (pthread_cond_broadcast(&scbtn_cv)) {
        slog(SLOG_ERROR, "pthread_cond_broadcast: %s", strerror(errno));
    }
cleanup:
    if (pthread_mutex_unlock(&scbtn_mutex) < 0) {
        // if we can't unlock the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
        return;
    }
}

// cleanup handler for sane_poll
static void scbtn_thread_cleanup_mutex(void* arg) {
    assert(arg != NULL);
    slog(SLOG_DEBUG, "scbtn_thread_cleanup_mutex");
    pthread_mutex_t* mutex = (pthread_mutex_t*)arg;
    if (pthread_mutex_unlock(mutex) < 0) {
        // if we can't unlock the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
    }
}

// cleanup handler for sane_poll
static void scbtn_thread_cleanup_value(void* arg) {
    assert(arg != NULL);
    slog(SLOG_DEBUG, "sane_thread_cleanup_value");
    //    sane_opt_value_t* v = (sane_opt_value_t*)arg;
    //    sane_option_value_free(v);
}

// this function can only be used in the critical region of *st
static void scbtn_find_matching_options(scbtn_thread_t* st, cfg_t* sec) {
    slog(SLOG_DEBUG, "sane_find_matching_options");
    const char* title = cfg_title(sec);
    if (title == NULL) {
        title = SCANBD_NULL_STRING;
    }
    // TODO: use of recursive mutex???
    int actions = cfg_size(sec, C_ACTION);
    if (actions <= 0) {
        slog(SLOG_INFO, "no matching actions in section %s",  title);
        return;
    }

    slog(SLOG_INFO, "found %d actions in section %s", actions, title);

    // iterate over all global actions
    for(int i = 0; i < actions; i += 1) {
        // get the action from the config file
        cfg_t* action_i = cfg_getnsec(sec, C_ACTION, i);
        assert(action_i != NULL);

        // get the filter-regex from the config-file
        const char* opt_regex = cfg_getstr(action_i, C_FILTER);
        assert(opt_regex != NULL);

        const char* title = cfg_title(action_i);
        if (title == NULL) {
            title = "(none)";
        }
        // compile the filter-regex
        slog(SLOG_DEBUG, "checking action %s with filter: %s",
             title, opt_regex);
        regex_t creg;
        int ret = regcomp(&creg, opt_regex, REG_EXTENDED | REG_NOSUB);
        if (ret < 0) {
            char err_text[1024];
            regerror(ret, &creg, err_text, 1024);
            slog(SLOG_WARN, "Can't compile regex: %s : %s", opt_regex, err_text);
            continue;
        }
        // look for matching option-names
        for(int opt = 0; opt < st->num_of_options; opt += 1) {
            const char* name = scanbtnd_button_name(st->dev->meta_info, opt + 1);
            assert(name);

            slog(SLOG_INFO, "found active option[%d] %s for device %s",
                 opt, name, st->dev->product);
            // regex compare with the filter
            if (regexec(&creg, name, 0, NULL, 0) != 0) {
                // no match
                continue;
            }
            // match

            // now get the script
            const char* script = cfg_getstr(action_i, C_SCRIPT);

            if (!script || (strlen(script) == 0)) {
                script = SCANBD_NULL_STRING;
            }

            assert(script != NULL);
            slog(SLOG_INFO, "installing action %s (%d) for %s, option[%d]: %s as: %s",
                 title, st->num_of_options_with_scripts, st->dev->product, opt, name, script);

            cfg_t* cfg_sec_global = NULL;
            cfg_sec_global = cfg_getsec(cfg, C_GLOBAL);
            assert(cfg_sec_global);

            bool multiple_actions = cfg_getbool(cfg_sec_global, C_MULTIPLE_ACTIONS);
            slog(SLOG_INFO, "multiple actions allowed");

            // looking for option already present in the
            // array
            int n = 0;
            for(n = 0; n < st->num_of_options_with_scripts; n += 1) {
                if (st->opts[n].number == opt + 1) {
                    if (!multiple_actions) {
                        slog(SLOG_WARN, "action %s overrides script %s of option[%d] with %s",
                             title, st->opts[n].script, opt, script);
                        // break out with n == index_of_found_option
                        break;
                    }
                    else {
                        if (n < st->num_of_options) {
                            n = st->num_of_options_with_scripts;
                            slog(SLOG_INFO, "adding additional action %s (%d) for option[%d] with %s",
                                 title, n, opt, script);
                            break;
                        }
                        else {
                            slog(SLOG_INFO, "can't add additional action %s for option[%d] with %s",
                                 title, opt, script);
                            break;
                        }
                    }
                }
            }
            // 0 <= n < st->num_of_options_with_scripts:
            // we found it
            // n == st->num_of_options_with_scripts:
            // not found => new

            st->opts[n].number = opt + 1;
            st->opts[n].action_name = title;
            st->opts[n].script = script;
            st->opts[n].from_value.num_value = 0;
            st->opts[n].to_value.num_value = 0;
            st->opts[n].value.num_value = 0;

            cfg_t* num_trigger = cfg_getsec(action_i, C_NUMERICAL_TRIGGER);
            assert(num_trigger);
            st->opts[n].from_value.num_value = cfg_getint(num_trigger,
                                                          C_FROM_VALUE);
            st->opts[n].to_value.num_value = cfg_getint(num_trigger, C_TO_VALUE);

            st->opts[n].value.num_value = 0;

            if (n == st->num_of_options_with_scripts) {
                // not found in the list
                // we have a new option to be polled
                st->num_of_options_with_scripts += 1;
            }
        } // foreach option
        // this compiled regex isn't used anymore
        regfree(&creg);
    } // foreach action
}


void scbtn_find_matching_functions(scbtn_thread_t* st, cfg_t* sec) {
    // TODO: use of recursive mutex???
    slog(SLOG_DEBUG, "sane_find_matching_functions");
    const char* title = cfg_title(sec);
    if (title == NULL) {
        title = SCANBD_NULL_STRING;
    }
    int functions = cfg_size(sec, C_FUNCTION);
    if (functions <= 0) {
        slog(SLOG_INFO, "no matching functions in section %s", title);
        return;
    }
    slog(SLOG_INFO, "scanbuttond backends can't use function definitions");
    st->num_of_options_with_functions = 0;
}

void* scbtn_poll(void* arg) {
    scbtn_thread_t* st = (scbtn_thread_t*)arg;
    assert(st != NULL);
    slog(SLOG_DEBUG, "scbtn_poll");
    // we only expect the main thread to handle signals
    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    // this thread uses the device and the san_thread_t datastructure
    // lock it
    pthread_cleanup_push(scbtn_thread_cleanup_mutex, ((void*)&st->mutex));
    if (pthread_mutex_lock(&st->mutex) < 0) {
        // if we can't get the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        pthread_exit(NULL);
    }

    int ores = backend->scanbtnd_open((scanner_t*)st->dev);
    if (ores != 0) {
        slog(SLOG_WARN, "scanbtnd_open failed, error code: %s", strerror(ores));
        slog(SLOG_WARN, "abandon polling of %s", st->dev->product);
        if (ores == -ENODEV) {
            slog(SLOG_WARN, "scanbtnd_open failed, no device -> canceling thread");
        }
        if (alarm(SCANBUTTOND_ALARM_TIMEOUT) > 0) {
            slog(SLOG_WARN, "alarm error, there was a pending alarm");
        }
        pthread_exit(NULL);
    }

    // figure out the number of options this device has
    // option 0 (zero) is guaranteed to exist with the total number of
    // options of that device (including option 0)
    st->num_of_options = st->dev->num_buttons;
    if (st->num_of_options == 0) {
        // no options -> nothing to poll
        slog(SLOG_INFO, "No options for device %s", st->dev->product);
        pthread_exit(NULL);
    }
    slog(SLOG_INFO, "found %d options for device %s", st->num_of_options, st->dev->product);

    // allocate an array of options for the  matching actions
    //
    // only one script is possible per option, later matching
    // actions overwrite previous ones

    // initialize the list of matching options
    if (st->opts != NULL) {
        slog(SLOG_ERROR, "possible memory leak: %s, %d", __FILE__, __LINE__);
    }
    st->opts = NULL;
    st->opts = (scbtn_dev_option_t*) calloc(st->num_of_options,
                                            sizeof(scbtn_dev_option_t));
    assert(st->opts != NULL);
    for(int i = 0; i < st->num_of_options; i += 1) {
        st->opts[i].from_value.num_value = 0;
        st->opts[i].to_value.num_value = 0;
        st->opts[i].value.num_value = 0;
    }

    // the number of valid entries in the above list
    st->num_of_options_with_scripts = 0;

    // initialize the list of matching functions
    if (st->functions != NULL) {
        slog(SLOG_ERROR, "possible memory leak: %s, %d", __FILE__, __LINE__);
    }
    st->functions = NULL;
    st->functions = (scbtn_dev_function_t*) calloc(st->num_of_options,
                                                   sizeof(scbtn_dev_function_t));
    assert(st->functions != NULL);
    for(int i = 0; i < st->num_of_options; i += 1) {
        st->functions[i].number = 0;
        st->functions[i].env = NULL;
    }
    // the number of valid entries in the above list
    st->num_of_options_with_functions = 0;

    // find out the functions and actions
    // get the global sconfig section
    cfg_t* cfg_sec_global = NULL;
    cfg_sec_global = cfg_getsec(cfg, C_GLOBAL);
    assert(cfg_sec_global);

    // find the global actions
    scbtn_find_matching_options(st, cfg_sec_global);

    // find the global functions
    scbtn_find_matching_functions(st, cfg_sec_global);

    // find (if any) device specifc sections
    // these override global definitions, if any
    int local_sections = cfg_size(cfg, C_DEVICE);
    slog(SLOG_DEBUG, "found %d local device sections", local_sections);

    for(int loc = 0; loc < local_sections; loc += 1) {
        cfg_t* loc_i = cfg_getnsec(cfg, C_DEVICE, loc);
        assert(loc_i != NULL);

        // get the filter-regex from the config-file
        const char* loc_regex = cfg_getstr(loc_i, C_FILTER);
        assert(loc_regex != NULL);

        const char* title = cfg_title(loc_i);
        if (title == NULL) {
            title = "(none)";
        }
        // compile the filter-regex
        slog(SLOG_INFO, "checking device section %s with filter: %s",
             title, loc_regex);
        regex_t creg;
        int ret = regcomp(&creg, loc_regex, REG_EXTENDED | REG_NOSUB);
        if (ret < 0) {
            char err_text[1024];
            regerror(ret, &creg, err_text, 1024);
            slog(SLOG_WARN, "Can't compile regex: %s : %s", loc_regex, err_text);
            continue;
        }
        // compare the regex against the device name
        if (regexec(&creg, st->dev->product, 0, NULL, 0) == 0) {
            // match
            int loc_actions = cfg_size(loc_i, C_ACTION);
            slog(SLOG_INFO, "found %d local action for device %s [%s]",
                 loc_actions, st->dev->product, title);
            // get the local actions for this device
            scbtn_find_matching_options(st, loc_i);
            // get the local functions for this device
            scbtn_find_matching_functions(st, loc_i);
        }
        regfree(&creg);
    } // foreach local section

    int timeout = cfg_getint(cfg_sec_global, C_TIMEOUT);
    if (timeout <= 0) {
        timeout = C_TIMEOUT_DEF;
    }
    slog(SLOG_DEBUG, "timeout: %d ms", timeout);

    slog(SLOG_DEBUG, "Start the polling for device %s", st->dev->product);

    while(true) {
        slog(SLOG_DEBUG, "polling thread for %s cancellation point", st->dev->product);
        // special cancellation point
        pthread_testcancel();
        slog(SLOG_DEBUG, "polling device %s", st->dev->product);

        int button = backend->scanbtnd_get_button((scanner_t*)st->dev);
        slog(SLOG_INFO, "button %d", button);

        for(int i = 0; i < st->num_of_options_with_scripts; i += 1) {
            //	    const scbtn_Option_Descriptor* odesc = NULL;
            //	    assert((odesc = scbtn_get_option_descriptor(st->h, st->opts[i].number)) != NULL);

            const backend_t* b = st->dev->meta_info;
            slog(SLOG_INFO, "option: %d", st->opts[i].number);
            const char* name = scanbtnd_button_name(b, st->opts[i].number);
            assert(name);

            if (st->opts[i].script != NULL) {
                if (strlen(st->opts[i].script) <= 0) {
                    slog(SLOG_WARN, "No valid script for option %s for device %s",
                         name, st->dev->product);
                    continue;
                }
            }
            else {
                slog(SLOG_WARN, "No script for option %s for device %s",
                     name, st->dev->product);
                continue;
            }
            assert(st->opts[i].script != NULL);
            assert(strlen(st->opts[i].script) > 0);

            //	    scbtn_opt_value_t value;
            //	    scbtn_option_value_init(&value);
            // push the cleanup-handle to free the value storage
            pthread_cleanup_push(scbtn_thread_cleanup_value, NULL);

            slog(SLOG_INFO, "checking option %s number %d (%d) for device %s",
                 name, st->opts[i].number, i,
                 st->dev->product);


            unsigned long value = 0;

            if ((button > 0) && (button == st->opts[i].number)) {
                value = 1;
                slog(SLOG_INFO, "button %d has been pressed.", button);
                if ((st->opts[i].from_value.num_value == st->opts[i].value.num_value) &&
                        (st->opts[i].to_value.num_value == value)) {
                    slog(SLOG_DEBUG, "value trigger: numerical");
                    st->triggered = true;
                    st->triggered_option = i;
                    // we need to trigger all waiting threads
                    if (pthread_cond_broadcast(&st->cv) < 0) {
                        slog(SLOG_ERROR, "pthread_cond_broadcats: this shouln't happen");
                    }
                }
            }

            // pass the responsibility to free the value to the main
            // thread, if this thread gets canceled
            if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) < 0) {
                slog(SLOG_ERROR, "pthread_setcancelstate: %s", strerror(errno));
            }
            st->opts[i].value.num_value = value;
            pthread_cleanup_pop(0);
            if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) < 0) {
                slog(SLOG_ERROR, "pthread_setcancelstate: %s", strerror(errno));
            }

            // was there a value change?
            if (st->triggered && (st->triggered_option >= 0)) {
                assert(st->triggered_option >= 0); // index into the opts-array
                assert(st->triggered_option < st->num_of_options_with_scripts);

                slog(SLOG_ERROR, "trigger action for device %s with script %s",
                     st->dev->product, st->opts[st->triggered_option].script);

                // prepare the environment for the script to be called

                // number of env-vars =
                // number of found function-options
                // plus the values in the environment-section (2):
                // device, action
                // plus those 4:
                // PATH, PWD, USER, HOME
                // plus the sentinel
                cfg_t* global_envs = cfg_getsec(cfg_sec_global, C_ENVIRONMENT);

                assert(st->num_of_options_with_functions == 0);

                int number_of_envs = st->num_of_options_with_functions + 4 + 2 + 1;
                char** env = calloc(number_of_envs, sizeof(char*));
                for(int e = 0; e < number_of_envs; e += 1) {
                    env[e] = calloc(NAME_MAX + 1, sizeof(char));
                }
                int e = 0;
                const char* ev = "PATH";
                if (getenv(ev) != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, getenv(ev));
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                else {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, "/usr/sbin:/usr/bin:/sbin:/bin");
                    slog(SLOG_DEBUG, "No PATH, setting env: %s", env[e]);
                    e += 1;
                }
                ev = "PWD";
                if (getenv(ev) != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, getenv(ev));
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                else {
                    char buf[PATH_MAX];
                    char* ptr = getcwd(buf, PATH_MAX - 1);
                    if (!ptr) {
                        slog(SLOG_ERROR, "can't get pwd");
                    }
                    else {
                        assert(ptr);
                        snprintf(env[e], NAME_MAX, "%s=%s", ev, ptr);
                        slog(SLOG_DEBUG, "No PWD, setting env: %s", env[e]);
                        e += 1;
                    }
                }
                ev = "USER";
                if (getenv(ev) != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, getenv(ev));
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                else {
                    struct passwd* pwd = NULL;
                    pwd = getpwuid(geteuid());
                    assert(pwd);
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, pwd->pw_name);
                    slog(SLOG_DEBUG, "No USER, setting env: %s", env[e]);
                    e += 1;
                }
                ev = "HOME";
                if (getenv(ev) != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, getenv(ev));
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                else {
                    struct passwd* pwd = 0;
                    pwd = getpwuid(geteuid());
                    assert(pwd);
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, pwd->pw_dir);
                    slog(SLOG_DEBUG, "No HOME, setting env: %s", env[e]);
                    e += 1;
                }
                ev = cfg_getstr(global_envs, C_DEVICE);
                if (ev != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev, st->dev->sane_device);
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                ev = cfg_getstr(global_envs, C_ACTION);
                if (ev != NULL) {
                    snprintf(env[e], NAME_MAX, "%s=%s", ev,
                             st->opts[st->triggered_option].action_name);
                    slog(SLOG_DEBUG, "setting env: %s", env[e]);
                    e += 1;
                }
                env[e] = NULL;
                assert(e == number_of_envs-1);

                // sendout an dbus-signal with all the values as
                // arguments
                dbus_send_signal(SCANBD_DBUS_SIGNAL_SCAN_BEGIN, st->dev->product);

                //dbus_send_signal_argv_async(SCANBD_DBUS_SIGNAL_TRIGGER, env);
                dbus_send_signal_argv(SCANBD_DBUS_SIGNAL_TRIGGER, env);

                // the action-script will use the device,
                // so we have to release the device
                //		scbtn_close(st->h);
                //		st->h = NULL;

                backend->scanbtnd_close((scanner_t*)st->dev);

                assert(st->triggered_option >= 0);
                assert(st->opts[st->triggered_option].script);
                assert(strlen(st->opts[st->triggered_option].script) > 0);

                // need to copy the values because we leave the
                // critical section
                // int triggered_option = st->triggered_option;
                char* script = strdup(st->opts[st->triggered_option].script);
                assert(script != NULL);

                // leave the critical section
                if (pthread_mutex_unlock(&st->mutex) < 0) {
                    // if we can't unlock the mutex, something is heavily wrong!
                    slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
                    pthread_exit(NULL);
                }

                if (strcmp(script, SCANBD_NULL_STRING) != 0) {

                    assert(timeout > 0);
                    usleep(timeout * 1000); //ms

                    pid_t cpid;
                    if ((cpid = fork()) < 0) {
                        slog(SLOG_ERROR, "Can't fork: %s", strerror(errno));
                    }
                    else if (cpid > 0) { // parent
                        slog(SLOG_INFO, "waiting for child: %s", script);
                        int status;
                        if (waitpid(cpid, &status, 0) < 0) {
                            slog(SLOG_ERROR, "waitpid: %s", strerror(errno));
                        }
                        if (WIFEXITED(status)) {
                            slog(SLOG_INFO, "child %s exited with status: %d",
                                 script, WEXITSTATUS(status));
                        }
                        if (WIFSIGNALED(status)) {
                            slog(SLOG_INFO, "child %s signaled with signal: %d",
                                 script, WTERMSIG(status));
                        }
                    }
                    else { // child
                        slog(SLOG_DEBUG, "exec for %s", script);
                        if (execle(script, script, NULL, env) < 0) {
                            slog(SLOG_ERROR, "execlp: %s", strerror(errno));
                        }
                        exit(EXIT_FAILURE); // not reached
                    }
                } // script == SCANBD_NULL_STRING

                assert(script != NULL);
                free(script);

                // free (last element is the sentinel!)
                assert(env != NULL);
                for(int e = 0; e < number_of_envs - 1; e += 1) {
                    assert(env[e] != NULL);
                    free(env[e]);
                }
                free(env);

                // enter the critical section
                if (pthread_mutex_lock(&st->mutex) < 0) {
                    // if we can't get the mutex, something is heavily wrong!
                    slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
                    pthread_exit(NULL);
                }

                st->triggered = false;
                st->triggered_option = -1; // invalid
                // we need to trigger all waiting threads
                if (pthread_cond_broadcast(&st->cv) < 0) {
                    slog(SLOG_ERROR, "pthread_cond_broadcats: this shouln't happen");
                }

                // leave the critical section
                if (pthread_mutex_unlock(&st->mutex) < 0) {
                    // if we can't release the mutex, something is heavily wrong!
                    slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
                    pthread_exit(NULL);
                }
                // sleep the timeout to settle devices, necessary?
                usleep(timeout * 1000); //ms

                // send out the debus signal
                dbus_send_signal(SCANBD_DBUS_SIGNAL_SCAN_END, st->dev->product);

                // enter the critical section
                if (pthread_mutex_lock(&st->mutex) < 0) {
                    // if we can't get the mutex, something is heavily wrong!
                    slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
                    pthread_exit(NULL);
                }

                slog(SLOG_DEBUG, "reopen device %s", st->dev->product);

                int ores = backend->scanbtnd_open((scanner_t*)st->dev);
                if (ores != 0) {
                    slog(SLOG_WARN, "scanbtnd_open failed, error code: %d", ores);
                    slog(SLOG_WARN, "abandon polling of %s", st->dev->product);
                    if (ores == -ENODEV) {
                        slog(SLOG_WARN, "scanbtnd_open failed, no device -> canceling thread");
                    }
                    if (alarm(SCANBUTTOND_ALARM_TIMEOUT) > 0) {
                        slog(SLOG_WARN, "alarm error, there was a pending alarm");
                    }
                    pthread_exit(NULL);
                }
            } // if triggered
        } // foreach option

        // release the mutex

        // because pthread_cleanup_pop is a macro we can't use it here
        //	 pthread_cleanup_pop(1);
        if (pthread_mutex_unlock(&st->mutex) < 0) {
            // if we can't unlock the mutex, something is heavily wrong!
            slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
            pthread_exit(NULL);
        }

        // sleep the polling timeout
        usleep(timeout * 1000); //ms

        // regain the mutex
        // because pthread_cleanup_push is a macro we can't use it here
        // pthread_cleanup_push(scbtn_thread_cleanup_mutex, ((void*)&st->mutex));
        if (pthread_mutex_lock(&st->mutex) < 0) {
            // if we can't get the mutex, something is heavily wrong!
            slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
            pthread_exit(NULL);
        }
    }
    pthread_cleanup_pop(1); // release the mutex
    pthread_exit(NULL);
}

void start_scbtn_threads() {
    slog(SLOG_DEBUG, "start_scbtn_threads");

    if (pthread_mutex_lock(&scbtn_mutex) < 0) {
        // if we can't get the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        return;
    }

    if (scbtn_poll_threads != NULL) {
        // if there are active threads kill them
        stop_scbtn_threads();
    }
    // allocate the thread list
    assert(scbtn_poll_threads == NULL);
    scbtn_poll_threads = (scbtn_thread_t*) calloc(num_devices, sizeof(scbtn_thread_t));
    if (scbtn_poll_threads == NULL) {
        slog(SLOG_ERROR, "Can't allocate memory for polling threads");
        goto cleanup;
    }
    // starting for each device a seperate thread
    slog(SLOG_DEBUG, "start the threads (%d)", num_devices);
    const scanner_t* dev = scbtn_device_list;
    for(int i = 0; i < num_devices && dev != NULL; i += 1, dev = dev->next) {
        slog(SLOG_DEBUG, "Starting poll thread for %s", dev->product);
        scbtn_poll_threads[i].tid = 0;
        scbtn_poll_threads[i].dev = dev;
        scbtn_poll_threads[i].opts = NULL;
        scbtn_poll_threads[i].functions = NULL;
        scbtn_poll_threads[i].num_of_options = 0;
        scbtn_poll_threads[i].triggered = false;
        scbtn_poll_threads[i].triggered_option = -1;
        scbtn_poll_threads[i].num_of_options_with_scripts = 0;
        scbtn_poll_threads[i].num_of_options_with_functions = 0;

        if (pthread_mutex_init(&scbtn_poll_threads[i].mutex, NULL) < 0) {
            slog(SLOG_ERROR, "pthread_mutex_init: should not happen");
        }
        if (pthread_cond_init(&scbtn_poll_threads[i].cv, NULL) < 0) {
            slog(SLOG_ERROR, "pthread_cond_init: should not happen");
        }
        if (pthread_create(&scbtn_poll_threads[i].tid, NULL, scbtn_poll,
                           (void*)&scbtn_poll_threads[i]) < 0) {
            slog(SLOG_ERROR, "Can't start scbtn_poll_thread: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        slog(SLOG_DEBUG, "Thread started for device %s", dev->product);
    }
    if (pthread_cond_broadcast(&scbtn_cv)) {
        slog(SLOG_ERROR, "pthread_cond_broadcast: %s", strerror(errno));
    }
cleanup:
    if (pthread_mutex_unlock(&scbtn_mutex) < 0) {
        // if we can't unlock the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
        return;
    }
}

void stop_scbtn_threads() {
    slog(SLOG_DEBUG, "stop_scbtn_threads");

    if (pthread_mutex_lock(&scbtn_mutex) < 0) {
        // if we can't get the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        return;
    }

    if (scbtn_poll_threads == NULL) {
        // we don't have any active threads
        slog(SLOG_DEBUG, "stop_scbtn_threads: nothing to stop");
        goto cleanup;
    }
    // sending cancel request to all threads
    const scanner_t* dev = scbtn_device_list;
    for(int i = 0; i < num_devices && dev != NULL; i += 1, dev = dev->next) {
        if (pthread_mutex_lock(&scbtn_poll_threads[i].mutex) < 0) {
            slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        }
        while(scbtn_poll_threads[i].triggered == true) {
            slog(SLOG_DEBUG, "stop_scbtn_threads: an action is active, waiting ...");

            if (pthread_cond_wait(&scbtn_poll_threads[i].cv,
                                  &scbtn_poll_threads[i].mutex) < 0) {
                slog(SLOG_ERROR, "pthread_cond_wait: %s", strerror(errno));
            }
        }
        if (pthread_mutex_unlock(&scbtn_poll_threads[i].mutex) < 0) {
            slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        }

        slog(SLOG_DEBUG, "stopping poll thread for device %s", dev->product);
        if (pthread_cancel(scbtn_poll_threads[i].tid) < 0) {
            if (errno == ESRCH) {
                slog(SLOG_ERROR, "poll thread for device %s was already cancelled");
            }
            else {
                slog(SLOG_ERROR, "unknown error from pthread_cancel: %s", strerror(errno));
            }
        }
        slog(SLOG_INFO, "cancelled thread for device %s", dev->product);
    }
    // waiting for all threads to vanish
    slog(SLOG_INFO, "waiting ...");
    dev = scbtn_device_list;
    for(int i = 0; i < num_devices && dev != NULL; i += 1, dev = dev->next) {
        slog(SLOG_DEBUG, "waiting for poll thread for device %s",
             dev->product);
        // joining all threads to prevent memory leaks
        if (pthread_join(scbtn_poll_threads[i].tid, NULL) < 0) {
            slog(SLOG_ERROR, "pthread_join: %s", strerror(errno));
        }
        scbtn_poll_threads[i].tid = 0;
        // close the associated device of the thread
        slog(SLOG_DEBUG, "closing device %s", scbtn_poll_threads[i].dev->product);
        assert(scbtn_poll_threads[i].dev);
        backend->scanbtnd_close((scanner_t*)scbtn_poll_threads[i].dev);

        if (scbtn_poll_threads[i].opts) {
            slog(SLOG_DEBUG, "freeing opt ressources for device %s thread",
                 scbtn_poll_threads[i].dev->product);
            // free the matching options list of that device / threads
            //	    for (int k = 0; k < scbtn_poll_threads[i].num_of_options; k += 1) {
            //		scbtn_option_value_free(&scbtn_poll_threads[i].opts[k].from_value);
            //		scbtn_option_value_free(&scbtn_poll_threads[i].opts[k].to_value);
            //		scbtn_option_value_free(&scbtn_poll_threads[i].opts[k].value);
            //	    }
            free(scbtn_poll_threads[i].opts);
            scbtn_poll_threads[i].opts = NULL;
        }
        if (scbtn_poll_threads[i].functions) {
            slog(SLOG_DEBUG, "freeing funtion ressources for device %s thread",
                 scbtn_poll_threads[i].dev->product);
            free(scbtn_poll_threads[i].functions);
            scbtn_poll_threads[i].functions = NULL;
        }

        if (pthread_cond_destroy(&scbtn_poll_threads[i].cv) < 0) {
            slog(SLOG_ERROR, "pthread_cond_destroy: %s", strerror(errno));
        }
        if (pthread_mutex_destroy(&scbtn_poll_threads[i].mutex) < 0) {
            slog(SLOG_ERROR, "pthread_mutex_destroy: %s", strerror(errno));
        }
    }
    // free the thread list
    free(scbtn_poll_threads);
    scbtn_poll_threads = NULL;
    // no threads active anymore
    if (pthread_cond_broadcast(&scbtn_cv)) {
        slog(SLOG_ERROR, "pthread_cond_broadcast: %s", strerror(errno));
    }
cleanup:
    if (pthread_mutex_unlock(&scbtn_mutex) < 0) {
        // if we can't unlock the mutex, something is heavily wrong!
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
        return;
    }
}

void scbtn_trigger_action(int number_of_dev, int action) {
    assert(number_of_dev >= 0);
    assert(action >= 0);
    slog(SLOG_DEBUG, "sane_trigger_action device=%d, action=%d", number_of_dev, action);

    if (pthread_mutex_lock(&scbtn_mutex) < 0) {
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        return;
    }
    if (num_devices <= 0) {
        slog(SLOG_WARN, "No devices at all");
        goto cleanup_scbtn;
    }
    if (number_of_dev >= num_devices) {
        slog(SLOG_WARN, "No such device number %d", number_of_dev);
        goto cleanup_scbtn;
    }

    while(scbtn_poll_threads== NULL) {
        // no devices actually polling
        slog(SLOG_WARN, "No polling at the moment, waiting ...");
        if (pthread_cond_wait(&scbtn_cv, &scbtn_mutex) < 0) {
            slog(SLOG_ERROR, "pthread_cond_wait: ", strerror(errno));
            goto cleanup_scbtn;
        }
    }
    assert(scbtn_poll_threads != NULL);
    scbtn_thread_t* st = &scbtn_poll_threads[number_of_dev];
    assert(st != NULL);

    // this thread uses the device and the sane_thread_t datastructure
    // lock it
    if (pthread_mutex_lock(&st->mutex) < 0) {
        slog(SLOG_ERROR, "pthread_mutex_lock: %s", strerror(errno));
        goto cleanup_scbtn;
    }

    if (action >= st->num_of_options_with_scripts) {
        slog(SLOG_WARN, "No such action %d for device number %d", action, number_of_dev);
        goto cleanup_dev;
    }

    while(st->triggered == true) {
        slog(SLOG_DEBUG, "sane_trigger_action: an action is active, waiting ...");
        if (pthread_cond_wait(&st->cv, &st->mutex) < 0) {
            slog(SLOG_ERROR, "pthread_cond_wait: %s", strerror(errno));
            goto cleanup_dev;
        }
    }

    slog(SLOG_DEBUG, "sane_trigger_action: an action is active, waiting ...");

    st->triggered = true;
    st->triggered_option = action;
    // we need to trigger all waiting threads
    if (pthread_cond_broadcast(&st->cv) < 0) {
        slog(SLOG_ERROR, "pthread_cond_broadcats: this shouln't happen");
    }

cleanup_dev:
    if (pthread_mutex_unlock(&st->mutex) < 0) {
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
    }
cleanup_scbtn:
    if (pthread_mutex_unlock(&scbtn_mutex) < 0) {
        slog(SLOG_ERROR, "pthread_mutex_unlock: %s", strerror(errno));
    }
    return;
}

void scbtn_shutdown(void)
{
    slog(SLOG_INFO, "shutting down...");
    backend->scanbtnd_exit();
    scanbtnd_unload_backend(backend);
    scanbtnd_loader_exit();
    slog(SLOG_INFO, "shutdown complete");
    closelog();
}

const char* scanbtnd_button_name(const backend_t* backend, unsigned int button) {
    slog(SLOG_INFO, "scanbtnd_button_name (%d)", button);
    const char* backend_name = backend->scanbtnd_get_backend_name();
    slog(SLOG_INFO, "scanbtnd_button_name, backend: %s", backend_name);
    assert(backend_name);

    if (strcmp("snapscan", backend_name)) {
        assert(button <= 4);
        switch(button) {
        case 0:
            return NULL;
            break;
        case 1:
            return "web";
            break;
        case 2:
            return "email";
            break;
        case 3:
            return "copy";
            break;
        case 4:
            return "send";
            break;
        default:
            return NULL;
            break;
        }
    }
    return NULL;
}
