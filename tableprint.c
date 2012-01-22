/* ******************************************************************** *
 * tableprint.c - PrintOperations file for the gtktblprintpg library    $
 *                                                                      $
 * $Id::                                                                $
 * ******************************************************************** */

#include <pango/pango.h>
#include <glib-object.h>
#include <cairo.h>
#include "gtktblprintpg.h"

GtkPrintOperation *po;
static GError *g_err;

extern CELLINF *defaultcell;
void report_error (GLOBDAT *, char *);

/* ******************************************************************** *
 * pq_data_from_colname() - Retrieve data from a column specified by    *
 *          its name rather than its number.                            *
 * ******************************************************************** */

static char *
pq_data_from_colname (PGresult *res, int row, char *colname)
{
    return PQgetvalue (res, row, PQfnumber(res, colname));
}

/* ******************************************************************** *
 * set_col_values() - Set the cellwidth and left position for the cell  *
 *      This is called on the first encounter with the cell.  It is not *
 *      called in the initialization routine since the Page setup might *
 *      be changed after this cell is initialized.                      *
 * ******************************************************************** */

static void
set_col_values (CELLINF *cell, GLOBDAT *self)
{
    double ps = gtk_page_setup_get_page_width(self->Page_Setup, GTK_UNIT_POINTS);
    double pc = gtk_print_context_get_width(self->context);
    printf ("\nPage width returned by 'gtk_page_setup_get_page_width' = %f\n",ps);
    printf ("\nPage width returned by 'gtk_print_context_get_width' = %f\n\n",pc);
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
set_cell_font_description (GLOBDAT * self, CELLINF *cell)
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
render_cell (CELLINF *cell, int rownum, GLOBDAT *self)
{
    char *celltext;
    int CellHeight = 0;
    PangoRectangle log_rect;

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
        case TSRC_PRINTF:
            //TODO:
            break;
    }

    if (celltext && strlen(celltext))
    {
        PangoLayout *layout =
                    gtk_print_context_create_pango_layout (self->context);
        pango_layout_set_font_description (layout, cell->pangofont);
        pango_layout_set_width (layout, (int)(cell->cellwidth * PANGO_SCALE));
        pango_layout_set_alignment (layout, cell->layoutalign);
        pango_layout_set_text(layout, celltext, -1);
        pango_layout_get_extents(layout, NULL, &log_rect);
        CellHeight = log_rect.height;

        if (self->DoPrint)
        {
            cairo_move_to (self->cr, cell->x, self->ypos);
            pango_cairo_show_layout (self->cr, layout);
        }

        g_object_unref (layout);
    }
    else
    {
        fprintf (stderr, "---Empty data for column: %s\n", cell->celltext);
    }

    return CellHeight/PANGO_SCALE;
}

/* ******************************************************************** *
 * render_row() - render a single row of data                           *
 * Returns the height of the line in points                             *
 * ******************************************************************** */

static int
render_row (GLOBDAT *self,
            GtkPrintContext *context,
            GPtrArray *coldefs,
            int rownum)
{
    int colnum;
    int MaxHeight = 0;

    for (colnum = 0; colnum < coldefs->len; colnum++)
    {
        int CellHeight;
        
        CellHeight =
            render_cell ((CELLINF *)(coldefs->pdata[colnum]), rownum, self);

        if (CellHeight > MaxHeight)
        {
            MaxHeight = CellHeight;
        }
    }

    return MaxHeight;
}

