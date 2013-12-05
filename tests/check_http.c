#include <check.h>
#include <string.h>
#include "check_cometd.h"
#include "test_helper.h"
#include "http.h"

void
http_setup (void)
{
}

void
http_teardown (void)
{
}

START_TEST (test_http_json_post_success)
{
  char* res = http_json_post(TEST_SUCCESS_CODE_URL, "{}", 100000);

  fail_if(res == NULL);

  // CK_FORK: This is a really ghetto way of continuing to run this suite
  // in CK_FORK=no mode.  fail_if doesn't break flow control if CK_FORK=no
  // because check has no way of catching the assertion and continuing to run.
  if (res != NULL){
    fail_unless(strstr(res, "echo") != NULL);
    free(res);
  }
}
END_TEST

START_TEST (test_http_json_post_error_code)
{
  char* res = http_json_post(TEST_ERROR_CODE_URL, "{}", 100000);
  fail_unless(res == NULL);
}
END_TEST

Suite* make_http_integration_suite(void)
{
  Suite *s = suite_create("Http");

  TCase *tc = tcase_create ("Integration");
  tcase_add_checked_fixture (tc, http_setup, http_teardown);
  tcase_add_test (tc, test_http_json_post_success);
  tcase_add_test (tc, test_http_json_post_error_code);
  suite_add_tcase(s, tc);

  return s;
}
