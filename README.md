# Moved to https://gitlab.com/debmint/gtk-tableprint


This is a library to add the ability to print hardcopy reports in
tabular form.  It will do the calculations for print position and
do the rendering.  In order to do so, the formatting will need to
be passed to the library in the form of an xml document, which can
either be embedded in the program or read from an external file.
Please point your browser to the documentation in the subdirectory
of this distribution
           docs/reference/html/index.html
for details of the format for this xml data.

This library implements GObject-Introspetion, and so it can be used
by any programming language that supports GObject-Introspection.

===================================================================

### INSTALLATION NOTES

Please read the file "INSTALL" for general instructions on how to
build and install the program.  In addition to the instructions in
that file whey you run "./configure", you need to include the option

                          --enable-gtk-doc

to enable the building of the Gtk Documentation, as this feature is
disabled by default.

===================================================================

### SELECTING COMPONENTS TO BUILD

The primary module - which is _always_ built, is StylePrintTable.
Additional modules for use with specific databases, are optional,
but by default all are built.

These additional modules (currently StylePrint::Pg and StylePrint::My
add the ability to directly access a Postgresql or MySQL database
respectively bypassing the need to program the database access functions
in the application.

It can be determined which are built by specifying certain options when
./configure is run.

            --with-mysql[=(yes|no|/path/to/config-prog)]

This controls whether StylePrintMy for mysql/mariaDB is built.  If a parameter
is not included with this option, the default behavior of "yes" is implied,
and causes it to try "mysql_config".  If "mysql_config" is not present on
the system, the path to mariadb_config must be provided, or a symlink in
the $PATH environment pointing to "mariadb_config" having the name
"mysql_config" must be provided.  This can be either /path/to/mysql_config
or /path/to/mariadb_config.  If the config is "mysql_config" and is in your
PATH list, there is no need to even specify this option.

            --with-postgresql[=(yes|no|/path/to/pg_config)]

This controls whether StylePrintPg - for postgresql is build.  The parameter
usage is the sames as for --with-mysql, and everything said above applies to
this option (except for an alternate database).
