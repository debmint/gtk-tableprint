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
 * styleprinttableprivate.h - Private Header file for StylePrintTable       *
 * library function                                                         $
 * $Id:: tblprint.h 40 2016-11-27 22:30:07Z dlb                             $
 * ************************************************************************ */

#include "styleprinttable.h"

// A structure to determine the padding for a row, defined in
// PrintContext units
typedef struct pg_mar {
    double left,
           top,
           right,
           bottom;
} ROWPAD, *PROWPAD;

// This structure contains all that is needed to insert a single cell
// Each column of a Header or Body description will contain an array of these.
typedef struct cell_info {
    int grptype;            // Group-type - Must be FIRST entry in struct
    PangoFontDescription *pangofont;      // pangofont from this
    double x,               // X position for the current insert
           y;               // Y position for the current insert (not used???)
    int padleft,            // Padding for left side of cell
        padright;           // Padding for right side of cell
    int borderstyle;        // Border style for group or cell
    double percent;         // percent of page width to use for this cell
    double cellwidth;       // Cell width in POINTS
    int   txtsource;        // Where to get source
    char  *celltext;        // Text to insert into the cell
    //gchar *cell_col;        // The col # for cell text (only "data" txtsource)
    PangoAlignment layoutalign;     // Justification - right/left/center
} CELLINF, *PCELLINF;

typedef struct grp_info {
    int grptype;                    // The group-type - must be the first entry
                                    // to insert into fontdescriptor
    PangoFontDescription *pangofont; // Font to use for this group, also
                                    // used for any cells with no font spec
    int colcount;
    int pointsabove,                // Points for space above the group hdr
        pointsbelow;                // Points for space below the group
    int borderstyle;                // Border style for group or cell
    struct grp_info *grpparent;
    struct grp_info *grpchild;      // NULL if it's body
    ROWPAD *padding;                // Padding around the row;
    struct grp_info *header;        // Display for group-type groups
    gboolean cells_formatted;       // TRUE if cols have been reformatted 
    GPtrArray *celldefs;            // Pointer to CELLINF array
    gchar *grpcol;                  // PGresult col number for group text
} GRPINF, *PGRPINF;

typedef struct page_def {
    int firstrow,
        lastrow;
} PG_DEF;

struct xml_udat {
    int depth;
    PGRPINF parent;
    PGRPINF curfmt;
} XMLData;

