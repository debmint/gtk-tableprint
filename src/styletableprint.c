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
along with tableprint (see the file "COPYING").  If not,
see <http://www.gnu.org/licenses/>.
*/

/* ************************************************************************ *
 * styletableprint.c - Mainline module for TablePrint library function      $
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
#include <string.h>
#include <pango/pango.h>
#include <glib-object.h>
#include <cairo.h>
#include "styletableprintpriv.h"

/**
 * SECTION: styletableprint
 * @Title: StylePrintTable
 * @Short_description: Print data from PostGres, in tabular form using #GtkPrintOperation
 * @See_also: #GtkPrintOperation, #libpq (in PostGreSQL documentation)
 *
 * StylePrintTable a subclass of GtkPrintOperation.  It formats data obtained
 * from a query sent to a PostGreSQL database into columns and rows.
 * The data can be grouped and sub-grouped, if need be.
 * The formatting of the printout is defined by an XML document, either
 * obtained from a file, or an embedded string.
 *
 * To print a document, you first create a #StylePrintTable object with
 * style_print_table_new().  Before actually printing, you need to establish
 * a connection with style_print_table_connect(), and if you are going to send
 * your query with parameters, you need to establish them (one at a time), with
 * style_print_table_appendParam().  You then begin printing by calling the
 * function style_print_table_from_xmlstring() or
 * style_print_table_from_xmlfile(), depending on the source of the xml
 * data.
 * Parameters to pass to the the function which are:
 * the #StylePrintTable object, a window (normally the main window)
 * is to be the parent window of any warning dialogs, etc.  This window
 * can be passed as a NULL value, in which case error messages are the sent
 * to STDERR.  Additional parameters are the PGresult of the database query,
 * and a string which represents either the pointer to the string or the name
 * of the file containing the xml specification for the output.
 *
 * There is no need to set up the "begin_print" or "draw_page" callbacks,
 * because this object handles this setup itself.
 */

/**
 * Constants:
 * @DFLTFONT: Default font name
 * @DFLTSIZE: Default size in points
 *
 */

#define DFLTFONT "Sans Serif"
#define DFLTSIZE 10

//GtkPrintOperation *po;
static void render_report (StylePrintTable*);
static void start_element_main(GMarkupParseContext *, const gchar *,
        const gchar **, const gchar **, gpointer, GError **);
static void end_element_main(GMarkupParseContext *, const gchar *,
        gpointer, GError **);
static void prs_err(GMarkupParseContext *, GError *, gpointer);

static void report_error (StylePrintTable *, gchar *);
static void render_page(StylePrintTable *);
static void style_print_table_begin_print (GtkPrintOperation *,
        GtkPrintContext *);
static void style_print_table_draw_page (GtkPrintOperation *,
                                GtkPrintContext *, int);
static void style_print_table_draw_page (GtkPrintOperation *,
                                GtkPrintContext *, int);

#define CELLPAD_DFLT 10

struct _StylePrintTable
{
    GtkPrintOperation parent_instance;

    //instance variables for subclass
    GtkWindow *w_main;

    // Connection Variables
    PGconn *conn;           // The connection
    PGresult *pgresult;     // Pointer to the PostGreSQL PGresult
    GPtrArray *qryParams;   // Array of pointers to Query Params

    // Headers
    GRPINF *DocHeader;  // The Document Header (header for first page)
    GRPINF *PageHeader; // The Pagheader (header for each page)
    GRPINF *grpHd;

    double pageheight;
    gint TotPages;           // Total Pages
    GtkPageSetup *Page_Setup;
    GPtrArray *PageEndRow;  // Last Data Row for each page.
    gboolean DoPrint;       // Flag that we want to actually print (pass 2)
    cairo_t *cr;            // The Cairo Print Context
    GtkPrintContext *context;
    PangoLayout *layout;
    ROWPAD *DefaultPadding;
    GSList *elList;

    // Current vars
    gint pageno;             // Current Page #
    gint datarow;            // Current row in the Data Array
    double xpos;        // Current Horizontal position on page
    double ypos;        // Current Vertical Position on page
    double textheight;
    gint CellHeight;
};

G_DEFINE_TYPE(StylePrintTable, style_print_table, GTK_TYPE_PRINT_OPERATION)

// Forward references within this module

//static void setup_formatting(GHashTable *);
//static void alt_cell_end_element(GMarkupParseContext *, const gchar *,
//        gpointer, GError **);

char *GroupElements[] = {"pageheader", "group", "body", NULL};
gboolean Formatted = FALSE;
//PangoFontDescription *DefaultPangoFont;
CELLINF *defaultcell;
PGRPINF FmtList;    // The formatting tree;

GMarkupParser prsr = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};

//GMarkupParser sub_prs = {start_element_main, end_element_main, NULL,
//                        NULL, prs_err};

/**
 * style_print_table_greet:
 * @self: The StylePrintTable
 * @ary: The array to parse
 *
 * This is only a temporary function to do tests, and will be deleted
 * later on.
 */

void
style_print_table_greet (StylePrintTable *self,gchar *ary)
{
        printf ("%s\n", (ary));
}

static void
style_print_table_init (StylePrintTable *op)
{
    // initialisation goes here
    ///g_signal_connect (po, "begin-print", G_CALLBACK(begin_print), self);
    //g_signal_connect (po, "draw-page", G_CALLBACK(draw_page), self);
    gtk_print_operation_set_default_page_setup (GTK_PRINT_OPERATION(op),
            op->Page_Setup);
    op->w_main = NULL;
    op->conn = NULL;
    op->pgresult = NULL;
    op->qryParams = NULL;
}

static void
style_print_table_class_init (StylePrintTableClass *class)
{
    // virtual function overrides go here
    //GObjectClass *gobject_class = (GObjectClass *) class;
    GtkPrintOperationClass *print_class = (GtkPrintOperationClass *) class;

    // property and signal definitions go here
    //gobject_class->finalize = style_print_table_finalize;
    print_class->begin_print = style_print_table_begin_print;
    print_class->draw_page = style_print_table_draw_page;
    //print_class->end_print = style_print_table_end_print;
}

/* **************************************************************** *
 * match_attrib() - Utility function to match a string constant.    *
 *      A NULL-terminated array of string values to match is        *
 *      passed.  The calling function provides this string, and     *
 *      also has an array with corresponding values for each        *
 *      element of this array, with some value provided as the      *
 *      final element, corresponding to the final NULL.  It can be  *
 *      either a default value, or value not to be expected to      *
 *      signal that a match was not found.                          *
 * Passed : (1) - txtlist : A NULL-terminated list of usable string *
 *                values.                                           *
 *          (2) - matchval: The string value to match.              *
 * **************************************************************** */

static int
match_attrib (const char **txtlist, const char *matchval)
{
    int idx = 0;

    while (txtlist[idx] && !STRMATCH(matchval, txtlist[idx]))
    {
        ++idx;
    }

    return idx;
}

/* **************************************************************** *
 * name_to_pango_style() - Converts a name (text string) to a       *
 *          numeric Pango Style.                                    *
 * **************************************************************** */

PangoStyle
name_to_pango_style (const char *name)
{
    const char *style_txt[] = {"normal", "oblique", "italic", NULL};
    PangoStyle style[] = {PANGO_STYLE_NORMAL, PANGO_STYLE_OBLIQUE,
                        PANGO_STYLE_ITALIC, PANGO_STYLE_NORMAL};

    return style[match_attrib (style_txt, name)];
}

