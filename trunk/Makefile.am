ACLOCAL_AMFLAGS = -I m4
if USE_SCANBUTTOND
SUBDIRS = src/scanbuttond src/scanbd conf 
else
SUBDIRS = src/scanbd
endif
SUBDIRS += conf integration scripts contrib doc

EXTRA_DIST = \
	Makefile.simple \
	Makefile.conf.in \
	Makefile.conf

noinst_DATA = \
	Makefile.conf

Makefile.conf: Makefile.conf.in configure
	echo "Generating $@...";
	sed  -e 's|@PACKAGEVERSION@|$(PACKAGE_VERSION)|g' \
	     -e 's|@PACKAGENAME@|$(PACKAGE)|g' $@.in  >$@


