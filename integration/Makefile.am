EXTRA_DIST = \
	99-saned.rules \
	FreeBSD.rc \
	saned.fdi \
	scanbd_dbus.conf \
	scanbd.debian \
	scanbdservice.conf \
	systemd/de.kmux.scanbd.server.service \
	systemd/README.systemd 

if HAVE_SYSTEMD
systemdsystemunit_DATA = \
	systemd/scanbd.service \
	systemd/scanbm@.service \
	systemd/scanbm.socket
endif