/* **************************************************************** *
 * name_to_pango_weight() - Converts a name string to a PangoWeight *
 * **************************************************************** */

PangoWeight
name_to_pango_weight (const char *name)
{
    const char *weight_txt[] = {
        "thin", "ultralight", "light", "book", "normal", "medium",
        "semibold", "bold", "heavy", "ultraheavy", NULL};
    PangoWeight weight[] = {
        PANGO_WEIGHT_THIN, PANGO_WEIGHT_ULTRALIGHT, PANGO_WEIGHT_LIGHT,
        PANGO_WEIGHT_BOOK, PANGO_WEIGHT_NORMAL, PANGO_WEIGHT_MEDIUM,
        PANGO_WEIGHT_SEMIBOLD, PANGO_WEIGHT_BOLD, PANGO_WEIGHT_ULTRABOLD,
        PANGO_WEIGHT_HEAVY, PANGO_WEIGHT_ULTRAHEAVY, PANGO_WEIGHT_NORMAL};

    return weight[match_attrib (weight_txt, name)];
}

PangoVariant
name_to_pango_variant (const char *name)
{
    const char *variant_name[] = {"normal", "small-caps", NULL};
    PangoVariant variant[] = {PANGO_VARIANT_NORMAL,
                            PANGO_VARIANT_SMALL_CAPS, PANGO_VARIANT_NORMAL};

    return variant[match_attrib (variant_name, name)];
}

PangoStretch
name_to_pango_stretch (const char *name)
{
    const char *stretch_name[] = {"ultra-condensed", "extra-condensed",
                    "condensed", "semi-condensed", "normal", "semi-expanded",
                    "expanded", "extra-expanded", "ultra-expanded", NULL};
    PangoStretch stretch[] = {
                PANGO_STRETCH_ULTRA_CONDENSED, PANGO_STRETCH_EXTRA_CONDENSED,
                PANGO_STRETCH_CONDENSED, PANGO_STRETCH_SEMI_CONDENSED,
                PANGO_STRETCH_NORMAL, PANGO_STRETCH_SEMI_EXPANDED,
                PANGO_STRETCH_EXPANDED, PANGO_STRETCH_EXTRA_EXPANDED,
                PANGO_STRETCH_ULTRA_EXPANDED, PANGO_STRETCH_NORMAL};

    return stretch[match_attrib (stretch_name, name)];
}

PangoAlignment
name_to_layout_align (const char *name)
{
    const char *align_name[] = {"l", "c", "r", NULL};
    PangoAlignment align[] = {PANGO_ALIGN_LEFT, PANGO_ALIGN_CENTER,
            PANGO_ALIGN_RIGHT, PANGO_ALIGN_LEFT};

    return align[match_attrib (align_name, name)];
}

/* Error reporting routine.
 * If self->w_main is defined, the message will be reported in a dialog box
 * else it is sent to stderr
 */

static void
report_error (StylePrintTable *self, char *message)
{
    if (self->w_main)
    {
        GtkWidget *dlg;

        dlg = gtk_message_dialog_new (self->w_main,
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                "%s", message);
        gtk_dialog_run (GTK_DIALOG(dlg));
        gtk_widget_destroy (dlg);
    }
    else
    {
        fprintf (stderr, "%s", message);
    }
}

static void 
prs_err(GMarkupParseContext *context, GError *error, gpointer user_data)
{
    fprintf(stderr, "Error encountered: %s\n", error->message);
}

/* **************************************************************** *
 * add_cell_attribs() - Add attributes to a cell definition.  This  *
 *      portion is broken out of append_cell_defs() in order to     *
 *      handle attribute settings for cells that are not made up    *
 *      of arrays, such as "defaultcell".                           *
 * **************************************************************** */

static void
add_cell_attribs (CELLINF *cell, const gchar **attrib_names,
                                    const gchar **attrib_vals)
{
    int idx = 0;

    while (attrib_names[idx])
    {
        if (STRMATCH(attrib_names[idx], "percent"))
        {
            cell->percent = strtof (attrib_vals[idx], NULL);
        }

        else if (STRMATCH(attrib_names[idx], "cellwidth"))
        {
            cell->cellwidth = strtof (attrib_vals[idx], NULL);
        }

        else if (STRMATCH(attrib_names[idx], "textsource"))
        {
            const char *val = attrib_vals[idx];

            if (STRMATCH(val, "data"))
            {
                cell->txtsource = TSRC_DATA;
            }
            else if (STRMATCH(val, "static"))
            {
                cell->txtsource = TSRC_STATIC;
            }
            else if (STRMATCH(val, "now"))
            {
                cell->txtsource = TSRC_NOW;
            }
            else if (STRMATCH(val, "printf"))
            {
                cell->txtsource = TSRC_PRINTF;
            }
            else if (STRMATCH(val, "pageof"))
            {
                cell->txtsource = TSRC_PAGEOF;
            }
            //TODO: add error reporting...
        }

        else if (STRMATCH(attrib_names[idx], "celltext"))
        {
            cell->celltext = g_strdup (attrib_vals[idx]);
        }

        else if (STRMATCH(attrib_names[idx], "align"))
        {
            cell->layoutalign = name_to_layout_align (attrib_vals[idx]);
        }
        // TODO: add error reporting

        ++idx;
    }
}

/* **************************************************************** *
 * append_cell_defs() : Populate a cell definition in the row       *
 *          array, and append this new cell onto the array of cells *
 *  If the array has not already been created, it is now created.   *
 * **************************************************************** */

static CELLINF *
append_cell_def (StylePrintTable *self, const gchar **attrib_names,
                    const gchar **attrib_vals, GRPINF *parentgrp)
{
    CELLINF *mycell;
    GPtrArray *cellary;

    if (parentgrp)
    {
        if (!(cellary = parentgrp->celldefs))
        {
            parentgrp->celldefs = g_ptr_array_new();
        }
    }

    mycell = calloc (1, sizeof(CELLINF));
    mycell->grptype = GRPTY_CELL;
    mycell->padleft = CELLPAD_DFLT;
    mycell->padright = CELLPAD_DFLT;

    add_cell_attribs (mycell, attrib_names, attrib_vals);

    if (mycell->txtsource == TSRC_DATA)
    {
        if (mycell->celltext && strlen(mycell->celltext))
        {
            mycell->cell_col = PQfnumber (self->pgresult, mycell->celltext);

            if (mycell->cell_col == -1)
            {
                if (self->w_main)
                {
                    GtkWidget *dlg = gtk_message_dialog_new (self->w_main,
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
                            "Error! '%s' is not a valid column name",
                            mycell->celltext);
                    gtk_dialog_run(GTK_DIALOG(dlg));
                    gtk_widget_destroy(dlg);
                }
                else
                {
                    fprintf (stderr,"Error: '%s' is not a valid column\n",
                            mycell->celltext);
                }
            }

            if (PQresultStatus (self->pgresult) != PGRES_TUPLES_OK)
            {
                if (self->w_main)
                {
                    GtkWidget *dlg = gtk_message_dialog_new (self->w_main,
                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                            PQresultErrorMessage (self->pgresult));
                    gtk_dialog_run (GTK_DIALOG(dlg));
                    gtk_widget_destroy (dlg);
                }
                else
                {
                    fprintf (stderr, "Cannot determine column # for: %s\n",
                                PQresultErrorMessage (self->pgresult));
                }

            }
        }
    }

    if (parentgrp)
    {
        g_ptr_array_add (parentgrp->celldefs, mycell);
    }

    return mycell;
}

