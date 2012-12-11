$Id$

scanbd - KMUX scanner button daemon

Copyright (C) 2008, 2009, 2010, 2011, 2012  Wilhelm Meier (wilhelm.meier@fh-kl.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

0.0) the problem

The majority of the desktop scanners are more or less "passive" devices, say they can be 
queried from a suitable application but they don't do anything themselves. The more expensive
departmental scanners typically include a network interface and the capability to send the scanned 
images via email or place them on a fileserver.

scanbd tries to solve the problem with managing the inexpensive, desktop scanners to make use of the
scanner-buttons they have (in the case, they are supported by sane).
Because the scanners are passive, the scan-software has to poll the buttons on a regularly basis (e.g. all
100 - 500ms). For this purpose the software has to open the device and look for value changes of the buttons.
But if you do so, the device is locked and no other scan-application (e.g. xsane) can open the device for 
scanning. This is also true for saned, the network scanning daemon. Periodically querying the buttons
and scanning on the same host is mutual exclusive.

0.1) the solution

scanbd (the scanner button daemon) opens and polls the scanner and therefore locks the device. So no other 
application can access the device directly (open the /dev/..., or via libusb, etc). To solve this, we use a so called
"manager-mode" of scanbd: scanbd is configured as a "proxy" to access the scanner and, if another application
tries to use the scanner, the polling is disabled for the time the other scan-application wants to
use the scanner. To make this happen, scanbd is configured instead of saned as the network scanning daemon. If
a scan request arrives to scanbd (in manager-mode) on the sane-port, scanbd stops the polling by sending
a dbus-signal (default) or a posix-signal (SIGUSR1) (signal-mode) to the polling scanbd-daemon. Then it 
starts the real saned which scans and sends the data back to the requesting application. Afterwords the scanbd-manager
restarts the polling by sending another dbus-signal (or the posix-signal SIGUSR2) to the scanbd. scanbd now reenables
polling of the devices.

So all applications must be enabled to use network-scanning (even if the scanner is local, see below)!

1) build

There are two general options for building scanbd: using configure (preferred) (see 1.1) or using the (old) 
simple Makefiles (see 1.2).

1.1) using autotools configure script

Please execute:

./configure [--enable-scanbuttond]
make all

The use of HAL instead of libudev is now detected via configure. 
If you want to use the scanbuttond-backends instead of sane-backends, use --enable-scanbuttond for configure.
For all other options consult the output of

./configure --help

If neither HAL nor libudev is available (e.g. OpenBSD), scanbd works without dynamic detection of inserted or
removed devices (but can be triggered by sending SIGHUP).

1.2) using plain Makefiles

If you don't want to use the configure script to generate the Makefiles, you can still use the (old) Makefile.simple 
as an input-file for the make tool.
scanbd now uses libudev by default instead of HAL.
If you want scanbd to use HAL, comment the line in the Makefile.simple (USE_LIBUDEV=yes).
If you want to use the sane-backends, set USE_SANE=yes in the Makefile.simple. Otherwise the
scanbuttond backends are used.

make -f Makefile.simple clean all
-or-
USE_SCANBUTTOND=yes make -e Makefile.simple clean all
-or-
USE_SANE=yes make -e Makefile.simple clean all

Then copy the executable scanbd and config-files scanbd.conf, etc. to useful places
(or use "make install" to copy to /usr/local/bin and /usr/local/etc/scanbd).
If you use the scanbuttond-backends, copy the shared-objects from scanbuttond/backends directory
to /usr/local/etc/scanbd/scanbuttond/backends (or use "make install").

Needed packages on debian-based systems:
libconfuse-dev libsane-dev libudev-dev libusb-dev

To use HAL instead of libudev you need too:
libhal-dev

Hint to ArchLinux users using old scanbuttond drivers:
The libusb / libusb-compat seems to be broken for at least using it with the old scanbuttond drivers.
Please find under contrib a PKGBUILD to build an old version of libusb-compat at your own risk (other applications
may fail now). Do the following:

cd contrib
makepkg
makepkg -i

and (re)build scanbd:
USE_SCANBUTTOND=yes make -e clean all

2) scanbd_dbus.conf

copy this file into the dbus system-bus configuration directory,
then restart dbus and hald.

3) inetd.conf

edit the saned-line to use scanbd as a wrapper, e.g.:
---
sane-port stream tcp4 nowait saned /usr/local/bin/scanbd scanbd -m -c /usr/local/etc/scanbd/scanbd.conf
---

or 

(if the saned user can't open the devices, e.g. ubuntu)
---
sane-port stream tcp4 nowait root /usr/local/bin/scanbd scanbd -m -c /usr/local/etc/scanbd/scanbd.conf
---

(if you are worried about running scanbd with root privileges, see below on 7)

be sure to give the -m (and perhaps -s) flag to scanbd to enable the "manager-mode" (if you
add also the -s flag the "signal-mode" is used, e.g. if dbus isn't available).

4) scanbd.conf

edit the config file (in /usr/local/etc/scanbd)

5) scripts

make some useful scripts (see the examples test.script, example.script or scanadf.script)
(scanbd compiles and runs well on FreeBSD/NetBDS/OpenBSD, but on these plattforms /bin/bash is usually not
avaliable. So be sure to adapt the scripts!)

6) sane config

all desktop applications should only get access to the scanners via the net-backend, so edit /etc/sane.d/dll.conf
to only contain the net-backend on the desktop machines. 

example dll.conf:
---
net
---

