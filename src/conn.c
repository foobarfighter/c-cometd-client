#include "conn.h"
#include <stdlib.h>

cometd_conn*
cometd_conn_new(void)
{
  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_UNINITIALIZED;
  conn->transport = NULL;
  conn->_msg_id_seed = 0;

  conn->advice = NULL;

  return conn;
}

cometd_advice*
cometd_advice_new(void)
{
  cometd_advice* advice = malloc(sizeof(cometd_advice));
  advice->reconnect = COMETD_RECONNECT_NONE;
  advice->interval  = 0;
  return advice;
}

void
cometd_advice_destroy(cometd_advice* advice)
{
  free(advice);
}

cometd_advice*
cometd_conn_advice(const cometd_conn* conn)
{
  return conn->advice;
}

/**
 * Frees the existing advice if there is advice and sets the connection's
 * advice to "advice".  The pointer to which advice points is to be owned
 * by the connection and should not be free'd by the caller.
 */
void
cometd_conn_take_advice(cometd_conn* conn, const cometd_advice* advice)
{
  if (conn->advice != NULL)
    cometd_advice_destroy(conn->advice);
  conn->advice = advice;
}

gboolean
cometd_advice_is_handshake(const cometd_advice* advice)
{
  if (advice == NULL)
    return FALSE;

  return advice->reconnect == COMETD_RECONNECT_HANDSHAKE;
}

gboolean
cometd_advice_is_none(const cometd_advice* advice)
{
  if (advice == NULL)
    return FALSE;

  return advice->reconnect == COMETD_RECONNECT_NONE;
}
