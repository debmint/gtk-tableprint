# ######################################################################### $
# Makefile for gtktableprint.                                          		$
# $Id::                                                                     $
# ######################################################################### $

AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` -lexpat

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-introspection

source_h = gtktableprint.h $(NULL)

lib_LTLIBRARIES	= libtableprint.la
libtableprint_la_SOURCES = gtktableprint.c
include_HEADERS = gtktableprint.h

tableprintincludedir = $(srcdir)

tableprintinclude_DATA = $(sources_h) $(NULL)

CLEANFILES =

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = GtkTablePrint_1_0_gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all

if HAVE_INTROSPECTION
introspection_sources = $(libtableprint_la_SOURCES)

GtkTablePrint-1.0-gir: libtableprint.la
GtkTablePrint_1_0_gir_INCLUDES = Gtk-3.0
GtkTablePrint_1_0_gir_CFLAGS = $(INCLUDES)
GtkTablePrint_1_0_gir_LIBS =libtableprint.la
GtkTablePrint_1_0_gir_FILES = $(introspection_sources) $(include_HEADERS)
GtkTablePrint_1_0_gir_NAMESPACE = GtkTablePrint
INTROSPECTION_GIRS += GtkTablePrint-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
