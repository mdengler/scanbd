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

void daemonize(void) {
    pid_t pid;

    if ((pid = fork()) < 0) {
	perror("fork");
	exit(EXIT_FAILURE);
    }
    else if (pid > 0) { // Parent
	exit(EXIT_SUCCESS);
    }
    // Child
    if (setsid() < 0) {
	perror("setsid");
	exit(EXIT_FAILURE);
    }
    if ((pid = fork()) < 0) {
	perror("fork");
	exit(EXIT_FAILURE);
    }
    else if (pid > 0) { // Parent
	exit(EXIT_SUCCESS);
    }
    // Child-Child
    int ofd;
    if ((ofd = open("/dev/null", O_RDWR)) < 0) {
	perror("open /dev/null");
	exit(EXIT_FAILURE);
    }
    if (dup2(ofd, STDIN_FILENO) < 0) {
	perror("dup2");
	exit(EXIT_FAILURE);
    }
    if (dup2(ofd, STDOUT_FILENO) < 0) {
	perror("dup2");
	exit(EXIT_FAILURE);
    }
    if (dup2(ofd, STDERR_FILENO) < 0) {
	perror("dup2");
	exit(EXIT_FAILURE);
    }
    if (chdir("/") < 0) {
	perror("chdir");
	exit(EXIT_FAILURE);
    }
}
