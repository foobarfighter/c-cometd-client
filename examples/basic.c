#include <stdio.h>
#include "cometd.h"
#include "cometd/exts/logger.h"

static int handler(const cometd* h, JsonNode* msg)
{
  return COMETD_SUCCESS;
}

int main(void)
{
  cometd* cometd = cometd_new();

  cometd_configure(cometd, COMETDOPT_URL, "http://localhost:8089/cometd");
  cometd_configure(cometd, COMETDOPT_MAX_BACKOFF, 5000);

  cometd_ext* logger = cometd_ext_logger_new();
  cometd_ext_add(&cometd->exts, logger);

  cometd_connect(cometd);
  cometd_subscribe(cometd, "/foo/bar/baz", handler);
  cometd_listen(cometd);

  return 0;
}
