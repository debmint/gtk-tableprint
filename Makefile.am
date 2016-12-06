# ######################################################################### $
# Makefile for tableprintoperation.                                              		$
# $Id::                                                                     $
# ######################################################################### $

AM_CPPFLAGS = -g -Wall `pkg-config --cflags gtk+-3.0 libpq`

AM_LDFLAGS = -g -Wall @noundef@ `pkg-config --libs gtk+-3.0 libpq`\
			-L`pg_config --libdir` -lexpat

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES	= libtableprintoperation.la
libtableprintoperation_la_SOURCES = tableprintoperation.c
include_HEADERS = tableprintoperation.h
