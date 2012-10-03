//int
//main (void)
//{
//  return 0;
//}

#include <stdlib.h>
#include <check.h>
#include "../src/cometd.h"

cometd_config* config = NULL;

void setup (void)
{
  config = NULL;
}

void teardown (void)
{
  if (config != NULL)
    free(config);
}

START_TEST (test_cometd_init)
{
  config = (cometd_config*) malloc(sizeof(cometd_config));

  cometd_set_default_config(config);

  char* actual_url = "http://example.com/cometd/";

  config->url = actual_url;
  cometd_configure(config);

  cometd_config *actual = cometd_configure(NULL);
  fail_unless(strncmp(actual->url, actual_url, sizeof(actual_url)) == 0);
  fail_unless(actual->backoff_increment == DEFAULT_BACKOFF_INCREMENT);
}
END_TEST

Suite *
cometd_suite (void)
{
  Suite *s = suite_create ("Cometd");

  /* Core test case */
  TCase *tc_core = tcase_create ("Client");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_cometd_init);
  suite_add_tcase (s, tc_core);

  /* Limits test case */
  //TCase *tc_limits = tcase_create ("Limits");
  //tcase_add_test (tc_limits, test_cometd_create_neg);
  //tcase_add_test (tc_limits, test_cometd_create_zero);
  //suite_add_tcase (s, tc_limits);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = cometd_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
