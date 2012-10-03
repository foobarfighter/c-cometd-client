## Process this file with automake to produce Makefile.in

TESTS = check_cometd
check_PROGRAMS = check_cometd
check_cometd_SOURCES = check_cometd.c $(top_builddir)/src/cometd.h
check_cometd_CFLAGS = @CHECK_CFLAGS@
check_cometd_LDADD = $(top_builddir)/src/libcometd.la @CHECK_LIBS@
