bin_PROGRAMS = ppplogger-gtk

gladedir = $(datadir)/ppplogger-gtk/glade
glade_DATA = ppplog-gtk.glade

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(PACKAGE_CFLAGS) \
	`@MYSQL_CONFIG@ --cflags`


AM_CFLAGS =\
	-Wall -O2 -export-dynamic

ppplogger_gtk_SOURCES = \
        main.c


ppplogger_gtk_LDFLAGS = `@MYSQL_CONFIG@ --libs` -export-dynamic

ppplogger_gtk_LDADD = $(PACKAGE_LIBS)

EXTRA_DIST = $(glade_DATA)
