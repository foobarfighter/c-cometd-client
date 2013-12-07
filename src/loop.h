#ifndef COMETD_LOOP_H
#define COMETD_LOOP_H

#include "cometd.h"

typedef struct _cometd_loop cometd_loop;

typedef unsigned int (*CometdLoopStartFunc) (cometd_loop* h);
typedef void (*CometdLoopStopFunc) (cometd_loop* h);
typedef void (*CometdLoopDestroyFunc) (cometd_loop* h);

struct _cometd_loop {
  cometd*               cometd;
  void*                 internal;

	CometdLoopStartFunc   start;
	CometdLoopStopFunc    stop;
  CometdLoopDestroyFunc destroy;
};

#define cometd_loop_new(type, cometd) cometd_loop_##type##_new(cometd)
unsigned int cometd_loop_start (cometd_loop* h);
void cometd_loop_stop (cometd_loop* h);
void cometd_loop_destroy (cometd_loop* h);

/******** loop types ********/

cometd_loop* cometd_loop_gthread_new(cometd* cometd);

/******** /loop types ********/

#endif /* COMETD_LOOP_H */