/* **************************************************************** *
 * allocate_new_group() - Allocates space for a new group, body,    *
 *          header group and initializes it.                        *
 * Passed : (1) - Pointer to the group under which this will exist  *
 *          (2) - The GRPTY_* of this grouping.                     *
 * Returns: Pointer to this new group's definition.                 *
 * **************************************************************** */


static GRPINF *
allocate_new_group (StylePrintTable *self, const char **attrib_names,
                    const char **attrib_vals, GRPINF *parent, int grptype)
{
    int grpidx = 0;
    GRPINF *newgrp = calloc (1, sizeof(GRPINF));

    if ((grptype == GRPTY_GROUP) || (grptype == GRPTY_BODY))
    {
        if (!parent && !self->grpHd)
        {
            self->grpHd = (GRPINF *)newgrp;
            newgrp = self->grpHd;
        }
    }

    newgrp->grptype = grptype;
    newgrp->font = malloc (sizeof(FONTINF));

    if (parent && parent->font)
    {
        memcpy (newgrp->font, parent->font, sizeof(FONTINF));
    }
    else
    {
        memcpy (newgrp->font, defaultcell->font, sizeof(FONTINF));
    }

    if (parent && parent->pangofont)
    {
        newgrp->pangofont = pango_font_description_copy (parent->pangofont);
    }
    else
    {
        newgrp->pangofont =
                    pango_font_description_copy (defaultcell->pangofont);
    }

    while (attrib_names[grpidx])
    {
        if (STRMATCH(attrib_names[grpidx], "groupsource"))
        {
            newgrp->grpcol = PQfnumber (self->pgresult,
                                            attrib_vals[grpidx]);

            if (newgrp->grpcol == -1)
            {
                if (self->w_main)
                {
                    GtkWidget *dlg = gtk_message_dialog_new (self->w_main,
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
                            "Error! '%s' is not a valid column name",
                            attrib_vals[grpidx]);
                    gtk_dialog_run(GTK_DIALOG(dlg));
                    gtk_widget_destroy(dlg);
                }
                else
                {
                    fprintf (stderr, "Error: '%s' is not a valid column\n",
                            attrib_vals[grpidx]);
                }
            }
        }
        else if (STRMATCH(attrib_names[grpidx], "pointsabove"))
        {
            newgrp->pointsabove = atoi (attrib_vals[grpidx]);
        }
        else if (STRMATCH(attrib_names[grpidx], "pointsbelow"))
        {
            newgrp->pointsbelow = atoi (attrib_vals[grpidx]);
        }
        else if (STRMATCH(attrib_names[grpidx], "outerborder"))
        {
            if (STRMATCH(attrib_vals[grpidx], "singlebar"))
            {
                newgrp->borderstyle = SINGLEBAR;
            }
            else if (STRMATCH(attrib_vals[grpidx], "doublebar"))
            {
                newgrp->borderstyle = DBLBAR;
            }
            else if (STRMATCH(attrib_vals[grpidx], "singlebarheavy"))
            {
                newgrp->borderstyle = SINGLEBAR_HVY;
            }
            else if (STRMATCH(attrib_vals[grpidx], "singlebox"))
            {
                newgrp->borderstyle = SINGLEBOX;
            }
            else if (STRMATCH(attrib_vals[grpidx], "doublebox"))
            {
                newgrp->borderstyle = DBLBOX;
            }
        }
        // We may wish to move this into the body allocation segment
        else if (STRMATCH(attrib_names[grpidx], "cellborder"))
        {
            if (STRMATCH(attrib_vals[grpidx], "hline"))
            {
                newgrp->borderstyle = BDY_HLINE;
            }
            else if (STRMATCH(attrib_vals[grpidx], "vbar"))
            {
                newgrp->borderstyle = BDY_VBAR;
            }
            else if (STRMATCH(attrib_vals[grpidx], "boxed"))
            {
                newgrp->borderstyle = BDY_HLINE | BDY_VBAR;
            }
        }

        ++grpidx;
    }

    return newgrp;
}

static ROWPAD *
set_padding_attribs (ROWPAD *pad,
                     const char **attrib_names, const char **attrib_vals)
{
    int idx = 0;

    while (attrib_names[idx])
    {
        if (STRMATCH(attrib_names[idx], "padleft"))
        {
            pad->left = atoi (attrib_vals[idx]);
        }
        else if (STRMATCH(attrib_names[idx], "padright"))
        {
            pad->right = atoi (attrib_vals[idx]);
        }
        else if (STRMATCH(attrib_names[idx], "padtop"))
        {
            pad->top = atoi (attrib_vals[idx]);
        }
        else if (STRMATCH(attrib_names[idx], "padbottom"))
        {
            pad->bottom = atoi (attrib_vals[idx]);
        }
        //TODO: else error???
    }

    return pad;
}

