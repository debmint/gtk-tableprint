#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.62)
AC_INIT(gtktableprint,0.1)
AC_CONFIG_SRCDIR([gtktableprint.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wno-portability])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

#
# GObject Introspection
#

GOBJECT_INTROSPECTION_CHECK([1.30.0])

# Checks for libraries.

AC_FUNC_MALLOC
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.

AC_CANONICAL_HOST
case $host_os in
	*mingw* | *cygwin*)
		noundef="-no-undefined"
    ;;
	*)
	;;
esac

AC_SUBST(noundef)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
