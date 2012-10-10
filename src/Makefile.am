## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libcometd.la
libcometd_la_SOURCES = json.c json.h cometd.c cometd.h

bin_PROGRAMS = main
main_SOURCES = main.c
main_LDADD = libcometd.la
