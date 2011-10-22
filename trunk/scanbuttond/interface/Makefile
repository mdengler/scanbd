#
# $Id$
#
#  scanbd - KMUX scanner button daemon
#
#  Copyright (C) 2008  Wilhelm Meier (wilhelm.meier@fh-kl.de)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

OSTYPE = $(shell uname)

CFLAGS   += -Wall -Wextra -std=c99 -g

ifeq ($(OSTYPE),NetBSD)
CPPFLAGS += -DNetBSD -I/usr/pkg/include -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN
endif

ifeq ($(OSTYPE),FreeBSD)
CPPFLAGS += -DFreeBSD -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN
endif 

ifeq ($(OSTYPE),OpenBSD)
CPPFLAGS += -DOpenBSD -I../include -I/usr/local/include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN
endif

ifeq ($(OSTYPE),Linux)
CPPFLAGS += -DLinux -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN
endif 

all: libusbi.o

libusbi.o: libusbi.c 

clean:
	$(RM) *.o *~
