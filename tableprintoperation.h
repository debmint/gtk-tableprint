/* ************************************************************************ *
 * tblprint.h - Header file for GtkTblPrintPg library function              $
 * $Id::                                                                    $
 * ************************************************************************ */

#ifndef __TABLE_PRINT_OPERATION_H
#define __TABLE_PRINT_OPERATION_H

G_BEGIN_DECLS

#ifdef _cplusplus
extern "C"
{       //}     // To make vim quit trying to indent...
#endif

//#include <string.h>
#include <gtk/gtk.h>
//#include <glib-object.h>
//#include <glib.h>
#include <libpq-fe.h>

#define GTK_TYPE_PRINT_OPERATION_TABLE (gtk_print_operation_table_get_type())
//#define TABLE_TYPE_PRINT_OPERATION  (table_print_operation_get_type())

//#define TABLE_PRINT_OPERATION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), TABLE_TYPE_PRINT_OPERATION, TablePrintOperation))

//#define TABLE_IS_PRINT_OPERATION(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), TABLE_TYPE_PRINT_OPERATION))

G_DECLARE_FINAL_TYPE(GtkPrintOperationTable,gtk_print_operation_table, GTK, PRINT_OPERATION_TABLE, GtkPrintOperation)
//G_DECLARE_FINAL_TYPE(TablePrintOperation, table_print_operation, TABLE, PRINT_OPERATION, GtkPrintOperation)

//typedef struct _TablePrintOperationClass TablePrintOperationClass;
//typedef struct _GtkPrintOperationTable GtkPrintOperationTable
//typedef struct _TablePrintOperation TablePrintOperation;

struct _GtkPrintOperationTableClass
{
    GtkPrintOperationClass parent_class;
};

#define DFLTFONT "Sans Serif"
#define DFLTSIZE 10

#ifndef STRMATCH
#   define STRMATCH(a,b) (strcmp(a, b) == 0)
#endif

#ifndef STRNMATCH
#   define STRNMATCH(a,b,n) (strncmp(a,b,) == 0)
#endif

/**
 * Enumerations for data source for text
 *
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
};

/**
 * Definitions for Block type
 *
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
 * Border Styles
 * @SINGLEBAR:     Single bar (thin)
 * @SINGLEBAR_HVY: Single bar (wide)
 * @DBLBAR:        Double line bar
 * @SINGLEBOX:      Box around group with single line
 * @DBLBOX:         Box around group with double lines
 *
 */

#define SINGLEBAR     1
#define SINGLEBAR_HVY 1 << 1
#define DBLBAR        1 << 2
#define SINGLEBOX     1 << 3
#define DBLBOX        1 << 4

#define BDY_HLINE 1 << 7
#define BDY_VBAR  1 << 8

GtkPrintOperationTable *gtk_print_operation_table_new(void);
void *gtk_print_operation_table_from_xmlfile (GtkPrintOperationTable *,
                GtkWindow *, PGresult *, char *);
void *gtk_print_operation_table_from_xmlstring (GtkPrintOperationTable *,
                GtkWindow *, PGresult *, char *);

//typedef struct tbl_data {
//} TBLDATA, *PTBLDATA;
#ifdef _cplusplus
}
#endif

G_END_DECLS

#endif      //ifndef __TABLE_PRINT_OPERATION_H
