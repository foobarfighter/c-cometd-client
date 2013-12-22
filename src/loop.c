#include <stdlib.h>
#include "cometd.h"

cometd_loop*
cometd_loop_malloc(cometd* cometd)
{
  cometd_loop* loop = malloc(sizeof(cometd_loop));
  loop->cometd = cometd;

  loop->start = NULL;
  loop->stop = NULL;
  loop->wait = NULL;
  loop->destroy = NULL;

  return loop;
}

unsigned int
cometd_loop_start(cometd_loop* h)
{
  g_assert(h->start != NULL);

	return h->start(h);
}

void
cometd_loop_stop(cometd_loop* h)
{
  g_assert(h->stop != NULL);

	h->stop(h);
}

void
cometd_loop_wait(cometd_loop* h, long millis)
{
  g_assert(h->wait != NULL);

  h->wait(h, millis);
}

void
cometd_loop_destroy(cometd_loop* h)
{
  if (h->destroy != NULL)
    h->destroy(h);

  free(h);
}
