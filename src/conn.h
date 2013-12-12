#ifndef COMETD_CONN_H
#define COMETD_CONN_H

#include "cometd.h"

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
  long               state;
  long               _msg_id_seed;
  cometd_transport*  transport;
  cometd_advice*     advice;
  
  char client_id[COMETD_MAX_CLIENT_ID_LEN];
};

cometd_conn* cometd_conn_new(void);
void         cometd_conn_set_transport(cometd_conn* conn, cometd_transport* t);
const char*  cometd_conn_client_id(const cometd_conn* conn);
void         cometd_conn_set_client_id(const cometd_conn* conn, const char* id);
long         cometd_conn_status(const cometd_conn* conn);
void         cometd_conn_set_status(cometd_conn* conn, long status);
long         cometd_conn_is_status(const cometd_conn* conn, long status);
void         cometd_conn_clear_status(cometd_conn* conn);


cometd_advice* cometd_advice_new(void);
void           cometd_advice_destroy(cometd_advice* advice);
gboolean       cometd_advice_is_handshake(const cometd_advice* advice);
gboolean       cometd_advice_is_none(const cometd_advice* advice);
void           cometd_conn_take_advice(cometd_conn* conn, cometd_advice* advice);

#endif /* COMETD_CONN_H */
