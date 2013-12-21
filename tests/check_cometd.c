#include "check_cometd.h"

static cometd* g_instance = NULL;
static guint test_transport_send_calls = 0;
static guint test_transport_recv_calls = 0;

static JsonNode* test_transport_send(const cometd* h, JsonNode* node) {
  test_transport_send_calls++;
  return cometd_json_str2node("[{ \"successful\": true }]");
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
  fail_unless(cometd_conn_is_state(g_instance->conn, COMETD_UNINITIALIZED));
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

START_TEST (test_cometd_new_handshake_message){
  long seed = g_instance->conn->msg_id_seed;

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

START_TEST (test_cometd_is_meta_channel)
{
  fail_unless(cometd_is_meta_channel("/meta/*"));
  fail_unless(cometd_is_meta_channel("/meta/connect"));
  fail_if(cometd_is_meta_channel("/foo"));
  fail_if(cometd_is_meta_channel(NULL));
}
END_TEST

START_TEST(test_cometd_should_handshake)
{
  cometd_conn_set_state(g_instance->conn, COMETD_HANDSHAKE_SUCCESS);
  fail_if(cometd_should_handshake(g_instance));

  cometd_conn_set_state(g_instance->conn, COMETD_CONNECTED);
  fail_if(cometd_should_handshake(g_instance));

  cometd_conn_set_state(g_instance->conn, COMETD_UNCONNECTED);
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

  cometd_configure(g_instance, COMETDOPT_BACKOFF_INCREMENT, 10);

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

  // test when advice is retry
  cometd_advice* retry_advice = cometd_advice_new();
  retry_advice->reconnect = COMETD_RECONNECT_RETRY;
  retry_advice->interval = 10;
  cometd_conn_take_advice(conn, retry_advice);
  ck_assert_int_eq(10, cometd_get_backoff(g_instance, 20));  

  // test when advice is none
  cometd_advice* none_advice = cometd_advice_new();
  none_advice->reconnect = COMETD_RECONNECT_NONE;
  none_advice->interval = 10;
  cometd_conn_take_advice(conn, none_advice);
  ck_assert_int_eq(-1, cometd_get_backoff(g_instance, 20));

  // test when interval is 0 and the attempt is > 1, use backoff increment
  cometd_advice* zero_interval = cometd_advice_new();
  zero_interval->reconnect = COMETD_RECONNECT_RETRY;
  zero_interval->interval = 0;
  cometd_conn_take_advice(conn, zero_interval);
  ck_assert_int_eq(0, cometd_get_backoff(g_instance, 1));
  ck_assert_int_eq(10, cometd_get_backoff(g_instance, 2));
  ck_assert_int_eq(20, cometd_get_backoff(g_instance, 3));

  cometd_conn_take_advice(conn, NULL);
}
END_TEST

START_TEST (test_cometd_process_handshake_success)
{
  cometd_conn* conn = g_instance->conn;
  JsonNode* n = json_from_fixture("handshake_resp_lp");

  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);

  int code = cometd_process_handshake(g_instance, n);

  fail_unless(code == COMETD_SUCCESS);
  fail_unless(cometd_conn_is_state(conn, COMETD_HANDSHAKE_SUCCESS));
  fail_if(cometd_current_transport(g_instance) == NULL);
  fail_if(cometd_conn_client_id(conn) == NULL);

  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_process_handshake_no_transport)
{
  cometd_conn* conn = g_instance->conn;
  JsonNode* n = json_from_fixture("handshake_resp_unsupported_transports");

  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);

  int code = cometd_process_handshake(g_instance, n);

  fail_unless(code == ECOMETD_NO_TRANSPORT);
  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));
  fail_unless(cometd_current_transport(g_instance) == NULL);
  fail_unless(cometd_conn_client_id(conn) == NULL);
  fail_if(cometd_conn_advice(conn) == NULL);

  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_process_connect_success)
{
  cometd_conn* conn = g_instance->conn;
  cometd_conn_set_client_id(conn, "testid");
  cometd_conn_set_transport(conn, &TEST_TRANSPORT);

  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));

  JsonNode* msg = cometd_msg_connect_new(g_instance);
  cometd_msg_set_boolean_member(msg, "successful", TRUE);

  int code = cometd_process_connect(g_instance, msg);
  fail_unless(cometd_conn_is_state(conn, COMETD_CONNECTED));
  ck_assert_int_eq(COMETD_SUCCESS, code);

  json_node_free(msg);
}
END_TEST

