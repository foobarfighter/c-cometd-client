#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <cometd.h>

#include "check_cometd.h"
#include "test_helper.h"

static cometd_config* g_config   = NULL;
static cometd*        g_instance = NULL;

static void
teardown (void)
{
}

static void
setup (void)
{
}

static cometd*
create_cometd(char *url){
  g_config = (cometd_config*) malloc(sizeof(cometd_config));
  cometd_default_config(g_config);

  g_config->url = url;

  return cometd_new(g_config);
}


/*
 *  Integration Suite
 */
START_TEST (test_cometd_successful_init)
{
  g_instance = create_cometd(TEST_SERVER_URL);
  cometd_init(g_instance);
  fail_unless(g_instance->conn->state == COMETD_CONNECTED);
}
END_TEST

START_TEST (test_cometd_handshake_success)
{
  g_instance = create_cometd(TEST_SERVER_URL);

  int code = cometd_handshake(g_instance, NULL);
  fail_unless(code == 0);
  fail_unless(strcmp(g_instance->conn->transport->name, "long-polling") == 0);
  fail_unless(g_instance->conn->client_id != NULL);
}
END_TEST

START_TEST (test_cometd_handshake_failed_http)
{
  g_instance = create_cometd("http://localhost/service/does/not/exist");

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(COMETD_ERROR_HANDSHAKE, code); 
}
END_TEST

START_TEST (test_cometd_handshake_failed_http_timeout)
{
  g_instance = create_cometd(TEST_LONG_REQUEST_URL);
  g_instance->config->request_timeout = TEST_LONG_REQUEST_TIMEOUT;

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(COMETD_ERROR_HANDSHAKE, code); 
}
END_TEST

START_TEST (test_cometd_handshake_failed_json)
{
  g_instance = create_cometd(TEST_BAD_JSON_URL);
  g_instance->config->request_timeout = 1000;

  int code = cometd_handshake(g_instance, NULL);
  ck_assert_int_eq(COMETD_ERROR_JSON, code); 
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


static GCond cometd_init_cond;
static GMutex cometd_init_mutex;
static gboolean cometd_initialized = FALSE;

gpointer cometd_init_thread(gpointer data){
  cometd* instance = (cometd*) data;
  cometd_init(instance);
  fail_unless(g_instance->conn->state == COMETD_CONNECTED);

  g_mutex_lock(&cometd_init_mutex);
  cometd_initialized = TRUE;
  g_cond_signal(&cometd_init_cond);
  g_mutex_unlock(&cometd_init_mutex);
}

START_TEST (test_cometd_send_and_receive_message){
  g_instance = create_cometd(TEST_SERVER_URL);
  //cometd_add_listener(g_instance, "/meta/**", inbox_handler);
  //cometd_add_listener(g_instance, "/echo/message/test", inbox_handler);

  GThread* t = g_thread_new("cometd_init thread", &cometd_init_thread, g_instance);

  g_mutex_lock(&cometd_init_mutex);
  while (cometd_initialized == FALSE)
    g_cond_wait(&cometd_init_cond, &cometd_init_mutex);

  printf("this is where ill send messages\n");

  //cometd_send(g_instance, "/echo/message/test", create_message_from_json(json_str));
  //fail_unless(check_inbox_for_message(json_str, 10));
  //cometd_disconnect(g_instance);

  g_mutex_unlock(&cometd_init_mutex);

  gpointer ret = g_thread_join(t);

  fail();

}
END_TEST

Suite* make_cometd_integration_suite (void)
{
  Suite *s = suite_create("Cometd");

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Integration");

  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_set_timeout (tc_integration, 15);

  //tcase_add_test (tc_integration, test_cometd_successful_init);
  tcase_add_test (tc_integration, test_cometd_handshake_success);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_json);
  tcase_add_test (tc_integration, test_cometd_handshake_failed_http_timeout);
  suite_add_tcase (s, tc_integration);

  return s;
}
