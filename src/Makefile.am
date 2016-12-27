
AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` -lexpat

IR_SCANFLAGS = `pkg-config --cflags libpq`

lib_LTLIBRARIES	= libstyletableprint.la
libstyletableprint_la_SOURCES = styletableprint.c

source_h = styletableprint.h $(NULL)

include_HEADERS = styletableprint.h

styletableprintincludedir = $(srcdir)

styletableprintinclude_DATA = $(sources_h) $(NULL)

CLEANFILES =

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = StylePrint_1_0_gir
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all

if HAVE_INTROSPECTION
introspection_sources = $(libstyletableprint_la_SOURCES)

StylePrint-1.0-gir: libstyletableprint.la
StylePrint_1_0_gir_INCLUDES = Gtk-3.0
StylePrint_1_0_gir_CFLAGS = $(IR_SCANFLAGS)
StylePrint_1_0_gir_LIBS =libstyletableprint.la
StylePrint_1_0_gir_FILES = $(introspection_sources) $(include_HEADERS)
StylePrint_1_0_gir_NAMESPACE = StylePrint
INTROSPECTION_GIRS += StylePrint-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
