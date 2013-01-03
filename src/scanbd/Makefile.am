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

EXTRA_DIST = \
	Makefile.simple

AM_CFLAGS = \
	$(OS_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(OS_CPPFLAGS) \
	$(EXTRA_CFLAGS) \
	$(CONFUSE_CFLAGS) \
	$(UDEV_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(HAL_CFLAGS) \
	-DSCANBUTTOND_LIB_DIR=\"@SCANBUTTOND_LIB_DIR@/\" \
	-DSCANBD_CFG_DIR=\"@SCANBD_CFG_DIR@/\"


AM_LDFLAGS = \
	$(OS_LIBS) \
	$(PTHREAD_LIBS) \
	$(CONFUSE_LIBS)\
	$(UDEV_LIBS) \
	$(DBUS_LIBS) \
	$(HAL_LIBS) \
	$(LIBUSB_LIBS) \
	-rdynamic

install-exec-hook:
	cd  $(DESTDIR)$(sbindir) && \
	rm -f scanbm$(EXEEXT) && \
	$(LN_S) scanbd$(EXEEXT) scanbm$(EXEEXT)

uninstall-hook:
	cd  $(DESTDIR)$(sbindir) && \
	rm -f scanbm$(EXEEXT)


if USE_SANE
scanbd_SOURCES += \
	sane.c
AM_CFLAGS  +=  \
	$(SANE_CFLAGS)
AM_LDFLAGS +=  \
	$(SANE_LIBS)
endif

if USE_SCANBUTTOND
# testscanbuttond does not work when we compile with autofoo: libtool
# onl;y generates the .so files at install time
noinst_PROGRAMS = testscanbuttond

AM_CFLAGS += \
	-I ../scanbuttond/include 

AM_LDFLAGS += \
	../scanbuttond/interface/libusbi.o

scanbd_SOURCES += \
	scanbuttond_wrapper.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.h \
	scanbuttond_loader.h 


testscanbuttond_SOURCES = \
	testscanbuttond.c \
	slog.c \
	scanbuttond_loader.c \
	scanbuttond_wrapper.c \
	dbus.c 
	
endif
