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
  fail_unless(strstr(res, "echo") != NULL);
}
END_TEST

START_TEST (test_http_json_post_error_code)
{
  char* res = http_json_post(TEST_ERROR_CODE_URL, "{}", 100000);
  fail_unless(res == NULL);
}
END_TEST

START_TEST (test_http_valid_response)
{
  const char* valid = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application\r\n"
                      "\r\n\r\n"
                      "{}";

  fail_unless(http_valid_response(valid, strlen(valid)));

  const char* invalid = "NOT OK\r\n"
                        "Content-Type: application\r\n"
                        "\r\n\r\n"
                        "{}";

  fail_unless(!http_valid_response(invalid, strlen(invalid)));

  const char* error = "HTTP/1.1 500 Server Error\r\n"
                      "Content-Type: application\r\n"
                      "\r\n\r\n"
                      "{}";

  fail_unless(!http_valid_response(error, strlen(error)));
}
END_TEST

Suite* make_http_integration_suite(void)
{
  Suite *s = suite_create("Http");

  TCase *tc = tcase_create ("Integration");
  tcase_add_checked_fixture (tc, http_setup, http_teardown);
  tcase_add_test (tc, test_http_valid_response);
  tcase_add_test (tc, test_http_json_post_success);
  tcase_add_test (tc, test_http_json_post_error_code);
  suite_add_tcase(s, tc);

  return s;
}
