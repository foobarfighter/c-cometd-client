#ifndef COMETD_LOOP_H
#define COMETD_LOOP_H

#include "../cometd.h"

typedef unsigned int (*CometdLoopStartFunc) (cometd_loop* h);
typedef void (*CometdLoopStopFunc) (cometd_loop* h);
typedef void (*CometdLoopDestroyFunc) (cometd_loop* h);
typedef void (*CometdLoopWaitFunc) (cometd_loop* h, long millis);

struct _cometd_loop {
  cometd*               cometd;
  void*                 internal;

	CometdLoopStartFunc   start;
	CometdLoopStopFunc    stop;
  CometdLoopWaitFunc    wait;
  CometdLoopDestroyFunc destroy;
};

#define cometd_loop_new(type, cometd) cometd_loop_##type##_new(cometd)
cometd_loop* cometd_loop_malloc(cometd* cometd);
unsigned int cometd_loop_start (cometd_loop* h);
void cometd_loop_wait (cometd_loop* h, long millis);
void cometd_loop_stop (cometd_loop* h);
void cometd_loop_destroy (cometd_loop* h);

/******** loop types ********/

cometd_loop* cometd_loop_gthread_new(cometd* cometd);

/******** /loop types ********/

#endif /* COMETD_LOOP_H */
