EXTRA_DIST = \
	README.txt\
	scanbd.man \
	Makefile.simple

man8_MANS = scanbd.8

clean:
	-rm scanbd.8

%.8: %.man
	echo "Generating manpage $@...";
	sed  -e 's|@PACKAGEVERSION@|$(PACKAGE_VERSION)|g' \
	     -e 's|@PACKAGENAME@|$(PACKAGE)|g' \
	     -e 's|@SBINDIR@|$(sbindir)|g' \
	     -e 's|@SCANBDCFGDIR@|$(SCANBD_CFG_DIR)|g'  $^ >$@

		

