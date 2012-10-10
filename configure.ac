# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.60])
AC_INIT([libcometd], [0.1], [bob.remeika@gmail.com])

# unique source file --- primitive safety check 
AC_CONFIG_SRCDIR([src/cometd.c])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([cfg])

# fairly severe build strictness
# change foreign to gnu or gnits to comply with gnu standards
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.9.6])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# This macro is defined in check.m4 and tests if check.h and
# libcheck.a are installed in your system. It sets CHECK_CFLAGS and
# CHECK_LIBS accordingly.  
#  AM_PATH_CHECK([MINIMUM-VERSION,
#                [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
AM_PATH_CHECK()

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_LIB(glib-2.0, g_list_append)
AC_CHECK_LIB(curl, curl_easy_init)

# Output files
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])

AC_OUTPUT
