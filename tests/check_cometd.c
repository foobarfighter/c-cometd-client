#include <stdlib.h>
#include <check.h>
#include "../src/cometd.h"
#include "../tests/test_helper.h"

#define TEST_SERVER_URL "http://localhost:8089/cometd/"

cometd_config* g_config   = NULL;
cometd*        g_instance = NULL;

void setup (void)
{
}

void teardown (void)
{
  if (g_config != NULL){
    free(g_config);
  }
    
  if (g_instance != NULL){
    cometd_destroy(g_instance);
    g_instance = NULL;
  }
}

cometd* create_cometd(){
  g_config = (cometd_config*) malloc(sizeof(cometd_config));
  cometd_default_config(g_config);

  g_config->url = TEST_SERVER_URL;

  return cometd_new(g_config);
}

/*
 *  Unit Test Suite
 */
START_TEST (test_cometd_default_config)
{
  cometd_config config;
  cometd_default_config(&config);

  char* actual_url = "http://example.com/cometd/";

  config.url = actual_url;

  fail_unless(strncmp(config.url, actual_url, sizeof(actual_url)) == 0);
  fail_unless(config.backoff_increment == DEFAULT_BACKOFF_INCREMENT);
}
END_TEST

START_TEST (test_cometd_new)
{
  cometd_config config;
  cometd_default_config(&config);

  config.url = TEST_SERVER_URL;

  cometd* h = cometd_new(&config);
  fail_unless(h->conn->state == COMETD_DISCONNECTED);
  fail_unless(h->config == &config);
  cometd_destroy(h);
}
END_TEST

/*
 *  Integration Suite
 */
START_TEST (test_cometd_successful_init){
  g_instance = create_cometd();
  cometd_init(g_instance);
  fail_unless(g_instance->conn->state == COMETD_CONNECTED);
}
END_TEST

START_TEST (test_cometd_handshake_successful){
  g_instance = create_cometd();
  int code = cometd_handshake(g_instance, NULL);
  fail_unless(code == 0);
}
END_TEST

Suite* cometd_suite (void)
{
  Suite *s = suite_create ("Cometd");

  /* Unit tests */
  TCase *tc_unit = tcase_create ("Client::Unit");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_default_config);
  tcase_add_test (tc_unit, test_cometd_new);
  suite_add_tcase (s, tc_unit);

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Client::Integration");
  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_add_test (tc_integration, test_cometd_successful_init);
  tcase_add_test (tc_integration, test_cometd_handshake_successful);
  suite_add_tcase (s, tc_integration);

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
