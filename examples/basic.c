#include <stdio.h>
#include "cometd.h"

void print_log(const char* type, JsonNode* node)
{
  char* channel = cometd_msg_channel(node);
  char* json = cometd_json_node2str(node);

  printf("%s %s: %s\n", type, channel, json);
  
  free(json);
  free(channel);
}

static void logger_incoming(const cometd* h, JsonNode* msg)
{
  print_log("incoming", msg);
}

static void logger_outgoing(const cometd* h, JsonNode* msg)
{
  print_log("outgoing", msg);
}

static int handler(const cometd* h, JsonNode* msg)
{
  print_log("handler", msg);
  return COMETD_SUCCESS;
}

int main(void)
{
  cometd* cometd = cometd_new();

  cometd_configure(cometd, COMETDOPT_URL, "http://localhost:8089/cometd");
  cometd_configure(cometd, COMETDOPT_MAX_BACKOFF, 5000);

  cometd_ext* logger = cometd_ext_new();
  logger->incoming = logger_incoming;
  logger->outgoing = logger_outgoing;

  cometd_ext_add(&cometd->exts, logger);
  cometd_connect(cometd);
  cometd_subscribe(cometd, "/foo/bar/baz", handler);
  cometd_listen(cometd);

  return 0;
}
