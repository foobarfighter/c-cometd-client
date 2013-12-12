#include "check_cometd.h"

static void setup (void)
{
}

static void teardown (void)
{
}

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

  g_list_free(types);
  json_node_free(n);
}
END_TEST

START_TEST (test_cometd_msg_advice_none)
{
  JsonNode* n = json_from_fixture("advice_reconnect_none");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_NONE, advice->reconnect);
  ck_assert_int_eq(0, advice->interval);
}
END_TEST

START_TEST (test_cometd_msg_advice_handshake)
{
  JsonNode* n = json_from_fixture("advice_reconnect_handshake");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_HANDSHAKE, advice->reconnect);
  ck_assert_int_eq(100, advice->interval);
}
END_TEST

START_TEST (test_cometd_msg_advice_retry)
{
  JsonNode* n = json_from_fixture("advice_reconnect_retry");

  cometd_advice* advice = cometd_msg_advice(n);
  ck_assert_int_eq(COMETD_RECONNECT_RETRY, advice->reconnect);
  ck_assert_int_eq(100, advice->interval);
}
END_TEST

START_TEST (test_cometd_msg_advice_null)
{
  JsonNode* n = json_from_fixture("advice_reconnect_null");

  cometd_advice* advice = cometd_msg_advice(n);
  fail_unless(advice == NULL);
}
END_TEST

Suite* make_msg_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("msg");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  
  tcase_add_test (tc_unit, test_cometd_msg_is_successful);
  tcase_add_test (tc_unit, test_cometd_msg_has_data);
  tcase_add_test (tc_unit, test_cometd_msg_client_id);
  tcase_add_test (tc_unit, test_cometd_msg_supported_connection_types);
  tcase_add_test (tc_unit, test_cometd_msg_advice_none);
  tcase_add_test (tc_unit, test_cometd_msg_advice_handshake);
  tcase_add_test (tc_unit, test_cometd_msg_advice_retry);
  tcase_add_test (tc_unit, test_cometd_msg_advice_null);

  suite_add_tcase (s, tc_unit);

  return s;
}
