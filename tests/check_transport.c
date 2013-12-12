#include "check_cometd.h"

static cometd_transport TEST_TRANSPORT = { "test-transport", NULL, NULL };
static GList* g_registry = NULL;

static void setup (void)
{
  g_registry = g_list_prepend(g_registry, &TEST_TRANSPORT);
}

static void teardown (void)
{
  g_list_free(g_registry);
}

START_TEST (test_cometd_transport_negotiate)
{
  JsonNode* node;

  node = json_from_fixture("handshake_resp_test_transport");
  cometd_transport* t1 = cometd_transport_negotiate(g_registry, node);
  ck_assert_str_eq("test-transport", t1->name);
  json_node_free(node);

  node = json_from_fixture("handshake_resp_missing_transports");
  cometd_transport* t2 = cometd_transport_negotiate(g_registry, node);
  fail_unless(t2 == NULL);
  json_node_free(node);

  node = json_from_fixture("handshake_resp_unsupported_transports");
  cometd_transport* t3 = cometd_transport_negotiate(g_registry, node);
  fail_unless(t3 == NULL);
  json_node_free(node);
}
END_TEST

Suite* make_transport_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("transport");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_transport_negotiate);
  suite_add_tcase (s, tc_unit);

  return s;
}
