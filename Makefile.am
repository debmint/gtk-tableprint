# ######################################################################### $
# Makefile for gtktblprint-pg.                                              $
# $Id::                                                                     $
# ######################################################################### $

AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` -lexpat

lib_LTLIBRARIES	= libtblprint.la
libgtktblprintpg_la_SOURCES = tblprint.c unixcmds.c printout.c
include_HEADERS = tblprint.h
