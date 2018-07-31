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
 * styleprintmysql.c - Code for GtkTblPrintPg library function              $
 * $Id::                                                                    $
 * ************************************************************************ */

#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "styleprintmy.h"

/**
 * SECTION:styleprintmy
 * @short_description: MySql interface to #StylePrintTable
 * @Title: StylePrintMy
 * @See_also: #StylePrintTable
 *
 * #StylePrintTable is a utility to print data in a tabular form.  However,
 * it is generic, and its data is provided in a #GPtrArray containing
 * #GHashTable 's representing each row of data.
 *
 * #StylePrintMy enables one to retrieve the data from a MySql database
 * and organize this data into a form recognizable by #StylePrintTable.
 */

struct _StylePrintMy
{
    StylePrintTable parent_instance;

    /*< private >*/
    MYSQL     MYconn;
    GPtrArray *qryParams;
};

G_DEFINE_TYPE(StylePrintMy, style_print_my, STYLE_PRINT_TYPE_TABLE)

static GPtrArray * qry_get_data_direct (StylePrintMy *self,
                                         const gchar *qry,
                                           GPtrArray *params);

static GPtrArray * qry_get_data_params (StylePrintMy *self,
                                         const gchar *qry,
                                           GPtrArray *params);
void
style_print_my_init (StylePrintMy *my)
{
    //my->MYconn = NULL;
    my->qryParams = NULL;
    //my->myresult = NULL;
}

void
style_print_my_class_init (StylePrintMyClass *my)
{
//    StylePrintTableClass *table_class = (StylePrintTableClass *)my;
//    table_class->from_xmlfile = style_print_my_from_xmlfile;
//    table_class->from_xmlstring = style_print_my_from_xmlstring;
}

/* Error reporting routine.
 * If self->w_main is defined, the message will be reported in a dialog box
 * else it is sent to stderr
 */

static void
report_err (StylePrintMy *self, const char *message)
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
 * style_print_my_connect:
 * @self: The #StylePrintMy *
 * @host: (nullable): The host - can be either hostname or IP address
 * @user: (nullable): The username
 * @passwd: (nullable): The user's password
 * @db: (nullable): The name of the database to connect to.
 * @port: The port on which to connect - The default if not specified
 * @unix_socket: (nullable): the unix socket name
 * @client_flag: Flags for connection - Not often needed
 *
 * Establishes a connection to the database.  This must be done before any
 * queries or commands are sent to the database.
 *
 * Note: See mySQL documentation for details on connection parameters
 *
 * Free the string upon return if need be.
 *
 * Returns: 1 on success, 0 on failure.
 *
 */

gint
style_print_my_connect (StylePrintMy *self,
                        const gchar  *host,
                        const gchar  *user,
                        const gchar  *passwd,
                        const gchar  *db,
                        unsigned int  port,
                        const gchar  *unix_socket,
                        unsigned long client_flag)
{
    // FIXME: Check for connection;
//    if (self->MYconn)
//    {
//        mysql_close (self->MYconn);
//        self->MYconn = NULL;
//    }

    mysql_init (&(self->MYconn));

//    if (!self-MYconn)
//    {
//        fprintf(stderr,"Could not init MySQL connection.. aborting\n");
//        return NULL;
//    }

    if (!mysql_real_connect (&(self->MYconn), host, user,
                passwd, db, port, unix_socket, client_flag))
    {
        report_err (self, mysql_error (&(self->MYconn)));
        return 0;
    }

    return 1;
}

/**
 * style_print_my_appendParam:
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
style_print_my_appendParam (StylePrintMy *self, const gchar *param)
{
    if ( ! self->qryParams)
    {
        self->qryParams = g_ptr_array_new();
    }

    g_ptr_array_add (self->qryParams, (gpointer)param);
}

/**
 * style_print_my_select_db:
 * @self: A #StylePrintMy pointer
 * @db: The name of the database to connect to
 *
 * Sets (or changes) the database to which you want to use.
 *
 * Returns: 0 on success, non-zero on failure.
 */

