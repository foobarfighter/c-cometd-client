#include <stdlib.h>
#include "cometd.h"

cometd_loop*
cometd_loop_malloc(cometd* cometd)
{
  cometd_loop* loop = malloc(sizeof(cometd_loop));
  loop->cometd = cometd;
  return loop;
}

unsigned int
cometd_loop_start(cometd_loop* h)
{
	return h->start(h);
}

void
cometd_loop_stop(cometd_loop* h)
{
	h->stop(h);
}

void
cometd_loop_wait(cometd_loop* h, long millis)
{
  h->wait(h, millis);
}

void
cometd_loop_destroy(cometd_loop* h)
{
  h->destroy(h);
}
