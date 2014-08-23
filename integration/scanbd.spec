Name:            scanbd
Version:         1.3
Release:         6%{?dist}
Summary:         Scanner Button tools for SANE

Group:           System Environment/Libraries
License:         GPLv2
URL:             http://scanbd.sourceforge.net/
Source0:         http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
Source1:         scanbd.conf
Source2:         scanbd.service
BuildRoot:       %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:   libconfuse-devel
BuildRequires:   libusb-devel
BuildRequires:   libudev-devel
BuildRequires:   sane-backends-devel
BuildRequires:   dbus-devel
Requires:        sane-backends
Requires:        sane-backends-drivers-scanners
Requires:        dbus

%description
Scanbd is a scanner button daemon. It polls the scanner buttons looking for
buttons pressed or function knob changes or other scanner events as paper
inserts / removals and at the same time allows also scan-applications to access
the scanners. If buttons are pressed, etc., various actions can be submitted
(scan, copy, email, ...) via action scripts. The function knob values are passed
to the action-scripts as well. Scan actions are also signaled via dbus. This
can be useful for foreign applications. Scans can also be triggered via dbus
from foreign applications. On platforms which support signaling of dynamic
device insertion / removal (libudev, dbus, hal) scanbd supports this as well.
scanbd can use all sane-backends or some special backends from the (old)
scanbuttond project. Supported platforms: Linux, FreeBSD, NetBSD, OpenBSD

%prep
%setup -q
cp -p %SOURCE1 .
cp -f %SOURCE2 ./integration/systemd/

%build
%configure
make  %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

install -d -m 755 -d %{buildroot}/lib/systemd/system
install -m 644 integration/systemd/scanbd.service %{buildroot}/lib/systemd/system/
install -m 644 integration/systemd/scanbm@.service %{buildroot}/lib/systemd/system/
install -m 644 integration/systemd/scanbm.socket %{buildroot}/lib/systemd/system/
install -d -m 755 -d %{buildroot}/lib/udev/rules.d
install -m 644 integration/99-saned.rules %{buildroot}/lib/udev/rules.d/69-saned.rules

install -d -m 755 -d %{buildroot}/%{_datadir}/dbus-1/system-services
install -m 644 integration/systemd/de.kmux.scanbd.server.service %{buildroot}/%{_datadir}/dbus-1/system-services/

install -d -m 755 %{buildroot}/%{_sysconfdir}/dbus-1/system.d
install -m 644 integration/scanbd_dbus.conf %{buildroot}/%{_sysconfdir}/dbus-1/system.d/

mv %{buildroot}/%{_sysconfdir}/scanbd/scanbd.conf %{buildroot}/%{_sysconfdir}/scanbd/scanbd.conf.example
install scanbd.conf %{buildroot}/%{_sysconfdir}/scanbd/

%clean
rm -rf %{buildroot}

%post
if [ $1 == 1 ] ; then # we are doing an install
  useradd -r -M -s /sbin/nologin -U -G lp -d /etc/sane.d saned
  
  # let's make copies of everything we change to be able to undo it at uninstall
  cp -rf /etc/sane.d /etc/scanbd/
  cp -f /etc/sane.d/dll.conf /etc/sane.d/dll.conf.pre-scanbd
  cp -f /etc/sane.d/net.conf /etc/sane.d/net.conf.pre-scanbd
  
  # let's configure the sane part
  grep -v '^net$' /etc/sane.d/dll.conf > /etc/scanbd/sane.d/dll.conf
  echo net > /etc/sane.d/dll.conf

  echo "connect_timeout = 3" > /etc/sane.d/net.conf
  echo "localhost # scanbm is listening on localhost" >> /etc/sane.d/net.conf

  systemctl enable scanbd
fi

%preun
if [ $1 == 0 ] ; then # we will do an uninstall
  systemctl stop scanbd
  systemctl disable scanbd
fi

%postun
if [ $1 == 0 ] ; then # we are doing an uninstall, not an upgrade
  rm -rf /etc/scanbd/sane.d/*
  if [ -f /etc/sane.d/dll.conf.pre-scanbd ] ; then
    cp -f /etc/sane.d/dll.conf.pre-scanbd /etc/sane.d/dll.conf
    rm -f /etc/sane.d/dll.conf.pre-scanbd
  fi
  if [ -f /etc/sane.d/net.conf.pre-scanbd ] ; then
    cp -f /etc/sane.d/net.conf.pre-scanbd /etc/sane.d/net.conf
    rm -f /etc/sane.d/net.conf.pre-scanbd
  fi
fi

%files
%defattr(-,root,root,-)
#%doc README COPYING AUTHORS ChangeLog
%config(noreplace) %{_sysconfdir}/scanbd/action.script
%config(noreplace) %{_sysconfdir}/scanbd/example.script
%config(noreplace) %{_sysconfdir}/scanbd/scanadf.script
%config(noreplace) %{_sysconfdir}/scanbd/scanbd.conf
%config(noreplace) %{_sysconfdir}/scanbd/scanner.d/avision.conf
%config(noreplace) %{_sysconfdir}/scanbd/scanner.d/fujitsu.conf
%config(noreplace) %{_sysconfdir}/scanbd/scanner.d/hp.conf
%config(noreplace) %{_sysconfdir}/scanbd/scanner.d/pixma.conf
%config(noreplace) %{_sysconfdir}/scanbd/scanner.d/snapscan.conf
%config(noreplace) %{_sysconfdir}/scanbd/test.script
%config(noreplace) %{_sysconfdir}/dbus-1/system.d/scanbd_dbus.conf
%{_sysconfdir}/scanbd/scanbd.conf.example
%{_sbindir}/scanbd
%{_sbindir}/scanbm
%{_mandir}/man8/scanbd.8.gz
%{_mandir}/man8/scanbm.8.gz
/lib/systemd/system/scanbd.service
/lib/systemd/system/scanbm@.service
/lib/systemd/system/scanbm.socket
/lib/udev/rules.d/69-saned.rules
%{_datadir}/dbus-1/system-services/de.kmux.scanbd.server.service

%changelog
* Sun Mar 9 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-6
- Removed dependency to hp dbus rules as these scanners are not fully supported any way
- added corrected configuration file for scanbd.service
- fixed typo

* Sat Feb 15 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-5
- fixed dynamic update of /etc/sane.d/dll.conf
- provided default scanbd.conf that seems to be working

* Thu Feb 13 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-4
- fixed sane configuration

* Thu Feb 13 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-3
- now enable systemd service by default

* Wed Feb 12 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-2
- added configuration for sane, scandb and scanbm

* Sat Feb 08 2014 Jorrit Jorritsma <jsj@xs4all.nl> - 1.3-1
- initial build
