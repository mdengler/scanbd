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

.PHONY: backends interface clean

all: backends interface

backends:
	$(MAKE) -C backends all

interface:
	$(MAKE) -C interface all


clean:
	$(MAKE) -C backends clean
	$(MAKE) -C interface clean
	$(RM) *~
