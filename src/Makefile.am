
AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq` \
			  @MYSQL_CFLAGS@ @POSTGRESQL_CPPFLAGS@

AM_LDFLAGS = -g -Wall @noundef@ @MYSQL_LDFLAGS@ `pkg-config --libs gtk+-3.0 libpq`\
			 @POSTGRESQL_LDFLAGS@

IR_SCANFLAGS = @MYSQL_CFLAGS@ @POSTGRESQL_CPPFLAGS@ -I$(srcdir)

lib_LTLIBRARIES	= libstyleprinttable.la
libstyleprinttable_la_SOURCES = styleprinttable.c

source_h = styleprinttable.h

include_HEADERS = styleprinttable.h

if INCLUDE_POSTGRESQL
libstyleprinttable_la_SOURCES += styleprintpg.c
source_h += styleprintpg.h
include_HEADERS += styleprintpg.h
endif

if INCLUDE_MYSQL
libstyleprinttable_la_SOURCES += styleprintmy.c
source_h += styleprintmy.h
include_HEADERS += styleprintmy.h
endif

EXTRA_DIST = styleprinttablepriv.h

CLEANFILES =

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = StylePrint-1.0.gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all

introspection_sources = $(libstyleprinttable_la_SOURCES)

StylePrint-1.0.gir: $(lib_LTLIBRARIES)
StylePrint_1_0_gir_INCLUDES = Gtk-3.0
StylePrint_1_0_gir_CFLAGS = $(IR_SCANFLAGS)
StylePrint_1_0_gir_LIBS = $(lib_LTLIBRARIES)
StylePrint_1_0_gir_FILES = $(introspection_sources) $(include_HEADERS)
StylePrint_1_0_gir_NAMESPACE = StylePrint

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
