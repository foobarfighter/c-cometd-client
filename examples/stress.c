// Used this to stress test the client lib and look for memory leaks.
// This doesn't cover all of the functionality in the library but it exersices
// quite a bit of it.

#include <stdio.h>
#include "cometd.h"
#include "exts/logger.h"

static int handler(const cometd* h, JsonNode* msg)
{
  return COMETD_SUCCESS;
}

static gpointer cometd_listen_thread_run(gpointer data)
{
  cometd_listen((cometd*) data);
}

static GThread* cometd_listen_async(cometd* cometd)
{
  GThread* t = g_thread_new("cometd_listen_thread_run",
                            cometd_listen_thread_run,
                            cometd);
  return t;
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

  GThread* thread = cometd_listen_async(cometd);

  JsonNode* n = cometd_json_str2node("{ \"hey\": \"now\" }");

  guint i;
  for (i = 0; i < 500000; ++i)
    cometd_publish(cometd, "/foo/bar/baz", n);
  json_node_free(n);

  cometd_disconnect(cometd, 0);
  g_thread_join(thread);
  cometd_destroy(cometd);

  printf("sleeping now...\n");
  sleep(1000);

  return 0;
}