START_TEST (test_cometd_process_connect_unsuccessful)
{
  cometd_conn* conn = g_instance->conn;
  cometd_conn_set_client_id(conn, "testid");
  cometd_conn_set_transport(conn, &TEST_TRANSPORT);

  fail_unless(cometd_conn_is_state(conn, COMETD_UNINITIALIZED));

  JsonNode* msg = cometd_msg_connect_new(g_instance);
  cometd_msg_set_boolean_member(msg, "successful", FALSE);

  int code = cometd_process_connect(g_instance, msg);
  fail_unless(cometd_conn_is_state(conn, COMETD_UNCONNECTED));
  ck_assert_int_eq(COMETD_SUCCESS, code);
}
END_TEST

START_TEST (test_cometd_process_connect_takes_advice_when_it_exists)
{
  cometd_conn* conn = g_instance->conn;
  cometd_conn_set_client_id(conn, "testid");
  cometd_conn_set_transport(conn, &TEST_TRANSPORT);

  // set advice
  JsonNode* msg = cometd_msg_connect_new(g_instance);
  cometd_advice* advice = cometd_advice_new();
  cometd_msg_set_advice(msg, advice);
  cometd_process_connect(g_instance, msg);

  // no advice
  JsonNode* bad_connect = cometd_msg_bad_connect_new(g_instance);
  cometd_process_connect(g_instance, bad_connect);

  // assert advice exists
  fail_unless(cometd_conn_advice(conn)->reconnect == COMETD_RECONNECT_NONE);

  cometd_advice_destroy(advice);
  json_node_free(msg);
  json_node_free(bad_connect);
}
END_TEST

static long backoff_attempts = 0;
static void increment_backoff_count(cometd_loop* h, long millis)
{
  backoff_attempts++;
}

START_TEST (test_cometd_handshake_backoff)
{
  cometd_loop backoff_loop;
  backoff_loop.wait = increment_backoff_count;

  // bad url so handshake fails
  cometd_configure(g_instance, COMETDOPT_URL, "");
  cometd_configure(g_instance, COMETDOPT_BACKOFF_INCREMENT, 1);
  cometd_configure(g_instance, COMETDOPT_MAX_BACKOFF, 10);
  cometd_configure(g_instance, COMETDOPT_LOOP, &backoff_loop);

  int code = cometd_handshake(g_instance, NULL);

  fail_if(code == COMETD_SUCCESS);
  ck_assert_int_eq(10, backoff_attempts);
}
END_TEST

static void add_foo(const cometd* h, JsonNode* n)
{
  JsonObject* obj = json_node_get_object(n);
  json_object_set_int_member(obj, "foo", 1);
}

START_TEST (test_cometd_process_msg_fires_incoming_ext)
{
  cometd_ext* ext = cometd_ext_new();
  ext->incoming = add_foo;
  cometd_ext_add(&g_instance->exts, ext);

  JsonNode* node = cometd_json_str2node("{}");
  cometd_process_msg(g_instance, node);

  JsonNode* expected = cometd_json_str2node("{ \"foo\": 1}");
  gboolean ret = json_node_equal(expected, node, NULL);

  fail_unless(ret);

  json_node_free(node);
  json_node_free(expected);
}
END_TEST

