/*
Copyright (c) 2017 David Breeding

This file is part of tableprint.

tableprint is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

tableprint is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with tableprint (see the files "COPYING" and "COPYING"). If not,
see <http://www.gnu.org/licenses/>.
*/

/* ************************************************************************ *
 * tblprint.h - Header file for GtkTblPrintPg library function              $
 * $Id::                                                                    $
 * ************************************************************************ */

#ifndef __GTK_TABLE_PRINT_H
#define __GTK_TABLE_PRINT_H

#ifdef _cplusplus
extern "C"
{       //}     // To make vim quit trying to indent...
#endif

//#include <string.h>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <glib.h>
#include <libpq-fe.h>

G_BEGIN_DECLS

#define STYLE_PRINT_TYPE_TABLE (style_print_table_get_type())

//#define GTK_TABLE_PRINT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TABLE_TYPE_PRINT_OPERATION, TablePrintOperation))

//#define TABLE_IS_PRINT_OPERATION(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), TABLE_TYPE_PRINT_OPERATION))

G_DECLARE_DERIVABLE_TYPE(StylePrintTable,style_print_table, STYLE_PRINT, TABLE, GtkPrintOperation)

struct _StylePrintTableClass
{
    GtkPrintOperationClass parent_class;


//    void (*from_xmlfile) (StylePrintTable *tblprnt,
//                         GtkWindow *win,
//                         GPtrArray *data,
//                         char *filename);
//
//    void (*from_xmlstring) (StylePrintTable *tp,
//                           GtkWindow *w,
//                           GPtrArray *data,
//                           char *c);
    /* <private> */
    
    /* Padding for future expansion */
    void (*_gtk_reserved1) (void);
    void (*_gtk_reserved2) (void);
    void (*_gtk_reserved3) (void);
    void (*_gtk_reserved4) (void);
    void (*_gtk_reserved5) (void);
    void (*_gtk_reserved6) (void);
    void (*_gtk_reserved7) (void);
    void (*_gtk_reserved8) (void);
    void (*_gtk_reserved9) (void);
    void (*_gtk_reserved10) (void);
};

#ifndef STRMATCH
#   define STRMATCH(a,b) (strcmp(a, b) == 0)
#endif

#ifndef STRNMATCH
#   define STRNMATCH(a,b,n) (strncmp(a,b,) == 0)
#endif

/**
 * DataSources:
 * @TSRC_STATIC: Constant text
 * @TSRC_DATA:   Table column
 * @TSRC_NOW:    Current time
 * @TSRC_PAGE:   Cell prints "Page"
 * @TSRC_PAGEOF: Cell prints "Page x of y"
 * @TSRC_PRINTF: Printf-formatted data
 *
 */

enum {
    TSRC_STATIC = 1,    // Constant text
    TSRC_DATA,          // Table column
    TSRC_NOW,           // Current time
    TSRC_PAGE,          // Cell prints "Page"
    TSRC_PAGEOF,        // Cell prints "Page X of Y"
    TSRC_PRINTF         // Printf-formatted data
} DataSources;

/**
 * GRPTY:
 * @GRPTY_GROUP:    Group
 * @GRPTY_HEADER:   Header
 * @GRPTY_BODY:     Body text (data for main body)
 * @GRPTY_PAGEHEADER: Header for each page
 * @GRPTY_DOCHD:    Header for first page
 * @GRPTY_CELL:     Cell description
 * @GRPTY_FONT:     Fond description
 *
 */

enum {
    GRPTY_GROUP = 101,
    GRPTY_HEADER,
    GRPTY_BODY,
    GRPTY_PAGEHEADER,
    GRPTY_DOCHD,
    GRPTY_CELL,
    GRPTY_FONT
} GRPTY;

// Group border bitmap
/**
 * BorderStyles:
 * @SINGLEBAR:      Single bar (thin)
 * @SINGLEBAR_HVY:  Single bar (wide)
 * @DBLBAR:         Double line bar
 * @SINGLEBOX:      Box around group with single line
 * @DBLBOX:         Box around group with double lines
 *
 */

enum
{
    SINGLEBAR     = 1,
    SINGLEBAR_HVY = 1 << 1,
    DBLBAR        = 1 << 2,
    SINGLEBOX     = 1 << 3,
    DBLBOX        = 1 << 4
} BorderStyles;

/**
 * BodyLines:
 * BDY_HLINE: Horizontal line
 * BDY_VBAR:  Vertical bar
 *
 */

enum
{
    BDY_HLINE = 1 << 7,
    BDY_VBAR  = 1 << 8
} BodyLines;

void style_print_table_from_xmlfile (StylePrintTable *tblprnt,
                                           GtkWindow *win,
                                           GPtrArray *data,
                                                char *filename);

void style_print_table_from_xmlstring (StylePrintTable *tp,
                                             GtkWindow *w,
                                             GPtrArray *data,
                                                  char *c);

void style_print_table_greet(       StylePrintTable *self,
                                              GPtrArray *ary);

StylePrintTable *style_print_table_new(void);
void style_print_table_set_wmain (StylePrintTable *self, GtkWindow *win);
GtkWindow * style_print_table_get_wmain (StylePrintTable *self);

#ifdef _cplusplus
}
#endif

G_END_DECLS

#endif      //ifndef __GTK_TABLE_PRINT_H
