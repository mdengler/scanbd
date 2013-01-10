$Id$

scanbd - KMUX scanner button daemon

Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013  Wilhelm Meier (wilhelm.meier@fh-kl.de)

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

The majority of the desktop scanners are more or less "passive" devices, say 
they can be queried from a suitable application but they don't do anything 
themselves. The more expensive departmental scanners typically include a 
network interface and the capability to send the scanned images via email 
or place them on a fileserver.

scanbd tries to solve the problem with managing the inexpensive, desktop 
scanners to make use of the scanner-buttons they have (in the case, they are 
supported by sane).  Because the scanners are passive, the scan-software has 
to poll the buttons on a regularly basis (e.g. all 100 - 500ms). For this 
purpose the software has to open the device and look for value changes of the 
buttons.  But if you do so, the device is locked and no other scan-application 
(e.g. xsane) can open the device for scanning. This is also true for saned, 
the network scanning daemon. Periodically querying the buttons and scanning on 
the same host is mutual exclusive.

0.1) the solution

scanbd (the scanner button daemon) opens and polls the scanner and therefore 
locks the device. So no other application can access the device directly (open 
the /dev/..., or via libusb, etc). 

To solve this, we use a second daemon (in the so called "manager-mode" of 
scanbd): scanbm is configured as a "proxy" to access the scanner and, if another 
application tries to use the scanner, the polling daemon is ordered to disable 
polling for the time the other scan-application wants to use the scanner. 

To make this happen, scanbm is configured instead of saned as the network 
scanning daemon. If a scan request arrives to scanbm on the sane-port, scanbm 
stops the polling by sending a dbus-signal (default) or a posix-signal (SIGUSR1) 
(signal-mode) to the polling scanbd-daemon. Then it starts the real saned which 
scans and sends the data back to the requesting application. Afterwards the 
scanbd-manager scanbm restarts the polling by sending another dbus-signal (or the
posix-signal SIGUSR2) to scanbd. scanbd now reenables polling of the devices.

Scanbm is actually only a symbolic link to scanbd. Manager mode can be 
activatied by calling scanbd as scanbm OR as scanbd -m as in previous version
of scanbd.

So all applications must be enabled to use network-scanning (even if the 
scanner is local, see below)!

1) build
There are two general options for building scanbd: using configure (preferred) 
(see 1.2) or using the (old) simple Makefiles (see 1.3). 

===============================================================================
IMPORTANT NOTE: Both solutions require GNU make, so on systems where make is not 
GNU make, you may have to use gmake instead of make
===============================================================================

1.1) Dependencies

This chapter lists packages that are known to be required for compilation of 
scanbd on different platforms. The list may not be exhaustive.

1.1.1 Debian
Needed packages on debian-based systems:
libconfuse-dev libsane-dev libudev-dev libusb-dev

To use HAL instead of libudev you need:
libhal-dev

1.1.2) Fedora
Needed packages in Fedora systems:
libusb-devel libconfuse-devel libudev-devel dbus-devel sane-backends-devel

1.1.3 Suse
On Suse based systems you will need the same of similar packages installed as
on Fedora, but: sane-backends-devel in Suse does not pull in all required 
dependencies. Please make sure that you alse have installed:
libjpeg8-devel
libexif-devel
libgphoto2-devel
before you attempt compilation of scanbd.

1.1.4 OpenBSD:
You need to install the following packages from ports ($PORTSDIR is here the
directory where you have the ports collection installed, normally /usr/ports):

libconfuse: $PORTSDIR/ports/devel/libconfuse
dbus: $PORTSDIR/
libusb1: $PORTSDIR/devel/libusb1
sane-backends: $PORTSDIR/graphics/sane-backends

1.2) using autotools configure script

Please execute:

./configure [--enable-scanbuttond]
make all

The use of HAL instead of libudev is now detected via configure. 
If you want to use the scanbuttond-backends instead of sane-backends, use 
--enable-scanbuttond for configure.

For all other options consult the output of

./configure --help

If neither HAL nor libudev is available (e.g. OpenBSD), scanbd works without 
dynamic detection of inserted or removed devices (but can be triggered by 
sending SIGHUP).

1.3) using plain Makefiles

If you don't want to use the configure script to generate the Makefiles, you 
can still use the (old) Makefile.simple as an input-file for the make tool.
scanbd now uses libudev by default instead of HAL. If you want scanbd to use HAL,
comment the line

USE_LIBUDEV=yes 

out for your OS in the Makefile.conf.

If you want to use the the scanbuttond backends enable

USE_SCANBUTTOND=yes

at the start of Makefile.conf. Otherwise sane is used. You could also set the
variable on the make command-line:

make -f Makefile.simple clean all

-or-

USE_SCANBUTTOND=yes make -e Makefile.simple clean  all