gint
style_print_my_select_db (StylePrintMy *self, const gchar *db)
{
    gint status;

    if ((status = mysql_select_db (&(self->MYconn), db)))
    {
        report_err (self, mysql_error (&(self->MYconn)));
    }

    return status;
}

/**
 * style_print_my_do:
 * @self: The #StylePrintMy *
 * @qry: The query to execute
 *
 * Execute a non-data-returning query on the database
 * If parameters are used, they must be previously set
 */

void
style_print_my_do (StylePrintMy *self, const gchar * qry)
{


    if (mysql_query (&(self->MYconn), qry))
    {
        report_err (self, mysql_error (&(self->MYconn)));
        return;
    }

//    if (priv->qryParams)
//    {
//        g_ptr_array_free (priv->qryParams, FALSE);
//        priv->qryParams = NULL;
//    }

    //PQclear (rslt);     // Simply discard this result
}

/* GDestroyNotify function to free the data Array */
void
free_data_array (gpointer ht)
{
    g_hash_table_destroy ((GHashTable *)ht);
}

/* ==================================================================== *
 * Retrieve data from the database and convert it to format expected    *
 * by StylePrintTable where query is a prepared statement with separate *
 * parameters.                                                          *
 * ==================================================================== */

static GPtrArray *
qry_get_data_params (StylePrintMy *self, const gchar *qry, GPtrArray *params)
{
    MYSQL_STMT   *stmt;
    MYSQL_BIND   *inBinds, *outBinds;
    MYSQL_RES    *prepare_meta_result;
    unsigned int  param_count, numCols;
    GPtrArray    *colnames,
                 *data;
    GPtrArray    *inLens = g_ptr_array_new_with_free_func(g_free);
    GPtrArray    *is_null = g_ptr_array_new_with_free_func(g_free);
    GPtrArray    *length = g_ptr_array_new_with_free_func(g_free);
    GPtrArray    *error = g_ptr_array_new_with_free_func(g_free);

     int curCol;

//    if ( ! self->MYconn)
//    {
//        fprintf (stderr, "No connection to database\n");
//        return NULL;
//    }

    stmt = mysql_stmt_init (&(self->MYconn));

    if (!stmt)
    {
        fprintf(stderr, "mysql_stmt_init(): out of memory\n");
        return NULL;
    }

    if (mysql_stmt_prepare (stmt, qry, strlen(qry)))
    {
        fprintf (stderr, "mysql_stmt_prepare(), SELECT failed\n");
        fprintf (stderr, " %s\n", mysql_stmt_error (stmt));
        return NULL;
    }

    param_count = mysql_stmt_param_count (stmt);

    if (! param_count)
    {
        fprintf (stderr, " Statement returns no rows!\n");
        return NULL;
    }

    prepare_meta_result = mysql_stmt_result_metadata (stmt);

    if (!prepare_meta_result)
    {
        report_err (self,
                "mysql_stmt_result_metadata(): no meta information returned\n");
        return NULL;
    }

    numCols = mysql_num_fields (prepare_meta_result);
    inBinds = g_malloc (params->len * sizeof(MYSQL_BIND));
    memset (inBinds, 0, params->len * sizeof(MYSQL_BIND));

    for (curCol = 0; curCol < params->len; curCol++)
    {
        inBinds[curCol].buffer_type = MYSQL_TYPE_STRING;
        inBinds[curCol].buffer = (char *)g_ptr_array_index(params, curCol);
        inBinds[curCol].is_null = 0;
        g_ptr_array_add (inLens, g_malloc(sizeof(long)));
        inBinds[curCol].length = g_ptr_array_index (inLens, curCol);
        *(inBinds[curCol].length) = strlen(g_ptr_array_index(params, curCol));
    }

    if (mysql_stmt_bind_param (stmt, inBinds))
    {
        char msg[500];
        snprintf (msg, sizeof(msg), "mysql_stty_bind_params() failed:\n %s",
                mysql_stmt_error (stmt));
        report_err (self, msg);
        return NULL;
    }

    if (mysql_stmt_execute (stmt))
    {
        char msg[500];
        snprintf(msg, sizeof(msg),
            "mysql_stmt_execute error:\n%s", mysql_stmt_error (stmt));
        report_err (self, msg);
        return NULL;
    }

    // NOTE: mysql_store_result might be used to determine this...
    if (mysql_field_count (&(self->MYconn)) == 0)
    {
        report_err (self, "No data returned by query");
        return NULL;
    }

    /* Bind the result buffers for all columns before fetching them */

    outBinds = g_malloc (numCols * sizeof (MYSQL_BIND));
    memset (outBinds, 0, numCols * sizeof (MYSQL_BIND));

    colnames = g_ptr_array_new_with_free_func (g_free);
    //colnames = g_ptr_array_new_with_free_func ((GDestroyNotify)free_gchararray);

    for (curCol = 0; curCol < numCols; curCol++)
    {
        MYSQL_FIELD *myfld = mysql_fetch_field (prepare_meta_result);

        g_ptr_array_add (colnames, g_strdup (myfld->name));
        outBinds[curCol].buffer_type = MYSQL_TYPE_STRING;

        if (((myfld->length) < 200) && (myfld->length > 20))
        {
            outBinds[curCol].buffer_length = myfld->length;
        }
        else
        {
            outBinds[curCol].buffer_length = 200;
        }

        outBinds[curCol].buffer = g_malloc(outBinds[curCol].buffer_length);

        g_ptr_array_add (is_null, g_malloc(sizeof(my_bool)));
        outBinds[curCol].is_null = g_ptr_array_index (is_null, curCol);

        g_ptr_array_add (length, g_malloc(sizeof(long)));
        outBinds[curCol].length = g_ptr_array_index (length, curCol);

        g_ptr_array_add (error, g_malloc(sizeof(my_bool)));
        outBinds[curCol].error = g_ptr_array_index (error, curCol);
    }

    if (mysql_stmt_bind_result (stmt, outBinds))
    {
        report_err (self, mysql_stmt_error (stmt));
        return NULL;
    }

    data = g_ptr_array_new_with_free_func ((GDestroyNotify)free_data_array);

    while (!mysql_stmt_fetch (stmt))
    {
        GHashTable *colhash;

        colhash = g_hash_table_new_full (g_str_hash,
                                         g_str_equal, g_free, g_free);
                                         //(GDestroyNotify)free_gchararray,
                                         //(GDestroyNotify)free_gchararray);

        for (curCol = 0; curCol < numCols; curCol++)
        {
            g_hash_table_insert (colhash,
                        g_strdup (g_ptr_array_index (colnames, curCol)),
                        *(outBinds[curCol].is_null) ? g_strdup ("") :
                        g_strdup ((const gchar *)outBinds[curCol].buffer));
        }

        g_ptr_array_add (data, colhash);
    }

    mysql_free_result (prepare_meta_result);
    g_ptr_array_free (inLens, TRUE);
    g_ptr_array_free (is_null, TRUE);
    g_ptr_array_free (length, TRUE);
    g_ptr_array_free (error, TRUE);
    g_ptr_array_free (colnames, TRUE);
    g_free (inBinds);

    // Free the Buffer areas for outBinds
    for (curCol = 0; curCol < numCols; curCol++)
    {
        g_free (outBinds[curCol].buffer);
    }

    g_free (outBinds);

    if (mysql_stmt_close (stmt))
    {
        report_err (self, mysql_error (&(self->MYconn)));
    }

    mysql_close (&(self->MYconn));
    //self->MYconn = NULL;
    return data;
}

