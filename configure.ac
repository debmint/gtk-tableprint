#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT(styleprinttable,0.1)
AC_CONFIG_SRCDIR([src/styleprinttable.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wno-portability])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
LT_INIT

#
# GObject Introspection
#

GTK_DOC_CHECK([1.0], [--flavour no-tmpl])
GOBJECT_INTROSPECTION_CHECK([1.40.0])

# Checks for libraries.
AX_LIB_MYSQL()
AM_CONDITIONAL([INCLUDE_MYSQL],test -n "$MYSQL_VERSION")
AX_LIB_POSTGRESQL()
AM_CONDITIONAL([INCLUDE_POSTGRESQL],test -n "$POSTGRESQL_VERSION")

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC

AC_CANONICAL_HOST
case $host_os in
	*mingw* | *cygwin*)
		noundef="-no-undefined"
    ;;
	*)
	;;
esac

AC_SUBST(noundef)

AC_CONFIG_FILES([Makefile
                 docs/Makefile
                 docs/reference/Makefile
                 src/Makefile])
AC_OUTPUT
