#include "check_cometd.h"

static cometd_transport TEST_TRANSPORT = {
  "test-transport",
   NULL,
   NULL
};

static void
assert_bool_node_func(char *str,
                      gboolean (*cb)(JsonNode* n),
                      gboolean expected)
{
  JsonNode* n = cometd_json_str2node(str);
  gboolean ret = cb(n);
  fail_unless(ret == expected);
  json_node_free(n);
}

START_TEST (test_cometd_msg_is_successful)
{
  assert_bool_node_func("[{ \"successful\": true }]}",
    cometd_msg_is_successful, TRUE);

  assert_bool_node_func("[{ \"successful\": false }]}",
    cometd_msg_is_successful, FALSE);

  assert_bool_node_func("[1, { \"successful\": true }]}",
    cometd_msg_is_successful, FALSE);
}
END_TEST

START_TEST (test_cometd_msg_has_data)
{
  assert_bool_node_func("[]",
    cometd_msg_has_data, FALSE);

  assert_bool_node_func("{ \"data\": {} }",
    cometd_msg_has_data, TRUE);

  assert_bool_node_func("{ \"no data for you\": {} }",
    cometd_msg_has_data, FALSE);
}
END_TEST

START_TEST (test_cometd_msg_client_id)
{
  JsonNode* n = cometd_json_str2node("{ \"clientId\": \"abcd\" }");
  gchar* client_id = cometd_msg_client_id(n);

  ck_assert_str_eq("abcd", client_id);

  json_node_free(n);
  g_free(client_id);
}
END_TEST

START_TEST (test_cometd_msg_supported_connection_types)
{
  JsonNode* n = json_from_fixture("handshake_resp_test_transport");

  GList* types = cometd_msg_supported_connection_types(n);

  ck_assert_int_eq(3, g_list_length(types));
  ck_assert_str_eq("made-up-transport", g_list_nth_data(types, 0));
  ck_assert_str_eq("test-transport", g_list_nth_data(types, 1));
  ck_assert_str_eq("uno-mas", g_list_nth_data(types, 2));

  g_list_free_full(types, g_free);
  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_advice_none)
{
  JsonNode* n = json_from_fixture("advice_reconnect_none");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_NONE, advice->reconnect);
  ck_assert_int_eq(0, advice->interval);
  cometd_advice_destroy(advice);

  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_advice_handshake)
{
  JsonNode* n = json_from_fixture("advice_reconnect_handshake");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_HANDSHAKE, advice->reconnect);
  ck_assert_int_eq(100, advice->interval);
  cometd_advice_destroy(advice);

  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_advice_retry)
{
  JsonNode* n = json_from_fixture("advice_reconnect_retry");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_RETRY, advice->reconnect);
  ck_assert_int_eq(100, advice->interval);
  cometd_advice_destroy(advice);

  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_advice_null)
{
  JsonNode* n = json_from_fixture("advice_reconnect_null");

  cometd_advice* advice = cometd_msg_advice(n);
  fail_unless(advice == NULL);
  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_extract_connect)
{
  JsonNode* payload = json_from_fixture("connect_payload");
  JsonNode* connect = cometd_msg_extract_connect(payload);
  
  JsonNode* expected_connect = cometd_json_str2node(" \
    { \"channel\": \"/meta/connect\",       \
      \"clientId\": \"Un1q31d3nt1f13r\",    \
      \"connectionType\": \"long-polling\", \
      \"successful\": true }");
  fail_unless(json_node_equal(expected_connect, connect, NULL));

  JsonNode* expected_payload = cometd_json_str2node("[{ \"channel\": \"/foo/bar\"}]");
  fail_unless(json_node_equal(expected_payload, payload, NULL));

  json_node_free(payload);
  json_node_free(connect);
  json_node_free(expected_payload);
  json_node_free(expected_connect);
}
END_TEST

START_TEST (test_cometd_msg_connect_new)
{
  cometd* h = cometd_new();
  cometd_conn* conn = h->conn;

  cometd_conn_set_client_id(conn, "testid");
  cometd_conn_set_transport(conn, &TEST_TRANSPORT);

  JsonNode* msg   = cometd_msg_connect_new(h);
  JsonObject* obj = json_node_get_object(msg);

  const gchar* channel = json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
  fail_unless(strcmp(channel, COMETD_CHANNEL_META_CONNECT) == 0);

  json_node_free(msg);
  cometd_destroy(h);
}
END_TEST

START_TEST (test_cometd_msg_bad_connect_new)
{
  cometd* h = cometd_new();
  cometd_conn* conn = h->conn;

  cometd_conn_set_client_id(conn, "testid");
  cometd_conn_set_transport(conn, &TEST_TRANSPORT);

  JsonNode* msg   = cometd_msg_bad_connect_new(h);
  JsonObject* obj = json_node_get_object(msg);

  const gboolean successful = json_object_get_boolean_member(obj, COMETD_MSG_SUCCESSFUL_FIELD);
  fail_if(successful);
  
  json_node_free(msg);
  cometd_destroy(h);
}
END_TEST

Suite* make_msg_suite (void)
{
  Suite *s = suite_create ("msg");

  TCase *msg_util = tcase_create ("util");
  
  tcase_add_test (msg_util, test_cometd_msg_is_successful);
  tcase_add_test (msg_util, test_cometd_msg_has_data);
  tcase_add_test (msg_util, test_cometd_msg_client_id);
  tcase_add_test (msg_util, test_cometd_msg_supported_connection_types);
  tcase_add_test (msg_util, test_cometd_msg_advice_none);
  tcase_add_test (msg_util, test_cometd_msg_advice_handshake);
  tcase_add_test (msg_util, test_cometd_msg_advice_retry);
  tcase_add_test (msg_util, test_cometd_msg_advice_null);
  tcase_add_test (msg_util, test_cometd_msg_extract_connect);
  suite_add_tcase (s, msg_util);

  TCase *msg_new = tcase_create ("new");
  tcase_add_test (msg_new, test_cometd_msg_connect_new);
  tcase_add_test (msg_new, test_cometd_msg_bad_connect_new);
  suite_add_tcase (s, msg_new);

  return s;
}
