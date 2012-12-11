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

#ifndef SCANBUTTOND_WRAPPER_H
#define SCANBUTTOND_WRAPPER_H

#include "common.h"

#ifndef USE_SANE
#include "scanbuttond_loader.h"

extern backend_t* backend; // in scanbd.c

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
void scbtn_init_mutex();
#endif

void get_scbtn_devices(void);
const char* scanbtnd_button_name(const backend_t* backend, unsigned int button);
void start_scbtn_threads(void);
void stop_scbtn_threads(void);
void scbtn_trigger_action(int number_of_dev, int action);
void scbtn_shutdown(void);

#endif

#endif // SCANBUTTOND_WRAPPER_H