static void
start_element_main (GMarkupParseContext *context,
                    const gchar  *element_name,
                    const gchar **attrib_names,
                    const gchar **attrib_vals,
                    gpointer self,
                    GError **error)
{
    gpointer newgrp = NULL;
    GRPINF *cur_grp;
    GRPINF *parent = NULL;

    if (STYLE_PRINT_TABLE(self)->elList)
    {
        parent = g_slist_nth(STYLE_PRINT_TABLE(self)->elList, 0)->data;
    }

    if (STRMATCH(element_name, "cell"))
    {
        if (parent)
        {
            newgrp =
                append_cell_def (STYLE_PRINT_TABLE(self),
                        attrib_names, attrib_vals, (GRPINF *)parent);
        }
        else
        {
            // Error
        }
    }
    else if (STRMATCH(element_name, "defaultcell"))
    {
        if (!defaultcell)
        {
            defaultcell = calloc (1, sizeof (CELLINF));
        }

        add_cell_attribs (defaultcell, attrib_names, attrib_vals);
        newgrp = defaultcell;
    }
    else if (STRMATCH(element_name, "group"))
    {
        newgrp = allocate_new_group ( STYLE_PRINT_TABLE(self),
                attrib_names, attrib_vals, (GRPINF *)parent, GRPTY_GROUP);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
             STYLE_PRINT_TABLE(self)->grpHd = (GRPINF *)newgrp;
        }
    }
    else if (STRMATCH(element_name, "header"))
    {
        if (parent)
        {
            newgrp = allocate_new_group (STYLE_PRINT_TABLE(self),
                        attrib_names, attrib_vals, NULL, GRPTY_HEADER);
            ((GRPINF *)parent)->header = newgrp;
        }
    }
    else if (STRMATCH(element_name, "body"))
    {
        newgrp = allocate_new_group ( STYLE_PRINT_TABLE(self),
                attrib_names, attrib_vals, (GRPINF *)parent, GRPTY_BODY);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
             STYLE_PRINT_TABLE(self)->grpHd = (GRPINF *)newgrp;
        }
    }
    else if (STRMATCH(element_name, "font"))
    {
        int idx = 0;
        FONTINF *font_me;

        //if (!parent)
        //{error}
        cur_grp = parent;

        if (cur_grp->font)
        {
            font_me = cur_grp->font;
        }
        else
        {
            font_me = calloc (1, sizeof(FONTINF));
            cur_grp->font = font_me;
            font_me->grptype = GRPTY_FONT;
            // Set numeric values to -1 to flag not set
            font_me->size = -1;
            font_me->style = -1;
            font_me->weight = -1;
            font_me->variant = -1;
            font_me->stretch = -1;
        }

        while (attrib_names[idx])
        {
            const char *a = attrib_names[idx];

            if ( STRMATCH(a, "family"))
            {
                font_me->family = g_strdup (attrib_vals[idx]);
            }
            else if (STRMATCH(a, "size"))
            {
                font_me->size = strtof (attrib_vals[idx], NULL);
            }
            else if (STRMATCH(a, "style"))
            {
                font_me->style = name_to_pango_style (a);
            }
            else if (STRMATCH(a, "weight"))
            {
                font_me->weight = name_to_pango_weight (a);
            }
            else if (STRMATCH(a, "variant"))
            {
                font_me->variant = name_to_pango_variant (a);
            }
            else if (STRMATCH(a, "stretch"))
            {
                font_me->stretch = name_to_pango_stretch (a);
            }

            ++idx;
        }

        newgrp = cur_grp->font;
    }
    else if (STRMATCH(element_name, "pageheader"))
    {
        if (! STYLE_PRINT_TABLE(self)->PageHeader)
        {
             STYLE_PRINT_TABLE(self)->PageHeader =
                    allocate_new_group ( STYLE_PRINT_TABLE(self),
                            attrib_names, attrib_vals, NULL, GRPTY_PAGEHEADER);
        }

        newgrp =  STYLE_PRINT_TABLE(self)->PageHeader;
    }
    else if (STRMATCH(element_name, "padding"))
    {
        GRPINF *prnt = parent;    // Convenience
        // TODO: parent==0 ->> error??

        if (!prnt->padding)
        {
            prnt->padding = calloc (1, sizeof(ROWPAD));
            // Init all to -1 to flag "not set"
            prnt->padding->left = -1;
            prnt->padding->right = -1;
            prnt->padding->top = -1;
            prnt->padding->bottom = -1;
        }

        prnt->padding = set_padding_attribs (prnt->padding, attrib_names,
                                                            attrib_vals);
        newgrp = prnt->padding;
       
    }
    else if (STRMATCH(element_name, "defaultpadding"))
    {
        // TODO: if parent, error???

        if (! STYLE_PRINT_TABLE(self)->DefaultPadding)
        {
            STYLE_PRINT_TABLE(self)->DefaultPadding =
                            calloc (1, sizeof(ROWPAD));
        }

        STYLE_PRINT_TABLE(self)->DefaultPadding =
            set_padding_attribs (
                    STYLE_PRINT_TABLE(self)->DefaultPadding,
                                                attrib_names, attrib_vals);
        newgrp = STYLE_PRINT_TABLE(self)->DefaultPadding;
    }
    else //if (STRMATCH(element_name, "docheader"))
    {
        //newgrp = self->DocHeader;
    }

    if (newgrp)
    {
        STYLE_PRINT_TABLE(self)->elList =
            g_slist_prepend(STYLE_PRINT_TABLE(self)->elList, newgrp);
    }
    // Trying to elimintate this feature
    //g_markup_parse_context_push (context, &sub_prs, newgrp);
}

static void
end_element_main (GMarkupParseContext *context,
                    const gchar *element_name,
                    gpointer self,
                    GError **error)
{
//    if (this_grp)      // If anywhere but in top-level parse
//    {
    // Pop this item off the List
    if (STYLE_PRINT_TABLE(self)->elList)
    {
        STYLE_PRINT_TABLE(self)->elList =
            g_slist_delete_link(STYLE_PRINT_TABLE(self)->elList,
                    g_slist_nth(STYLE_PRINT_TABLE(self)->elList, 0));
    }
        //gpointer ptr = g_markup_parse_context_pop (context);
//    }
//    if (STRMATCH(element_name, "group") || STRMATCH(element_name, "body")
//            || STRMATCH(element_name, "defaultcell")
//            || STRMATCH(element_name, "pageheader"))

    // Set the PangoFontDescription
//    if (STRMATCH(element_name, "cell"))
//    {
//        PFONTINF fi;
//        CELLINF *cell = (CELLINF *)ptr;
//
//        if (!(cell->pangofont))
//        {
//            cell->pangofont =
//                pango_font_description_copy (defaultcell->pangofont);
//        }
//
//        if ((fi = cell->font))
//        {
//            if (fi->family)
//            {
//                pango_font_description_set_family (cell->pangofont,
//                        fi->family);
//            }
//
//            if (fi->size)
//            {
//                if (!cell->pangofont)
//                {
//                    printf ("Cannot set Font Size %d\n", fi->size);
//                }
//                pango_font_description_set_size (cell->pangofont,
//                        fi->size * PANGO_SCALE);
//            }
//
//            if (fi->style)
//            {
//                pango_font_description_set_style (cell->pangofont,
//                        fi->style);
//            }
//
//            if (fi->weight)
//            {
//                pango_font_description_set_weight (cell->pangofont,
//                        fi->weight);
//            }
//        }
//    }
}

// Set up default PrintSettings
static void
set_page_defaults (StylePrintTable *self)
{
    GtkPaperSize *pap_siz;

    if (! defaultcell)
    {
        defaultcell = calloc (1, sizeof(CELLINF));
        defaultcell->grptype = GRPTY_CELL;
    }

    if (!defaultcell->pangofont)
    {
        defaultcell->pangofont = pango_font_description_from_string (
                "Serif 10");
    }

    if (!defaultcell->font)
    {
        defaultcell->font = calloc (1, sizeof(FONTINF));
    }

    defaultcell->font->family =
        (char *)pango_font_description_get_family (defaultcell->pangofont);
    defaultcell->font->style =
        pango_font_description_get_style (defaultcell->pangofont);
    defaultcell->font->size =
        pango_font_description_get_size (defaultcell->pangofont);
    defaultcell->font->weight =
        pango_font_description_get_weight (defaultcell->pangofont);
    defaultcell->font->variant =
        pango_font_description_get_variant (defaultcell->pangofont);
    defaultcell->font->stretch =
        pango_font_description_get_stretch (defaultcell->pangofont);


//    if (!DefaultPangoFont)
//    {
//        DefaultPangoFont =
//            pango_font_description_copy (defaultcell->pangofont);
//    }


    if (!self->Page_Setup)
    {
        // We may need to get this by running gtk_print_run_page_setup_dialog()
        self->Page_Setup = gtk_page_setup_new();
        pap_siz = gtk_paper_size_new (GTK_PAPER_NAME_LETTER);
        gtk_page_setup_set_paper_size (self->Page_Setup, pap_siz);
//        gtk_paper_size_free (pap_siz);
    }
}

static void
free_default_cell()
{
    if (defaultcell->font) {
        free (defaultcell->font);
    }

    if (defaultcell->pangofont)
    {
        pango_font_description_free (defaultcell->pangofont);
    }

    free (defaultcell);
    defaultcell = NULL;
}

static void
reset_default_cell ()
{
    FONTINF *fi = defaultcell->font;
    PangoFontDescription *pfd = defaultcell->pangofont;

    if (defaultcell->font->family)
    {
        pango_font_description_set_family (pfd, fi->family);
    }

    if (pango_font_description_get_size (pfd) != fi->size)
    {
        pango_font_description_set_size (pfd, fi->size * PANGO_SCALE);
    }

    if (pango_font_description_get_style (pfd) != fi->style)
    {
        pango_font_description_set_style(pfd, fi->style);
    }

    if (pango_font_description_get_weight (pfd) != fi->weight)
    {
        pango_font_description_set_weight (pfd, fi->weight);
    }

    if (pango_font_description_get_variant (pfd) != fi->variant)
    {
        pango_font_description_set_variant (pfd, fi->variant);
    }

    if (pango_font_description_get_stretch (pfd) != fi->stretch)
    {
        pango_font_description_set_stretch (pfd, fi->stretch);
    }
}

