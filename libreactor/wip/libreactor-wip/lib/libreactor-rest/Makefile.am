ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I$(srcdir)/src

SOURCE_FILES = \
src/reactor_rest/reactor_rest.c

HEADER_FILES = \
src/reactor_rest/reactor_rest.h

MAIN_HEADER_FILES = \
src/reactor_rest.h

lib_LTLIBRARIES = libreactor-rest.la
libreactor_rest_la_LDFLAGS = -static
libreactor_rest_la_SOURCES =  $(SOURCE_FILES) $(HEADER_FILES) $(MAIN_HEADER_FILES)

headerfilesdir = $(includedir)/reactor_rest
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = $(MAIN_HEADER_FILES)

maintainer-clean-local:
	rm -rf aclocal.m4 configure Makefile.in autotools m4
