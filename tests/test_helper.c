#include <check.h>
#include "../tests/test_helper.h"

void await(int result){
  for (int i = 0; i < 100000; ++i){
    if (!result){
      sleep(1);
  }
}