static PGresult *
get_data (StylePrintTable *self, const char * qry)
{
    PGresult *rslt;

    if (self->qryParams == NULL)
    {
        rslt = PQexec (self->conn, qry);
    }
    else
    {
        rslt = PQexecParams(self->conn,
                            qry,
                            self->qryParams->len,
                            NULL,       // paramTypes not used
                            (const gchar **)self->qryParams->pdata,// Array of parameters
                            NULL,       // list of parameter lengths -ignore
                            NULL,
                            0);         // returned formats - make all text
    }

    if (PQresultStatus (rslt) != PGRES_TUPLES_OK)
    {
        // TODO: add functionality to show message in dialog box
        printf ("\nFailed to retrieve data!!\n");
        printf (PQresultErrorMessage(rslt));
        PQclear (rslt);
        rslt = NULL;
    }

    return rslt;
}

/* ******************************************************************** *
 * pq_data_from_colname() - Retrieve data from a column specified by    *
 *          its name rather than its number.                            *
 * ******************************************************************** */

//static char *
//pq_data_from_colname (PGresult *res, int row, char *colname)
//{
//    return PQgetvalue (res, row, PQfnumber(res, colname));
//}

/* ******************************************************************** *
 * set_col_values() - Set the cellwidth and left position for the cell  *
 *      This is called on the first encounter with the cell.  It is not *
 *      called in the initialization routine since the Page setup might *
 *      be changed after this cell is initialized.                      *
 * NOTE: We tried to use gtk_page_setup_get_page_width, (after defining *
 *      the PageSetup structure, but it gave a value of only a little   *
 *      more than 10% of what was needed to get the full page width.    *
 * ******************************************************************** */

static void
set_col_values (CELLINF *cell, StylePrintTable *self)
{
    // The following is for debugging purposes
/*    double ps = gtk_page_setup_get_page_width(self->Page_Setup, GTK_UNIT_POINTS);
    double pc = gtk_print_context_get_width(self->context);
    printf ("\nPage width returned by 'gtk_page_setup_get_page_width' = %f\n",ps);
    printf ("\nPage width returned by 'gtk_print_context_get_width' = %f\n\n",pc);*/
 //   cell->cellwidth =
 //       (cell->percent *  gtk_page_setup_get_page_width(self->Page_Setup,
 //               GTK_UNIT_POINTS)/100);
    cell->cellwidth =
        (cell->percent *  gtk_print_context_get_width(self->context))/100;
    cell->x = self->xpos;
    self->xpos += cell->cellwidth;
}

/* ******************************************************************** *
 * set_cell_font_desription() - Sets the font description for a cell    *
 *          Sets it to the default cell definition and then modifies    *
 *          any properties that are specified.                          *
 * ******************************************************************** */

static void
set_cell_font_description (StylePrintTable * self, CELLINF *cell)
{
    PFONTINF fi;

    if (! cell->pangofont)
    {
        cell->pangofont =
                pango_font_description_copy (defaultcell->pangofont);

        if (!defaultcell->pangofont)
        {
            report_error (self,
                    "Failed to create Pango Font Description for Cell");
        }
    }


    if ((fi = cell->font))
    {
        PangoFontDescription *pfd = cell->pangofont;

        if (fi->family)
        {
            pango_font_description_set_family (pfd, fi->family);
        }

        if (fi->size != -1)
        {
            pango_font_description_set_size (pfd,
                    fi->size * PANGO_SCALE);
        }

        if (fi->style != -1)
        {
            pango_font_description_set_style (pfd, fi->style);
        }

        if (fi->weight != -1)
        {
            pango_font_description_set_weight (pfd, fi->weight);
        }

        if (fi->variant != -1)
        {
            pango_font_description_set_variant (pfd, fi->variant);
        }

        if (fi->stretch != -1)
        {
            pango_font_description_set_stretch (pfd, fi->stretch);
        }
    }
}

/* ******************************************************************** *
 * render_cell() - g_ptr_array_foreach() callback to render a single    *
 *          cell.                                                       * 
 * Returns: Height of the rendered cell in points                       *
 * ******************************************************************** */

static int
render_cell (StylePrintTable *self, CELLINF *cell, int rownum,
        double rowtop)
{
    char *celltext = NULL;
    int CellHeight = 0;
    PangoRectangle log_rect;
    gboolean deletecelltext = FALSE;

    if (!cell->pangofont)
    {
        set_cell_font_description (self, cell);
    }

    switch (cell->txtsource)
    {
        case TSRC_STATIC:
            celltext = cell->celltext;
            break;
        case TSRC_DATA:
            celltext = PQgetvalue (self->pgresult, rownum, cell->cell_col);
            break;
        case TSRC_NOW:
            //TODO:
            break;
        case TSRC_PAGE:
            celltext = g_strdup_printf ("Page %d", self->pageno + 1);
            deletecelltext = TRUE;
            break;
        case TSRC_PAGEOF:
            celltext = g_strdup_printf ("Page %d of %d", self->pageno + 1,
                                                            self->TotPages);
            deletecelltext = TRUE;
            break;
        case TSRC_PRINTF:
            //TODO:
            break;
    }

    if (celltext && strlen (celltext))
    {
        PangoLayout *layout =
                    gtk_print_context_create_pango_layout (self->context);
        pango_layout_set_font_description (layout, cell->pangofont);
        pango_layout_set_width (layout,
                (cell->cellwidth - cell->padleft - cell->padright) *
                 PANGO_SCALE);
        pango_layout_set_alignment (layout, cell->layoutalign);
        pango_layout_set_text (layout, celltext, -1);
        pango_layout_get_extents (layout, NULL, &log_rect);
        CellHeight = log_rect.height;

        if (self->DoPrint)
        {
            cairo_move_to (self->cr, cell->x + cell->padleft, rowtop);
            pango_cairo_show_layout (self->cr, layout);
        }

        g_object_unref (layout);
    }
    else
    {
        fprintf (stderr, "---Empty data for column: %s\n", cell->celltext);
    }

    if (deletecelltext)
    {
        g_free (celltext);
    }

    return CellHeight/PANGO_SCALE;
}

/* ******************************************************************** *
 * hline () - Renders a horizontal line across the page                 *
 * ******************************************************************** */

double
hline (StylePrintTable *self, double ypos, double weight)
{
    if (self->DoPrint)
    {
        cairo_set_line_width (self->cr, weight);
        cairo_move_to (self->cr, 0, ypos);
        cairo_rel_line_to (self->cr,
                            gtk_print_context_get_width (self->context), 0);
        cairo_stroke (self->cr);
    }

    return 2;
}

/* **************************************************************** *
 * group_hline_top() - Render a single or double line at the top of *
 *          a group, header, etc.                                   *
 * Returns: The value to add to the current ypos.                   *
 * **************************************************************** */

