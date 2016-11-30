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

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libpq-fe.h>

#define TABLE_TYPE_PRINT_OPERATION  (table_print_operation_get_type())

G_DECLARE_FINAL_TYPE(TablePrintOperation, table_print_operation, TABLE, PRINT_OPERATION, GtkPrintOperation)

#define DFLTFONT "Sans Serif"
#define DFLTSIZE 10

#ifndef STRMATCH
#   define STRMATCH(a,b) (strcmp(a, b) == 0)
#endif

#ifndef STRNMATCH
#define STRNMATCH(a,b,n) (strncmp(a,b,) == 0)
#endif

// enumerations for data source for text
enum {
    TSRC_STATIC = 1,    // Constant text
    TSRC_DATA,          // Table column
    TSRC_NOW,           // Current time
    TSRC_PAGE,          // Cell prints "Page"
    TSRC_PAGEOF,        // Cell prints "Page X of Y"
    TSRC_PRINTF         // Printf-formatted data
};

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

#define SINGLEBAR     1
#define SINGLEBAR_HVY 1 << 1
#define DBLBAR        1 << 2
#define SINGLEBOX     1 << 3
#define DBLBOX        1 << 4

#define BDY_HLINE 1 << 7
#define BDY_VBAR  1 << 8

void *tblprint_from_xmlfile (GtkWindow *, PGresult *, char *);
void *tblprint_from_xmlstring (GtkWindow *, PGresult *, char *);

//typedef struct tbl_data {
//} TBLDATA, *PTBLDATA;
#ifdef _cplusplus
}
#endif

G_END_DECLS

#endif      //ifndef __TABLE_PRINT_OPERATION_H
