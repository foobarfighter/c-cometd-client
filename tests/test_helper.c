#include <check.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include "../tests/test_helper.h"

void
await(int result)
{
  int i;

  for (i = 0; i < 100000; ++i){
    if (!result)
      sleep(1);
  }
}

int
inbox_handler(const cometd* h, JsonNode* node)
{
  return 0;
}

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

