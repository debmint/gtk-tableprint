# ######################################################################### $
# Makefile for gtktblprint-pg.                                              $
# $Id::                                                                     $
# ######################################################################### $

#AM_CPPFLAGS = -g -Wall `pkg-config --cflags --libs gtk+-2.0 glib-2.0`
AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-2.0` -I`pg_config --includedir`

#AM_LDFLAGS = -g -Wall -Wl,--rpath -Wl,/us/local/lib `pkg-config --libs gtk+-2.0 glib-2.0`
#AM_LDFLAGS = -g -Wall `pkg-config --libs gtk+-2.0 glib-2.0 gmodule-2.0` libpq.lib gtk-win32-2.0 glib-2.0
AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-2.0`\
			-L`pg_config --libdir` -lpq -lexpat -lglib-2.0

lib_LTLIBRARIES	= libgtktblprintpg.la
libgtktblprintpg_la_SOURCES = gtktblprintpg.c unixcmds.c tableprint.c
include_HEADERS = gtktblprintpg.h
