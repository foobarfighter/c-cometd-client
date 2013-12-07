#include "cometd.h"

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
cometd_loop_destroy(cometd_loop* h)
{
  h->destroy(h);
}
