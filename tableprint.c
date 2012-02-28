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
 * NOTE: We tried to use gtk_page_setup_get_page_width, (after defining *
 *      the PageSetup structure, but it gave a value of only a little   *
 *      more than 10% of what was needed to get the full page width.    *
 * ******************************************************************** */

static void
set_col_values (CELLINF *cell, GLOBDAT *self)
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
render_cell (CELLINF *cell, int rownum, double rowtop, GLOBDAT *self)
{
    char *celltext;
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
hline (GLOBDAT *self, double ypos, double weight)
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

static double
group_hline_top (GLOBDAT *self, double rowtop, int borderstyle)
{
    double linerow = rowtop;

    if (borderstyle & (SINGLEBAR_HVY | DBLBAR))
    {
        linerow += hline (self, linerow, 1.0);
    }
    else if (borderstyle & SINGLEBAR)
    {
        linerow += hline (self, linerow, 0.5);
    }

    if (borderstyle & DBLBAR)
    {
        linerow += hline (self, linerow, 0.5);
    }

    return linerow - rowtop;
}

/* **************************************************************** *
 * group_hline_bottom() - Render a single or double line below a    *
 *            group, header, etc.                                   *
 * **************************************************************** */

static double
group_hline_bottom (GLOBDAT *self, double begintop, int borderstyle)
{
    double rowtop = begintop;

    if (borderstyle & (SINGLEBAR | DBLBAR))
    {
        rowtop += hline (self, rowtop, 0.5);
    }
    else if (borderstyle & SINGLEBAR_HVY)
    {
        rowtop += hline (self, rowtop, 1.0);
    }

    if (borderstyle & DBLBAR)
    {
        rowtop += hline (self, rowtop, 1.0);
    }

    return rowtop - begintop;
}

/* ******************************************************************** *
 * render_row() - render a single row of data                           *
 * Returns the height of the line in points                             *
 * ******************************************************************** */

static int
render_row (GLOBDAT *self,
            GtkPrintContext *context,
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
            render_cell ((CELLINF *)(coldefs->pdata[colnum]), rownum,
                   rowtop, self);

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
                    int rmargin = gtk_print_context_get_width (self->context);

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
render_row_grp (GLOBDAT *self,       // Global Data storage
        GtkPrintContext *context,   // The GtkPrintcontext
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

    // If no cell defs, return...
    if (!col_defs)
    {
        return cur_row;
    }

    // Render HLINE above first line, if applicable
    if (borderstyle & BDY_HLINE)
    {
        self->ypos += hline (self, self->ypos, 1.0);
    }

    for (cur_idx = cur_row; cur_idx < end_row; cur_idx++)
    {
        (self->ypos) += render_row (self, context, col_defs, padding,
                            borderstyle, cur_idx);

        // Render HLINE below each line, if applicable
        if (borderstyle & BDY_HLINE)
        {
            self->ypos += hline (self, self->ypos, 1.0);
        }

        if (self->ypos >= self->pageheight)
        {
            ++cur_idx;      // Position to next data row for return
            return cur_idx;
        }
    }

    return cur_idx;
}

static void
render_header (GLOBDAT *self, GtkPrintContext *context, GRPINF *curhdr)
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

        render_row_grp (self, context, curhdr->celldefs,
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
render_body (GLOBDAT *self,
                GtkPrintContext *context,
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

    self->datarow = render_row_grp (self, context,  bdy->celldefs,
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
render_group(GLOBDAT *self,
                GtkPrintContext *context,
                GRPINF *curgrp,
                int maxrow)
{
    int grp_idx = self->datarow;
    //double y_pos = self->ypos;
    int grp_y = self->ypos,
        grp_top;
    GRPINF *cg = curgrp;
    //PangoLayout *layout = set_layout(self, render_params, self->layout);

    grp_top = self->ypos;

    // Try to avoid leaving hanging group headers at the bottom of the
    // page with no body data...
    // TODO: We need to make this more correct.. This is just a hack

    while (cg->grpchild->grptype == GRPTY_GROUP)
    {
        //grp_y += cg->pointsabove + self->textheight + cg->pointsbelow;
        grp_y += self->textheight;
        cg = cg->grpchild;
    }

    if (grp_y + (self->textheight * 2) >= self->pageheight)
    {
        return self->datarow;
    }

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
            render_header (self, context, curgrp->header);
        }       // if (curgrp->header)

        //if(render_params->pts_above){self->ypos += render_params->pts_above;}
        // Possibly add group-header...

        // Now render the current range of data.  It will be
        // either another subgroup or the body of data.

        if (curgrp->grpchild)
        {
            if (curgrp->grpchild->grptype == GRPTY_GROUP)
            {
                render_group (self, context, curgrp->grpchild, grp_idx);

                // TODO: We're not even trying to access the group's cell
                // defs ATM.  There probably should not be any cell defs for
                // a group, but rather cell defs for a header and a footer.
            }
            else if (curgrp->grpchild->grptype == GRPTY_BODY)
            {
                render_body (self, context, curgrp->grpchild, grp_idx);
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
render_page(GLOBDAT *self, GtkPrintContext *context)
{
    GRPINF *curgrp;

    self->ypos = 0;

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

            render_row_grp (self, context, self->PageHeader->celldefs, NULL,
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
    self->context = context;
    self->pageheight = gtk_print_context_get_height (self->context);
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
    PangoLayout *lo;
    PangoRectangle log_rect;

    self->datarow = 0;
    self->pageno = 0;
    self->context = context;
    self->pageheight = gtk_print_context_get_height (self->context);
    self->TotPages = 0;
    self->DoPrint = FALSE;
//    gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
    self->layout = gtk_print_context_create_pango_layout (context);

    lo = pango_layout_copy (self->layout);
    pango_layout_set_font_description (lo, defaultcell->pangofont);
    pango_layout_set_width (lo, gtk_print_context_get_width (self->context));
    pango_layout_set_text (lo, "Ty", -1);
    pango_layout_get_extents (lo, NULL, &log_rect);
    self->textheight = log_rect.height/PANGO_SCALE;
    g_object_unref (lo);

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
