#include <check.h>
#include "check_cometd.h"

void
http_setup (void)
{
}

void
http_teardown (void)
{
}

START_TEST (test_http_json_post)
{
  fail_unless(0);
}
END_TEST

Suite* make_http_integration_suite(void)
{
  Suite *s = suite_create("Http");

  TCase *tc = tcase_create ("Integration");
  tcase_add_checked_fixture (tc, http_setup, http_teardown);
  tcase_add_test (tc, test_http_json_post);
  suite_add_tcase(s, tc);

  return s;
}
