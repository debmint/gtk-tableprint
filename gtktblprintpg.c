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
#include <string.h>
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
static void start_element_main(GMarkupParseContext *, const gchar *,
        const gchar **, const gchar **, gpointer, GError **);
static void end_element_main(GMarkupParseContext *, const gchar *,
        gpointer, GError **);
//static void alt_cell_end_element(GMarkupParseContext *, const gchar *,
//        gpointer, GError **);
static void prs_err(GMarkupParseContext *, GError *, gpointer);

char *GroupElements[] = {"pageheader", "group", "body", NULL};
GtkPageSetup *Page_Setup;
gboolean Formatted = FALSE;
GRPINF *DocHeader;
GRPINF *PageHeader;
PangoFontDescription *DefaultPangoFont;
CELLINF defaultcell;
PGRPINF FmtList;    // The formatting tree;
GRPINF *grpHd;
GError *error;
GMarkupParser prsr = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};
GMarkupParser sub_prs = {start_element_main, end_element_main, NULL,
                        NULL, prs_err};

static void
prs_err(GMarkupParseContext *context, GError *error, gpointer user_data)
{
    fprintf(stderr, "Error encountered: %s\n", error->message);
}

/* **************************************************************** *
 * append_cell_defs() : Populate a cell definition in the row       *
 *          array, and append this new cell onto the array of cells *
 *  If the array has not already been created, it is now created.   *
 * **************************************************************** */