//static double
//group_hline_top (StylePrintTable *self, double rowtop, int borderstyle)
//{
//    double linerow = rowtop;
//
//    if (borderstyle & (SINGLEBAR_HVY | DBLBAR))
//    {
//        linerow += hline (self, linerow, 1.0);
//    }
//    else if (borderstyle & SINGLEBAR)
//    {
//        linerow += hline (self, linerow, 0.5);
//    }
//
//    if (borderstyle & DBLBAR)
//    {
//        linerow += hline (self, linerow, 0.5);
//    }
//
//    return linerow - rowtop;
//}

/* **************************************************************** *
 * group_hline_bottom() - Render a single or double line below a    *
 *            group, header, etc.                                   *
 * **************************************************************** */

//static double
//group_hline_bottom (StylePrintTable *self, double begintop, int borderstyle)
//{
//    double rowtop = begintop;
//
//    if (borderstyle & (SINGLEBAR | DBLBAR))
//    {
//        rowtop += hline (self, rowtop, 0.5);
//    }
//    else if (borderstyle & SINGLEBAR_HVY)
//    {
//        rowtop += hline (self, rowtop, 1.0);
//    }
//
//    if (borderstyle & DBLBAR)
//    {
//        rowtop += hline (self, rowtop, 1.0);
//    }
//
//    return rowtop - begintop;
//}

/* ******************************************************************** *
 * render_row() - render a single row of data                           *
 * Returns the height of the line in points                             *
 * ******************************************************************** */

static int
render_row (StylePrintTable *self,
            GPtrArray *coldefs,
            ROWPAD *padding,
            int borderstyle,
            int rownum)
{
    int colnum;
    double rowtop = self->ypos;
    int MaxHeight = 0;

    if (padding)
    {
        if (padding->top == -1)
        {
            padding->top = self->DefaultPadding->top;
        }

        rowtop += padding->top;
    }

/*    if (rowtop >= self->pageheight)
    {
        return rowtop - self->ypos;
    }*/

    // Upper line for row
    for (colnum = 0; colnum < coldefs->len; colnum++)
    {
        int CellHeight;
        
        CellHeight =
            render_cell (self, (CELLINF *)(coldefs->pdata[colnum]), rownum,
                   rowtop);

        if (CellHeight > MaxHeight)
        {
            MaxHeight = CellHeight;
        }
    }

    // Do the VBars now while "rowtop" still points to the top of the row
    if (borderstyle & BDY_VBAR)
    {
        int idx;

        if (self->DoPrint)
        {
            for (idx = 1; idx < coldefs->len; idx++)
            {
                if (self->DoPrint)
                {
                    //int rmargin = gtk_print_context_get_width (self->context);

                    cairo_set_line_width (self->cr, 2.0);
                    cairo_move_to (self->cr,
                            ((CELLINF *)(coldefs->pdata[idx]))->x, rowtop);
                    cairo_rel_line_to (self->cr, 0, MaxHeight);
                    cairo_stroke (self->cr);
                }
            }
        }
    }

    rowtop += MaxHeight;

    if (padding)
    {
        if (padding->bottom == -1)
        {
            padding->bottom = self->DefaultPadding->bottom;
        }

        rowtop += padding->bottom;
    }

    return rowtop - self->ypos;
}

/* ******************************************************************** *
 * render_row_grp() - Render a series of rows from an array of cell     *
 *          defs.                                                       *
 * Passed:  (1) - self - the GlobalList of data                         *
 *          (3) - current group                                         *
 *          //(3) - self->formatting->body                                *
 *          //(4) - self->formatting                                      *
 *          (5) self->layout                                              *
 *          (6) cur_row - The first row to print                        *
 *          (7) end_row - The last row to print (+1)                    *
 *          //(8) TRUE                                                    *
 * Returns: The ending row                                              *
 * ******************************************************************** */

static int
render_row_grp (StylePrintTable *self,       // Global Data storage
        GPtrArray *col_defs,         // The coldefs array
        ROWPAD *padding,
        int borderstyle,
        // formatting->body,           // 
        // formatting
        PangoLayout *layout,
        int cur_row,                // The first row to print
        int end_row)                // The last row to print + 1
{
    int cur_idx = cur_row;
    double max_y = self->pageheight - self->textheight;

    // If no cell defs, return...
    if (!col_defs)
    {
        return cur_row;
    }

    // Render HLINE above first line, if applicable
    if (borderstyle & BDY_HLINE)
    {
        double line_ht = hline (self, self->ypos, 1.0);
        self->ypos += line_ht;
        max_y -= line_ht * 2;
    }

    for (cur_idx = cur_row; cur_idx < end_row; cur_idx++)
    {
        (self->ypos) += render_row (self, col_defs, padding,
                            borderstyle, cur_idx);

        // Render HLINE below each line, if applicable
        if (borderstyle & BDY_HLINE)
        {
            self->ypos += hline (self, self->ypos, 1.0);
        }

        if (self->ypos >= max_y)
        {
            ++cur_idx;      // Position to next data row for return
            return cur_idx;
        }
    }

    return cur_idx;
}

static void
render_header (StylePrintTable *self, GRPINF *curhdr)
{
    if (curhdr->pointsabove)
    {
        self->ypos += curhdr->pointsabove;
    }

    if (curhdr->celldefs)
    {
        if (!curhdr->cells_formatted)
        {
            //TODO: We may need to add in Left Margin
            self->xpos = 0;
            g_ptr_array_foreach (curhdr->celldefs,
                    (GFunc)set_col_values, self);
            curhdr->cells_formatted = TRUE;
        }

        render_row_grp (self, curhdr->celldefs,
                            curhdr->padding, curhdr->borderstyle, self->layout,
                            self->datarow, self->datarow + 1);
    }

    if (curhdr->pointsbelow)
    {
        self->ypos += curhdr->pointsbelow;
    }
}

/* ******************************************************************** *
 * render_body() : Render the data.                                     *
 * Passed:  (1) - self : The global data                                *
 *          (2) - The PangoContext for the printoperation               *
 *          (3) - The max row # to print in this category.  Note that   *
 *                the end of the page may be encountered before all the *
 *                data is printed.  In this case, this grouping is      *
 *                picked up on the next page.                           *
 * We don't need to worry about being at the page end..  the calling    *
 * function will check for this.                                        *
 * ******************************************************************** */

static void
render_body (StylePrintTable *self,
                GRPINF *bdy,
                int maxrow)
{
    if (bdy->celldefs)
    {
        if (!bdy->cells_formatted)
        {
            //TODO: We may need to add in Left Margin
            self->xpos = 0;
            g_ptr_array_foreach (bdy->celldefs, (GFunc)set_col_values,
                                self);
            bdy->cells_formatted = TRUE;
        }
    }

    self->datarow = render_row_grp (self, bdy->celldefs,
                bdy->padding, bdy->borderstyle,
                //self->formatting->body,
                //self->formatting,
                self->layout,
                self->datarow, maxrow
                );
}

/* ******************************************************************** *
 * render_group() : Render a group of data.  Parses the dataset and     *
 *      determines the max value for the row that contains this group   *
 *      value.  This grouping is further passed to the next level.  If  *
 *      it's a "group", it breaks its data down and passes it on to the *
 *      next level until a <body> is encountered.  The <body> does the  *
 *      actual printing of data.                                        *
 * ******************************************************************** */

