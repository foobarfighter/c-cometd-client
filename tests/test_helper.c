#include <check.h>
#include "../tests/test_helper.h"

void await(int result){
  int i;

  for (i = 0; i < 100000; ++i){
    if (!result)
      sleep(1);
  }
}

