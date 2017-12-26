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
 * styleprintpg.c - Code for GtkTblPrintPg library function                 $
 * $Id::                                                                    $
 * ************************************************************************ */

#include <libpq-fe.h>
#include "styleprintpg.h"

/**
 * SECTION:styleprintpg
 * @short_description: Postgresql interface to #StylePrintTable
 * @Title: StylePrintPg
 * @See_also: #StylePrintTable
 *
 * #StylePrintTable is a utility to print data in a tabular form.  However,
 * it is generic, and its data is provided in a #GPtrArray containing
 * #GHashTable 's representing each row of data.  This #GPtrArray must be
 * created and provided to #style_print_from_xmlfile or
 * #style_print_from_xmlstring.
 *
 * #StylePrintPg enables one to retrieve the data from a PostgreSQL database
 * and seamlessly print the data using #StylePrintTable.  In order to do this,
 * the xml specification must be provided - the same as when using
 * #style_print_table.  A query string, and optionally parameters for the
 * query.  These paramters are presented to #style_print_pg_fromxmlfile
 * or #style_print_pg_fromxmlstring and the data is retrieved and printed.
 */

struct _StylePrintPg
{
    StylePrintTable parent_instance;

    /*< private >*/
    PGconn    *conn;
    GPtrArray *qryParams;
};

G_DEFINE_TYPE(StylePrintPg, style_print_pg, STYLE_PRINT_TYPE_TABLE)

static GPtrArray * qry_get_data (StylePrintPg *self,
                                  const gchar *qry,
                                    GPtrArray *params);
void
style_print_pg_init (StylePrintPg *pg)
{
    pg->conn = NULL;
    pg->qryParams = NULL;
    //pg->pgresult = NULL;
}

void
style_print_pg_class_init (StylePrintPgClass *pg)
{
//    StylePrintTableClass *table_class = (StylePrintTableClass *)pg;
//    table_class->from_xmlfile = style_print_pg_from_xmlfile;
//    table_class->from_xmlstring = style_print_pg_from_xmlstring;
}

/* Error reporting routine.
 * If self->w_main is defined, the message will be reported in a dialog box
 * else it is sent to stderr
 */

