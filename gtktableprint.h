/* ************************************************************************ *
 * tblprint.h - Header file for GtkTblPrintPg library function              $
 * $Id::                                                              $
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
//#include <glib.h>
#include <libpq-fe.h>

G_BEGIN_DECLS

#define GTK_TYPE_TABLE_PRINT (gtk_table_print_get_type())

//#define GTK_TABLE_PRINT(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TABLE_TYPE_PRINT_OPERATION, TablePrintOperation))

//#define TABLE_IS_PRINT_OPERATION(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), TABLE_TYPE_PRINT_OPERATION))

G_DECLARE_FINAL_TYPE(GtkTablePrint,gtk_table_print, GTK, TABLE_PRINT, GtkPrintOperation)

//typedef struct _TablePrintOperationClass TablePrintOperationClass;
//typedef struct _GtkGtkTablePrint GtkGtkTablePrint
//typedef struct _TablePrintOperation TablePrintOperation;

struct _GtkTablePrintClass
{
    GtkPrintOperationClass parent_class;
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

GtkTablePrint *gtk_table_print_new(void);
void gtk_table_print_from_xmlfile (GtkTablePrint *tblprnt,
                GtkWindow *win, PGresult *res, char *filename);
void gtk_table_print_from_xmlstring (GtkTablePrint *tp,
                GtkWindow *w, PGresult *p, char *c);

//typedef struct tbl_data {
//} TBLDATA, *PTBLDATA;
#ifdef _cplusplus
}
#endif

G_END_DECLS

#endif      //ifndef __GTK_TABLE_PRINT_H
