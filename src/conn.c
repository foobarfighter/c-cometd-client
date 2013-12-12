#include "conn.h"
#include <stdlib.h>
#include <string.h>

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

void
cometd_conn_destroy(cometd_conn* conn)
{
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

  gboolean has_id = cometd_conn_is_status(conn, COMETD_HANDSHAKE_SUCCESS);

  return has_id ? conn->client_id : NULL;
}

void
cometd_conn_set_client_id(const cometd_conn* conn, const char* id)
{
  g_assert(id != NULL);
  g_assert(strnlen(id, COMETD_MAX_CLIENT_ID_LEN) < COMETD_MAX_CLIENT_ID_LEN);

  strncpy(conn->client_id, id, COMETD_MAX_CLIENT_ID_LEN - 1);
}

void
cometd_conn_set_status(cometd_conn* conn, long status)
{
  g_assert(conn != NULL);

  conn->state = cometd_conn_status(conn) | status;
}

long
cometd_conn_status(const cometd_conn* conn)
{
  g_return_val_if_fail(conn != NULL, COMETD_UNINITIALIZED);

  return conn->state;
}

long
cometd_conn_is_status(const cometd_conn* conn, long status)
{
  long actual = cometd_conn_status(conn);
  if (status == COMETD_UNINITIALIZED)
    return actual == COMETD_UNINITIALIZED;

  return actual & status;
}

void
cometd_conn_clear_status(cometd_conn* conn)
{
  g_assert(conn != NULL);

  conn->state = COMETD_UNINITIALIZED;
}
