#include <stdio.h>
#include "cometd.h"

int meta_handler(const cometd* cometd, JsonNode* node)
{
  char* channel = cometd_msg_channel(node);
  char* json = cometd_json_node2str(node);

  printf("%s: %s\n", channel, json);
  
  free(json);
  free(channel);

  return COMETD_SUCCESS;
}

int main(void)
{
  cometd* cometd = cometd_new();
  cometd_configure(cometd, COMETDOPT_URL, "http://localhost:8089/cometd");
  cometd_configure(cometd, COMETDOPT_MAX_BACKOFF, 1000);

  cometd_subscribe(cometd, "/meta/**", meta_handler);
  cometd_connect(cometd);
  cometd_listen(cometd);

  return 0;
}
