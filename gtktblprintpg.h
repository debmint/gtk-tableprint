/* ************************************************************************ *
 * gtktblprintpg.h - Header file for GtkTblPrintPg library function         $
 * $Id::                                                                    $
 * ************************************************************************ */

#include <gtk/gtk.h>
#include <glib.h>
#include <libpq-fe.h>

#define DFLTFONT "Sans Serif"
#define DFLTSIZE 10

// enumerations for data source for text
enum {
    TSRC_STATIC = 1,    // Constant text
    TSRC_DATA,          // Table column
    TSRC_NOW,           // Current time
    TSRC_PRINTF         // Printf-formatted data
};

typedef struct pg_mar {
    double left,
           top,
           right,
           bottom;
} PGMAR, *PPGMAR;

typedef struct font_inf {
    char *family;
    int   size;
    PangoStyle  style;
    PangoWeight weight;
} FONTINF, *PFONTINF;

// This structure contains all that is needed to insert a single cell
typedef struct cell_info {
    double x,               // X position for the current insert
           y;               // Y position for the current insert (not used???)
    double cellwidth;
    PFONTINF font;          // Used to pass to library, library sets up
                            // pangofont from this
    PangoFontDescription *pangofont;    // Derived from "font" pointer
    int   txtsource;        // Where to get source
    char  *celltext;        // Text to insert into the cell
    int    txtjust;         // Justification - right/left/center
} CELLINF, *PCELLINF;

//typedef struct line_props {
//} LINEPROP, *PLINEPROP;

typedef struct grp_info {
    int colcount;
    GRPTY grptype;                  // Whether it's group, body, etc
    struct grp_info *grpparent;
    struct grp_info *grpchild;      // NULL if it's body
    CELLINF **celldefs;             // Pointer to CELLINF array
} GRPINF, *PGRPINF;

typedef struct cell_props {
    double   pgwidth,
             pgheight;
    PGMAR    margins;
} CELLPROPS, *PCELLPROPS;

//typedef struct tbl_data {
//} TBLDATA, *PTBLDATA;

