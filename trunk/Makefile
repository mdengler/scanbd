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

PREFIX = /usr/local
SCANBD_DIR = $(PREFIX)/etc/scanbd
BIN_DIR = $(PREFIX)/bin

USE_HAL=yes

ifndef USE_SCANBUTTOND
USE_SANE=yes # otherwise USE_SCANBUTTOND
endif

ifdef NDEBUG
CPPFLAGS += -DNDEBUG
endif

CPPFLAGS += -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include
CFLAGS   += -Wall -Wextra -std=c99 -g
LDFLAGS  += -lconfuse -lpthread -ldbus-1

ifdef USE_SANE

CPPFLAGS += -DUSE_SANE -UUSE_SCANBUTTOND
LDFLAGS += -lsane

else # USE_SANE

CFG_DIR=$(SCANBD_DIR)/scanbuttond/backends
export CFG_DIR
CPPFLAGS += -UUSE_SANE -DUSE_SCANBUTTOND -I./scanbuttond/include -DCFG_DIR=\"$(CFG_DIR)\"
LDFLAGS += -rdynamic -lusb -ldl

endif # USE_SANE

ifdef USE_HAL
CPPFLAGS += -DUSE_HAL
LDFLAGS  += -lhal
endif # USE_HAL

.PHONY: scanbuttond all

ifdef USE_SANE

all: scanbd

scanbd: scanbd.o slog.o sane.o daemonize.o dbus.o

else # USE_SANE

all: scanbuttond scanbd test

testonly: scanbuttond test

scanbd: scanbd.o slog.o daemonize.o dbus.o scanbuttond_wrapper.o scanbuttond_loader.o
	$(LINK.c) $^ scanbuttond/interface/libusbi.o -o $@

test: test.o scanbuttond_loader.o slog.o scanbuttond_wrapper.o dbus.o
	$(LINK.c) $^ scanbuttond/interface/libusbi.o -o $@

endif # USE_SANE

scanbuttond_wrapper.o: scanbuttond_wrapper.c scanbuttond_wrapper.h

scanbuttond_loader.o: scanbuttond_loader.c scanbuttond_loader.h

scanbd.o: scanbd.c scanbd.h common.h slog.h scanbd_dbus.h

dbus.o: dbus.c scanbd.h common.h slog.h scanbd_dbus.h

slog.o: slog.c common.h

daemonize.o: daemonize.c common.h

sane.o: sane.c scanbd.h common.h

scanbuttond:
	$(MAKE) -C scanbuttond all

clean:
	$(MAKE) -C scanbuttond clean
	$(RM) -f scanbd test *.o *~

install: scanbd
	echo "Make $(SCANBD_DIR)"
	mkdir -p $(SCANBD_DIR)
	echo "Copy files to $(SCANBD_DIR)"
	cp scanbd.conf $(SCANBD_DIR)
	cp example.script $(SCANBD_DIR)
	cp scanadf.script $(SCANBD_DIR)
	cp test.script $(SCANBD_DIR)
	echo "Copy scanbd to $(BIN_DIR)"
	cp scanbd $(BIN_DIR)
	echo "Copy scanbuttond backends to $(SCANBD_DIR)/scanbuttond/backends"
	mkdir -p "$(SCANBD_DIR)/scanbuttond/backends"
	cp scanbuttond/backends/*.so "$(SCANBD_DIR)/scanbuttond/backends"
	echo "Copy scanbd_dbus.conf to /etc/dbus-1/system.d/"
	cp scanbd_dbus.conf /etc/dbus-1/system.d/
	echo "Edit /etc/inetd.conf"
