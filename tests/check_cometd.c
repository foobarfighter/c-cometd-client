#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <glib.h>
#include <signal.h>

#include "check_cometd.h"
#include "cometd.h"
#include "json.h"
#include "transport_long_polling.h"
#include "test_helper.h"

static cometd* g_instance = NULL;

static guint test_transport_send_calls = 0;
static guint test_transport_recv_calls = 0;

static JsonNode* test_transport_send(const cometd* h, JsonNode* node) {
  test_transport_send_calls++;
  return cometd_json_str2node("[{ \"successful\"; true }]");
}

static JsonNode* test_transport_recv(const cometd* h) {
  test_transport_recv_calls++;
  return json_node_new(JSON_NODE_ARRAY);
}

static cometd_transport TEST_TRANSPORT = {
  "test-transport",
   test_transport_send,
   test_transport_recv
};


static void setup (void)
{
  log_clear();
  test_transport_recv_calls = 0;
  test_transport_send_calls = 0;

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
  fail_unless(cometd_conn_is_status(g_instance->conn, COMETD_UNINITIALIZED));
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
  cometd_conn_set_client_id(g_instance->conn, "testid");
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

START_TEST (test_cometd_new_subscribe_message)
{
  cometd_conn_set_client_id(g_instance->conn, "testid");
  const char* expected_channel = "/foo/bar/baz";

  JsonNode* msg = cometd_new_subscribe_message(g_instance,
                                               expected_channel);
  JsonObject* obj = json_node_get_object(msg);

  const gchar* actual_channel = json_object_get_string_member(obj,
                                  COMETD_MSG_SUBSCRIPTION_FIELD);

  ck_assert_str_eq(expected_channel, actual_channel);

  json_node_free(msg);
}
END_TEST

START_TEST (test_cometd_new_unsubscribe_message)
{
  cometd_conn_set_client_id(g_instance->conn, "testid");
  const char* expected_channel = "/foo/bar/baz";

  JsonNode* msg = cometd_new_unsubscribe_message(g_instance,
                                                 expected_channel);
  JsonObject* obj = json_node_get_object(msg);

  const gchar* actual_channel = json_object_get_string_member(obj,
                                  COMETD_MSG_SUBSCRIPTION_FIELD);

  ck_assert_str_eq(expected_channel, actual_channel);

  json_node_free(msg);
}
END_TEST

START_TEST (test_cometd_new_publish_message)
{
  cometd_conn_set_client_id(g_instance->conn, "testid");
  const char* expected_channel = "/baz/bar";
  JsonNode* node = cometd_json_str2node("{ \"hey\": \"now\" }");
  JsonNode* message = cometd_new_publish_message(g_instance,
                                                 expected_channel,
                                                 node);

  JsonObject* obj = json_node_get_object(message);

  const gchar* actual_channel = json_object_get_string_member(obj,
                                  COMETD_MSG_CHANNEL_FIELD);

  ck_assert_str_eq(expected_channel, actual_channel);

  JsonObject* data = json_object_get_object_member(obj,
                                  COMETD_MSG_DATA_FIELD);

  const char* value = json_object_get_string_member(data, "hey");
  ck_assert_str_eq("now", value);

  json_node_free(message);
  json_node_free(node);
}
END_TEST

START_TEST (test_cometd_unsubscribe)
{
  cometd_conn_set_client_id(g_instance->conn, "testid");
  cometd_conn_set_transport(g_instance->conn, &TEST_TRANSPORT);

  cometd_subscription *s1, *s2;

  s1 = cometd_subscribe(g_instance, "/foo/bar/baz", test_empty_handler);
  s2 = cometd_subscribe(g_instance, "/foo/bar/baz", test_empty_handler);

  ck_assert_int_eq(2, test_transport_send_calls);

  cometd_unsubscribe(g_instance, s1);
  cometd_unsubscribe(g_instance, s2);

  ck_assert_int_eq(3, test_transport_send_calls);
}
END_TEST

START_TEST (test_cometd_meta_subscriptions)
{
  cometd_conn_set_client_id(g_instance->conn, "testid");
  cometd_conn_set_transport(g_instance->conn, &TEST_TRANSPORT);

  cometd_subscription *s1;

  s1 = cometd_subscribe(g_instance, "/meta/connect", test_empty_handler);
  ck_assert_int_eq(0, test_transport_send_calls);

  cometd_unsubscribe(g_instance, s1); 
  ck_assert_int_eq(0, test_transport_send_calls);
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

START_TEST (test_cometd_is_meta_channel)
{
  fail_unless(cometd_is_meta_channel("/meta/*"));
  fail_unless(cometd_is_meta_channel("/meta/connect"));
  fail_if(cometd_is_meta_channel("/foo"));
  fail_if(cometd_is_meta_channel(NULL));
}
END_TEST

START_TEST (test_cometd_conn_status)
{
  cometd_conn* conn = g_instance->conn;

  fail_unless(cometd_conn_is_status(conn, COMETD_UNINITIALIZED));

  cometd_conn_set_status(conn, COMETD_HANDSHAKE_SUCCESS);
  fail_unless(cometd_conn_is_status(conn, COMETD_HANDSHAKE_SUCCESS));

  cometd_conn_set_status(conn, COMETD_CONNECTED);
  fail_unless(cometd_conn_is_status(conn, COMETD_HANDSHAKE_SUCCESS));
  fail_unless(cometd_conn_is_status(conn, COMETD_CONNECTED));

  cometd_conn_clear_status(conn);

  fail_unless(cometd_conn_is_status(conn, COMETD_UNINITIALIZED));
}
END_TEST

static void
dump_channel_matches(const char* c, GList* channels)
{
  GList* channel;
  printf("dumping matching channels for: %s\n", c);
  for (channel = channels; channel; channel = g_list_next(channel))
  {
    printf("matching channel: %s\n", (char*) channel->data);
  }
  printf("\n\n");
}

START_TEST (test_cometd_channel_matches)
{
  GList* c1 = cometd_channel_matches("/foo/bar/baz");
  dump_channel_matches("/foo/bar/baz", c1);

  ck_assert_int_eq(5, g_list_length(c1));
  fail_if(g_list_find_custom(c1, "/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/*", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/baz", (GCompareFunc)strcmp) == NULL);

  GList* c2 = cometd_channel_matches("/");
  dump_channel_matches("/", c2);

  fail_if(g_list_find_custom(c2, "/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c2, "/*", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c2, "/", (GCompareFunc)strcmp) == NULL);

  cometd_channel_matches_free(c1);
  cometd_channel_matches_free(c2);
}
END_TEST

START_TEST (test_cometd_channel_is_wildcard)
{
  fail_unless(cometd_channel_is_wildcard("/meta/**"));
  fail_if(cometd_channel_is_wildcard("/meta"));
}
END_TEST

START_TEST(test_cometd_should_handshake)
{
  cometd_conn_set_status(g_instance->conn, COMETD_HANDSHAKE_SUCCESS);
  fail_if(cometd_should_handshake(g_instance));

  cometd_conn_set_status(g_instance->conn, COMETD_CONNECTED);
  fail_if(cometd_should_handshake(g_instance));

  cometd_conn_clear_status(g_instance->conn);

  cometd_advice* advice = cometd_advice_new();
  advice->reconnect = COMETD_RECONNECT_HANDSHAKE;
  cometd_conn_take_advice(g_instance->conn, advice);
  
  fail_unless(cometd_should_handshake(g_instance));

  cometd_conn_take_advice(g_instance->conn, NULL);
}
END_TEST

START_TEST (test_cometd_get_backoff)
{
  cometd_conn* conn = g_instance->conn;
  cometd_config* config = g_instance->config;

  config->backoff_increment = 10;

  // test incremental backoff when no advice
  cometd_conn_take_advice(conn, NULL);
  ck_assert_int_eq(0, cometd_get_backoff(g_instance, 0));
  ck_assert_int_eq(10, cometd_get_backoff(g_instance, 1));
  ck_assert_int_eq(20, cometd_get_backoff(g_instance, 2));

  // test when advice is hanshake
  cometd_advice* handshake_advice = cometd_advice_new();
  handshake_advice->reconnect = COMETD_RECONNECT_HANDSHAKE;
  handshake_advice->interval = 10;
  cometd_conn_take_advice(conn, handshake_advice);
  ck_assert_int_eq(10, cometd_get_backoff(g_instance, 20));

  // test when advice is none
  cometd_advice* none_advice = cometd_advice_new();
  handshake_advice->reconnect = COMETD_RECONNECT_NONE;
  handshake_advice->interval = 10;
  cometd_conn_take_advice(conn, none_advice);
  ck_assert_int_eq(-1, cometd_get_backoff(g_instance, 20));

  cometd_conn_take_advice(conn, NULL);
}
END_TEST

START_TEST (test_cometd_process_message_success)
{
  cometd_conn* conn = g_instance->conn;
  JsonNode* n = json_from_fixture("handshake_resp_lp");

  fail_unless(cometd_conn_is_status(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);

  int code = cometd_process_handshake(g_instance, n);

  fail_unless(code == COMETD_SUCCESS);
  fail_unless(cometd_conn_is_status(conn, COMETD_HANDSHAKE_SUCCESS));
  fail_if(cometd_current_transport(g_instance) == NULL);
  fail_if(cometd_conn_client_id(conn) == NULL);
}
END_TEST

START_TEST (test_cometd_process_message_no_transport)
{
  cometd_conn* conn = g_instance->conn;
  JsonNode* n = json_from_fixture("handshake_resp_unsupported_transports");

  fail_unless(cometd_conn_is_status(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);

  int code = cometd_process_handshake(g_instance, n);

  fail_unless(code == ECOMETD_NO_TRANSPORT);
  fail_unless(cometd_conn_is_status(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);
  fail_if(cometd_conn_advice(conn) == NULL);
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
  tcase_add_test (tc_unit, test_cometd_new_subscribe_message);
  tcase_add_test (tc_unit, test_cometd_new_unsubscribe_message);
  tcase_add_test (tc_unit, test_cometd_new_publish_message);
  tcase_add_test (tc_unit, test_cometd_unsubscribe);
  tcase_add_test (tc_unit, test_cometd_meta_subscriptions);
  tcase_add_test (tc_unit, test_cometd_transport);
  tcase_add_test (tc_unit, test_cometd_error);
  tcase_add_test (tc_unit, test_cometd_is_meta_channel);
  tcase_add_test (tc_unit, test_cometd_conn_status);
  tcase_add_test (tc_unit, test_cometd_channel_matches);
  tcase_add_test (tc_unit, test_cometd_channel_is_wildcard);
  tcase_add_test (tc_unit, test_cometd_should_handshake);
  tcase_add_test (tc_unit, test_cometd_get_backoff);
  tcase_add_test (tc_unit, test_cometd_process_message_success);
  tcase_add_test (tc_unit, test_cometd_process_message_no_transport);
  suite_add_tcase (s, tc_unit);

  return s;
}

