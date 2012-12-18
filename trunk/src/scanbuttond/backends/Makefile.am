backenddir = ${libdir}/scanbd/scanbuttond/
backendcfgdir =  @SCANBUTTOND_CFG_DIR@/

backendcfg_DATA = meta.conf

backend_LTLIBRARIES = \
	mustek.la \
	plustek.la \
	plustek_umax.la \
	snapscan.la \
	hp3500.la \
	meta.la \
	niash.la \
	artec_eplus48u.la \
	epson.la \
	genesys.la \
	gt68xx.la \
	hp3900.la \
	hp5590.la \
	epson_vphoto.la

AM_CFLAGS = \
	$(OS_CFLAGS) \
	$(OS_CPPFLAGS) \
	$(EXTRA_CFLAGS) \
	-I../include \
	-Dsyslog=slog \
	-DLOG_INFO=SLOG_INFO \
	-DLOG_WARNING=SLOG_WARN \
	-DSCANBUTTOND_CFG_DIR=${backendcfgdir}

AM_LDFLAGS = \
	-shared \
	-avoid-version \
	-module 

mustek_la_SOURCES = mustek.c mustek.h
plustek_la_SOURCES = plustek.c plustek.h
plustek_umax_la_SOURCES = plustek_umax.c plustek_umax.h
snapscan_la_SOURCES = snapscan.c snapscan.h
hp3500_la_SOURCES = hp3500.c hp3500.h
meta_la_SOURCES = meta.c meta.h
niash_la_SOURCES = niash.c niash.h
artec_eplus48u_la_SOURCES = artec_eplus48u.c artec_eplus48u.h
epson_la_SOURCES = epson.c epson.h
genesys_la_SOURCES = genesys.c genesys.h
gt68xx_la_SOURCES = gt68xx.c gt68xx.h
hp3900_la_SOURCES = hp3900.c hp3900.h
hp5590_la_SOURCES = hp5590.c hp5590.h
epson_vphoto_la_SOURCES = epson_vphoto.c epson_vphoto.h
