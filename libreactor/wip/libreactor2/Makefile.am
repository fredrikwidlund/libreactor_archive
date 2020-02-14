ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
#AM_CFLAGS = -std=gnu11 -O0 -I$(srcdir)/src
AM_CFLAGS = -std=gnu11 -O3 -march=native -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/dynamic/buffer.c \
src/dynamic/vector.c \
src/url/url.c \
src/reactor/reactor_user.c \
src/reactor/reactor_desc.c \
src/reactor/reactor_core.c \
src/reactor/reactor_async.c \
src/reactor/reactor_timer.c \
src/reactor/reactor_stream.c \
src/reactor/reactor_resolver.c \
src/reactor/reactor_tcp.c \
src/reactor/reactor_http_parser.c \
src/reactor/reactor_http.c \
src/reactor/reactor_http_client.c \
src/reactor/reactor_http_server.c

HEADER_FILES = \
src/reactor/reactor_core.h

MAIN_HEADER_FILES = \
src/reactor.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor.la
libreactor_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir = $(includedir)/reactor
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = $(MAIN_HEADER_FILES)

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor-?.?.?

### unit tests ###

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src
CHECK_LDADD = -L. -lreactor_test -lcmocka

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS = test/reactor_core
test_reactor_core_CFLAGS = $(CHECK_CFLAGS)
test_reactor_core_LDADD = $(CHECK_LDADD)
test_reactor_core_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_core_SOURCES = test/reactor_core.c

TESTS = $(check_PROGRAMS)

## examples ###

bin_PROGRAMS = \
example/socketpair_stream \
example/client \
example/server \
example/http_client_low_level \
example/http_server_low_level \
example/http_client \
example/http_server
#example/http_proxy

example_socketpair_stream_LDADD = ./libreactor.la
example_client_LDADD = ./libreactor.la
example_server_LDADD = ./libreactor.la
example_http_client_low_level_LDADD = ./libreactor.la
example_http_server_low_level_LDADD = ./libreactor.la
example_http_client_LDADD = ./libreactor.la
example_http_server_LDADD = ./libreactor.la

#example_http_proxy_LDADD = ./libreactor.la

CLEANFILES = {.,src/reactor,src/dynamic,src/url,test}/*.{gcno,gcda,gcov}
