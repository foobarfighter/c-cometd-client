## Process this file with automake to produce Makefile.in

TESTS = check_cometd
check_PROGRAMS = check_cometd
check_cometd_SOURCES = check_cometd.c \
  test_helper.c \
  $(top_builddir)/src/cometd.h \
  $(top_builddir)/deps/libev-4.11/ev.h

check_cometd_CFLAGS = @CHECK_CFLAGS@ `pkg-config --cflags glib-2.0`
check_cometd_LDADD = $(top_builddir)/src/libcometd.la $(top_builddir)/deps/libev-4.11/libev.la @CHECK_LIBS@
