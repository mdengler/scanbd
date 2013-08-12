dnl Process this file with autoconf to produce a configure script.
AC_INIT(scanbd, 1.92, wilhelm.meier@fh-kl.de)

# allow check that we are really in the source directory
AC_CONFIG_SRCDIR([src/scanbd/scanbd.c])
AC_CONFIG_MACRO_DIR([m4])

# Do not enforce GNU file conventions like ChangeLog README etc.
AM_INIT_AUTOMAKE( foreign )

# Do not automatically rebuild autoconf stuff
AM_MAINTAINER_MODE

# Initialize libtool support
LT_INIT([disable-static])

# Determine host information 
AC_CANONICAL_HOST

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LN_S

# Find path to saned
AC_PATH_PROG(SANED_PATH,[saned], 
             [AC_MSG_ERROR([Saned not found!]) ],
             [$PATH:/usr/libexec:/usr/sbin:/usr/local/libexec:/usr/local/sbin])
	

# autoscan complains about this being missing, but autoreconf thinks it is superfluous
# AC_PROG_RANLIB

# The dlopen() function is in the C library for *BSD and in
# libdl on GLIBC-based systems

AC_SEARCH_LIBS([dlopen], [dl dld], [], [
  AC_MSG_ERROR([unable to find the dlopen() function])
])

AC_SEARCH_LIBS([pthread_create], [pthread], [], [
  AC_MSG_ERROR([unable to find the pthread_create() function])
])

# Check for required functions
AC_CHECK_FUNCS([alarm dup2 ftruncate getcwd memset putenv regcomp strdup strerror],
		[], AC_MSG_ERROR([Required function is missing ]))

# Check for required header files
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h sys/socket.h sys/time.h syslog.h pthread.h],
		[], AC_MSG_ERROR([Required header file is missing ]))

#  check for required types

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_PID_T

# We define these to make autoscan happy, but we do not use the results
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_HEADER_STDBOOL

OS_CFLAGS="-Wall -Wextra -std=c99 -g"

dnl check for optional headers
AC_CHECK_HEADERS(linux/limits.h \
		 utmp.h \
		 utmpx.h) 

# check for mandatory dependencies 
#=================================

# check for libconfuse
PKG_CHECK_MODULES([CONFUSE], [libconfuse])

# check for dbus
PKG_CHECK_MODULES([DBUS], [dbus-1])

dnl Check for udev and if we do not have it, check for hal
PKG_CHECK_MODULES([UDEV],[libudev],
        [AC_DEFINE([USE_LIBUDEV], [1], [Use UDEV])],
	[PKG_CHECK_MODULES([HAL], [hal],
		[AC_DEFINE([USE_HAL], [1], [Use HAL])],
		[AC_MSG_WARN([Neither Udev nor Hal are found, dynamic detection of scanners is not supported!])]
	)]
)

# check configuration options 
#============================

# scanbd configurationdir
AC_ARG_WITH(scanbdconfdir,
	AC_HELP_STRING([--with-scanbdconfdir=DIR],
		[scanbd configuration directory (PREFIX/etc/scanbd)]),
	[SCANBD_CFG_DIR="${withval}"],
	[SCANBD_CFG_DIR="${sysconfdir}/scanbd"])
AC_SUBST(SCANBD_CFG_DIR)

# scanbuttond configurationdir
AC_ARG_WITH(scanbuttondlibdir,
	AC_HELP_STRING([--with-scanbuttondlibdir=DIR],
		[ scanbuttond backend configuration directory (LIBDIR/scanbd/scanbuttond/backends)]),
	[SCANBUTTOND_LIB_DIR="${withval}"],
	[SCANBUTTOND_LIB_DIR="${libdir}/scanbd/scanbuttond/backends"])
AC_SUBST(SCANBUTTOND_LIB_DIR)

# check for Werror
EXTRA_CFLAGS=""
AC_ARG_ENABLE(Werror,
        AC_HELP_STRING([--disable-Werror], [don't use gcc's -Werror option when building]))

if test x"${enable_Werror}" != "xno"
then
        EXTRA_CFLAGS=$EXTRA_CFLAGS" -Werror"
fi

# shall we set NDEBUG?
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--disable-debug], [disable debugging code (NDEBUG)]))

if test "x${enable_debug}" == "xno"
then
	EXTRA_CFLAGS=$EXTRA_CFLAGS" -DNDEBUG"
fi

# Do we have systemd?
PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
        AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
        [], [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
if test "x$with_systemdsystemunitdir" != "xno"; then
        AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
	AC_DEFINE(HAVE_SYSTEMD, [1], [Do we use systemd on this system?])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, test "x$with_systemdsystemunitdir" != "x")
# TODO: add check for udev
# TODO: add check for HAL

# check for enable-scanbuttond
AC_ARG_ENABLE(scanbuttond,
	AC_HELP_STRING([--enable-scanbuttond], [Use scanbuttond instead of Sane]))

