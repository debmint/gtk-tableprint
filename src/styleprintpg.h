/*
Copyright (c) 2017 David Breeding

This file is part of tableprint.

tableprint is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

tableprint is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
 along with tableprint (see the file "COPYING"). If not,
see <http://www.gnu.org/licenses/>.
*/

/* ************************************************************************ *
 * styleprintpg.h - Header file for StylePrintPg Postgresql interface for   *
 * StylePrintTable                                                          *
 * ************************************************************************ */

#ifndef __STYLE_TABLE_PG_H
#define __STYLE_TABLE_PG_H

#ifdef _cplusplus
extern "C"
{       //}  // To make vim quit trying to indent
#endif

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <styleprinttable.h>
#include <styleprintpg.h>

G_BEGIN_DECLS

#define STYLE_PRINT_TYPE_PG (style_print_pg_get_type())

G_DECLARE_FINAL_TYPE(StylePrintPg,style_print_pg, STYLE_PRINT, PG, StylePrintTable)

struct _StylePrintPgClass
{
    StylePrintTableClass parent_class;
};

StylePrintPg * style_print_pg_new (void);
gint style_print_pg_connect (StylePrintPg *self, gchar *dbn);

void style_print_pg_from_xmlfile ( StylePrintPg *pgprnt,
                                      GtkWindow *win,
                                    const gchar *qry,
                                           char *filename);

void style_print_pg_from_xmlstring ( StylePrintPg *pgprnt,
                                        GtkWindow *win,
                                      const gchar *qry,
                                             char *xmlstr);
void style_print_pg_do ( StylePrintPg *self, const gchar *qry);
void style_print_pg_appendParam ( StylePrintPg *self, const gchar *param);

G_END_DECLS

#ifdef _cplusplus
}
#endif

#endif      //ifndef __STYLE_TABLE_PG_H
