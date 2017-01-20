
AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq` \
			  `mysql_config --cflags`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` `mysql_config --libs` -lexpat

IR_SCANFLAGS = `pkg-config --cflags libpq` 	`mysql_config --cflags`

lib_LTLIBRARIES	= libstyleprinttable.la
libstyleprinttable_la_SOURCES = styleprinttable.c styleprintpg.c styleprintmy.c

source_h = styleprinttable.h styleprintpg.h styleprintmy.h $(NULL)

include_HEADERS = styleprinttable.h styleprintpg.h styleprintmy.h

styleprinttableincludedir = $(srcdir)

styleprinttableinclude_DATA = $(source_h) $(NULL)

EXTRA_DIST=styleprinttablepriv.h

CLEANFILES =

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = StylePrint_1_0_gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all

if HAVE_INTROSPECTION
introspection_sources = $(libstyleprinttable_la_SOURCES)

StylePrint-1.0-gir: libstyleprinttable.la
StylePrint_1_0_gir_INCLUDES = Gtk-3.0
StylePrint_1_0_gir_CFLAGS = $(IR_SCANFLAGS)
StylePrint_1_0_gir_LIBS =libstyleprinttable.la
StylePrint_1_0_gir_FILES = $(introspection_sources) $(include_HEADERS)
StylePrint_1_0_gir_NAMESPACE = StylePrint
INTROSPECTION_GIRS += StylePrint-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