START_TEST (test_cometd_transport_send_fires_outgoing_ext)
{
  cometd_conn_set_transport(g_instance->conn, &TEST_TRANSPORT);

  JsonNode* expected = cometd_json_str2node("{ \"foo\": 1}");
  gboolean ret = FALSE;

  cometd_ext* ext = cometd_ext_new();
  ext->outgoing = add_foo;
  cometd_ext_add(&g_instance->exts, ext);

  JsonNode* node = cometd_json_str2node("{}");

  cometd_transport_send(g_instance, node);
  ret = json_node_equal(expected, node, NULL);
  fail_unless(ret);

  json_node_free(node);
  json_node_free(expected);
}
END_TEST

START_TEST (test_cometd_should_recv)
{
  cometd_conn* conn = g_instance->conn;
  
  cometd_conn_set_state(conn, COMETD_HANDSHAKE_SUCCESS);
  fail_unless(cometd_should_recv(g_instance));

  cometd_conn_set_state(conn, COMETD_CONNECTED);
  fail_unless(cometd_should_recv(g_instance));

  cometd_conn_set_state(conn, COMETD_DISCONNECTED);
  fail_if(cometd_should_recv(g_instance));
}
END_TEST

START_TEST (test_cometd_should_retry_recv)
{
  cometd_advice* handshake_advice = cometd_advice_new();
  handshake_advice->reconnect = COMETD_RECONNECT_HANDSHAKE;

  cometd_advice* retry_advice = cometd_advice_new();
  retry_advice->reconnect = COMETD_RECONNECT_RETRY;

  cometd_conn* conn = g_instance->conn;

  cometd_conn_set_state(conn, COMETD_DISCONNECTED);
  fail_if(cometd_should_retry_recv(g_instance));

  cometd_conn_set_state(conn, COMETD_UNCONNECTED);
  cometd_conn_take_advice(conn, handshake_advice);
  fail_if(cometd_should_retry_recv(g_instance));

  cometd_conn_set_state(conn, COMETD_UNCONNECTED);
  cometd_conn_take_advice(conn, NULL);
  fail_if(cometd_should_retry_recv(g_instance));

  cometd_conn_set_state(conn, COMETD_UNCONNECTED);
  cometd_conn_take_advice(conn, retry_advice);
  fail_unless(cometd_should_retry_recv(g_instance));
}
END_TEST

Suite* make_cometd_unit_suite (void)
{
  Suite *s = suite_create ("cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("core");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_new);
  tcase_add_test (tc_unit, test_cometd_new_handshake_message);
  tcase_add_test (tc_unit, test_cometd_new_subscribe_message);
  tcase_add_test (tc_unit, test_cometd_new_unsubscribe_message);
  tcase_add_test (tc_unit, test_cometd_new_publish_message);
  tcase_add_test (tc_unit, test_cometd_unsubscribe);
  tcase_add_test (tc_unit, test_cometd_meta_subscriptions);
  tcase_add_test (tc_unit, test_cometd_transport);
  tcase_add_test (tc_unit, test_cometd_is_meta_channel);
  tcase_add_test (tc_unit, test_cometd_should_handshake);
  tcase_add_test (tc_unit, test_cometd_get_backoff);
  tcase_add_test (tc_unit, test_cometd_process_handshake_success);
  tcase_add_test (tc_unit, test_cometd_process_handshake_no_transport);
  tcase_add_test (tc_unit, test_cometd_process_connect_success);
  tcase_add_test (tc_unit, test_cometd_process_connect_takes_advice_when_it_exists);
  tcase_add_test (tc_unit, test_cometd_handshake_backoff);
  tcase_add_test (tc_unit, test_cometd_process_msg_fires_incoming_ext);
  tcase_add_test (tc_unit, test_cometd_transport_send_fires_outgoing_ext);
  tcase_add_test (tc_unit, test_cometd_should_recv);
  tcase_add_test (tc_unit, test_cometd_should_retry_recv);
  suite_add_tcase (s, tc_unit);

  return s;
}

