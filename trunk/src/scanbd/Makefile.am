## Process this file with automake to produce Makefile.in
## AUTOMAKE_OPTIONS = foreign
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
	$(EXTRA_CFLAGS) \
	$(CONFUSE_CFLAGS) \
	$(UDEV_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(HAL_CFLAGS) \
	-DSCANBUTTOND_CFG_DIR=\"@SCANBUTTOND_CFG_DIR@/\" \
	-DSCANBD_CFG_DIR=\"@SCANBD_CFG_DIR@/\" \
	-DSCANBUTTOND_LIB_DIR=\"${libdir}/scanbd/scanbuttond\"


AM_LDFLAGS = \
	$(OS_LIBS) \
	$(CONFUSE_LIBS)\
	$(UDEV_LIBS) \
	$(DBUS_LIBS) \
	$(HAL_LIBS) \
	-rdynamic

if USE_SANE
scanbd_SOURCES += \
	sane.c
AM_CFLAGS  +=  \
	$(SANE_CFLAGS)
AM_LDFLAGS +=  \
	$(SANE_LIBS)
endif

if USE_SCANBUTTOND

noinst_PROGRAMS = scanbd_test

AM_CFLAGS += \
	-I ../scanbuttond/include 

AM_LDFLAGS += \
	$(LIBUSB_LIBS)

scanbd_LDADD = \
	-L../scanbuttond/interface \
	-lusbi

scanbd_SOURCES += \
	scanbuttond_wrapper.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.h \
	scanbuttond_loader.h 


scanbd_test_SOURCES = \
	testscanbuttond.c \
	slog.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.c \
	dbus.c 
	
endif
