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
CPPFLAGS += -DNetBSD -I/usr/pkg/include -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN -DCFG_DIR=$(CFG_DIR)
endif

ifeq ($(OSTYPE),Linux)
CPPFLAGS += -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN -DCFG_DIR=$(CFG_DIR)
endif 

ifeq ($(OSTYPE),FreeBSD)
CPPFLAGS += -I../include -Dsyslog=slog -DLOG_INFO=SLOG_INFO -DLOG_WARNING=SLOG_WARN -DCFG_DIR=$(CFG_DIR)
endif 

%.so: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared -o $@ $<

all: mustek.o plustek.o plustek_umax.o snapscan.so hp3500.so meta.so niash.so artec_eplus48u.o epson.o genesys.o gt68xx.o hp3900.o hp5590.o

snapscan.so: snapscan.c snapscan.h

hp3500.so: hp3500.c hp3500.h

meta.so: meta.c meta.h

niash.so: niash.c niash.h

clean:
	$(RM) *.o *~ *.so