-or-

USE_SANE=yes make -e Makefile.simple clean all

After building, copy the executable scanbd and config-files scanbd.conf, etc. 
to useful places (or use 

USE_SCANBUTTOND make install

-or-

USE_SANE make install

to copy to /usr/local/bin and /usr/local/etc/scanbd).

If you use the scanbuttond-backends, copy the shared-objects from 
scanbuttond/backends directory to /usr/local/lib/scanbd/scanbuttond/backends
(make install as described above does this for you).

Needed packages on debian-based systems:
libconfuse-dev libsane-dev libudev-dev libusb-dev

To use HAL instead of libudev you need:
libhal-dev

Needed packages in Fedora systems:
libusb-devel libconfuse-devel libudev-devel dbus-devel sane-backends-devel

Hint to ArchLinux users using old scanbuttond drivers:
The libusb / libusb-compat seems to be broken for at least using it with the 
old scanbuttond drivers.  Please find under contrib a PKGBUILD to build an old 
version of libusb-compat at your own risk (other applications may fail now). 
Do the following:

cd contrib
makepkg
makepkg -i

and (re)build scanbd:
USE_SCANBUTTOND=yes make -e clean all

2) scanbd_dbus.conf

copy this file into the dbus system-bus configuration directory 
(/etc/dbus-1/system.d/ or /usr/local/etc/dbus-1/system.d/),
then restart dbus and hald.

3.1) inetd.conf

edit the saned-line to use scanbd as a wrapper, e.g.:
---
sane-port stream tcp4 nowait saned /usr/local/bin/scanbm scanbm 
---

-or-

(if the saned user can't open the devices, e.g. ubuntu)
---
sane-port stream tcp4 nowait root /usr/local/bin/scanbm scanbm 
---

If you installed the config files in a different location, you may have to add
the  -c option to point to the configuration file with an absolute path (add
-c /your/location/scanbd.conf to the configuation lines above).

If you are worried about running scanbd with root privileges, see below on 7).

Be sure to use scanbm or give the -m (and perhaps -s) flag to scanbd to enable
the "manager-mode" (if you add also the -s flag the "signal-mode" is used, e.g. 
if dbus isn't available).

3.2) xinetd

Systems with xinetd instead of inetd (e.g. openSuse) must use a xinetd configuration
file for sane. This is mostly /etc/xinetd.d/sane-port. The following is an example of
sane-port:

service sane-port
{
        port        = 6566
        socket_type = stream
        wait        = no
        user        = saned
        group       = scanner
        server      = /usr/local/bin/scanbm
        server_args = scanbm
        disable     = no
}

4) scanbd.conf

edit the config file (in /usr/local/etc/scanbd)

5) scripts

Make some useful scripts (see the examples test.script, example.script or
scanadf.script) (scanbd compiles and runs well on FreeBSD/NetBDS/OpenBSD, 
but on these plattforms /bin/bash is usually not avaliable. So be sure to adapt
the scripts!)

6) sane config

All desktop applications should only get access to the scanners via the net
backend, so edit /etc/sane.d/dll.conf to only contain the net-backend on the 
desktop machines. 

example dll.conf:
---
net
---

Give the net-backend a suitable configuration and add the machines, where the 
scanners (and scanbd/scanbm) are (this could be also localhost, but see below!):
Now the desktop applications (wich use libsane) use the above dll.conf with only
the net backend. This prevents them from using the locally attached scanners 
directly (and blocking them). 

example net.conf remove the comments from the appropriate lines!
---
connect_timeout = 3
scanbd-host1.local.net   # the host with the scanbd running 
			 # (and the attached scanners with buttons)
# scanbd-host2.local.net # there can be more than one scanbd one the net 
			 # (and attached scanners with buttons)
# sane-host.local.net    # there can also be a sane host without scanbd 
			 # running (and scanner without buttons)
# localhost              # if desktop machine with scanbd AND 
			 # scan-applications like xsane
---

IMPORTANT:
If scanbd is running on a desktop machine (e.g. a machine using a 
sane-frontend: xsane, scanimage), saned needs another config which only 
includes local scanners. 

Therefore the env-var SANE_CONFIG_DIR is used in the debian 
start-stop-script and the upstart-job of scanbd (scanbdservice.conf). 
Put the directory of this dll.conf into this env-var (e.g. 
/usr/local/etc/scanbd) and remove the net-backend from this config-file. 
All other backends can be included as needed.  If a scan is issued, 
the scan application contacts (via net backend) scanbm, which in turn starts 
saned with the environment variable SANE_CONFIG_DIR set to 
/usr/local/etc/scanbd (or any other directory, see scanbd.conf). 
Here saned and therefore libsane find a different dll.conf without the net 
backend but with all needed local scanner backends!

