#include <check.h>
#include "check_cometd.h"
#include "cometd.h"

static cometd_inbox* inbox = NULL;

static cometd_loop* cometd_loop_test_new(cometd* cometd)
{
  return cometd_loop_malloc(cometd);
}

static void setup (void)
{
  cometd* h = cometd_new();
  cometd_loop* loop = cometd_loop_new(test, h);
  cometd_configure(h, COMETDOPT_LOOP, loop);
  inbox = cometd_inbox_new(loop);
}

static void teardown (void)
{
  cometd_inbox_destroy(inbox);
}

START_TEST (test_cometd_inbox_push)
{

}
END_TEST

Suite* make_cometd_inbox_suite (void)
{
  Suite *s = suite_create ("cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("inbox");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_inbox_push);
  suite_add_tcase (s, tc_unit);

  return s;
}
