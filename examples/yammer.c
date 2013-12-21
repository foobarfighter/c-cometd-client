#include <stdio.h>
#include <stdlib.h>
#include "cometd.h"
#include "exts/logger.h"
#include "exts/yammer.h"

static int handler(const cometd* h, JsonNode* msg)
{
  return COMETD_SUCCESS;
}

int main(void)
{
  char* token = getenv("TOKEN");
  char* feed  = getenv("FEED");
  char* url   = getenv("URL");

  if (token == NULL || feed == NULL || url == NULL)
  {
    printf("TOKEN, FEED and URL are required environment variables\n");
    return 1;
  }

  cometd* cometd = cometd_new();
  cometd_configure(cometd, COMETDOPT_URL, url);

  cometd_ext* logger = cometd_ext_logger_new();
  cometd_ext* yammer = cometd_ext_yammer_new(token);

  cometd_ext_add(&cometd->exts, yammer);
  cometd_ext_add(&cometd->exts, logger);

  char primary[512], secondary[512];

  sprintf(primary, "/feeds/%s/primary", feed);
  sprintf(secondary, "/feeds/%s/secondary", feed);

  cometd_connect(cometd);
  cometd_subscribe(cometd, primary, handler);
  cometd_subscribe(cometd, secondary, handler);
  cometd_listen(cometd);

  return 0;
}
