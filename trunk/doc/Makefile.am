EXTRA_DIST = \
	README.txt\
	scanbd.man \
	scanbm.man \
	Makefile.simple

man8_MANS = \
	scanbd.8 \
	scanbm.8

clean:
	-rm scanbd.8 
	-rm scanbm.8

distclean: 
	-rm scanbd.8
	-rm scanbm.8
	rm Makefile

%.8: %.man
	echo "Generating manpage $@...";
	sed  -e 's|@PACKAGEVERSION@|$(PACKAGE_VERSION)|g' \
	     -e 's|@PACKAGENAME@|$(PACKAGE)|g' \
	     -e 's|@SBINDIR@|$(sbindir)|g' \
	     -e 's|@SCANBDCFGDIR@|$(SCANBD_CFG_DIR)|g'  $^ >$@

		

