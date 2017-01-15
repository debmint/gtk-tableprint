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

typedef struct _StylePrintTablePrivate StylePrintTablePrivate;

struct _StylePrintTablePrivate
{
    GtkWindow *w_main;

    // Connection Variables
    GPtrArray *pgresult;     // Pointer to the PostGreSQL PGresult

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
    CELLINF *defaultcell;
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
static void start_element_main (GMarkupParseContext *, const gchar *,
        const gchar **, const gchar **, gpointer, GError **);
static void end_element_main (GMarkupParseContext *, const gchar *,
                             gpointer, GError **);
static void prs_err (GMarkupParseContext *, GError *, gpointer);
static void reset_default_cell (StylePrintTable *);
static void free_default_cell (StylePrintTable *);
static void set_page_defaults (StylePrintTable *);

static void report_error (StylePrintTable *, gchar *);
static void render_page (StylePrintTable *);
static void style_print_table_begin_print (GtkPrintOperation *,
                                           GtkPrintContext *);

static void style_print_table_draw_page (GtkPrintOperation *,
                                         GtkPrintContext *,
                                         int);

static void style_print_table_draw_page (GtkPrintOperation *,
                                         GtkPrintContext *,
                                         int);

#define CELLPAD_DFLT 10

G_DEFINE_TYPE_WITH_PRIVATE(StylePrintTable, style_print_table,
                            GTK_TYPE_PRINT_OPERATION)

char *GroupElements[] = {"pageheader", "group", "body", NULL};
gboolean Formatted = FALSE;
//PangoFontDescription *DefaultPangoFont;
PGRPINF FmtList;    // The formatting tree;

GMarkupParser prsr = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};

//GMarkupParser sub_prs = {start_element_main, end_element_main, NULL,
//                        NULL, prs_err};

void
htlist(gchar *key,gchar *val, gpointer usrdat)
{
    fprintf(stderr,"%s=>%s  ",key,val);
}

void
arylist (GHashTable *ht, gpointer udat)
{
    gchar *val;
    val = (gchar *)g_hash_table_lookup(ht,"who");
    if (val)
    {
        fprintf(stderr, val);
    }
    else
    {
        fprintf(stderr, "---");
    }
    fprintf(stderr,"\n");
}

/**
 * style_print_table_greet:
 * @self: The StylePrintTable
 * @ary: (element-type GHashTable): The array to parse
 *
 * This is only a temporary function to do tests, and will be deleted
 * later on.
 */

void
style_print_table_greet (StylePrintTable *self,GPtrArray *ary)
{
        g_ptr_array_foreach(ary,(GFunc)arylist, NULL);
}

static void
style_print_table_init (StylePrintTable *op)
{
    StylePrintTablePrivate *priv = style_print_table_get_instance_private (op);
    // initialisation goes here

    /* Should these be set up elsewhere???? */
    priv->Page_Setup = gtk_page_setup_new();
    gtk_print_operation_set_default_page_setup (GTK_PRINT_OPERATION(op),
            priv->Page_Setup);
    set_page_defaults (op);
    priv->w_main = NULL;
    priv->pgresult = NULL;
    //priv->qryParams = NULL;
}

