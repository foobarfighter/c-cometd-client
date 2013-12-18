#include <stdlib.h>
#include <unistd.h>
#include "../cometd.h"

#define INTERNAL(loop) ((loop_internal*)loop->internal)

typedef struct {
  GThread* thread;
} loop_internal;

static unsigned int cometd_loop_gthread_start(cometd_loop* h);
static void cometd_loop_gthread_stop(cometd_loop* h);
static void cometd_loop_gthread_wait(cometd_loop* loop, long millis);
static void cometd_loop_gthread_destroy(cometd_loop* h);
static gpointer cometd_loop_gthread_run(gpointer cometd);

cometd_loop*
cometd_loop_gthread_new(cometd* cometd)
{
	cometd_loop* loop = cometd_loop_malloc(cometd);

	loop->start = cometd_loop_gthread_start;
	loop->stop = cometd_loop_gthread_stop;
	loop->destroy = cometd_loop_gthread_destroy;
  loop->wait = cometd_loop_gthread_wait;
  loop->internal = malloc(sizeof(loop_internal));

	return loop;
}

gpointer
cometd_loop_gthread_run(gpointer data)
{
  const cometd* h = (const cometd*) data;

  cometd_conn* conn = h->conn;
  cometd_loop* loop = h->loop;

  JsonNode *connect = NULL, *payload = NULL;

  long backoff = 0;

  guint attempt;
  for (attempt = 1; cometd_should_recv(h); ++attempt)
  {
    cometd_loop_wait(loop, backoff);

    payload = cometd_recv(h);
    connect = cometd_msg_extract_connect(payload);

    cometd_inbox_push(h->inbox, payload);
    cometd_process_msg(h, connect);

    if (cometd_should_retry_recv(h))
      backoff = cometd_get_backoff(h, attempt);
    else
      backoff = attempt = 0;

    json_node_free(payload);
    json_node_free(connect);
  }

  // If we've bailed from the loop, it's because we gave up
  // on the COMETD_UNCONNECTED state and we are no longer retrying or we
  // have intentially disconnected.
  //
  // If we gave up retrying, then this ensures that our state gets set correctly
  // and it should be a signal to the inbox queue to stop waiting.
  cometd_conn_set_state(conn, COMETD_DISCONNECTED);

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
cometd_loop_gthread_wait(cometd_loop* loop, long millis)
{
  g_return_if_fail(millis > 0);
  usleep(millis * 1000);
}

void
cometd_loop_gthread_destroy(cometd_loop* loop)
{
  free(loop->internal);
	free(loop);
}
