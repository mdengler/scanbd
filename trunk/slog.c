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

#include "common.h"

bool debug = false;
unsigned int debug_level = 0;

static char lpre[LINE_MAX];
static int isInitialized = 0;

void slog_init(const char *string) {
    strncpy(lpre, string, LINE_MAX);
    isInitialized = 1;
}

void
slog(unsigned int level, const char *format, ...) {
    va_list	ap;
    char	buffer[LINE_MAX]="";
    char	*p = NULL;
    char* 	string = NULL;

    if (isInitialized == 0) {
	slog_init("");
	isInitialized = 1;
    }

    if (!(level <= debug_level))
	return;
    
    va_start(ap, format);
    for (p = (char *) format; (*p != '\0') && (strlen(buffer) < LINE_MAX); p++) {
	if (*p != '%') {
	    sprintf(buffer, "%s%c", buffer, *p);
	    continue;
	}
	switch (*++p) {
	case 'c':
	    sprintf(buffer, "%s%c", buffer, (char)va_arg(ap, int));
	    break;
	case 'd':
	    sprintf(buffer, "%s%d", buffer, va_arg(ap, int));
	    break;
	case 'f':
	    sprintf(buffer, "%s%f", buffer, va_arg(ap, double));
	    break;
	case 's':
	    string = va_arg(ap, char *);
	    if ((strlen(buffer) + strlen(string)) < LINE_MAX)
		sprintf(buffer, "%s%s", buffer, string);
	    break;
	case 'x':
	    sprintf(buffer, "%s%x", buffer, va_arg(ap, int));
	    break;
	case '%':
	    sprintf(buffer, "%s%%", buffer);
	    break;
	default:
	    sprintf(buffer, "%s%c", buffer, *p);
	}
    }
    va_end(ap);

    if (debug) {
	printf("%s: %s\n", lpre, buffer);
	syslog(LOG_AUTHPRIV | LOG_DEBUG, "%s: %s\n", lpre, buffer);
    }
    else {
	syslog(LOG_AUTHPRIV | LOG_DEBUG, "%s: %s\n", lpre, buffer);
    }
}