Give the net-backend a suitable configuration and add the machines, where the 
scanners (and scanbd) are (this could be also localhost, but see below!):

example net.conf
---
connect_timeout = 3
scanbd-host1.local.net   # the host with the scanbd running (and the attached scanners with buttons)
# scanbd-host2.local.net # there can be more than one scanbd one the net (and attached scanners with buttons)
# sane-host.local.net    # there can also be a sane host without scanbd running (and scanner without buttons)
# localhost              # if desktop machine with scanbd AND scan-applications like xsane
---

IMPORTANT:
If scanbd is running on a desktop machine (e.g. a machine using a sane-frontend: xsane, scanimage), saned needs another config 
which only includes local scanners. 

Therefore the env-var SANE_CONFIG_DIR is used in the debian-start-stop-script and the upstart-job of scanbd (scanbdservice.conf). 
Put the directory of this dll.conf into this env-var (e.g. /usr/local/etc/scanbd) and remove the net-backend from this config-file. 
All other backends can be included as needed. Now the desktop applications (wich use libsane) use the above dll.conf with only
the net backend. This prevents them from using the locally attached scanners directly (and blocking them). 
If a scan is issued, the scan application contacts (via net backend) the scanbd, whih in turn starts saned with the environment variable
SANE_CONFIG_DIR set to /usr/local/etc/scanbd (or any other directory, see scanbd.conf). Here saned and therefore libsane 
find a different dll.conf without the net-backend but with all needed local scanner backends!

So as a safe rule: 
- the file /etc/sane.d/dll.conf should only contain the net backend   
- copy all config-files from /etc/sane.d to /usr/local/etc/scanbd and edit dll.conf as stated above (not include the net backend)

6) start the scanbd 

e.g.:

export SANE_CONFIG_DIR=/usr/local/etc/scanbd
/usr/local/bin/scanbd -c /usr/local/etc/scanbd/scanbd.conf

or use the debian script (scanbd.script) or the upstart service (scanbdservice.conf)

If you are unfamiliar with scanbd it would be best to first try starting scanbd in the foreground (-f)
and in debug mode (-d):

/usr/local/bin/scanbd -df -c /usr/local/etc/scanbd/scanbd.conf

Then you can watch scanbd recognizing all scanners and polling the options / buttons. If you press the buttons
or modify the function knob or insert/remove sheets of paper some of the options / buttons must change their value.
Theses value changes can be used defining actions in scanbd.conf.

7) some words on access rights

if the saned-user can't access the scanners, e.g. if 

SANE_CONFIG_DIR=/usr/local/etc/scanbd scanimage -L

doesn't get any results, but if you run the same command as root with success, you have wrong access rights to the 
scanner devices. If you have a scan-only device (not an all-in-one type), use the file 99-saned.rules
and copy this file to /lib/udev/rules.d and then issue a rescan:

udevadm trigger

If you have an all-in-one scanner the device is matched via a printer rule and the access is granted to the
lp group. In this case you should simply add the saned user to the lp group:

usermod -a -G lp saned

Also edit/check scanbd.conf to set the effective group = lp !

8) Additional hints:

- There is another howto for scanbd: https://bbs.archlinux.de/viewtopic.php?id=20954 by <alex@tomisch.de>
  It is written for ArchLinux but will also be valuable for other distros.

- if there are problems recognizing new / removed devices, it is possible to send scanbd the SIGHUP signal to
  reconfigure and look for new devices. This can be done also via udev-rules.

- be sure to include only the necessary drivers in /usr/local/scanbd/dll.conf, since scanadf will hang (timeout)

- to test an action for a specific device, use

  scanbd -m -t <device_number> -a <name_of_action>

  e.g. for the first scanner found trigger the action "scan" (action name as in config file scanbd.conf)

  scanbd -m -t 0 -a scan

- problematic desktop scan applications

  most desktop-scan-applications open the scanner but never closes them, therefore the saned is started but 
  never (as long as the scan-application runs) exits. This blocks the scanbd from gaining again the control 
  over the scanner and go on polling. This can also be a problem of the net backend (see connect_timeout parameter
  in net.conf).
  The better way to do scanning would be to first get a list of available scanners (scanimage -L or scanadf -L),
  store the list but then close the connection to the found scanners. If a scan is requested, a device from
  the list is choosen, the scan happens and thereafter the connection to the scanner is closed!

  One can achieve this with simple shell scripting:
  a) to get a list of all scanners available: scanimage -L
  b) select a scanner from the list
  c) do the scan: scanadf -d <device>
  
  Or use a web based application for scanning (see the frontends documentation on the sane-project page)

9) Actual problems

- if you use USE_SCANBUTTOND: the usb-device can't be opened if another applications (say cupsd) does the
  same. In this case scanbd stops polling the device for a predefined amount of time and restarts polling
  after that time again.

- dynamic device recognition:
  HAL(hald) is deprecated: on recent distributions hald isn't used or isn't started by default anymore.
  udisks online sends out storage related dbus events (no DeviceAdded events for scanner, etc., that's why
  it is called udisks ;-)). So, if you have udisks-daemon and can't start hald, be sure to compile scanbd 
  with USE_LIBUDEV=yes. If you don't have hal and libudev e.g. OpenBSD, dynamic recognition of scanners
  isn't possible.

10) TODO
- refactor dbus.c, udev.c
- make a clean rewrite using boost/Qt
- make a UI to list options/buttons, watch value changes an setup actions