/* ******************************************************************** *
 * render_row_grp() - Render a series of rows from an array of cell     *
 *          defs.                                                       *
 * Passed:  (1) - self - the GlobalList of data                         *
 *          (2) - context - The GtkPrintContext                         *
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
render_row_grp(GLOBDAT *self,       // Global Data storage
        GtkPrintContext *context,   // The GtkPrintcontext
        GPtrArray *col_defs,         // The coldefs array
        // formatting->body,           // 
        // formatting
        PangoLayout *layout,
        int cur_row,                // The first row to print
        int end_row)                // The last row to print + 1
{
    int cur_idx = cur_row;

    // If no cell defs, return...
    if (!col_defs)
    {
        return cur_row;
    }

    for (cur_idx = cur_row; cur_idx < end_row; cur_idx++)
    {
        (self->ypos) += render_row (self, context, col_defs, cur_idx);

        if (self->ypos >= gtk_page_setup_get_page_height (self->Page_Setup,
                                                            GTK_UNIT_POINTS))
        {
            ++cur_idx;      // Position to next data row for return
            return cur_idx;
        }
    }

    return cur_idx;
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
render_body (GLOBDAT *self,
                GtkPrintContext *context,
                GRPINF *curgrp,
                int maxrow)
{
    if (curgrp->celldefs)
    {
        if (!curgrp->cells_formatted)
        {
            //TODO: We may need to add in Left Margin
            self->xpos = 0;
            g_ptr_array_foreach(curgrp->celldefs, (GFunc)set_col_values,
                                self);
            curgrp->cells_formatted = TRUE;
        }
    }

    self->datarow = render_row_grp(self, context,  curgrp->celldefs,
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
render_group(GLOBDAT *self,
                GtkPrintContext *context,
                GRPINF *curgrp,
                int maxrow)
{
    int grp_idx = self->datarow;
    double y_pos = self->ypos;
    //int grp_col = PQfnumber(self->pgresult, curgrp->grpcol);
    //PangoLayout *layout = set_layout(self, render_params, self->layout);

    if (curgrp->header)
    {
        if (curgrp->header->celldefs)
        {
            if (!curgrp->header->cells_formatted)
            {
                //TODO: We may need to add in Left Margin
                self->xpos = 0;
                g_ptr_array_foreach(curgrp->header->celldefs,
                        (GFunc)set_col_values, self);
                curgrp->header->cells_formatted = TRUE;
            }

            render_row_grp (self, context, curgrp->header->celldefs,
                            self->layout, self->datarow, self->datarow + 1);
        }
    }       // if (curgrp->header)

    while ((grp_idx < maxrow) &&
            (y_pos < gtk_page_setup_get_page_height (self->Page_Setup,
                                                     GTK_UNIT_POINTS)))
    {
        char *grptxt = PQgetvalue(self->pgresult, grp_idx, curgrp->grpcol);

        do {
            ++grp_idx;
        } while ((grp_idx < maxrow) && STRMATCH(grptxt,
                        PQgetvalue(self->pgresult,grp_idx, curgrp->grpcol)));

        //if(render_params->pts_above){self->ypos += render_params->pts_above;}
        // Possibly add group-header...

        if (curgrp->grpchild)
        {
            if (curgrp->grpchild->grptype == GRPTY_GROUP)
            {
                render_group (self, context, curgrp->grpchild, maxrow);

                // TODO: We're not even trying to access the group's cell
                // defs ATM.  There probably should not be any cell defs for
                // a group, but rather cell defs for a header and a footer.
            }
            else if (curgrp->grpchild->grptype == GRPTY_BODY)
            {
                render_body (self, context, curgrp->grpchild, maxrow);
            }
        }
        else
        {
            // Would this be an error????
        }

        // Render footers here???
        if (self->ypos >= gtk_page_setup_get_page_height (self->Page_Setup,
                                                            GTK_UNIT_POINTS))
        {
            break;
        }
    }

    return grp_idx;
}

static void
render_page(GLOBDAT *self, GtkPrintContext *context)
{
    GRPINF *curgrp;

    self->ypos = 0;

    if (!self->pageno)       // If first page, print Docheader if present
    {
        if (self->DocHeader)
        {
        }
    }
    else if (self->PageHeader)
    {
    }

    curgrp = self->grpHd;

    // Now we're ready to render the data...
    if (curgrp->grptype == GRPTY_GROUP)
    {
        render_group (self, context, curgrp, PQntuples (self->pgresult));
    }
    // TODO: Need to check for some other type than GRPTY_BODY???
    // Also, this is wrong, as we don't have a "render_group" function
    else
    {
        render_body (self, context, curgrp, PQntuples (self->pgresult));
    }
}

static void
draw_page (GtkPrintOperation *operation, GtkPrintContext *context,
                                int page_nr, GLOBDAT *self)
{
    self->pageno = page_nr;
    self->pageheight = gtk_print_context_get_height (self->context);
    self->context = context;
//    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
    self->cr = gtk_print_context_get_cairo_context (context);
    self->layout = gtk_print_context_create_pango_layout (context);
    render_page (self, context);
    g_object_unref (self->layout);
}

static void
begin_print (GtkPrintOperation *operation, GtkPrintContext *context,
                                        GLOBDAT *self)
{
    self->datarow = 0;
    self->pageno = 0;
    self->TotPages = 0;
    self->DoPrint = FALSE;
    self->context = context;
//    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
    self->layout = gtk_print_context_create_pango_layout (context);

    while (self->datarow < PQntuples (self->pgresult))
    {
        self->ypos = 0;
        render_page (self, context);
        ++self->TotPages;
    }

    gtk_print_operation_set_n_pages (operation,
            self->TotPages ? self->TotPages : 1);
    // Re-initialize Global variables
    g_object_unref (self->layout);
    self->DoPrint = TRUE;
    self->datarow = 0;
    self->pageno = 0;
}

void
render_report (GLOBDAT *self)
{
    if (!po)
    {
        po = gtk_print_operation_new();
        g_signal_connect (po, "begin-print", G_CALLBACK(begin_print), self);
        g_signal_connect (po, "draw-page", G_CALLBACK(draw_page), self);
        gtk_print_operation_set_default_page_setup (po, self->Page_Setup);
    }

    gtk_print_operation_run (po, GTK_PRINT_OPERATION_ACTION_PRINT,
            self->w_main, &g_err);
}
