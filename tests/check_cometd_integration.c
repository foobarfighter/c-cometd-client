#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <cometd.h>

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
  cometd_destroy(g_instance);
}

/*
 *  Integration Suite
 */

START_TEST (test_cometd_handshake_success)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(COMETD_SUCCESS, code);
  fail_unless(strcmp(g_instance->conn->transport->name, "long-polling") == 0);
  fail_unless(g_instance->conn->client_id != NULL);
  fail_unless(cometd_conn_status(g_instance) & COMETD_HANDSHAKE_SUCCESS);

  g_instance->conn->state = COMETD_DISCONNECTED;
  g_thread_join(g_instance->conn->inbox_thread);

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

int test_init_fail_loop(const cometd* h) { return 1; }

START_TEST (test_cometd_handshake_fail_init_loop)
{
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);
  cometd_configure(g_instance, COMETDOPT_INIT_LOOPFUNC, test_init_fail_loop);

  int code = cometd_handshake(g_instance, NULL);
  fail_unless(cometd_conn_status(g_instance) & COMETD_HANDSHAKE_SUCCESS);
  ck_assert_int_eq(ECOMETD_INIT_LOOP, code);
}
END_TEST

//START_TEST (test_cometd_unsuccessful_handshake_without_advice)
//{
//  g_instance = create_cometd();
//
//  ck_assert_int_eq(0, cometd_unregister_transport(g_instance->config, "long-polling"));
//  ck_assert_int_eq(0, g_list_length(g_instance->config->transports));
//  ck_assert_int_eq(0, cometd_register_transport(g_instance->config, &TEST_TRANSPORT));
//
//  ck_assert_int_eq(0, cometd_handshake(g_instance, NULL));
//}
//END_TEST
//
//START_TEST (test_cometd_unsuccessful_handshake_with_advice)
//{
//  //g_instance = create_cometd();
//
//  //int code = cometd_handshake(g_instance, logger_handler)
//}
//END_TEST


//static GCond cometd_init_cond;
//static GMutex cometd_init_mutex;
//static gboolean cometd_initialized = FALSE;
//
//gpointer cometd_init_thread(gpointer data){
//  cometd* instance = (cometd*) data;
//  cometd_init(instance);
//  fail_unless(g_instance->conn->state == COMETD_CONNECTED);
//
//  g_mutex_lock(&cometd_init_mutex);
//  cometd_initialized = TRUE;
//  g_cond_signal(&cometd_init_cond);
//  g_mutex_unlock(&cometd_init_mutex);
//}
//
//START_TEST (test_cometd_send_and_receive_message){
//  g_instance = create_cometd(TEST_SERVER_URL);
//  //cometd_add_listener(g_instance, "/meta/**", inbox_handler);
//  //cometd_add_listener(g_instance, "/echo/message/test", inbox_handler);
//
//  GThread* t = g_thread_new("cometd_init thread", &cometd_init_thread, g_instance);
//
//  g_mutex_lock(&cometd_init_mutex);
//  while (cometd_initialized == FALSE)
//    g_cond_wait(&cometd_init_cond, &cometd_init_mutex);
//
//  printf("this is where ill send messages\n");
//
//  //cometd_send(g_instance, "/echo/message/test", create_message_from_json(json_str));
//  //fail_unless(check_inbox_for_message(json_str, 10));
//  //cometd_disconnect(g_instance);
//
//  g_mutex_unlock(&cometd_init_mutex);
//
//  gpointer ret = g_thread_join(t);
//
//  fail();
//
//}
//END_TEST

Suite* make_cometd_integration_suite (void)
{
  Suite *s = suite_create("Cometd");

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Integration");

  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_set_timeout (tc_integration, 15);

  tcase_add_test (tc_integration, test_cometd_handshake_success);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_json);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http_timeout);
  tcase_add_test (tc_integration, test_cometd_handshake_fail_init_loop);
  suite_add_tcase (s, tc_integration);

  return s;
}
