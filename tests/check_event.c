#include "cometd.h"
#include "check_cometd.h"

static GHashTable* listeners = NULL;

static void setup(void) {
  log_clear();
  listeners = cometd_listener_new();
}
static void teardown(void) {
  cometd_listener_destroy(listeners);
}

START_TEST (test_cometd_listener_add)
{
  ck_assert_int_eq(0, log_size());

  cometd_subscription* s;
  int code;

  JsonNode* message = cometd_json_str2node("{}");

  s    = cometd_listener_add(listeners, "/foo/bar/baz", log_handler);
  code = cometd_listener_fire(listeners, "/foo/bar/baz", NULL, message);

  ck_assert_int_eq(COMETD_SUCCESS, code);
  wait_for_message(100, NULL, "{}");

  code = cometd_listener_remove(listeners, s);
  ck_assert_int_eq(COMETD_SUCCESS, code);

  code = cometd_listener_fire(listeners, "/foo/bar/baz", NULL, message);
  ck_assert_int_eq(COMETD_SUCCESS, code);
  ck_assert_int_eq(0, log_size());

  json_node_free(message);
}
END_TEST

START_TEST (test_cometd_listener_get)
{
  cometd_subscription *s1, *s2, *s3, *s4;

  s1 = cometd_listener_add(listeners, "/foo/*", test_empty_handler);
  s2 = cometd_listener_add(listeners, "/**", test_empty_handler);
  s3 = cometd_listener_add(listeners, "/foo/bar/baz/*", test_empty_handler);
  s4 = cometd_listener_add(listeners, "/foo/bar/baz/bang", test_empty_handler);

  GList* subscriptions = cometd_listener_get(listeners, "/foo/bar/baz/bang");

  fail_if(subscriptions == NULL);
  ck_assert_int_eq(3, g_list_length(subscriptions));

  fail_if(g_list_find(subscriptions, s2) == NULL);
  fail_if(g_list_find(subscriptions, s3) == NULL);
  fail_if(g_list_find(subscriptions, s4) == NULL);

  g_list_free(subscriptions);
}
END_TEST


static GList* handler_data = NULL;
static int handler1(const cometd* h, JsonNode* node)
{
  handler_data = g_list_append(handler_data, "one");
  return 0;
}
static int handler2(const cometd* h, JsonNode* node)
{
  handler_data = g_list_append(handler_data, "two");
  return 0;
}

START_TEST (test_cometd_listener_fire_in_order)
{
  handler_data = NULL;

  cometd_listener_add(listeners, "/foo/bar", handler1);
  cometd_listener_add(listeners, "/foo/bar", handler2);
  cometd_listener_fire(listeners, "/foo/bar", NULL, NULL);

  ck_assert_str_eq("one", g_list_nth_data(handler_data, 0));
  ck_assert_str_eq("two", g_list_nth_data(handler_data, 1));
}
END_TEST

START_TEST (test_cometd_listener_fire_exact_matches_first)
{
  printf("EXACT MATCHES\n");
  handler_data = NULL;

  cometd_listener_add(listeners, "/foo/*", handler1);
  cometd_listener_add(listeners, "/**", handler1);
  cometd_listener_add(listeners, "/foo/bar", handler2);

  cometd_listener_fire(listeners, "/foo/bar", NULL, NULL);
  ck_assert_str_eq("two", g_list_nth_data(handler_data, 0));
  ck_assert_str_eq("one", g_list_nth_data(handler_data, 1));
  ck_assert_str_eq("one", g_list_nth_data(handler_data, 2));
  printf("/EXACT MATCHES\n");
}
END_TEST

START_TEST (test_cometd_listener_fire_with_wildcards)
{
  JsonNode* message = cometd_json_str2node("{}");

  cometd_subscription *s1, *s2;

  s1 = cometd_listener_add(listeners, "/foo/**", log_handler);
  s2 = cometd_listener_add(listeners, "/quux/*", log_handler);

  cometd_listener_fire(listeners, "/foo/bar/baz", NULL, message);
  ck_assert_int_eq(1, log_size());
  cometd_listener_fire(listeners, "/quux/wat/wut", NULL, message);
  ck_assert_int_eq(1, log_size());
  cometd_listener_fire(listeners, "/quux/wat", NULL, message);
  ck_assert_int_eq(2, log_size());

  json_node_free(message);
}
END_TEST

Suite* make_cometd_event_suite (void)
{
  Suite *s = suite_create ("cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("event");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_listener_add);
  tcase_add_test (tc_unit, test_cometd_listener_get);
  tcase_add_test (tc_unit, test_cometd_listener_fire_in_order);
  tcase_add_test (tc_unit, test_cometd_listener_fire_exact_matches_first);
  tcase_add_test (tc_unit, test_cometd_listener_fire_with_wildcards);
  suite_add_tcase (s, tc_unit);

  return s;
}
