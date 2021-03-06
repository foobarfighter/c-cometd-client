#include "check_cometd.h"

START_TEST (test_cometd_channel_matches)
{
  GList* c1 = cometd_channel_matches("/foo/bar/baz");

  ck_assert_int_eq(5, g_list_length(c1));
  fail_if(g_list_find_custom(c1, "/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/*", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/**", (GCompareFunc)strcmp) == NULL);
  fail_if(g_list_find_custom(c1, "/foo/bar/baz", (GCompareFunc)strcmp) == NULL);

  GList* c2 = cometd_channel_matches("/");
  
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

Suite* make_channel_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("channel");
  tcase_add_test (tc_unit, test_cometd_channel_matches);
  tcase_add_test (tc_unit, test_cometd_channel_is_wildcard);
  suite_add_tcase (s, tc_unit);

  return s;
}
