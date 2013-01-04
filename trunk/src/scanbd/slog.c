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

#include "common.h"
#include "slog.h"

bool debug = false;
unsigned int debug_level = 0;

static char lpre[LINE_MAX] = "";
static int isInitialized = 0;

void slog_init(const char *string) {
    strncpy(lpre, string, LINE_MAX);
    isInitialized = 1;
}

void
slog(unsigned int level, const char *format, ...) {
    va_list	ap;
    char	buffer[LINE_MAX] = "";

    if (isInitialized == 0) {
        slog_init("");
        isInitialized = 1;
    }
    if (!(level <= debug_level))
        return;
    
    va_start(ap, format);
    vsnprintf(buffer, LINE_MAX, format, ap);
    va_end(ap);

    if (debug) {
        printf("%s: %s\n", lpre, buffer);
        syslog(LOG_DAEMON | LOG_DEBUG, "%s: %s\n", lpre, buffer);
    }
    else {
        syslog(LOG_DAEMON | LOG_DEBUG, "%s: %s\n", lpre, buffer);
    }
}
