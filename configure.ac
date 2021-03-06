dnl Process this file with autoconf to produce a configure script.

dnl configure.ac
dnl
dnl This file is part of bino, a 3D video player.
dnl
dnl Copyright (C) 2010-2011
dnl Martin Lambers <marlam@marlam.de>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Autotools init stuff
AC_PREREQ([2.65])
AC_INIT([Bino], [0.9.2], [bino-list@nongnu.org], [bino], [http://bino.nongnu.org/])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AM_INIT_AUTOMAKE([1.11.1 color-tests silent-rules tar-pax no-texinfo.tex -Wall -Wno-portability])
AM_SILENT_RULES([yes])
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_LANG([C++])
AC_C_BIGENDIAN

dnl Useful definitions
AC_DEFINE_UNQUOTED([PLATFORM], ["${target}"], [Platform triplet])

dnl Special handling of W32
case "${target}" in *-*-mingw32*) w32="yes" ;; *) w32="no " ;; esac
if test "$w32" = "yes"; then
    LDFLAGS="-mwindows $LDFLAGS"
    AC_CHECK_TOOL([WINDRES], [windres], [:])
    if test "$WINDRES" = ":"; then
        AC_MSG_ERROR([windres not found])
    fi
    AC_CHECK_TOOL([MAKENSIS], [makensis], [:])
    if test "$MAKENSIS" = ":"; then
        AC_MSG_WARN([makensis not found: you will not be able to create a W32 installer])
    fi
else
    MAKENSIS=":"
fi
AM_CONDITIONAL([HAVE_MAKENSIS], [test "$MAKENSIS" != ":"])
AM_CONDITIONAL([W32], [test "$w32" = "yes"])

dnl Configure switches
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [Enable debugging checks. Disabled by default.])],
    [if test "$enableval" = "yes"; then ndebug=0; else ndebug=1; fi], [ndebug=1])
if test "$ndebug" = "1"; then
    AC_DEFINE_UNQUOTED([NDEBUG], [1], [Define to 1 to disable assertions and other debugging checks.])
fi