So as a safe rule: 
- the file /etc/sane.d/dll.conf should only contain the net backend   
- copy all config-files from /etc/sane.d to /usr/local/etc/scanbd and edit 
dll.conf as stated above (not include the net backend)

6) start scanbd 

e.g.:

export SANE_CONFIG_DIR=/usr/local/etc/scanbd
/usr/local/bin/scanbd 
(add a -c /usr/local/etc/scanbd/scanbd.conf if you installed in another
directory than the one configured at compile time)

or use one of the startup files from the integration directory.

If you are unfamiliar with scanbd it would be best to first try starting scanbd 
in the foreground (-f) and in debug mode (-d):

/usr/local/bin/scanbd -d7 -f -c /usr/local/etc/scanbd/scanbd.conf

Then you can watch scanbd recognizing all scanners and polling the options / 
buttons. If you press the buttons or modify the function knob or insert/remove 
sheets of paper some of the options / buttons must change their value.
Theses value changes can be used to define actions in scanbd.conf.

7) some words on access rights

if the saned-user can't access the scanners, e.g. if 

SANE_CONFIG_DIR=/usr/local/etc/scanbd scanimage -L

doesn't get any results, but if you run the same command as root with success, 
you have wrong access rights to the scanner devices. If you have a scan-only 
device (not an all-in-one type), use the file 99-saned.rules and copy this file 
to /lib/udev/rules.d and then issue a rescan:

udevadm trigger

On Fedora (and probably on Redhat systems, I have not tried that yet) you need 
to copy that file to  /lib/udev/rules.d/69-saned.rules (not 99...). This will 
ensure that we set the group id before setting the access to the console user.

If you have an all-in-one scanner the device is matched via a printer rule and 
the access is granted to the lp group. In this case you should simply add the 
saned user to the lp group:

usermod -a -G lp saned

Also edit/check scanbd.conf to set the effective group = lp !

8) Additional hints:

- if you are using scanner devices that don't support usb-autosuspend, please find
  an udev-rule 98-snapscan.rule in the integration directory. You can put this into
  /etc/udev/rules.d. Be sure to add your device fingerprints to this file, e.g. change
  idVendor and idProduct in the rule line or add a new one. Actually I'm aware of
  the Epson 1670 having this problem.

- There is another howto for scanbd by <alex@tomisch.de>:
  https://bbs.archlinux.de/viewtopic.php?id=20954 
  It is written for ArchLinux but will also be valuable for other distros.

- if there are problems recognizing new / removed devices, it is possible to 
  send scanbd the SIGHUP signal to reconfigure and look for new devices. 
  This can be done also via udev-rules.

- be sure to include only the necessary drivers in /usr/local/etc/scanbd/dll.conf,
  since scanadf will hang (timeout)

- to test an action for a specific device, use

  scanbd -m -t <device_number> -a <name_of_action>

  e.g. for the first scanner found trigger the action "scan" (action name as in 
  config file scanbd.conf)

  scanbd -m -t 0 -a scan

- problematic desktop scan applications

  most desktop-scan-applications open the scanner but never closes them, 
  therefore saned is started but never (as long as the scan-application runs) 
  exits. This blocks the scanbd from gaining again the control over the scanner 
  and go on polling. This can also be a problem of the net backend (see 
  connect_timeout parameter in net.conf).
  The better way to do scanning would be to first get a list of available 
  scanners (scanimage -L or scanadf -L), store the list but then close the 
  connection to the found scanners. If a scan is requested, a device from the 
  list is choosen, the scan happens and thereafter the connection to the 
  scanner is closed!

  One can achieve this with simple shell scripting:
  a) to get a list of all scanners available: scanimage -L
  b) select a scanner from the list
  c) do the scan: scanadf -d <device>
  
  Or use a web based application for scanning (see the frontends documentation 
  on the sane-project page)

9) Actual problems

- if you use USE_SCANBUTTOND: the usb-device can't be opened if another 
  applications (say cupsd) does the same. In this case scanbd stops polling the 
  device for a predefined amount of time and restarts polling after that time 
  again.

- dynamic device recognition:
  HAL(hald) is deprecated: on recent distributions hald isn't used or isn't 
  started by default anymore.  udisks online sends out storage related dbus 
  events (no DeviceAdded events for scanner, etc., that's why it is called 
  udisks ;-)). So, if you have udisks-daemon and can't start hald, be sure to 
  compile scanbd with USE_LIBUDEV=yes. If you don't have hal and libudev e.g. 
  OpenBSD, dynamic recognition of scanners isn't possible.

10) TODO
- refactor dbus.c, udev.c
- make a clean rewrite using boost/Qt
- make a UI to list options/buttons, watch value changes an setup actions
