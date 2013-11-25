#include <check.h>
#include "../tests/test_helper.h"

static char log[255]; 

int
log_handler(const cometd* h, JsonNode* message)
{
  return 1;
}

int
log_has_message(JsonNode* message)
{
  return 1;
}


