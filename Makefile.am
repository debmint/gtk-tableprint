# ######################################################################### $
# Makefile for gtktblprint-pg.                                              $
# $Id::                                                                     $
# ######################################################################### $

AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` -lexpat

lib_LTLIBRARIES	= libgtktblprintpg.la
libgtktblprintpg_la_SOURCES = gtktblprintpg.c unixcmds.c tableprint.c
include_HEADERS = gtktblprintpg.h
