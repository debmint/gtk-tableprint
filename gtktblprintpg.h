/* ************************************************************************ *
 * gtktblprintpg.h - Header file for GtkTblPrintPg library function         $
 * $Id::                                                                    $
 * ************************************************************************ */

#include <gtk/gtk.h>
#include <libpq-fe.h>

#define DFLTFONT "Sans Serif"
#define DFLTSIZE 10

typedef struct pg_mar {
    double left,
           top,
           right,
           bottom
} PGMAR *PPGMAR;

// This structure contains all that is needed to insert a single cell
typedef struct cell_info {
    double x,               // X position for the current insert
           y;               // Y position for the current insert
    double cellwidth;
    char  *celltext;        // Text to insert into the cell
} CELLINF, PCELLINF;

typedef struct cell_props {
    double pgwidth,
           pgheight;
    PGMAR  margins;
    char  *fontname;
    double fontsize;
    char  *txtalign;
} CELLPROPS, *PCELLPROPS

typedef struct tbl_data {
} TBLDATA, *PTBLDATA;

