ACLOCAL_AMFLAGS = -I m4
if USE_SCANBUTTOND
SUBDIRS = src/scanbuttond src/scanbd conf 
else
SUBDIRS = src/scanbd
endif
SUBDIRS += conf integration scripts contrib doc

EXTRA_DIST = \
	Makefile.simple \
	Makefile.conf
