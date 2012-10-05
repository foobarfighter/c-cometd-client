#include <stdlib.h>
#include <check.h>
#include "../src/cometd.h"
#include "../tests/test_helper.h"

#define TEST_SERVER_URL "http://localhost:8089/cometd/"

cometd_config* config = NULL;
cometd* instance = NULL;

void setup (void)
{
  config = (cometd_config*) malloc(sizeof(cometd_config));
}

void teardown (void)
{
  if (config != NULL)
    free(config);
  if (instance != NULL)
    free(instance);
}

cometd* create_cometd(){
  cometd_default_config(config);
  config->url = TEST_SERVER_URL;
  cometd_configure(config);
  cometd* h = cometd_new(EV_DEFAULT);
}

START_TEST (test_cometd_configure)
{

  cometd_default_config(config);

  char* actual_url = "http://example.com/cometd/";

  config->url = actual_url;
  cometd_configure(config);

  cometd_config *actual = cometd_configure(NULL);
  fail_unless(strncmp(actual->url, actual_url, sizeof(actual_url)) == 0);
  fail_unless(actual->backoff_increment == DEFAULT_BACKOFF_INCREMENT);
}
END_TEST

START_TEST (test_cometd_new)
{
  cometd_default_config(config);
  config->url = TEST_SERVER_URL;
  cometd_configure(config);

  struct ev_loop* loop = EV_DEFAULT;

  cometd* h = cometd_new(loop);
  // FIXME: This tests passes even if loop isn't assigned
  fail_unless(h->loop == loop);
  fail_unless(h->conn->state == COMETD_DISCONNECTED);
  //fail_unless(h->config == config);
  cometd_destroy(h);
}
END_TEST

START_TEST (test_cometd_successful_init){
  instance = create_cometd();
  cometd_init(instance);

  while (true){
    if (instance->conn->state != COMETD_CONNECTED)
      sleep(1);
  }
}
END_TEST

Suite* cometd_suite (void)
{
  Suite *s = suite_create ("Cometd");

  /* Core test case */
  TCase *tc_unit = tcase_create ("Client::Unit");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_configure);
  tcase_add_test (tc_unit, test_cometd_new);
  suite_add_tcase (s, tc_unit);

  TCase *tc_integration = tcase_create ("Client::Integration");
  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_add_test (tc_integration, test_cometd_successful_init);
  suite_add_tcase (s, tc_integration);

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
