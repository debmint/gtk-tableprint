#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(tableprintoperation,0.1)
AC_CONFIG_SRCDIR([tableprintoperation.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL

# Checks for libraries.

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
