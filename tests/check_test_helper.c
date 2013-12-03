#include <check.h>
#include "check_cometd.h"
#include "test_helper.h"
#include "cometd_json.h"

static gboolean
is_match(char* a, char* b, GList* exclude_props)
{
  JsonNode* node_a = cometd_json_str2node(a);
  JsonNode* node_b = cometd_json_str2node(b);

  gboolean ret = json_node_equal(node_a, node_b, exclude_props);

  json_node_free(node_a);
  json_node_free(node_b);

  return ret;
}

static void
assert_json_match(char* a, char* b)
{
  gboolean match = is_match(a, b, NULL);
  fail_unless(match);
}
static void
assert_not_json_match(char* a, char* b)
{
  gboolean match = is_match(a, b, NULL);
  fail_if(match);
}

static void
assert_json_match_excluding(char* a, char* b, GList* excludes)
{
  gboolean match = is_match(a, b, excludes);
  fail_unless(match);
}

START_TEST (test_json_nodes_match)
{
  // JSON_NODE_VALUE
  assert_json_match("1", "1");                  // int
  assert_not_json_match("1", "2");              // int
  assert_not_json_match("1", "\"1\"");          // no conversion
  assert_json_match("\"hi\"", "\"hi\"");        // string
  assert_not_json_match("\"hi\"", "\"hey\"");   // string

  // JSON_NODE_NULL
  assert_json_match("null", "null");            // null equality
  assert_not_json_match("0", "null");           // no conversion
  assert_not_json_match("null", "1");           // null w/ some other type

  // JSON_NODE_ARRAY
  assert_json_match("[1,2,3]", "[1,2,3]");
  assert_not_json_match("[1,2,3]", "[1,3,2]");

  // JSON_NODE_OBJECT
  assert_json_match("{ \"hey\": \"now\" }",
                    "{ \"hey\": \"now\" }");

  assert_json_match("{ \"hey\": [1,2,3] }",
                    "{ \"hey\": [1,2,3] }");

  assert_not_json_match("{ \"hey\": [1,2,3] }",
                        "{ \"hey\": [2,3] }");

  assert_not_json_match("{ \"BYE\": \"now\" }",
                        "{ \"hey\": \"now\" }");

  GList* exclusions = NULL;
  exclusions = g_list_prepend(exclusions, "client_id");

  assert_json_match_excluding("{ \"hey\": \"now\", \"client_id\": 1234 }",
                              "{ \"hey\": \"now\", \"client_id\": 4567 }",
                              exclusions);
}
END_TEST

Suite* make_test_helper_suite (void)
{
  Suite *s = suite_create ("test_helper");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("unit");
  tcase_add_test (tc_unit, test_json_nodes_match);
  suite_add_tcase (s, tc_unit);

  return s;
}