dnl Feature checks needed by the base module
AC_CHECK_DECLS([optreset], [#include <getopt.h>])
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([pthread_create], [pthread])
AC_CHECK_FUNCS([backtrace clock_gettime sigaction vasprintf])

dnl pkg-config (required to detect libraries)
PKG_PROG_PKG_CONFIG([])
if test -z "$PKG_CONFIG"; then
    AC_MSG_WARN([pkg-config not found])
    AC_MSG_WARN([required libraries cannot be detected])
    AC_MSG_WARN([please install pkg-config; Debian package: pkg-config])
fi

dnl FFmpeg
PKG_CHECK_MODULES([libavformat], [libavformat >= 52.64.2 libavcodec libavutil], [HAVE_LIBAVFORMAT=1], [HAVE_LIBAVFORMAT=0])
if test "$HAVE_LIBAVFORMAT" != "1"; then
    AC_MSG_WARN([library libavformat >= 52.64.2 not found:])
    AC_MSG_WARN([$libavformat_PKG_ERRORS])
    AC_MSG_WARN([libavformat is provided by FFmpeg; Debian package: libavformat-dev])
fi
PKG_CHECK_MODULES([libswscale], [libswscale >= 0.11.0], [HAVE_LIBSWSCALE=1], [HAVE_LIBSWSCALE=0])
if test "$HAVE_LIBSWSCALE" != "1"; then
    AC_MSG_WARN([library libswscale >= 0.11.0 not found:])
    AC_MSG_WARN([$libswscale_PKG_ERRORS])
    AC_MSG_WARN([libswscale is provided by FFmpeg; Debian package: libswscale-dev])
fi
AC_CHECK_FUNCS([sysconf])

dnl OpenAL
PKG_CHECK_MODULES([libopenal], [openal >= 0.0], [HAVE_LIBOPENAL=1], [HAVE_LIBOPENAL=0])
if test "$HAVE_LIBOPENAL" != "1"; then
    AC_MSG_WARN([library libopenal not found:])
    AC_MSG_WARN([$libopenal_PKG_ERRORS])
    AC_MSG_WARN([libopenal is provided by OpenAL; Debian package: libopenal-dev])
fi

dnl Qt
PKG_CHECK_MODULES([libqtopengl], [QtOpenGL >= 0.0], [HAVE_LIBQTOPENGL=1], [HAVE_LIBQTOPENGL=0])
if test "$HAVE_LIBQTOPENGL" != "1"; then
    AC_MSG_WARN([library QtOpenGL not found:])
    AC_MSG_WARN([$libqtopengl_PKG_ERRORS])
    AC_MSG_WARN([QtOpenGL is provided by Qt; Debian package: libqt4-opengl-dev])
fi
RCC=""
AC_ARG_VAR([RCC], [rcc command])
AC_CHECK_TOOLS([RCC], [rcc-qt4 rcc])
MOC=""
AC_ARG_VAR([MOC], [moc command])
AC_CHECK_TOOLS([MOC], [moc-qt4 moc])

dnl Equalizer
dnl We assume that Equalizer includes GLEW. If we don't use Equalizer, we need GLEW separately.
AC_ARG_WITH([equalizer],
    [AS_HELP_STRING([--with-equalizer], [Enable multi-display support via Equalizer (disabled by default)])],
    [if test "$withval" = "yes"; then equalizer="yes"; else equalizer="no "; fi], [equalizer="no"])
if test "$equalizer" = "yes"; then
    PKG_CHECK_MODULES([libEqualizer], [Equalizer >= 0.9.3], [HAVE_LIBEQUALIZER=1], [HAVE_LIBEQUALIZER=0])
    if test "$HAVE_LIBEQUALIZER" != "1"; then
        AC_MSG_WARN([library libEqualizer not found:])
        AC_MSG_WARN([$libEqualizer_PKG_ERRORS])
        AC_MSG_WARN([libEqualizer is provided by Equalizer])
    fi
else
    HAVE_LIBEQUALIZER=0
    dnl glew
    PKG_CHECK_MODULES([libglew], [glew >= 1.5.0], [HAVE_LIBGLEW=1], [HAVE_LIBGLEW=0])
    if test "$HAVE_LIBGLEW" != "1"; then
        AC_MSG_WARN([library libglew >= 1.5.0 not found:])
        AC_MSG_WARN([$libglew_PKG_ERRORS])
        AC_MSG_WARN([libglew is provided by glew; Debian package: libglew-dev])
    fi
fi
AC_DEFINE_UNQUOTED([HAVE_LIBEQUALIZER], [$HAVE_LIBEQUALIZER], [Have Equalizer?])
AM_CONDITIONAL([HAVE_LIBEQUALIZER], [test "$HAVE_LIBEQUALIZER" = "1"])

dnl Check if all libraries were found
if test "$HAVE_LIBAVFORMAT" != "1" \
    -o "$HAVE_LIBSWSCALE" != "1" \
    -o "$HAVE_LIBOPENAL" != "1" \
    -o "$HAVE_LIBQTOPENGL" != "1" \
    -o \( "$equalizer" = "yes" -a "$HAVE_LIBEQUALIZER" != "1" \) \
    -o \( "$ftgl" = "yes" -a "$HAVE_LIBFTGL" != "1" \) \
    -o \( "$HAVE_LIBEQUALIZER" != "1" -a "$HAVE_LIBGLEW" != "1" \) ; then
    AC_MSG_ERROR([One or more libraries were not found. See messages above.])
fi

dnl Output
AC_CONFIG_FILES([Makefile	\
	src/base/Makefile	\
	src/Makefile		\
	doc/Makefile		\
	pkg/macosx/Info.plist	\
	])
AC_OUTPUT
