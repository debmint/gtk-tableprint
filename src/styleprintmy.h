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
 * styleprintmy.h - Header file for StylePrintMy Postgresql interface for   *
 * StylePrintMy                                                             *
 * ************************************************************************ */

#ifndef __STYLE_TABLE_MY_H
#define __STYLE_TABLE_MY_H

#ifdef _cplusplus
extern "C"
{       //}  // To make vim quit trying to indent
#endif

#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <styleprinttable.h>
#include <mysql.h>

G_BEGIN_DECLS

#define STYLE_PRINT_TYPE_MY (style_print_my_get_type())

G_DECLARE_FINAL_TYPE(StylePrintMy,style_print_my, STYLE_PRINT, MY, StylePrintTable)

struct _StylePrintMyClass
{
    StylePrintTableClass parent_class;
};

StylePrintMy * style_print_my_new (void);
gint style_print_my_connect (StylePrintMy *self,
                             const gchar  *host,
                             const gchar  *user,
                             const gchar  *passwd,
                             const gchar  *db,
                             unsigned int  port,
                             const gchar  *unix_socket,
                             unsigned long client_flag);

void style_print_my_fromxmlfile ( StylePrintMy *myprnt,
                                      GtkWindow *win,
                                    const gchar *qry,
                                      GPtrArray *params,
                                           char *filename);

void style_print_my_fromxmlstring ( StylePrintMy *myprnt,
                                       GtkWindow *win,
                                     const gchar *qry,
                                       GPtrArray *params,
                                            char *xmlstr);
void style_print_my_do ( StylePrintMy *self, const gchar *qry);
void style_print_my_appendParam ( StylePrintMy *self, const gchar *param);
gint style_print_my_select_db (StylePrintMy *self, const gchar *db);

G_END_DECLS

#ifdef _cplusplus
}
#endif

#endif      //ifndef __STYLE_TABLE_MY_H