/* ==================================================================== *
 * Retrieve data from the database and convert it to format expected    *
 * by StylePrintTable where query is direct inline string with no       *
 * parameters.                                                          *
 * ==================================================================== */

static GPtrArray *
qry_get_data_direct (StylePrintMy *self, const gchar *qry, GPtrArray *params)
{
    MYSQL_RES   *rslt;
    MYSQL_ROW    myRow;
    MYSQL_FIELD *myField;
    GPtrArray   *colnames,
                *data;
    unsigned int numCols;

//    if ( ! self->MYconn)
//    {
//        fprintf (stderr, "No connection to database\n");
//        return NULL;
//    }

    if (mysql_query (&(self->MYconn), qry))
    {
        report_err (self, mysql_error (&(self->MYconn)));
        return NULL;
    }

    // NOTE: mysql_store_result might be used to determine this...
    if (mysql_field_count (&(self->MYconn)) == 0)
    {
        report_err (self, "No data returned by query");
        return NULL;
    }

    // If we get here, then we have data.  Now convert to GPtrArray->hash
    rslt = mysql_use_result (&(self->MYconn));
    numCols = mysql_num_fields (rslt);

    // Populate the column names array

    colnames = g_ptr_array_new_with_free_func (g_free);

    while ((myField = mysql_fetch_field (rslt)))
    {
        g_ptr_array_add (colnames, g_strdup (myField->name));
    }

    data = g_ptr_array_new_with_free_func ((GDestroyNotify)free_data_array);

    while ((myRow = mysql_fetch_row (rslt)))
    {
        GHashTable *colhash;
        gint col;

        colhash = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_free);

        for (col = 0; col < numCols; col++)
        {
            gchar *colname;
            colname = g_strdup (g_ptr_array_index (colnames, col));

            g_hash_table_insert (colhash, colname,
                            g_strdup ((const gchar *)myRow[col]));
        }

        g_ptr_array_add (data, colhash);
    }

    mysql_free_result (rslt);
    mysql_close (&(self->MYconn));
    //self->MYconn = NULL;
    return data;
}

