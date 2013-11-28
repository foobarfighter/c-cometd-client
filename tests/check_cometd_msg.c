#include <check.h>
#include <cometd.h>

static void setup (void)
{
}

static void teardown (void)
{
}

START_TEST (test_cometd_msg_is_successful)
{
  JsonNode* n;
  gboolean ret;

  n = cometd_json_str2node("[{ \"successful\": true }]}");
  ret = cometd_msg_is_successful(n);
  fail_unless(ret == TRUE);
  json_node_free(n);

  n = cometd_json_str2node("[{ \"successful\": false }]}");
  ret = cometd_msg_is_successful(n);
  fail_unless(ret == FALSE);
  json_node_free(n);

  n = cometd_json_str2node("[1, { \"successful\": true }]}");
  ret = cometd_msg_is_successful(n);
  fail_unless(ret == FALSE);
  json_node_free(n);
}
END_TEST

Suite* make_cometd_msg_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("msg");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_msg_is_successful);
  suite_add_tcase (s, tc_unit);

  return s;
}
