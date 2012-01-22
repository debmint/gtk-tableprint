/* ************************************************************************ *
 * gtktblprintpg.h - Header file for GtkTblPrintPg library function         $
 * $Id::                                                                    $
 * ************************************************************************ */

#ifndef GTKPRINTTABLE_DEFINED
#define GTKPRINTTABLE_DEFINED

#ifdef _cplusplus
extern "C"
{       //}     // To make vim quit trying to indent...
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libpq-fe.h>

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
    TSRC_PRINTF         // Printf-formatted data
};

enum {
    GRPTY_GROUP = 101,
    GRPTY_HEADER,
    GRPTY_BODY,
    GRPTY_PGHD,
    GRPTY_DOCHD,
    GRPTY_CELL,
    GRPTY_FONT
} GRPTY;

typedef struct pg_mar {
    double left,
           top,
           right,
           bottom;
} PGMAR, *PPGMAR;

// FontInf structure - Contains individual components to build a
// PangoFontDescription.

typedef struct font_inf {
    int grptype;          // Group-type - Must be first entry in strucct
    char *family;
    int   size;         // Font-size in POINTS
    PangoStyle  style;
    PangoWeight weight;
    PangoVariant variant;
    PangoStretch stretch;
} FONTINF, *PFONTINF;

// This structure contains all that is needed to insert a single cell
// Each column of a Header or Body description will contain an array of these.
typedef struct cell_info {
    int grptype;              // Group-type - Must be FIRST entry in struct
    PFONTINF font;          // Used to pass to library, library sets up
    double x,               // X position for the current insert
           y;               // Y position for the current insert (not used???)
    double percent;         // percent of page width to use for this cell
    double cellwidth;       // Cell width in POINTS
                            // pangofont from this
    PangoFontDescription *pangofont;    // Derived from "font" pointer
    int   txtsource;        // Where to get source
    char  *celltext;        // Text to insert into the cell
    int    cell_col;        // The col # for cell text (only "data" txtsource)
    PangoAlignment layoutalign;     // Justification - right/left/center
} CELLINF, *PCELLINF;

//typedef struct line_props {
//} ROWPROP, *PROWPROP;

typedef struct grp_info {
    int grptype;              // The group-type - must be the first entry
    PFONTINF font;                  // Ptr to FONTINF containing descriptions
    int colcount;
//    GRPTY grptype;                  // Whether it's group, body, etc
    struct grp_info *grpparent;
    struct grp_info *grpchild;      // NULL if it's body
                                    // to insert into fontdescriptor
    PangoFontDescription *pangofont; // Font to use for this group, also
                                    // used for any cells with no font spec
    struct grp_info *header;        // Display for group-type groups
    gboolean cells_formatted;        // TRUE if cols have been reformatted 
    GPtrArray *celldefs;            // Pointer to CELLINF array
    int grpcol;                     // PGresult col number for group text
} GRPINF, *PGRPINF;

typedef struct cell_props {
    PFONTINF font;
    int       cellwidth,
              cellheight;       // Not used????
    PGMAR     margins;
} CELLPROPS, *PCELLPROPS;

// GLOBDAT: Data structure containing all data needed throughout all processes
typedef struct data_struct {
    int pageno;
    int TotPages;
    double pageheight;
    int datarow;
    double xpos;
    double ypos;
    gboolean DoPrint;
    int CellHeight;
    cairo_t *cr;
    GtkWindow *w_main;
    GtkPageSetup *Page_Setup;
    GtkPrintContext *context;
    PangoLayout *layout;
    GRPINF *DocHeader;
    GRPINF *PageHeader;
    GRPINF *grpHd;
    PGresult *pgresult;
} GLOBDAT;

void *tblprint_from_xmlfile (GtkWindow *, PGresult *, char *);
void *tblprint_from_xmlstring (GtkWindow *, PGresult *, char *);

//typedef struct tbl_data {
//} TBLDATA, *PTBLDATA;
#ifdef _cplusplus
}
#endif
#endif      //ifndef GTKPRINTTABLE_DEFINED