if test x"${enable_scanbuttond}" == "xyes"
then
	use_scanbuttond=yes
	# Check for libusb except for FreeBSD that does not have libusb.pc
	if test `uname` == "FreeBSD"
	then
		if test x"${LIBUSB_CFLAGS}" == "x"
		then
			LIBUSB_CFLAGS="-I/usr/include "
		fi
		if test x"${LIBUSB_LIBS}" == "x"
		then
			LIBUSB_LIBS="-L/usr/lib -lusb "
		fi
		AC_SUBST(LIBUSB_CFLAGS)
		AC_SUBST(LIBUSB_LIBS)
	else
		PKG_CHECK_MODULES([LIBUSB], [libusb])
	fi

	SCANNER_CFLAGS="-DUSE_SCANBUTTOND"

	AC_DEFINE([USE_SCANBUTTOND], [1], ["Use scanbuttond"])
	
else
	use_sane=yes
	AC_DEFINE([USE_SANE], [1], [Use Sane])
	PKG_CHECK_MODULES([SANE], [sane-backends >= 1.0],
		[test_sane_config=no],
		[test_sane_config=yes])
fi

# fallback to usage of sane-config if pkg-config fails
if test x"${use_sane}" == "xyes" -a x"${test_sane_config}" == "xyes" 
then
	if test x"${SANE_LIBS}" == "x" -a  x"${SANE_CFLAGS}" = "x"
	then
		AC_CHECK_PROG([SANE_CONFIG], [sane-config], sane-config, no)
		if test x"${SANE_CONFIG}" = "xno"
		then
			AC_MSG_ERROR([Cannot find configuration for sane using pkg-config and sane-config does not exist])
		fi

		SANE_LIBS=`sane-config --libs`
		SANE_CFLAGS=`sane-config --cflags`
	fi
fi

#  user setting
AC_ARG_WITH( user,
	AC_HELP_STRING([--with-user=USER],
	[userid to run as (guess)]),
	[SCANBD_USER="${withval}"],
	[ 
	if test "x${UDEV_LIBS}" != "x"; then 
		if ! grep saned: /etc/passwd > /dev/null; then
			AC_MSG_NOTICE([For scanbd with udev we assume that user saned is available on your system!])
		fi
		SCANBD_USER=saned
	else 
		case ${host_os} in
		*-*-openbsd* | *-*-freebsd* | *-*-netbsd*)
			SCANBD_USER=root
			;;
		*-*-linux*)
			SCANBD_USER=saned
			AC_MSG_NOTICE([On linux we use user saned, plase make sure you add it!])
			;;
		*)
			AC_MSG_ERROR([Target OS ${host_os} not known, please set option --with-user for configure!])
			;;
	
		esac
	fi
	]
)
AC_MSG_NOTICE([Using user: ${SCANBD_USER}])
AC_SUBST(SCANBD_USER)

#  group setting
AC_ARG_WITH( group,
	AC_HELP_STRING([--with-group=GROUP],
	[Groupid to run as (guess)]),
	[SCANBD_GROUP="${withval}"],
	[ 
	if test "x${UDEV_LIBS}" != "x"; then 
		if ! grep saned /etc/group > /dev/null; then
			AC_MSG_NOTICE([For scanbd with udev we assume that group saned is available on your system!])
		fi
		SCANBD_GROUP=saned
	else 
		case ${host_os} in
		*-*-openbsd* | *-*-freebsd* | *-*-netbsd*)
			SCANBD_GROUP=daemon
			;;
		*-*-linux*)
			if grep scanner: /etc/group > /dev/null; then
				SCANBD_GROUP=scanner
			else
				SCANBD_GROUP=lp
			fi
			;;
		*)
			AC_MSG_ERROR([Target OS ${host_os} not known, please set option --with-group for configure!])
			;;
	
		esac
	fi
	]
)
AC_MSG_NOTICE([Using group: ${SCANBD_GROUP}])
AC_SUBST(SCANBD_GROUP)


AM_CONDITIONAL(USE_SANE, test "x$use_sane" == "xyes")
AM_CONDITIONAL(USE_SCANBUTTOND, test "x$use_scanbuttond" == xyes)

# define substitutions to be applied in the output
AC_SUBST([OS_CFLAGS])
AC_SUBST([OS_CPPFLAGS])
AC_SUBST([EXTRA_CFLAGS])

# header file that contains output from configure
# We do not use it now, so all flags are set as -D on the compiler command line
# AC_CONFIG_HEADERS([config.h])

# Generate the output files
AC_OUTPUT([
		Makefile 
		src/scanbd/Makefile 
		src/scanbuttond/Makefile 
		src/scanbuttond/backends/Makefile 
		src/scanbuttond/interface/Makefile 
		src/scanbuttond/include/Makefile
		integration/Makefile
		contrib/Makefile
		doc/Makefile
		conf/Makefile
		scripts/Makefile
])
