#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <glib.h>
#include <signal.h>

#include "check_cometd.h"
#include "cometd.h"
#include "cometd_json.h"
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

static int test_empty_handler(const cometd* h, JsonNode* message) {
  return COMETD_SUCCESS;
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
  cometd_conn_set_client_id(g_instance, "testid");
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
  cometd_conn_set_client_id(g_instance, "testid");
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
  cometd_conn_set_client_id(g_instance, "testid");
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
  cometd_conn_set_client_id(g_instance, "testid");
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
  cometd_conn_set_client_id(g_instance, "testid");
  cometd_conn_set_transport(g_instance, &TEST_TRANSPORT);

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
  cometd_conn_set_client_id(g_instance, "testid");
  cometd_conn_set_transport(g_instance, &TEST_TRANSPORT);

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

START_TEST (test_cometd_add_listener)
{
  ck_assert_int_eq(0, log_size());

  cometd_subscription* s;
  int code;

  JsonNode* message = cometd_json_str2node("{}");

  s    = cometd_add_listener(g_instance, "/foo/bar/baz", log_handler);
  code = cometd_fire_listeners(g_instance, "/foo/bar/baz", message);

  ck_assert_int_eq(COMETD_SUCCESS, code);
  wait_for_message(100, NULL, "{}");

  code = cometd_remove_listener(g_instance, s);
  ck_assert_int_eq(COMETD_SUCCESS, code);

  code = cometd_fire_listeners(g_instance, "/foo/bar/baz", message);
  ck_assert_int_eq(COMETD_SUCCESS, code);
  ck_assert_int_eq(0, log_size());

  json_node_free(message);
}
END_TEST

START_TEST (test_cometd_fire_listeners_wildcard)
{
  JsonNode* message = cometd_json_str2node("{}");

  cometd_subscription *s1, *s2;

  s1 = cometd_add_listener(g_instance, "/foo/**", log_handler);
  s2 = cometd_add_listener(g_instance, "/quux/*", log_handler);

  cometd_fire_listeners(g_instance, "/foo/bar/baz", message);
  ck_assert_int_eq(1, log_size());
  cometd_fire_listeners(g_instance, "/quux/wat/wut", message);
  ck_assert_int_eq(1, log_size());
  cometd_fire_listeners(g_instance, "/quux/wat", message);
  ck_assert_int_eq(2, log_size());

  json_node_free(message);
}
END_TEST

START_TEST (test_cometd_channel_subscriptions)
{
  cometd_subscription *s1, *s2, *s3, *s4;

  s1 = cometd_add_listener(g_instance, "/foo/*", test_empty_handler);
  s2 = cometd_add_listener(g_instance, "/**", test_empty_handler);
  s3 = cometd_add_listener(g_instance, "/foo/bar/baz/*", test_empty_handler);
  s4 = cometd_add_listener(g_instance, "/foo/bar/baz/bang", test_empty_handler);

  GList* subscriptions = cometd_channel_subscriptions(g_instance,
                                                      "/foo/bar/baz/bang");

  fail_if(subscriptions == NULL);
  ck_assert_int_eq(3, g_list_length(subscriptions));

  fail_if(g_list_find(subscriptions, s2) == NULL);
  fail_if(g_list_find(subscriptions, s3) == NULL);
  fail_if(g_list_find(subscriptions, s4) == NULL);

  g_list_free(subscriptions);
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
  tcase_add_test (tc_unit, test_cometd_add_listener);
  tcase_add_test (tc_unit, test_cometd_fire_listeners_wildcard);
  tcase_add_test (tc_unit, test_cometd_channel_subscriptions);
  tcase_add_test (tc_unit, test_cometd_channel_matches);
  tcase_add_test (tc_unit, test_cometd_channel_is_wildcard);
  suite_add_tcase (s, tc_unit);

  return s;
}

