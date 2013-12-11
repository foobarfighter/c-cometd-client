#include <check.h>
#include <cometd.h>

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

Suite* make_msg_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("msg");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_msg_is_successful);
  tcase_add_test (tc_unit, test_cometd_msg_has_data);
  tcase_add_test (tc_unit, test_cometd_msg_client_id);
  suite_add_tcase (s, tc_unit);

  return s;
}
