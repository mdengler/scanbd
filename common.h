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

#ifndef COMMON_H
#define COMMON_H

// we require POSIX 200809 compatibility

#define _POSIX_C_SOURCE 200809L
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE	700
#endif

#ifdef FreeBSD
// otherwise usleep() isn't declared
#define __BSD_VISIBLE 1
#endif

#if defined(__STDC__) && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#endif

#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <assert.h>
#include <regex.h>

#if defined(_POSIX_THREADS) && ((_POSIX_THREADS - 0) >= 0)
#include <pthread.h>
#endif

#if (_POSIX_MAPPED_FILES - 0) >= 0 || (_POSIX_SHARED_MEMORY_OBJECTS - 0) >= 0 || \
    (_POSIX_MEMLOCK - 0) >= 0 || (_POSIX_MEMORY_PROTECTION - 0) >= 0 || \
    (_POSIX_TYPED_MEMORY_OBJECTS - 0) >= 0 || (_POSIX_SYNCHRONIZED_IO - 0) >= 0 || \
    (_POSIX_ADVISORY_INFO - 0) >= 0
#include <sys/mman.h>
#endif

#if (_POSIX_MAPPED_FILES - 0) >= 0 || (_POSIX_SHARED_MEMORY_OBJECTS - 0) >= 0 || \
    (_POSIX_TYPED_MEMORY_OBJECTS - 0) >= 0
#define _MC3 /* see POSIX Standard margin notes */
#endif

#if defined(_POSIX_MESSAGE_PASSING) && ((_POSIX_MESSAGE_PASSING - 0) >= 0)
#include <mqueue.h>
#endif

#ifdef _XOPEN_UNIX
#include <strings.h>
#ifndef FreeBSD
#include <utmpx.h>
#else
#include <utmp.h>
#endif
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#ifndef CYGWIN
#include <ftw.h>
#endif
#endif

#if __cplusplus > 199711L || __GXX_EXPERIMENTAL_CXX0X__
#define CXX11
#endif

#define PIPE_READ 0
#define PIPE_WRITE 1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
