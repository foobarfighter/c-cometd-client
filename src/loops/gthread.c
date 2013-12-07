#include <stdlib.h>
#include "../cometd.h"

#define INTERNAL(loop) ((loop_internal*)loop->internal)

typedef struct {
  GThread* thread;
} loop_internal;

static unsigned int cometd_loop_gthread_start(cometd_loop* h);
static void cometd_loop_gthread_stop(cometd_loop* h);
static void cometd_loop_gthread_destroy(cometd_loop* h);
static gpointer cometd_loop_gthread_run(gpointer cometd);

cometd_loop*
cometd_loop_gthread_new(cometd* cometd)
{
	cometd_loop* loop = malloc(sizeof(cometd_loop));

  loop->cometd = cometd;
	loop->start = cometd_loop_gthread_start;
	loop->stop = cometd_loop_gthread_stop;
	loop->destroy = cometd_loop_gthread_destroy;

  loop->internal = malloc(sizeof(loop_internal));

	return loop;
}

gpointer
cometd_loop_gthread_run(gpointer data)
{
  JsonNode* node;

  const cometd* h = (const cometd*) data;
  while (!(cometd_conn_is_status(h, COMETD_DISCONNECTED)) &&
         (node = cometd_recv(h)) != NULL)
  {
    cometd_process_payload(h, node);
    json_node_free(node);
  }
  return NULL;
}

unsigned int
cometd_loop_gthread_start(cometd_loop* loop)
{
  INTERNAL(loop)->thread = g_thread_new("cometd_loop_gthread_run",
                                                cometd_loop_gthread_run,
                                                loop->cometd);
	return 0;
}

void
cometd_loop_gthread_stop(cometd_loop* loop)
{
	g_thread_join(INTERNAL(loop)->thread);
  INTERNAL(loop)->thread = NULL;
}

void
cometd_loop_gthread_destroy(cometd_loop* loop)
{
  free(loop->internal);
	free(loop);
}
