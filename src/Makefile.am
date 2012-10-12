## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libcometd.la
libcometd_la_CFLAGS = `pkg-config --cflags glib-2.0` # needed to compile libcometd
libcometd_la_LIBS = `pkg-config --libs glib-2.0`
libcometd_la_SOURCES = json.c json.h cometd.c cometd.h

bin_PROGRAMS = main
main_SOURCES = main.c
main_LDADD = libcometd.la
main_CFLAGS = `pkg-config --cflags glib-2.0`         # needed because cometd.h includes glib.h (weak)
#main_LIBS = -lglib-2.0
