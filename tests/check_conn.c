#include "cometd.h"
#include "check_cometd.h"

static cometd_conn* conn = NULL;
static cometd_advice* advice = NULL;

static void setup(void) {
  conn = cometd_conn_new();
  advice = cometd_advice_new();
}

static void teardown(void) {
  cometd_conn_destroy(conn);
  cometd_advice_destroy(advice);
}

START_TEST (test_cometd_advice_is_handshake)
{
  advice->reconnect = COMETD_RECONNECT_HANDSHAKE;
  fail_unless(cometd_advice_is_handshake(advice));
}
END_TEST

START_TEST (test_cometd_advice_is_none)
{
  advice->reconnect = COMETD_RECONNECT_NONE;
  fail_unless(cometd_advice_is_none(advice));
}
END_TEST

START_TEST (test_cometd_conn_state)
{
  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));

  cometd_conn_set_state(conn, COMETD_HANDSHAKE_SUCCESS | COMETD_CONNECTED);
  fail_unless(cometd_conn_is_state(conn, COMETD_HANDSHAKE_SUCCESS));
}
END_TEST

Suite* make_conn_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("conn");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_advice_is_handshake);
  tcase_add_test (tc_unit, test_cometd_advice_is_none);
  tcase_add_test (tc_unit, test_cometd_conn_state);
  suite_add_tcase (s, tc_unit);

  return s;
}
