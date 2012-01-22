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


struct xml_udat {
    int depth;
    PGRPINF parent;
    PGRPINF curfmt;
} XMLData;

/* ************************************************************************ *
 
 * ************************************************************************ */

// Forward references within this module
//static void setup_formatting(GHashTable *);
void render_report (GLOBDAT *); // maybe need to move this to header file...
static void start_element_main(GMarkupParseContext *, const gchar *,
        const gchar **, const gchar **, gpointer, GError **);
static void end_element_main(GMarkupParseContext *, const gchar *,
        gpointer, GError **);
//static void alt_cell_end_element(GMarkupParseContext *, const gchar *,
//        gpointer, GError **);
static void prs_err(GMarkupParseContext *, GError *, gpointer);

GLOBDAT GlobalData;

char *GroupElements[] = {"pageheader", "group", "body", NULL};
gboolean Formatted = FALSE;
//PangoFontDescription *DefaultPangoFont;
CELLINF *defaultcell;
PGRPINF FmtList;    // The formatting tree;
static GError *error;
GMarkupParser prsr = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};
GMarkupParser sub_prs = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};

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

void
report_error (GLOBDAT *self, char *message)
{
    if (self->w_main)
    {
        GtkWidget *dlg;

        dlg = gtk_message_dialog_new (self->w_main,
                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                message);
        gtk_dialog_run (GTK_DIALOG(dlg));
        gtk_widget_destroy (dlg);
    }
    else
    {
        fprintf (stderr, message);
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

        if (STRMATCH(attrib_names[idx], "cellwidth"))
        {
            cell->cellwidth = strtof (attrib_vals[idx], NULL);
        }

        if (STRMATCH(attrib_names[idx], "textsource"))
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
            //TODO: add error reporting...
        }

        if (STRMATCH(attrib_names[idx], "celltext"))
        {
            cell->celltext = g_strdup (attrib_vals[idx]);
        }

        if (STRMATCH(attrib_names[idx], "align"))
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
append_cell_def (const gchar **attrib_names, const gchar **attrib_vals,
        GRPINF *parentgrp)
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

    add_cell_attribs (mycell, attrib_names, attrib_vals);

    if (mycell->txtsource == TSRC_DATA)
    {
        if (mycell->celltext && strlen(mycell->celltext))
        {
            mycell->cell_col = PQfnumber (GlobalData.pgresult,
                                            mycell->celltext);

            if (PQresultStatus (GlobalData.pgresult) != PGRES_TUPLES_OK)
            {
                if (GlobalData.w_main)
                {
                    GtkWidget *dlg = gtk_message_dialog_new (GlobalData.w_main,
                            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                            PQresultErrorMessage (GlobalData.pgresult));
                    gtk_dialog_run (GTK_DIALOG(dlg));
                    gtk_widget_destroy (dlg);
                }
                else
                {
                    fprintf (stderr, "Cannot determine column # for: %s\n",
                                PQresultErrorMessage (GlobalData.pgresult));
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
allocate_new_group (const char **attrib_names, const char **attrib_vals,
                    GRPINF *parent, int grptype)
{
    GRPINF *newgrp = calloc (1, sizeof(GRPINF));

    if (!parent && !GlobalData.grpHd)
    {
        GlobalData.grpHd = (GRPINF *)newgrp;
        newgrp = GlobalData.grpHd;
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

    return newgrp;
}

static void start_element_main (GMarkupParseContext *context,
                                const gchar  *element_name,
                                const gchar **attrib_names,
                                const gchar **attrib_vals,
                                gpointer parent,
                                GError **error)
{
    gpointer newgrp;
    GRPINF *cur_grp;

    if (STRMATCH(element_name, "cell"))
    {
        if (parent)
        {
            newgrp =
                append_cell_def (attrib_names, attrib_vals, (GRPINF *)parent);
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
        newgrp = allocate_new_group (attrib_names, attrib_vals,
                                        (GRPINF *)parent, GRPTY_GROUP);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
            GlobalData.grpHd = (GRPINF *)newgrp;
        }
    }
    else if (STRMATCH(element_name, "header"))
    {
        if (parent)
        {
            newgrp = allocate_new_group (attrib_names, attrib_vals,
                                            NULL, GRPTY_HEADER);
            ((GRPINF *)parent)->header = newgrp;
        }
    }
    else if (STRMATCH(element_name, "body"))
    {
        newgrp = allocate_new_group (attrib_names, attrib_vals,
                                        (GRPINF *)parent, GRPTY_BODY);
        if (parent)
        {
            ((GRPINF *)parent)->grpchild = newgrp;
        }
        else
        {
            GlobalData.grpHd = (GRPINF *)newgrp;
        }
    }
    else if (STRMATCH(element_name, "font"))
    {
        int idx = 0;
        FONTINF *font_me;

        //if (!parent)
        //{error
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
        newgrp = GlobalData.PageHeader;
    }
    else //if (STRMATCH(element_name, "docheader"))
    {
        newgrp = GlobalData.DocHeader;
    }

    g_markup_parse_context_push (context, &sub_prs, newgrp);
}

static void end_element_main (GMarkupParseContext *context,
                                const gchar *element_name,
                                gpointer this_grp,
                                GError **error)
{
//    if (this_grp)      // If anywhere but in top-level parse
//    {
        gpointer ptr = g_markup_parse_context_pop (context);
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
static void set_page_defaults (GLOBDAT *self)
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

void *tblprint_from_xmlfile (GtkWindow *wmain, PGresult *res, char *fname)
{
    char buf[300];
    FILE *fp;
    int rdcount;
    GMarkupParseContext *gmp_contxt;

    GlobalData.w_main = wmain;
    GlobalData.pgresult = res;

    if (!GlobalData.Page_Setup)
    {
        set_page_defaults (&GlobalData);
    }

    if (!(fp = fopen (fname, "rb")))
    {
        return NULL;
    }

    gmp_contxt =
        g_markup_parse_context_new (&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                                        NULL, NULL);
    
    while ((rdcount = fread (buf, 1, sizeof(buf), fp)))
    {
        g_markup_parse_context_parse (gmp_contxt, buf, rdcount, &error);
    }

    fclose(fp);
    reset_default_cell ();
    render_report (&GlobalData);
    //free_default_cell();
    return GlobalData.grpHd;       // Temporary - for debugging
}

void *tblprint_from_xmlstring (GtkWindow *wmain, PGresult *res, char *xml)
{
    GMarkupParseContext *gmp_contxt;

    GlobalData.w_main = wmain;
    GlobalData.pgresult = res;

    if (!GlobalData.Page_Setup)
    {
        set_page_defaults (&GlobalData);
    }

    gmp_contxt =
        g_markup_parse_context_new (&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                NULL, NULL);
    g_markup_parse_context_parse (gmp_contxt, xml, strlen(xml), &error);
    reset_default_cell ();
    render_report (&GlobalData);
    free_default_cell();
    return GlobalData.grpHd;   // For debugging - see what is produced in the GlobalData.grpHd struct
}