static int
render_group(StylePrintTable *self,
                GRPINF *curgrp,
                int maxrow)
{
    int grp_idx = self->datarow;
    //double y_pos = self->ypos;
    //int grp_y = self->ypos,
    int    grp_top;
    //GRPINF *cg = curgrp;
    //PangoLayout *layout = set_layout(self, render_params, self->layout);

    grp_top = self->ypos;

    // Try to avoid leaving hanging group headers at the bottom of the
    // page with no body data...
    // TODO: We need to make this more correct.. This is just a hack

    /* NOTE:: Since we are now doing a dry run during the "begin-print"
     * portion of the printing, this may not be necessary
     */

/*    while (cg->grpchild->grptype == GRPTY_GROUP)
    {
        //grp_y += cg->pointsabove + self->textheight + cg->pointsbelow;
        grp_y += self->textheight;
        cg = cg->grpchild;
    }

    if (grp_y + (self->textheight * 2) >= self->pageheight)
    {
        return self->datarow;
    }*/

    // This loop parses the entire range passed to the group.
    while ((grp_idx < maxrow) && (self->ypos < self->pageheight))
    {
        char *grptxt = PQgetvalue(self->pgresult, grp_idx, curgrp->grpcol);

        /* Break the main group down into subgroups (or the body)
           Do this by comparing the string in the column defining the group.
           The variable "grp_idx" is the max value (+ 1) for the rows that
           will be members of the subgroup (or body set)
        */

        do {
            ++grp_idx;
        } while ((grp_idx < maxrow) && STRMATCH(grptxt,
                    PQgetvalue (self->pgresult, grp_idx, curgrp->grpcol)));

        // Print Group Header, if applicable...

        if (curgrp->pointsabove)
        {
            self->ypos += curgrp->pointsabove;
        }

        if (curgrp->header)
        {
            render_header (self, curgrp->header);
        }       // if (curgrp->header)

        //if(render_params->pts_above){self->ypos += render_params->pts_above;}
        // Possibly add group-header...

        // Now render the current range of data.  It will be
        // either another subgroup or the body of data.

        if (curgrp->grpchild)
        {
            if (curgrp->grpchild->grptype == GRPTY_GROUP)
            {
                render_group (self, curgrp->grpchild, grp_idx);

                // TODO: We're not even trying to access the group's cell
                // defs ATM.  There probably should not be any cell defs for
                // a group, but rather cell defs for a header and a footer.
            }
            else if (curgrp->grpchild->grptype == GRPTY_BODY)
            {
                render_body (self, curgrp->grpchild, grp_idx);
            }
        }
        else
        {
            // Would this be an error????
        }

        // Render footers here???
        if (self->DoPrint)
        {
            if (curgrp->borderstyle & (SINGLEBOX | DBLBOX))
            {
                cairo_set_line_width (self->cr, 4.0);
                cairo_rectangle (self->cr, 0, grp_top,
                                   gtk_print_context_get_width (self->context),
                                   self->ypos - grp_top); 
                cairo_stroke (self->cr);
            }

            if (curgrp->borderstyle & DBLBOX)
            {
                cairo_set_line_width (self->cr, 2.0);
                cairo_rectangle (self->cr, 16, grp_top + 16,
                            gtk_print_context_get_width (self->context) - 32,
                            self->ypos - grp_top - 32); 
                cairo_stroke (self->cr);
            }
        }

        if (curgrp->pointsbelow)
        {
            self->ypos += curgrp->pointsbelow;
        }
        if (self->ypos >= self->pageheight)
        {
            break;
        }
    }

    return grp_idx;
}

static void
render_page(StylePrintTable *self)
{
    GRPINF *curgrp;
    int lastrow;

    self->ypos = 0;

    if (self->DoPrint)
    {
        lastrow = (int)g_ptr_array_index (self->PageEndRow, self->pageno);
    }
    else
    {
        lastrow = PQntuples (self->pgresult);
    }

    if (self->PageHeader)
    {
        if (self->PageHeader->celldefs)
        {
            if (!self->PageHeader->cells_formatted)
            {
                //TODO: We may need to add in Left Margin
                self->xpos = 0;
                g_ptr_array_foreach(self->PageHeader->celldefs,
                        (GFunc)set_col_values, self);
                self->PageHeader->cells_formatted = TRUE;
            }

            render_row_grp (self, self->PageHeader->celldefs, NULL,
                            0, self->layout, self->datarow, self->datarow + 1);
        }
    }
    else if (!self->pageno)       // If first page, print Docheader if present
    {
        if (self->DocHeader)
        {
        }
    }

    curgrp = self->grpHd;

    // Now we're ready to render the data...
    if (curgrp->grptype == GRPTY_GROUP)
    {
        render_group (self, curgrp, lastrow);
    }
    // TODO: Need to check for some other type than GRPTY_BODY???
    // Also, this is wrong, as we don't have a "render_group" function
    else
    {
        render_body (self, curgrp, lastrow);
    }
}

static void
style_print_table_draw_page (GtkPrintOperation *operation,
                                GtkPrintContext *context, int page_nr)
{
    STYLE_PRINT_TABLE(operation)->pageno = page_nr;
    STYLE_PRINT_TABLE(operation)->context = context;
    STYLE_PRINT_TABLE(operation)->pageheight =
                    gtk_print_context_get_height (STYLE_PRINT_TABLE(operation)->context);
//    gtk_print_STYLE_PRINT_TABLE(operation)_set_unit (operation, GTK_UNIT_POINTS);
    STYLE_PRINT_TABLE(operation)->cr =
                    gtk_print_context_get_cairo_context (context);
    STYLE_PRINT_TABLE(operation)->layout =
                    gtk_print_context_create_pango_layout (context);
    render_page (STYLE_PRINT_TABLE(operation));
    g_object_unref (STYLE_PRINT_TABLE(operation)->layout);
}

/* ************************************************************************ *
 * begin_print() - Called after print settings have been set up.            *
 *      Here we set up the default font, determine the height of a line.    *
 *      We paginate the output by doing a "dry run" through the data        *
 * ************************************************************************ */

static void
style_print_table_begin_print (GtkPrintOperation *self,
        GtkPrintContext *context)
{
    PangoLayout *lo;
    PangoRectangle log_rect;

    //int firstrow = 0;   // First datarow for the page;
    STYLE_PRINT_TABLE(self)->datarow = 0;
    STYLE_PRINT_TABLE(self)->pageno = 0;
    STYLE_PRINT_TABLE(self)->context = context;
    STYLE_PRINT_TABLE(self)->pageheight =
                gtk_print_context_get_height (STYLE_PRINT_TABLE(self)->context);
    STYLE_PRINT_TABLE(self)->TotPages = 0;
    STYLE_PRINT_TABLE(self)->DoPrint = FALSE;
//    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
    STYLE_PRINT_TABLE(self)->layout = gtk_print_context_create_pango_layout (context);

    lo = pango_layout_copy (STYLE_PRINT_TABLE(self)->layout);
    pango_layout_set_font_description (lo, defaultcell->pangofont);
    pango_layout_set_width (lo, gtk_print_context_get_width (STYLE_PRINT_TABLE(self)->context));
    pango_layout_set_text (lo, "Ty", -1);
    pango_layout_get_extents (lo, NULL, &log_rect);
    STYLE_PRINT_TABLE(self)->textheight = log_rect.height/PANGO_SCALE;
    g_object_unref (lo);

    STYLE_PRINT_TABLE(self)->PageEndRow = g_ptr_array_new();

    while ((STYLE_PRINT_TABLE(self)->datarow) <
            PQntuples (STYLE_PRINT_TABLE(self)->pgresult))
    {
        STYLE_PRINT_TABLE(self)->ypos = 0;
        render_page (STYLE_PRINT_TABLE(self));
        g_ptr_array_add (STYLE_PRINT_TABLE(self)->PageEndRow,
                (gpointer)(STYLE_PRINT_TABLE(self)->datarow));
        ++(STYLE_PRINT_TABLE(self)->TotPages);
    }

    gtk_print_operation_set_n_pages (self,
            STYLE_PRINT_TABLE(STYLE_PRINT_TABLE(self))->TotPages ? STYLE_PRINT_TABLE(self)->TotPages : 1);
    // Re-initialize Global variables
    g_object_unref (STYLE_PRINT_TABLE(self)->layout);
    STYLE_PRINT_TABLE(self)->DoPrint = TRUE;
    STYLE_PRINT_TABLE(self)->datarow = 0;
    STYLE_PRINT_TABLE(self)->pageno = 0;
}

