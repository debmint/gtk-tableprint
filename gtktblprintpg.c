/* ************************************************************************ *
 * gtktblprintpg.c - Mainline module for GtkTblPrintPg library function     $
 *                                                                          $
 * The setup of the control for the print output is contained in a          $
 * GHashTable structure.  This contains headers for each part of the        $
 * document.  The divisions are:                                            $
 * "docheader" - the line printed at the begin of the document              $
 * "pageheader" - the header for each page                                  $
 * "groups" - The data to be grouped                                        $
 * "defaultcell" - The default font specifications for cells which are not  $
 *      specifically defined.                                               $
 * Each of the above (GHashTable's), can have the following divisions       $
 * "cols" - A GPtrArray defining the column specifications for each column. $
 *      This can have the following defs.                                   $
 *      "colwidth" (mandatory) - the percentage of the page width for this  $
 *              column.                                                     $
 *      "font" - a GHashTable with specs for "family", "size", "alignment", $
 *          and "weight". These will be converted to a PangoFontDescription $
 *                                                                          $
 * $Id::                                                                    $
 * ************************************************************************ */

#include <stdlib.h>
#include "gtktblprintpg.h"

/* ************************************************************************ *
 
 * ************************************************************************ */

static void setup_formatting(GHashTable *);

gboolean Formatted = FALSE;
GPtrArray *dochead;
FONTINF defaultfont = {"Serif", 10, PANGO_STYLE_NORMAL, PANGO_WEIGHT_NORMAL};

CELLINF defaultcell =
{
    0, 0, 0,            // x, y position, and cell width not defined
    &defaultfont,       // font
    NULL,               // PangoFont
    0,                  // txtsource
    NULL,               // celltext
    PANGO_ALIGN_LEFT    // txtalign
};

struct xml_udat {
    int depth;
    PGRPINF parent;
    PGRPINF curfmt;
} XMLData;

PGRPINF FmtList;    // The formatting tree;

/* ******************************************************************** *
 * render_document() - Entry routine for rendering document             *
 * Passed:  (1) data : a PGresult ordered in the way it is to appear    *
 *                  in the printout.                                    *
 *          (2) formatting : A GHashTable containing all the formatting *
 *                  data.                                               *
 * ******************************************************************** */

void render_document(PGresult *data, GHashTable *formatting)
{
    defaultfont.pangofont = pango_font_description_new_from_string(
                "Serif PANGO_STYLE_NORMAL 10");
    pango_font_description_set_weight(defaultfont.pangofont,
            PANGO_WEIGHT_NORMAL);
    setup_formatting(formatting);
}

