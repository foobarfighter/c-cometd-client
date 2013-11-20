#include <check.h>
#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "check_cometd.h"

void error_handler(int sig);

void
error_handler(int sig)
{
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int
main (void)
{
  signal(SIGSEGV, error_handler);
  g_type_init();

  int number_failed;

  SRunner *sr = srunner_create (make_cometd_unit_suite ());
  srunner_add_suite (sr, make_cometd_integration_suite ());
  srunner_add_suite (sr, make_http_integration_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
