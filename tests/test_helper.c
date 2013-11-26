#include <check.h>
#include <stdio.h>
#include "cometd_json.h"
#include "../tests/test_helper.h"

static GList* log = NULL;

int
log_handler(const cometd* h, JsonNode* message)
{
  log = g_list_prepend(log, message);

  gchar* str = cometd_json_node2str(message);
  printf("== added message to log\n%s\n\n", str);
  g_free(str);
  return 0;
}

int
log_has_message(JsonNode* message)
{
  return 0;
}

guint
log_size(void)
{
  return g_list_length(log);
}

void
log_clear(void)
{
  log = NULL;
}