/*
 * render_report (StylePrintTable *self)
 * @self: The #StylePrintTable
 *
 * Renders the report, I.E, does the actual printout
 *
 */

static void
render_report (StylePrintTable *self)
{
    GError *g_err;

    gtk_print_operation_run (GTK_PRINT_OPERATION(self),
            GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, self->w_main, &g_err);
    
    // Now free up everything that has been allocated...
    g_ptr_array_free (STYLE_PRINT_TABLE(self)->PageEndRow, TRUE);
}

/**
 * style_print_table_connect:
 * @self: The #StylePrintTable *
 * @dbn: The connection string set up in the way it is sent to the database
 *(see PostgreSQL documentation for details on format)
 *
 * Returns: 1 on success, 0 on failure.
 *
 * Establishes a connection to the database.  This must be done before any
 * queries or commands are sent to the database.
 *
 * Free the string upon return if need be.
 *
 */

gint
style_print_table_connect (StylePrintTable *self, gchar *dbn)
{

    self->conn = PQconnectdb(dbn);

    if (PQstatus(self->conn) != CONNECTION_OK)
    {
        report_error (self, "Failed to connect to db\n");
        self->conn = NULL;
        return 0;
    }

    return 1;
}

/**
 * style_print_table_appendParam:
 * @self: The #StylePrintTable *
 * @param: The parameter value to add.
 *
 * Appends a parameter onto the list of parameters for the upcoming query.
 * The query refers to these parameters with the nomenclature of '$1', '$2',
 * etc.
 *
 * The order is important!  The parameters must appear in the list in the
 * order in which they are referred to in the query.
 *
 * If the values need to be freed, do this after the query is executed.
 */

void
style_print_table_appendParam (StylePrintTable *self, const gchar *param)
{
    if ( ! self->qryParams)
    {
        self->qryParams = g_ptr_array_new();
    }

    g_ptr_array_add (self->qryParams, (gpointer)param);
}

/**
 * style_print_table_from_xmlfile:
 * @tblprnt: The StylePrintTable
 * @win: (nullable): The parent window - NULL if none
 * @qry: query to send which will retrieve the data to print
 * @filename: The filename to open and read to get the xml definition for the printout.
 *
 * Print a tabular form where the xml definition for the output is
 * contained in a file.
 */

void
style_print_table_from_xmlfile (StylePrintTable *self,
                                        GtkWindow *wmain,
                                        gchar *qry,
                                        //GPtrArray *parms,
                                        char *fname)
{
    char buf[3000];
    FILE *fp;
    int rdcount;
    GMarkupParseContext *gmp_contxt;
    GError *error;
    PGresult *res;

    if ( ! self->conn)
    {
        report_error (self,
                "No connection!  Please connect to database first");
    }

    if (wmain)
    {
        self->w_main = wmain;
    }

    res = get_data (self, qry);

    if (res == NULL)
    {
        return;
    }

    self->pgresult = res;

    if (!self->Page_Setup)
    {
        set_page_defaults (self);
    }

    if ( ! (fp = fopen (fname, "rb")))
    {
        GString *errmsg;
        errmsg = g_string_new (NULL);
        g_string_printf (errmsg, "Failed to open file: '%s'\n", fname);
        report_error (self, errmsg->str);
        g_string_free (errmsg, TRUE);
        return;
    }

    gmp_contxt =
        g_markup_parse_context_new (&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                                        self, NULL);
    
    while ((rdcount = fread (buf, 1, sizeof(buf), fp)))
    {
        error = NULL;

        if (!g_markup_parse_context_parse (gmp_contxt, buf, rdcount, &error))
        {
            report_error (self, error->message);
        }
    }

    fclose (fp);
    reset_default_cell ();

    if ( ! error)
    {
        render_report (self);
    }

    PQclear (self->pgresult);
    //free_default_cell();
    //return STYLE_PRINT_TABLE(tbl)->grpHd; // Temporary - for debugging
}

/**
 * style_print_table_from_xmlstring:
 * @tp: The #StylePrintTable
 * @w: (nullable): The parent window - NULL if none
 * @qry: The query to send to retrieve the data to print
 * @c: Pointer to the string containing the xml formatting
 *
 * Print a table where the definition for the format is contained in an
 * xml string.
 *
 */

void
style_print_table_from_xmlstring (  StylePrintTable *self,
                                            GtkWindow *wmain,
                                            gchar *qry,
                                            char *xml)
{
    GMarkupParseContext *gmp_contxt;
    GError *error;
    PGresult *res;

    if ( ! self->conn)
    {
        report_error (self,
                "No connection!  Please connect to database first");
    }

    if (wmain)
    {
        self->w_main = wmain;
    }

    res = get_data (self, qry);

    if (res == NULL)
    {
        return;
    }

    self->pgresult = res;

    if (!self->Page_Setup)
    {
        set_page_defaults (self);
    }

    gmp_contxt =
        g_markup_parse_context_new (&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                self, NULL);
    g_markup_parse_context_parse (gmp_contxt, xml, strlen(xml), &error);
    reset_default_cell ();
    render_report (self);
    PQclear (self->pgresult);
    free_default_cell();
    //return tbl->grpHd;   // For debugging - see what is produced in the GlobalData.grpHd struct
}

/**
 * style_print_table_new:
 *
 * Creates a new #StylePrintTable.
 *
 * Returns: A new #StylePrintTable
 *
 */

StylePrintTable *
style_print_table_new (void)
{
    return g_object_new (STYLE_PRINT_TYPE_TABLE,NULL); 
}

/**
 * style_print_table_do:
 * @self: The #StylePrintTable *
 * @qry: The query to execute
 * 
 * Execute a non-data-returning query on the database
 * If parameters are used, they must be previously set
 */

void
style_print_table_do (StylePrintTable *self, const gchar * qry)
{
    PGresult *rslt;

    if ( ! self->conn)
    {
        report_error (self,  "No connection has been established");
        return;
    }

    if (self->qryParams == NULL)
    {
        rslt = PQexec (self->conn, qry);
    }
    else
    {
        rslt = PQexecParams(self->conn,
                    qry,
                    self->qryParams->len,
                    NULL,       // paramTypes not used
                    (const gchar **)self->qryParams->pdata,// Array of params
                    NULL,       // list of parameter lengths -ignore
                    NULL,
                    0);         // returned formats - make all text
    }

    if (PQresultStatus (rslt) != PGRES_COMMAND_OK)  
    {
        report_error (self, PQresultErrorMessage(rslt));
    }

    if (self->qryParams)
    {
        g_ptr_array_free (self->qryParams, FALSE);
        self->qryParams = NULL;
    }

    PQclear (rslt);     // Simply discard this result
}
