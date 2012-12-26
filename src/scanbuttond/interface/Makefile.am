noinst_LIBRARIES = libusbi.a
libusbi_a_SOURCES = libusbi.c

AM_CFLAGS = \
	$(OS_CFLAGS) \
	$(OS_CPPFLAGS) \
	$(LIBUSB_CFLAGS) \
	$(EXTRA_CFLAGS) \
	-Dsyslog=slog \
	-DLOG_INFO=SLOG_INFO \
	-DLOG_WARNING=SLOG_WARN \
	-I../include

EXTRA_DIST = \
	Makefile.simple
