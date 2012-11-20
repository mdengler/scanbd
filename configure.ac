dnl Process this file with autoconf to produce a configure script.
AC_INIT(scanbd, 1.2.1, wilhelm.meier@fh-kl.de)

# allow check that we are really in the source directory
AC_CONFIG_SRCDIR([scanbd.c])
AC_CONFIG_MACRO_DIR([m4])

# Do not enforce GNU file conventions like ChangeLog README etc.
AM_INIT_AUTOMAKE( foreign )

# Initialize libtool support
LT_INIT

# Determine host information 
AC_CANONICAL_HOST

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

# The dlopen() function is in the C library for *BSD and in
# libdl on GLIBC-based systems

AC_SEARCH_LIBS([dlopen], [dl dld], [], [
  AC_MSG_ERROR([unable to find the dlopen() function])
])

#AC_MSG_CHECKING([OS-type])
#case "$host" in
#        *-*-*linux*)
#		OS_CFLAGS="-Wall -Wextra -std=c99 -g"
#		OS_CPPFLAGS="-DLinux"
#	;;
#        *-*-*freebsd*)
#		OS_CFLAGS="-Wall -Wextra -std=c99 -g"
#		OS_CPPFLAGS="-DFreeBSD"
#	;;
#        *-*-*netbsd*)
#		OS_CFLAGS="-Wall -Wextra -std=c99 -g"
#		OS_CPPFLAGS="-DNetBSD"
#	;;
#        *-*-*openbsd*)
#		OS_CFLAGS="-Wall -Wextra -std=c99 -g"
#		OS_CPPFLAGS="-DOpenBSD"
#	;;
#	*)
#		AC_MSG_ERROR([Your OS is not yet supported])
#esac
OS_CFLAGS="-Wall -Wextra -std=c99 -g"

dnl check for optional headers
AC_CHECK_HEADERS(linux/limits.h \
		 utmp.h \
		 utmpx.h) 

# check for mandatory dependencies 
#=================================

#check for libconfuse
PKG_CHECK_MODULES([CONFUSE], [libconfuse])

PKG_CHECK_MODULES([DBUS], [dbus-1])

dnl Check for udev and if we do not have it, check for hal
PKG_CHECK_MODULES([UDEV],[libudev],
	[AC_DEFINE([USE_UDEV], [1], [Use UDEV])],
	[PKG_CHECK_MODULES([HAL], [hal],
	[AC_DEFINE([USE_HAL], [1], [Use HAL])
	])
])

# check configuration options 
#============================

# check for Werror
EXTRA_CFLAGS=""
AC_ARG_ENABLE(Werror,
        AC_HELP_STRING([--disable-Werror], [don't use gcc's -Werror option when building]))

if test x"${enable_Werror}" != "xno"
then
        EXTRA_CFLAGS+="-Werror"
fi

# shall we set NDEBUG?
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--disable-debug], [disable debugging code (NDEBUG)]))

if test x"${enable_debug}" == "xno"
then
	EXTRA_CFLAGS+=" -DNDEBUG"
fi

# check for enable-scanbuttond
AC_ARG_ENABLE(scanbuttond,
	AC_HELP_STRING([--enable-scanbuttond], [Use scanbuttond instead of Sane]))


if test x"${enable_scanbuttond}" == "xyes"
then
	use_scanbuttond=yes
	SCANNER_CFLAGS="-UUSE_SANE -DUSE_SCANBUTTOND"
	
else
	use_sane=yes
	SCANNER_CFLAGS="-DUSE_SANE -UUSE_SCANBUTTOND"
	PKG_CHECK_MODULES([SANE], [sane-backends >= 1.0])
fi
AM_CONDITIONAL(USE_SANE, test "x$use_sane" == xyes)
AM_CONDITIONAL(USE_SCANBUTTOND, test "x$use_scanbuttond" == xyes)

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# define substitutions to be applied in the output
AC_SUBST([OS_CFLAGS])
AC_SUBST([OS_CPPFLAGS])
AC_SUBST([EXTRA_CFLAGS])
AC_SUBST([SCANNER_CFLAGS])

AM_MAINTAINER_MODE([enable])

# header file that contains output from configure
# We do not use it now, so all flags are set as -D on the compiler command line
# AC_CONFIG_HEADERS([config.h])

# Generate the output files
AC_OUTPUT([Makefile scanbuttond/Makefile scanbuttond/backends/Makefile] scanbuttond/interface/Makefile scanbuttond/include/Makefile)
