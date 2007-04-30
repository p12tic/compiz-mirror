## Process this file with automake to produce Makefile.in

INCLUDES = @GLIB_CFLAGS@ \
	   @GCONF_CFLAGS@ \
	   @CCS_CFLAGS@ 

libgconf_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries)
libgconf_la_LIBADD = @CCS_LIBS@ @GCONF_LIBS@ @GLIB_LIBS@
libgconf_la_SOURCES = gconf.c
backenddir = $(libdir)/ccs/backends

METASOURCES = AUTO

backend_LTLIBRARIES = libgconf.la