static CELLINF *
append_cell_def(const gchar **attrib_names, const gchar **attrib_vals,
        GRPINF *mygroup)
{
    CELLINF *mycell;
    GPtrArray *cellary;
    int idx = 0;

    if (!(cellary = mygroup->celldefs))
    {
        mygroup->celldefs = g_ptr_array_new();
        
    }

    mycell = calloc(1, sizeof(CELLINF));
    mycell->grptype = GRPTY_CELL;

    while (attrib_names[idx])
    {
        if (STRMATCH(attrib_names[idx], "percent"))
        {
            mycell->percent = strtof(attrib_vals[idx], NULL);
        }

        if (STRMATCH(attrib_names[idx], "cellwidth"))
        {
            mycell->cellwidth = strtof(attrib_vals[idx], NULL);
        }

        if (STRMATCH(attrib_names[idx], "textsource"))
        {
            mycell->txtsource = atoi(attrib_vals[idx]);
        }

        if (STRMATCH(attrib_names[idx], "celltext"))
        {
            mycell->celltext = g_strdup(attrib_vals[idx]);
        }

        if (STRMATCH(attrib_names[idx], "textalign"))
        {
            mycell->txtalign = atoi(attrib_vals[idx]);
        }

        ++idx;
    }

    g_ptr_array_add(mygroup->celldefs, mycell);
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
allocate_new_group(GRPINF *parent, int grptype)
{
    GRPINF *newgrp = calloc(1, sizeof(GRPINF));

    if (parent)
    {
        parent->grpchild = newgrp;
        //newgrp->grpparent = cur_grp;
    }
    else
    {
        grpHd = (GRPINF *)newgrp;
        newgrp = grpHd;
    }

    newgrp->grptype = grptype;
    newgrp->font = malloc(sizeof(FONTINF));

    if (parent && parent->font)
    {
        memcpy(newgrp->font, parent->font, sizeof(FONTINF));
    }
    else
    {
        memcpy(newgrp->font, defaultcell.font, sizeof(FONTINF));
    }

    if (parent && parent->pangofont)
    {
        newgrp->pangofont = pango_font_description_copy(parent->pangofont);
    }
    else
    {
        newgrp->pangofont = pango_font_description_copy(defaultcell.pangofont);
    }

    return newgrp;
}

static void start_element_main(GMarkupParseContext *context,
                                const gchar  *element_name,
                                const gchar **attrib_names,
                                const gchar **attrib_vals,
                                gpointer user_data,
                                GError **error)
{
    gpointer newgrp;
    GRPINF *cur_grp;

    if (STRMATCH(element_name, "cell"))
    {
        newgrp =
            append_cell_def(attrib_names, attrib_vals, (GRPINF *)user_data);
    }
    else if (STRMATCH(element_name, "defaultcell"))
    {
        newgrp =
            append_cell_def(attrib_names, attrib_vals, (GRPINF *)user_data);
    }
    else if (STRMATCH(element_name, "group"))
    {
        newgrp = allocate_new_group((GRPINF *)user_data, GRPTY_GROUP);
//        newgrp = calloc(1, sizeof(GRPINF));   
//
//        if (user_data)
//        {
//            cur_grp = (GRPINF *)user_data;
//            cur_grp->grpchild = newgrp;
//            //newgrp->grpparent = cur_grp;
//        }
//        else
//        {
//            grpHd = (GRPINF *)newgrp;
//        }
    }
    else if (STRMATCH(element_name, "body"))
    {
        newgrp = allocate_new_group((GRPINF *)user_data, GRPTY_BODY);
    }
    else if (STRMATCH(element_name, "font"))
    {
        int idx = 0;

        cur_grp = user_data;

        if ( ! cur_grp->font)
        {
            cur_grp->font = calloc(1, sizeof(FONTINF));
        }

        while (attrib_names[idx])
        {
            const char *a = attrib_names[idx];

            if ( STRMATCH(a, "family"))
            {
                cur_grp->font->family = g_strdup(attrib_vals[idx]);
            }
            else if ( ! strcmp(a, "size"))
            {
                cur_grp->font->size = strtof(attrib_vals[idx], NULL);
            }

            ++idx;
        }

        newgrp = cur_grp->font;
    }
    else if (STRMATCH(element_name, "pageheader"))
    {
        newgrp = PageHeader;
    }
    else if (STRMATCH(element_name, "docheader"))
    {
        newgrp = DocHeader;
    }

    g_markup_parse_context_push(context, &sub_prs, newgrp);
}

static void end_element_main(GMarkupParseContext *context,
                                const gchar *element_name,
                                gpointer user_data,
                                GError **error)
{
//    if (user_data)      // If anywhere but in top-level parse
//    {
        gpointer ptr = g_markup_parse_context_pop(context);
//    }
//    if (STRMATCH(element_name, "group") || STRMATCH(element_name, "body")
//            || STRMATCH(element_name, "defaultcell")
//            || STRMATCH(element_name, "pageheader"))

    // Set the PangoFontDescription
    if (STRMATCH(element_name, "cell"))
    {
        PFONTINF fi;
        CELLINF *cell = (CELLINF *)ptr;

        if (!cell->pangofont)
        {
            cell->pangofont =
                pango_font_description_copy(defaultcell.pangofont);
        }

        if ((fi = cell->font))
        {
            if (fi->family)
            {
                pango_font_description_set_family (cell->pangofont,
                        fi->family);
            }

            if (fi->size)
            {
                pango_font_description_set_size (cell->pangofont,
                        fi->size * PANGO_SCALE);
            }

            if (fi->style)
            {
                pango_font_description_set_style (cell->pangofont,
                        fi->style);
            }

            if (fi->weight)
            {
                pango_font_description_set_weight (cell->pangofont,
                        fi->weight);
            }
        }
    }

}

// Set up default PrintSettings
static void set_page_defaults()
{
    GtkPaperSize *pap_siz;

    if (!defaultcell.font)
    {
        defaultcell.font = calloc(1, sizeof(FONTINF));
        defaultcell.font->family = "Serif";
        defaultcell.font->style = PANGO_STYLE_NORMAL;
        defaultcell.font->size = 10;
        defaultcell.font->weight = PANGO_STYLE_NORMAL;
        defaultcell.pangofont =
                pango_font_description_from_string(defaultcell.font->family);
        pango_font_description_set_style(defaultcell.pangofont,
                                        defaultcell.font->style);
        pango_font_description_set_size(defaultcell.pangofont,
                                        defaultcell.font->size * PANGO_SCALE);
        pango_font_description_set_weight(defaultcell.pangofont,
                                        defaultcell.font->weight);
    }

    if (!DefaultPangoFont)
    {
        DefaultPangoFont = pango_font_description_copy(defaultcell.pangofont);
    }


    if (!Page_Setup)
    {
        // We may need to get this by running gtk_print_run_page_setup_dialog()
        Page_Setup = gtk_page_setup_new();
        pap_siz = gtk_paper_size_new(GTK_PAPER_NAME_LETTER);
        gtk_page_setup_set_paper_size(Page_Setup, pap_siz);
        gtk_paper_size_free(pap_siz);
    }
}

void *tblprint_from_xmlfile(PGresult *res, char *fname)
{
    char buf[300];
    FILE *fp;
    int rdcount;
    GMarkupParseContext *gmp_contxt;

    if (!Page_Setup)
    {
        set_page_defaults();
    }

    if ( !(fp = fopen(fname, "rb")))
    {
        return NULL;
    }

    gmp_contxt =
        g_markup_parse_context_new(&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                NULL, NULL);
    
    while ((rdcount = fread(buf, 1, sizeof(buf), fp)))
    {
        g_markup_parse_context_parse(gmp_contxt, buf, rdcount, &error);
    }

    fclose(fp);
    return grpHd;       // Temporary - for debugging
}

void *tblprint_from_xmlstring(PGresult *res, char *xml)
{
    GMarkupParseContext *gmp_contxt;

    if (!Page_Setup)
    {
        set_page_defaults();
    }

    gmp_contxt =
        g_markup_parse_context_new(&prsr, G_MARKUP_TREAT_CDATA_AS_TEXT,
                NULL, NULL);

    g_markup_parse_context_parse(gmp_contxt, xml, strlen(xml), &error);

    return grpHd;   // For debugging - see what is produced in the grpHd struct
}
