#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <glib.h>
#include <signal.h>

#include "check_cometd.h"
#include "cometd.h"
#include "transport_long_polling.h"
#include "test_helper.h"

static cometd* g_instance = NULL;
static int       test_transport_send(const cometd* h, JsonNode* node);
static JsonNode* test_transport_recv(const cometd* h);
static int test_transport_send(const cometd* h, JsonNode* node) { return 0; }
static JsonNode* test_transport_recv(const cometd* h) { return NULL; }

static cometd_transport TEST_TRANSPORT = {
  "test-transport",
   test_transport_send,
   test_transport_recv
};


static void setup (void)
{
  g_instance = cometd_new();
  cometd_configure(g_instance, COMETDOPT_URL, TEST_SERVER_URL);
}

static void teardown (void)
{
  cometd_destroy(g_instance);
}

/*
 *  Unit Test Suite
 */

START_TEST (test_cometd_transport)
{
  cometd_config* config = g_instance->config;

  // should have default transports + test-transport
  cometd_register_transport(config, &TEST_TRANSPORT);

  // default transports + test-transport
  fail_unless(g_list_length(config->transports) == 2);

  // should not be able to find a transport that doesn't exist
  cometd_transport* nullptr = cometd_find_transport(config, "0xdeadbeef");
  fail_unless(nullptr == NULL);

  // transport should be found by name
  const cometd_transport* t = cometd_find_transport(config, "test-transport");
  fail_if(t == NULL);
  fail_unless(strcmp(t->name, "test-transport") == 0);

  // removing the transport should make it un-findable
  cometd_unregister_transport(config, t->name);
  fail_unless(cometd_find_transport(config, "test-transport") == NULL);
}
END_TEST

START_TEST (test_cometd_new)
{
  fail_unless(cometd_conn_is_status(g_instance, COMETD_UNINITIALIZED));
  ck_assert_int_eq(COMETD_SUCCESS, g_instance->last_error->code);

  char* actual_url = "http://example.com/cometd/";
  cometd_configure(g_instance, COMETDOPT_URL, actual_url);

  cometd_config* config = g_instance->config;
  ck_assert_str_eq(actual_url, config->url);
  ck_assert_int_eq(DEFAULT_BACKOFF_INCREMENT, config->backoff_increment);
  ck_assert_int_eq(DEFAULT_REQUEST_TIMEOUT, config->request_timeout);
  fail_if(cometd_find_transport(config, "long-polling") == NULL);
}
END_TEST

START_TEST (test_cometd_new_connect_message)
{
  strcpy(g_instance->conn->client_id, "testid");
  cometd_register_transport(g_instance->config, &TEST_TRANSPORT);
  g_instance->conn->transport = &TEST_TRANSPORT;

  JsonNode* msg   = cometd_new_connect_message(g_instance);
  JsonObject* obj = json_node_get_object(msg);

  const gchar* channel = json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
  fail_unless(strcmp(channel, COMETD_CHANNEL_META_CONNECT) == 0);

  json_node_free(msg);
}
END_TEST

START_TEST (test_cometd_new_handshake_message){
  long seed = g_instance->conn->_msg_id_seed;

  JsonNode* msg = cometd_new_handshake_message(g_instance);
  JsonObject* obj = json_node_get_object(msg);
  int id = json_object_get_int_member(obj, COMETD_MSG_ID_FIELD);

  json_node_free(msg);

  fail_unless(id == 1);
}
END_TEST

START_TEST (test_cometd_error)
{
  char* message = "hey now";
  int code = 1234;
  int ret = cometd_error(g_instance, 1234, message);

  ck_assert_int_eq(code, ret);

  cometd_error_st* error = cometd_last_error(g_instance);

  ck_assert_int_eq(code, error->code);
  ck_assert_str_eq(message, error->message);
}
END_TEST

START_TEST (test_cometd_conn_status)
{
  fail_unless(cometd_conn_is_status(g_instance, COMETD_UNINITIALIZED));

  cometd_conn_set_status(g_instance, COMETD_HANDSHAKE_SUCCESS);
  fail_unless(cometd_conn_is_status(g_instance, COMETD_HANDSHAKE_SUCCESS));

  cometd_conn_set_status(g_instance, COMETD_CONNECTED);
  fail_unless(cometd_conn_is_status(g_instance, COMETD_HANDSHAKE_SUCCESS));
  fail_unless(cometd_conn_is_status(g_instance, COMETD_CONNECTED));

  cometd_conn_clear_status(g_instance);

  fail_unless(cometd_conn_is_status(g_instance, COMETD_UNINITIALIZED));
}
END_TEST

Suite* make_cometd_unit_suite (void)
{
  Suite *s = suite_create ("Cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("Client::Unit");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_new);
  tcase_add_test (tc_unit, test_cometd_new_connect_message);
  tcase_add_test (tc_unit, test_cometd_new_handshake_message);
  tcase_add_test (tc_unit, test_cometd_transport);
  tcase_add_test (tc_unit, test_cometd_error);
  tcase_add_test (tc_unit, test_cometd_conn_status);
  suite_add_tcase (s, tc_unit);

  return s;
}

