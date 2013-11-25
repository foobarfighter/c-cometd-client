#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <cometd.h>
#include <cometd_json.h>

#include "check_cometd.h"
#include "test_helper.h"

static cometd* g_instance = NULL;

static void
setup (void)
{
  g_instance = cometd_new();
}

static void
teardown (void)
{
  // Try to exit clean
  if (cometd_conn_is_status(g_instance, COMETD_CONNECTED))
    cometd_disconnect(g_instance, 0);

  cometd_destroy(g_instance);
}

/*
 *  Integration Suite
 */

START_TEST (test_cometd_connect_success)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);
  
  int code = cometd_connect(g_instance);
  ck_assert_int_eq(COMETD_SUCCESS, code);
  fail_unless(cometd_conn_is_status(g_instance, COMETD_HANDSHAKE_SUCCESS));
  fail_unless(cometd_conn_is_status(g_instance, COMETD_CONNECTED));
}
END_TEST

int test_init_fail_loop(const cometd* h) { return 1; }

START_TEST (test_cometd_connect_fail_init_loop)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);
  cometd_configure(g_instance, COMETDOPT_INIT_LOOPFUNC, test_init_fail_loop);

  int code = cometd_connect(g_instance);
  fail_unless(cometd_conn_is_status(g_instance, COMETD_HANDSHAKE_SUCCESS));
  ck_assert_int_eq(ECOMETD_INIT_LOOP, code);
}
END_TEST


START_TEST (test_cometd_handshake_success)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(COMETD_SUCCESS, code);
  fail_unless(strcmp(g_instance->conn->transport->name, "long-polling") == 0);
  fail_unless(g_instance->conn->client_id != NULL);
  fail_unless(cometd_conn_status(g_instance) & COMETD_HANDSHAKE_SUCCESS);
}
END_TEST

START_TEST (test_cometd_handshake_failed_http)
{
  cometd_configure(g_instance, COMETDOPT_URL, "http://localhost/service/does/not/exist");

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(ECOMETD_HANDSHAKE, code); 
}
END_TEST

START_TEST (test_cometd_handshake_failed_http_timeout)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_LONG_REQUEST_URL);
  cometd_configure(g_instance, COMETDOPT_REQUEST_TIMEOUT, TEST_LONG_REQUEST_TIMEOUT);

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(ECOMETD_HANDSHAKE, code); 
}
END_TEST

START_TEST (test_cometd_handshake_failed_json)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_BAD_JSON_URL);
  cometd_configure(g_instance, COMETDOPT_REQUEST_TIMEOUT, 1000);

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(ECOMETD_JSON_DESERIALIZE, code); 
}
END_TEST

START_TEST (test_cometd_subscribe_success)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);
  ck_assert_int_eq(COMETD_SUCCESS, cometd_connect(g_instance));

  int code = cometd_subscribe(g_instance, "/foo/bar/baz", NULL);
  ck_assert_int_eq(COMETD_SUCCESS, code);
}
END_TEST

START_TEST (test_cometd_send_and_receive_message){
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);

  ck_assert_int_eq(COMETD_SUCCESS, cometd_connect(g_instance));
  ck_assert_int_eq(COMETD_SUCCESS, cometd_subscribe(g_instance, "/echo/message/test", log_handler));

  JsonNode* message = cometd_json_str2node("{ \"message\": \"hey now\" }");

  ck_assert_int_eq(COMETD_SUCCESS, cometd_publish(g_instance, "/echo/message/test", message));
  ck_assert_int_eq(0,              log_has_message(message));
  ck_assert_int_eq(COMETD_SUCCESS, cometd_disconnect(g_instance));
}
END_TEST

Suite* make_cometd_integration_suite (void)
{
  Suite *s = suite_create("Cometd");

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Integration");

  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_set_timeout (tc_integration, 15);

  tcase_add_test (tc_integration, test_cometd_connect_success);
  tcase_add_test (tc_integration, test_cometd_connect_fail_init_loop);
  tcase_add_test (tc_integration, test_cometd_handshake_success);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_json);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http_timeout);
  tcase_add_test (tc_integration, test_cometd_subscribe_success);
  tcase_add_test (tc_integration, test_cometd_send_and_receive_message);
  suite_add_tcase (s, tc_integration);

  return s;
}