static void
style_print_table_class_init (StylePrintTableClass *class)
{
    //GObjectClass *gobject_class = (GObjectClass *) class;
    GtkPrintOperationClass *print_class = (GtkPrintOperationClass *) class;

    // virtual function overrides go here
    print_class->begin_print = style_print_table_begin_print;
    print_class->draw_page = style_print_table_draw_page;
    //gobject_class->finalize = style_print_table_finalize;
    //print_class->end_print = style_print_table_end_print;

    // property and signal definitions go here
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

    if (priv->w_main)
    {
        GtkWidget *dlg;

        dlg = gtk_message_dialog_new (priv->w_main,
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

//    if (mycell->txtsource == TSRC_DATA)
//    {
//        if (mycell->celltext && strlen(mycell->celltext))
//        {
//            mycell->cell_col = PQfnumber (self->pgresult, mycell->celltext);
//
//        }
//    }

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
    StylePrintTablePrivate *priv;
    int grpidx = 0;
    GRPINF *newgrp = calloc (1, sizeof(GRPINF));
   
    priv = style_print_table_get_instance_private (self);

    if ((grptype == GRPTY_GROUP) || (grptype == GRPTY_BODY))
    {
        if (!parent && !priv->grpHd)
        {
            priv->grpHd = (GRPINF *)newgrp;
            newgrp = priv->grpHd;
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
        memcpy (newgrp->font, priv->defaultcell->font, sizeof(FONTINF));
    }

    if (parent && parent->pangofont)
    {
        newgrp->pangofont = pango_font_description_copy (parent->pangofont);
    }
    else
    {
        newgrp->pangofont =
                    pango_font_description_copy (priv->defaultcell->pangofont);
    }

    while (attrib_names[grpidx])
    {
        if (STRMATCH(attrib_names[grpidx], "groupsource"))
        {
            newgrp->grpcol = g_strdup (attrib_vals[grpidx]);

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
                       gpointer   myself,
                         GError **error)
{
    StylePrintTablePrivate *priv;
    gpointer newgrp = NULL;
    GRPINF *cur_grp;
    GRPINF *parent = NULL;
    StylePrintTable *self = STYLE_PRINT_TABLE(myself);
   
    priv = style_print_table_get_instance_private (self);


    if (priv->elList)
    {
        parent = g_slist_nth(priv->elList, 0)->data;
    }

    if (STRMATCH(element_name, "cell"))
    {
        if (parent)
        {
            newgrp =
                append_cell_def (self,
                        attrib_names, attrib_vals, (GRPINF *)parent);
        }
        else
        {
            // Error
        }
    }
    else if (STRMATCH(element_name, "defaultcell"))
    {
        if (!(priv->defaultcell))
        {
            priv->defaultcell = calloc (1, sizeof (CELLINF));
        }

        add_cell_attribs (priv->defaultcell, attrib_names, attrib_vals);
        newgrp = priv->defaultcell;
    }
    else if (STRMATCH(element_name, "group"))
    {
        newgrp = allocate_new_group ( self,
                attrib_names, attrib_vals, (GRPINF *)parent, GRPTY_GROUP);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
             priv->grpHd = (GRPINF *)newgrp;
        }
    }
    else if (STRMATCH(element_name, "header"))
    {
        if (parent)
        {
            newgrp = allocate_new_group (self,
                        attrib_names, attrib_vals, NULL, GRPTY_HEADER);
            ((GRPINF *)parent)->header = newgrp;
        }
    }
    else if (STRMATCH(element_name, "body"))
    {
        newgrp = allocate_new_group (self,
                attrib_names, attrib_vals, (GRPINF *)parent, GRPTY_BODY);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
             priv->grpHd = (GRPINF *)newgrp;
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
        if (! priv->PageHeader)
        {
             priv->PageHeader =
                    allocate_new_group ( self, attrib_names, attrib_vals,
                            NULL, GRPTY_PAGEHEADER);
        }

        newgrp =  priv->PageHeader;
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

        if (! priv->DefaultPadding)
        {
            priv->DefaultPadding = calloc (1, sizeof(ROWPAD));
        }

        priv->DefaultPadding = set_padding_attribs (
                    priv->DefaultPadding, attrib_names, attrib_vals);
        newgrp = priv->DefaultPadding;
    }
    else //if (STRMATCH(element_name, "docheader"))
    {
        //newgrp = self->DocHeader;
    }

    if (newgrp)
    {
        priv->elList = g_slist_prepend(priv->elList, newgrp);
    }
    // Trying to elimintate this feature
    //g_markup_parse_context_push (context, &sub_prs, newgrp);
}

static void
end_element_main (GMarkupParseContext  *context,
                          const gchar  *element_name,
                             gpointer   tbl,
                              GError  **error)
{
    StylePrintTable *self = STYLE_PRINT_TABLE(tbl);
    StylePrintTablePrivate *priv;
    priv = style_print_table_get_instance_private (self);
    
//    if (this_grp)      // If anywhere but in top-level parse
//    {
    // Pop this item off the List
    if (priv->elList)
    {
        priv->elList = g_slist_delete_link (priv->elList,
                                            g_slist_nth(priv->elList, 0));
    }
        //gpointer ptr = g_markup_parse_context_pop (context);
//    }
//    if (STRMATCH(element_name, "group") || STRMATCH(element_name, "body")
//            || STRMATCH(element_name, "priv->defaultcell")
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
//                pango_font_description_copy (priv->defaultcell->pangofont);
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
    StylePrintTablePrivate *priv = 
                style_print_table_get_instance_private (self);

    if (! priv->defaultcell)
    {
        priv->defaultcell = calloc (1, sizeof(CELLINF));
        priv->defaultcell->grptype = GRPTY_CELL;
    }

    if (!priv->defaultcell->pangofont)
    {
        priv->defaultcell->pangofont = pango_font_description_from_string (
                "Serif 10");
    }

    if (!priv->defaultcell->font)
    {
        priv->defaultcell->font = calloc (1, sizeof(FONTINF));
    }

    priv->defaultcell->font->family =
        (char *)pango_font_description_get_family (priv->defaultcell->pangofont);
    priv->defaultcell->font->style =
        pango_font_description_get_style (priv->defaultcell->pangofont);
    priv->defaultcell->font->size =
        pango_font_description_get_size (priv->defaultcell->pangofont);
    priv->defaultcell->font->weight =
        pango_font_description_get_weight (priv->defaultcell->pangofont);
    priv->defaultcell->font->variant =
        pango_font_description_get_variant (priv->defaultcell->pangofont);
    priv->defaultcell->font->stretch =
        pango_font_description_get_stretch (priv->defaultcell->pangofont);


//    if (!DefaultPangoFont)
//    {
//        DefaultPangoFont =
//            pango_font_description_copy (priv->defaultcell->pangofont);
//    }


    if (!priv->Page_Setup)
    {
        // We may need to get this by running gtk_print_run_page_setup_dialog()
        priv->Page_Setup = gtk_page_setup_new();
        pap_siz = gtk_paper_size_new (GTK_PAPER_NAME_LETTER);
        gtk_page_setup_set_paper_size (priv->Page_Setup, pap_siz);
//        gtk_paper_size_free (pap_siz);
    }
}

static void
free_default_cell (StylePrintTable *self)
{
    StylePrintTablePrivate *priv = 
                style_print_table_get_instance_private (self);

    if (priv->defaultcell->font) {
        free (priv->defaultcell->font);
    }

    if (priv->defaultcell->pangofont)
    {
        pango_font_description_free (priv->defaultcell->pangofont);
    }

    free (priv->defaultcell);
    priv->defaultcell = NULL;
}

static void
reset_default_cell (StylePrintTable *self)
{
    StylePrintTablePrivate *priv = 
                style_print_table_get_instance_private (self);
    PangoFontDescription *pfd = priv->defaultcell->pangofont;
    FONTINF *fi = priv->defaultcell->font;

    if (priv->defaultcell->font->family)
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

    // The following is for debugging purposes
/*    double ps = gtk_page_setup_get_page_width(priv->Page_Setup, GTK_UNIT_POINTS);
    double pc = gtk_print_context_get_width(priv->context);
    printf ("\nPage width returned by 'gtk_page_setup_get_page_width' = %f\n",ps);
    printf ("\nPage width returned by 'gtk_print_context_get_width' = %f\n\n",pc);*/
 //   cell->cellwidth =
 //       (cell->percent *  gtk_page_setup_get_page_width(priv->Page_Setup,
 //               GTK_UNIT_POINTS)/100);
    cell->cellwidth =
        (cell->percent *  gtk_print_context_get_width(priv->context))/100;
    cell->x = priv->xpos;
    priv->xpos += cell->cellwidth;
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);


    if (! cell->pangofont)
    {
        cell->pangofont =
                pango_font_description_copy (priv->defaultcell->pangofont);

        if (! (priv->defaultcell->pangofont))
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);


    if (!cell->pangofont)
    {
        set_cell_font_description (self, cell);
    }

    switch (cell->txtsource)
    {
        //GHashTable *cur_row;

        case TSRC_STATIC:
            celltext = cell->celltext;
            break;
        case TSRC_DATA:
            celltext = g_hash_table_lookup(g_ptr_array_index (priv->pgresult,
                                                            rownum),
                                                            cell->celltext);
            break;
        case TSRC_NOW:
            //TODO:
            break;
        case TSRC_PAGE:
            celltext = g_strdup_printf ("Page %d", priv->pageno + 1);
            deletecelltext = TRUE;
            break;
        case TSRC_PAGEOF:
            celltext = g_strdup_printf ("Page %d of %d", priv->pageno + 1,
                                                            priv->TotPages);
            deletecelltext = TRUE;
            break;
        case TSRC_PRINTF:
            //TODO:
            break;
    }

    if (celltext && strlen (celltext))
    {
        PangoLayout *layout =
                    gtk_print_context_create_pango_layout (priv->context);
        pango_layout_set_font_description (layout, cell->pangofont);
        pango_layout_set_width (layout,
                (cell->cellwidth - cell->padleft - cell->padright) *
                 PANGO_SCALE);
        pango_layout_set_alignment (layout, cell->layoutalign);
        pango_layout_set_text (layout, celltext, -1);
        pango_layout_get_extents (layout, NULL, &log_rect);
        CellHeight = log_rect.height;

        if (priv->DoPrint)
        {
            cairo_move_to (priv->cr, cell->x + cell->padleft, rowtop);
            pango_cairo_show_layout (priv->cr, layout);
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

    if (priv->DoPrint)
    {
        cairo_set_line_width (priv->cr, weight);
        cairo_move_to (priv->cr, 0, ypos);
        cairo_rel_line_to (priv->cr,
                            gtk_print_context_get_width (priv->context), 0);
        cairo_stroke (priv->cr);
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
    StylePrintTablePrivate *priv =
                    style_print_table_get_instance_private (self);
    int colnum;
    int MaxHeight = 0;
    double rowtop = priv->ypos;

    if (padding)
    {
        if (padding->top == -1)
        {
            padding->top = priv->DefaultPadding->top;
        }

        rowtop += padding->top;
    }

/*    if (rowtop >= priv->pageheight)
    {
        return rowtop - priv->ypos;
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

        if (priv->DoPrint)
        {
            for (idx = 1; idx < coldefs->len; idx++)
            {
                if (priv->DoPrint)
                {
                    //int rmargin = gtk_print_context_get_width (priv->context);

                    cairo_set_line_width (priv->cr, 2.0);
                    cairo_move_to (priv->cr,
                            ((CELLINF *)(coldefs->pdata[idx]))->x, rowtop);
                    cairo_rel_line_to (priv->cr, 0, MaxHeight);
                    cairo_stroke (priv->cr);
                }
            }
        }
    }

    rowtop += MaxHeight;

    if (padding)
    {
        if (padding->bottom == -1)
        {
            padding->bottom = priv->DefaultPadding->bottom;
        }

        rowtop += padding->bottom;
    }

    return rowtop - priv->ypos;
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
    StylePrintTablePrivate *priv;
    priv = style_print_table_get_instance_private (self);
    int cur_idx = cur_row;
    double max_y = priv->pageheight - priv->textheight;
   


    // If no cell defs, return...
    if (!col_defs)
    {
        return cur_row;
    }

    // Render HLINE above first line, if applicable
    if (borderstyle & BDY_HLINE)
    {
        double line_ht = hline (self, priv->ypos, 1.0);
        priv->ypos += line_ht;
        max_y -= line_ht * 2;
    }

    for (cur_idx = cur_row; cur_idx < end_row; cur_idx++)
    {
        (priv->ypos) += render_row (self, col_defs, padding,
                            borderstyle, cur_idx);

        // Render HLINE below each line, if applicable
        if (borderstyle & BDY_HLINE)
        {
            priv->ypos += hline (self, priv->ypos, 1.0);
        }

        if (priv->ypos >= max_y)
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

    if (curhdr->pointsabove)
    {
        priv->ypos += curhdr->pointsabove;
    }

    if (curhdr->celldefs)
    {
        if (!curhdr->cells_formatted)
        {
            //TODO: We may need to add in Left Margin
            priv->xpos = 0;
            g_ptr_array_foreach (curhdr->celldefs,
                    (GFunc)set_col_values, self);
            curhdr->cells_formatted = TRUE;
        }

        render_row_grp (self, curhdr->celldefs,
                            curhdr->padding, curhdr->borderstyle, priv->layout,
                            priv->datarow, priv->datarow + 1);
    }

    if (curhdr->pointsbelow)
    {
        priv->ypos += curhdr->pointsbelow;
    }
}

/* ******************************************************************** *
 * render_body() : Render the data.                                     *
 * Passed:  (1) - self : The Object instance of StyleTablePrint         *
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

    if (bdy->celldefs)
    {
        if (!bdy->cells_formatted)
        {
            //TODO: We may need to add in Left Margin
            priv->xpos = 0;
            g_ptr_array_foreach (bdy->celldefs, (GFunc)set_col_values,
                                self);
            bdy->cells_formatted = TRUE;
        }
    }

    priv->datarow = render_row_grp (self, bdy->celldefs,
                bdy->padding, bdy->borderstyle,
                //priv->formatting->body,
                //priv->formatting,
                priv->layout,
                priv->datarow, maxrow
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
    //double y_pos = self->ypos;
    //int grp_y = self->ypos,
    int    grp_top;
    //GRPINF *cg = curgrp;
    //PangoLayout *layout = set_layout(self, render_params, self->layout);
    StylePrintTablePrivate *priv;
    priv = style_print_table_get_instance_private (self);
    int grp_idx = priv->datarow;


    grp_top = priv->ypos;

    // Try to avoid leaving hanging group headers at the bottom of the
    // page with no body data...
    // TODO: We need to make this more correct.. This is just a hack

    /* NOTE:: Since we are now doing a dry run during the "begin-print"
     * portion of the printing, this may not be necessary
     */

/*    while (cg->grpchild->grptype == GRPTY_GROUP)
    {
        //grp_y += cg->pointsabove + priv->textheight + cg->pointsbelow;
        grp_y += priv->textheight;
        cg = cg->grpchild;
    }

    if (grp_y + (priv->textheight * 2) >= priv->pageheight)
    {
        return priv->datarow;
    }*/

    // This loop parses the entire range passed to the group.
    while ((grp_idx < maxrow) && (priv->ypos < priv->pageheight))
    {
        //char *grptxt = PQgetvalue(priv->pgresult, grp_idx, curgrp->grpcol);
        char *grptxt = g_hash_table_lookup (g_ptr_array_index (priv->pgresult,
                                                grp_idx),
                                            curgrp->grpcol);

        /* Break the main group down into subgroups (or the body)
           Do this by comparing the string in the column defining the group.
           The variable "grp_idx" is the max value (+ 1) for the rows that
           will be members of the subgroup (or body set)
        */

        do {
            ++grp_idx;
        } while ((grp_idx < maxrow) && STRMATCH(grptxt,
                    g_hash_table_lookup (g_ptr_array_index (priv->pgresult,
                                                grp_idx),
                                            curgrp->grpcol)));

        // Print Group Header, if applicable...

        if (curgrp->pointsabove)
        {
            priv->ypos += curgrp->pointsabove;
        }

        if (curgrp->header)
        {
            render_header (self, curgrp->header);
        }       // if (curgrp->header)

        //if(render_params->pts_above){priv->ypos += render_params->pts_above;}
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
        if (priv->DoPrint)
        {
            if (curgrp->borderstyle & (SINGLEBOX | DBLBOX))
            {
                cairo_set_line_width (priv->cr, 4.0);
                cairo_rectangle (priv->cr, 0, grp_top,
                                   gtk_print_context_get_width (priv->context),
                                   priv->ypos - grp_top); 
                cairo_stroke (priv->cr);
            }

            if (curgrp->borderstyle & DBLBOX)
            {
                cairo_set_line_width (priv->cr, 2.0);
                cairo_rectangle (priv->cr, 16, grp_top + 16,
                            gtk_print_context_get_width (priv->context) - 32,
                            priv->ypos - grp_top - 32); 
                cairo_stroke (priv->cr);
            }
        }

        if (curgrp->pointsbelow)
        {
            priv->ypos += curgrp->pointsbelow;
        }
        if (priv->ypos >= priv->pageheight)
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);


    priv->ypos = 0;

    if (priv->DoPrint)
    {
        lastrow = (int)g_ptr_array_index (priv->PageEndRow, priv->pageno);
    }
    else
    {
        lastrow = priv->pgresult->len;
    }

    if (priv->PageHeader)
    {
        if (priv->PageHeader->celldefs)
        {
            if (!priv->PageHeader->cells_formatted)
            {
                //TODO: We may need to add in Left Margin
                priv->xpos = 0;
                g_ptr_array_foreach(priv->PageHeader->celldefs,
                        (GFunc)set_col_values, self);
                priv->PageHeader->cells_formatted = TRUE;
            }

            render_row_grp (self, priv->PageHeader->celldefs, NULL,
                            0, priv->layout, priv->datarow, priv->datarow + 1);
        }
    }
    else if (!priv->pageno)       // If first page, print Docheader if present
    {
        if (priv->DocHeader)
        {
        }
    }

    curgrp = priv->grpHd;

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
style_print_table_draw_page (GtkPrintOperation *op,
                                GtkPrintContext *context, int page_nr)
{
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (STYLE_PRINT_TABLE(op));

    priv->pageno = page_nr;
    priv->context = context;
    priv->pageheight =
                    gtk_print_context_get_height (priv->context);
//    gtk_print_STYLE_PRINT_TABLE(operation)_set_unit (operation, GTK_UNIT_POINTS);
    priv->cr = gtk_print_context_get_cairo_context (context);
    priv->layout = gtk_print_context_create_pango_layout (context);
    render_page (STYLE_PRINT_TABLE(op));
    g_object_unref (priv->layout);
}

/* ************************************************************************ *
 * begin_print() - Called after print settings have been set up.            *
 *      Here we set up the default font, determine the height of a line.    *
 *      We paginate the output by doing a "dry run" through the data        *
 * ************************************************************************ */

static void
style_print_table_begin_print (GtkPrintOperation *po,
        GtkPrintContext *context)
{
    PangoLayout *lo;
    PangoRectangle log_rect;
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (STYLE_PRINT_TABLE(po));

    //int firstrow = 0;   // First datarow for the page;
    priv->datarow = 0;
    priv->pageno = 0;
    priv->context = context;
    priv->pageheight = gtk_print_context_get_height (priv->context);
    priv->TotPages = 0;
    priv->DoPrint = FALSE;
//    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
    priv->layout = gtk_print_context_create_pango_layout (context);

    lo = pango_layout_copy (priv->layout);
    pango_layout_set_font_description (lo, priv->defaultcell->pangofont);
    pango_layout_set_width (lo, gtk_print_context_get_width (priv->context));
    pango_layout_set_text (lo, "Ty", -1);
    pango_layout_get_extents (lo, NULL, &log_rect);
    priv->textheight = log_rect.height/PANGO_SCALE;
    g_object_unref (lo);

    priv->PageEndRow = g_ptr_array_new();

    while ((priv->datarow) < priv->pgresult->len)
    {
        priv->ypos = 0;
        render_page (STYLE_PRINT_TABLE(po));
        g_ptr_array_add (priv->PageEndRow,
                (gpointer)(priv->datarow));
        ++(priv->TotPages);
    }

    gtk_print_operation_set_n_pages (po,
            priv->TotPages ? priv->TotPages : 1);
    // Re-initialize Instance variables
    g_object_unref (priv->layout);
    priv->DoPrint = TRUE;
    priv->datarow = 0;
    priv->pageno = 0;
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
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);


    gtk_print_operation_run (GTK_PRINT_OPERATION(self),
            GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, priv->w_main, &g_err);
    
    // Now free up everything that has been allocated...
    if (priv->PageEndRow)
    {
        g_ptr_array_free (priv->PageEndRow, TRUE);
    }
}

/**
 * style_print_table_from_xmlfile:
 * @tblprnt: The StylePrintTable
 * @win: (nullable): The parent window - NULL if none
 * @data: (element-type GHashTable): The data to process - A GPtrArray of GHashTables
 * @filename: The filename to open and read to get the xml definition for the printout.
 *
 * Print a tabular form where the xml definition for the output is
 * contained in a file.
 */

void
style_print_table_from_xmlfile (StylePrintTable *self,
                                        GtkWindow *wmain,
                                        GPtrArray *data,
                                        //GPtrArray *parms,
                                        char *fname)
{
    char buf[3000];
    FILE *fp;
    int rdcount;
    GMarkupParseContext *gmp_contxt;
    GError *error;
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);

//    if ( ! priv->conn)
//    {
//        report_error (priv,
//                "No connection!  Please connect to database first");
//    }

    if (wmain)
    {
        priv->w_main = wmain;
    }

    priv->pgresult = data;

//    if (!priv->Page_Setup)
//    {
//        set_page_defaults (self);
//    }

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
    reset_default_cell (self);

    if ( ! error)
    {
        render_report (self);
    }

    //free_default_cell();
    //return STYLE_PRINT_TABLE(tbl)->grpHd; // Temporary - for debugging
}

/**
 * style_print_table_from_xmlstring:
 * @tp: The #StylePrintTable
 * @w: (nullable): The parent window - NULL if none
 * @data: (element-type GHashTable): The data to process - A GPtrArray of GHashTables
 * @c: Pointer to the string containing the xml formatting
 *
 * Print a table where the definition for the format is contained in an
 * xml string.
 *
 */

void
style_print_table_from_xmlstring (  StylePrintTable *self,
                                            GtkWindow *wmain,
                                            GPtrArray *data,
                                                 char *xml)
{
    GMarkupParseContext *gmp_contxt;
    GError *error;
    StylePrintTablePrivate *priv;
   
    priv = style_print_table_get_instance_private (self);


    if (wmain)
    {
        priv->w_main = wmain;
    }

    priv->pgresult = data;

    gmp_contxt =
        g_markup_parse_context_new (&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                priv, NULL);
    g_markup_parse_context_parse (gmp_contxt, xml, strlen(xml), &error);
    reset_default_cell (self);
    render_report (self);
    free_default_cell (self);
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
 * style_print_table_set_wmain:
 * @self: The #StylePrintTable instance
 * @win: The toplevel window
 *
 * Sets the top level window.  Needed for declaring the parent window
 * for dialogs, etc.
 */

void
style_print_table_set_wmain (StylePrintTable *self, GtkWindow *win)
{
    StylePrintTablePrivate *priv =
                style_print_table_get_instance_private (self);
    priv->w_main = win;
}

/**
 * style_print_table_get_wmain:
 * @self: the #StylePrintTable instance
 * returns: (transfer none): The #GtkWindow that is declared as top-level window.
 *
 * Get the top-level window which is useful as the parent window for
 * dialogs, etc.
 */

GtkWindow *
style_print_table_get_wmain (StylePrintTable *self)
{
    StylePrintTablePrivate *priv =
            style_print_table_get_instance_private (self);
    return priv->w_main;
}
