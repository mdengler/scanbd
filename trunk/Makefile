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

#CPPFLAGS += -DNDEBUG

CPPFLAGS += -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -I/usr/include/hal

CFLAGS += -Wall -Wextra -std=c99

LDFLAGS += -lconfuse -lsane -lpthread -ldbus-1 -lhal -lhal-storage 

scanbd: scanbd.o slog.o sane.o daemonize.o dbus.o

scanbd.o: scanbd.c scanbd.h common.h slog.h scanbd_dbus.h

dbus.o: dbus.c scanbd.h common.h slog.h scanbd_dbus.h

slog.o: slog.c common.h

daemonize.o: daemonize.c common.h

sane.o: sane.c scanbd.h common.h

clean:
	$(RM) -f scanbd *.o *~

install: scanbd
	echo "Copy scanbd"
	echo "Edit /etc/inetd.conf"
	echo "Install scan scripts"
