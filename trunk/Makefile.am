## Process this file with automake to produce Makefile.in
## AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
scanbdconfdir = $(sysconfdir)/scanbd/
scanbuttondconfdir = $(scanbdconfdir)/scanbuttond/backend/
sbin_PROGRAMS = scanbd

scanbd_SOURCES = \
	scanbd.c \
	common.h \
	daemonize.c \
	dbus.c \
	udev.c \
	udev.h \
	slog.c \
	slog.h \
	scanbd_dbus.h \
	scanbd.h 
	

AM_CFLAGS = \
	$(OS_CFLAGS) \
	$(OS_CPPFLAGS) \
	$(SCANNER_CFLAGS) \
	$(EXTRA_CFLAGS) \
	$(CONFUSE_CFLAGS) \
	$(UDEV_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(HAL_CFLAGS) \
	-DCFG_DIR=\"$(scanbuttondconfdir)\"


AM_LDFLAGS = \
	$(OS_LIBS) \
	$(CONFUSE_LIBS)\
	$(UDEV_LIBS) \
	$(DBUS_LIBS) \
	$(HAL_LIBS) 

if USE_SANE
scanbd_SOURCES += \
	sane.c
AM_CFLAGS  +=  \
	$(SANE_CFLAGS)
AM_LDFLAGS +=  \
	$(SANE_LIBS)
endif

if USE_SCANBUTTOND

SUBDIRS = scanbuttond

noinst_PROGRAMS = scanbd_test

AM_CFLAGS += \
	-I scanbuttond/include 

scanbd_LDADD = \
	-Lscanbuttond/interface \
	-lusbi

scanbd_SOURCES += \
	scanbuttond_wrapper.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.h \
	scanbuttond_loader.h 


scanbd_test_SOURCES = \
	test.c \
	slog.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.c \
	dbus.c 
	
endif

# TODO: Edit 99-saned.rules and sample config to set correct userid depending on OS
# Install 99-saned.rules or saned.fdi. Add automatic config for xinetd / systemd
dist_doc_DATA = \
	saned.fdi \
	scanadf.script \
	scanbd.conf \
	scanbd.conf.bsd \
	scanbd_dbus.conf \
	scanbd.debian \
	scanbdservice.conf \
	example.script \
	99-saned.rules \
	contrib/action.script \
	contrib/PKGBUILD

scanbdconf_DATA = \
	scanbd.conf \
	example.script \
	scanadf.script \
	test.script

