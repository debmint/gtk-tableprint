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
 * tblprintprivate.h - Private Header file for TblPrint library function    $
 * $Id:: tblprint.h 40 2016-11-27 22:30:07Z dlb                             $
 * ************************************************************************ */

#include "styletableprint.h"

// A structure to determine the padding for a row, defined in
// PrintContext units
typedef struct pg_mar {
    double left,
           top,
           right,
           bottom;
} ROWPAD, *PROWPAD;

// FontInf structure - Contains individual components to build a
// PangoFontDescription.

typedef struct font_inf {
    int grptype;          // Group-type - Must be first entry in struct
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
    int grptype;            // Group-type - Must be FIRST entry in struct
    PFONTINF font;          // Used to pass to library, library sets up
    double x,               // X position for the current insert
           y;               // Y position for the current insert (not used???)
    int padleft,            // Padding for left side of cell
        padright;           // Padding for right side of cell
    int borderstyle;        // Border style for group or cell
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
    int grptype;                    // The group-type - must be the first entry
    PFONTINF font;                  // Ptr to FONTINF containing descriptions
    int colcount;
    int pointsabove,                // Points for space above the group hdr
        pointsbelow;                // Points for space below the group
    int borderstyle;                // Border style for group or cell
    struct grp_info *grpparent;
    struct grp_info *grpchild;      // NULL if it's body
                                    // to insert into fontdescriptor
    PangoFontDescription *pangofont; // Font to use for this group, also
                                    // used for any cells with no font spec
    ROWPAD *padding;                // Padding around the row;
    struct grp_info *header;        // Display for group-type groups
    gboolean cells_formatted;        // TRUE if cols have been reformatted 
    GPtrArray *celldefs;            // Pointer to CELLINF array
    int grpcol;                     // PGresult col number for group text
} GRPINF, *PGRPINF;

/*typedef struct cell_props {
    PFONTINF font;
    int       cellwidth,
              cellheight;       // Not used????
    ROWPAD     margins;
} CELLPROPS, *PCELLPROPS;*/


typedef struct page_def {
    int firstrow,
        lastrow;
} PG_DEF;

struct xml_udat {
    int depth;
    PGRPINF parent;
    PGRPINF curfmt;
} XMLData;

