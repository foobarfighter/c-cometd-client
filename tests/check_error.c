#include "check_cometd.h"

START_TEST (test_cometd_error)
{
  cometd* h = cometd_new();

  char* message = "hey now";
  int code = 1234;
  int ret = cometd_error(h, 1234, message);

  ck_assert_int_eq(code, ret);

  cometd_error_st* error = cometd_last_error(h);

  ck_assert_int_eq(code, error->code);
  ck_assert_str_eq(message, error->message);
}
END_TEST

Suite* make_error_suite (void)
{
  Suite *s = suite_create ("cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("error");
  tcase_add_test (tc_unit, test_cometd_error);
  suite_add_tcase (s, tc_unit);

  return s;
}