static void
report_err (StylePrintPg *self, char *message)
{
    GtkWindow *w_main = style_print_table_get_wmain (STYLE_PRINT_TABLE(self));

    if (w_main)
    {
        GtkWidget *dlg;

        dlg = gtk_message_dialog_new (w_main,
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

/**
 * style_print_pg_connect:
 * @self: The #StylePrintPg
 * @dbn: The connection string set up in the way it is sent to the database
 *
 * Establishes a connection to the database.  This must be done before any queries or commands are sent to the database.
 * Note: see PostgreSQL documentation for details on the format for the @dbn string.
 *
 * Free the string upon return if need be.
 *
 * Returns: 1 on success, 0 on failure.
 *
 */

gint
style_print_pg_connect (StylePrintPg *self, gchar *dbn)
{

    self->conn = PQconnectdb(dbn);

    if (PQstatus(self->conn) != CONNECTION_OK)
    {
        report_err (self, "Failed to connect to db\n");
        self->conn = NULL;
        return 0;
    }

    return 1;
}

/**
 * style_print_pg_appendParam:
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
style_print_pg_appendParam (StylePrintPg *self, const gchar *param)
{
    if ( ! self->qryParams)
    {
        self->qryParams = g_ptr_array_new();
    }

    g_ptr_array_add (self->qryParams, (gpointer)param);
}

/**
 * style_print_pg_do:
 * @self: The #StylePrintPg *
 * @qry: The query to execute
 * 
 * Execute a non-data-returning query on the database
 * If parameters are used, they must be previously set
 */

void
style_print_pg_do (StylePrintPg *self, const gchar * qry)
{
    PGresult *rslt;


    if ( ! self->conn)
    {
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
        report_err (self, PQresultErrorMessage(rslt));
    }

//    if (priv->qryParams)
//    {
//        g_ptr_array_free (priv->qryParams, FALSE);
//        priv->qryParams = NULL;
//    }

    //PQclear (rslt);     // Simply discard this result
}

/* ==================================================================== *
 * Retrieve data from the database and convert it to format expected    *
 * by StylePrintTable.                                                  *
 * ==================================================================== */

static GPtrArray *
qry_get_data (StylePrintPg *self, const gchar *qry, GPtrArray *params)
{
    PGresult * rslt;
    GPtrArray *data;
    gint row;

    if ( ! self->conn)
    {
        fprintf (stderr, "No connection to database\n");
        return NULL;
    }

    if (params == NULL)
    {
        rslt = PQexec (self->conn, qry);
    }
    else
    {
        rslt = PQexecParams(self->conn,
                    qry,
                    params->len,
                    NULL,       // paramTypes not used
                    (const gchar **)params->pdata,// Array of params
                    NULL,       // list of parameter lengths -ignore
                    NULL,
                    0);         // returned formats - make all text
    }

    if (PQresultStatus (rslt) != PGRES_TUPLES_OK)  
    {
        report_err (self, PQresultErrorMessage(rslt));
        return NULL;
    }

    if (PQntuples (rslt) == 0)
    {
        report_err (self, "No data returned by query\n");
        return NULL;
    }

    // If we get here, then we have data.  Now convert to GPtrArray->hash
    data = g_ptr_array_new ();

    for (row = 0; row < PQntuples (rslt); row++)
    {
        GHashTable *colhash;
        gint col;

        colhash = g_hash_table_new (g_str_hash, g_str_equal);

        for (col = 0; col < PQnfields (rslt); col++)
        {
            gchar *coldata;

            coldata = PQgetvalue (rslt, row, col);
            g_hash_table_insert (colhash, PQfname (rslt, col),coldata);
        }
        
        g_ptr_array_add (data, colhash);
    }

    return data;
}

/**
 * style_print_pg_fromxmlfile:
 * @pgprnt: The StylePrintPg
 * @win: (nullable): The parent window - NULL if none
 * @qry: The query that will retrieve the data
 * @params: (nullable) (element-type utf8): Parameters for the query - NULL if none
 * @filename: The filename to open and read to get the xml definition for the printout.
 *
 * Print a tabular form where the xml definition for the output is
 * contained in a file.
 */

void
style_print_pg_fromxmlfile ( StylePrintPg *pgprnt,
                                 GtkWindow *win,
                               const gchar *qry,
                                 GPtrArray *params,
                                      char *filename)
{
    GPtrArray *data;

    style_print_table_set_wmain (STYLE_PRINT_TABLE(pgprnt), win);
    data = qry_get_data (pgprnt, qry, params);

    if (data)
    {
        style_print_table_from_xmlfile (STYLE_PRINT_TABLE(pgprnt), win,
                                                            data, filename);
    }
}

/**
 * style_print_pg_fromxmlstring:
 * @pgprnt: The #StylePrintPg
 * @win: (nullable): The parent window - NULL if none
 * @qry: The query that will retrieve the data 
 * @params: (nullable) (element-type utf8): Parameters for the query
 * @xmlstr: Pointer to the string containing the xml formatting
 *
 * Print a table where the definition for the format is contained in an
 * xml string.
 *
 */

void
style_print_pg_fromxmlstring ( StylePrintPg  *pgprnt,
                                   GtkWindow  *win,
                                 const gchar  *qry,
                                   GPtrArray  *params,
                                        char  *xmlstr)
{
    GPtrArray *data;

    style_print_table_set_wmain (STYLE_PRINT_TABLE(pgprnt), win);
    data = qry_get_data (pgprnt, qry, params);

    if (data)
    {
        style_print_table_from_xmlstring (STYLE_PRINT_TABLE(pgprnt), win,
                                                            data, xmlstr);
    }
}

/**
 * style_print_pg_new:
 *
 * Creates a new #StylePrintPg instance
 *
 * Returns: (transfer none): The new #StylePrintPg pointer
 */

StylePrintPg *
style_print_pg_new ()
{
    return g_object_new (STYLE_PRINT_TYPE_PG, NULL);
}
