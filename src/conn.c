#include "conn.h"
#include <stdlib.h>
#include <string.h>

cometd_conn*
cometd_conn_new(void)
{
  cometd_conn* conn = malloc(sizeof(cometd_conn));

  conn->client_id = NULL;
  conn->transport = NULL;
  conn->advice    = NULL;
  conn->msg_id_seed = 0;
  conn->state = COMETD_UNINITIALIZED;

  return conn;
}

void
cometd_conn_destroy(cometd_conn* conn)
{
  if (conn->client_id != NULL)
    free(conn->client_id);

  cometd_advice_destroy(conn->advice);
  free(conn);
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
  if (advice != NULL)
    free(advice);
}

const cometd_advice*
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
cometd_conn_take_advice(cometd_conn* conn, cometd_advice* advice)
{
  cometd_advice_destroy(conn->advice);
  conn->advice = advice;
}

gboolean
cometd_advice_is_handshake(const cometd_advice* advice)
{
  g_assert(advice != NULL);

  return advice->reconnect == COMETD_RECONNECT_HANDSHAKE;
}

gboolean
cometd_advice_is_none(const cometd_advice* advice)
{
  g_assert(advice != NULL);

  return advice->reconnect == COMETD_RECONNECT_NONE;
}

gboolean
cometd_advice_is_retry(const cometd_advice* advice)
{
  g_assert(advice != NULL);

  return advice->reconnect == COMETD_RECONNECT_RETRY;
}

/**
 * Sets the current transport being used for the connection.
 */
void
cometd_conn_set_transport(cometd_conn* conn, cometd_transport* t)
{
  g_assert(conn != NULL);
  g_assert(t != NULL);

  conn->transport = t;
}

/**
 * Returns the client id of the current connection
 */
const char*
cometd_conn_client_id(const cometd_conn* conn)
{
  g_return_val_if_fail(conn != NULL, NULL);

  return conn->client_id;
}

void
cometd_conn_set_client_id(cometd_conn* conn, const char* id)
{
  g_assert(id != NULL);

  conn->client_id = strdup(id);
}

void
cometd_conn_set_state(cometd_conn* conn, int status)
{
  g_assert(conn != NULL);

  conn->state = status;
}

int
cometd_conn_state(const cometd_conn* conn)
{
  g_return_val_if_fail(conn != NULL, COMETD_DISCONNECTED);

  return conn->state;
}

int
cometd_conn_is_state(const cometd_conn* conn, int status)
{
  return cometd_conn_state(conn) & status;
}
