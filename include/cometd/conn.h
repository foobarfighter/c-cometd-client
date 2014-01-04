#ifndef COMETD_CONN_H
#define COMETD_CONN_H

#include "../cometd.h"

typedef enum {
  COMETD_RECONNECT_NONE = 0,
  COMETD_RECONNECT_RETRY,
  COMETD_RECONNECT_HANDSHAKE
} cometd_reconn_advice;

struct _cometd_advice {
  cometd_reconn_advice reconnect;
  long interval;
};

struct _cometd_conn {
  long  state;
  long  msg_id_seed;
  char* client_id;
  cometd_transport*  transport;
  cometd_advice*     advice;
};

cometd_conn*   cometd_conn_new(void);
const cometd_advice* cometd_conn_advice(const cometd_conn* conn);
void           cometd_conn_destroy(cometd_conn* conn);
void           cometd_conn_set_transport(cometd_conn* conn, cometd_transport* t);
const char*    cometd_conn_client_id(const cometd_conn* conn);
void           cometd_conn_set_client_id(cometd_conn* conn, const char* id);
int            cometd_conn_state(const cometd_conn* conn);
void           cometd_conn_set_state(cometd_conn* conn, int status);
int            cometd_conn_is_state(const cometd_conn* conn, int status);

cometd_advice* cometd_advice_new(void);
void           cometd_advice_destroy(cometd_advice* advice);
gboolean       cometd_advice_is_handshake(const cometd_advice* advice);
gboolean       cometd_advice_is_none(const cometd_advice* advice);
gboolean       cometd_advice_is_retry(const cometd_advice* advice);
void           cometd_conn_take_advice(cometd_conn* conn, cometd_advice* advice);


#endif /* COMETD_CONN_H */