/**
 * style_print_my_fromxmlfile:
 * @myprnt: The StylePrintMy
 * @win: (nullable): The parent window - NULL if none
 * @qry: The query that will retrieve the data
 * @params: (nullable) (element-type utf8): Parameters for the query
 * @filename: The filename to open and read to get the xml definition for the printout.
 *
 * Print a tabular form where the xml definition for the output is
 * contained in a file.
 */

void
style_print_my_fromxmlfile ( StylePrintMy *myprnt,
                                 GtkWindow *win,
                               const gchar *qry,
                                 GPtrArray *params,
                                      char *filename)
{
    GPtrArray *data;

    style_print_table_set_wmain (STYLE_PRINT_TABLE(myprnt), win);

    if (params)
    {
        data = qry_get_data_params (myprnt, qry, params);
    }
    else
    {
        data = qry_get_data_direct (myprnt, qry, NULL);
    }

    if (data)
    {
        style_print_table_from_xmlfile (STYLE_PRINT_TABLE(myprnt), win,
                                                            data, filename);
        g_ptr_array_free (data, TRUE);
    }

    mysql_close (&(myprnt->MYconn));
}

/**
 * style_print_my_fromxmlstring:
 * @myprnt: The #StylePrintMy
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
style_print_my_fromxmlstring ( StylePrintMy *myprnt,
                                   GtkWindow *win,
                                 const gchar *qry,
                                   GPtrArray *params,
                                        char *xmlstr)
{
    GPtrArray *data;

    style_print_table_set_wmain (STYLE_PRINT_TABLE(myprnt), win);

    if (params)
    {
        data = qry_get_data_params (myprnt, qry, params);
    }
    else
    {
        data = qry_get_data_direct (myprnt, qry, NULL);
    }

    if (data)
    {
        style_print_table_from_xmlstring (STYLE_PRINT_TABLE(myprnt), win,
                                                            data, xmlstr);
        g_ptr_array_free (data, TRUE);
    }

    mysql_close (&(myprnt->MYconn));
}

/**
 * style_print_my_new:
 *
 * Creates a new #StylePrintMy instance
 *
 * Returns: (transfer none): The new #StylePrintMy pointer
 */

StylePrintMy *
style_print_my_new ()
{
    return g_object_new (STYLE_PRINT_TYPE_MY, NULL);
}
