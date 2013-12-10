#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "cometd.h"
#include "json.h"
#include "test_helper.h"

START_TEST(test_cometd_json_str2node_stress)
{
  while (1){
    JsonNode* node = cometd_json_str2node("[1, { \"successful\": true } ]");
    json_node_free(node);
  }
}
END_TEST

START_TEST(test_cometd_handshake_stress)
{
  cometd* h = cometd_new();
  cometd_configure(h, COMETDOPT_URL, TEST_SERVER_URL);
  while (1){
    if (COMETD_SUCCESS != cometd_handshake(h, NULL)){
      printf("handshake failed: %s", cometd_last_error(h)->message);
      //sleep(1);
    }
  }
  cometd_destroy(h);
}
END_TEST

static unsigned int test_start_loop (cometd_loop* h) { return 1; }

// leaks
START_TEST (test_cometd_failed_init_loop)
{
  while (1){
    cometd* h = cometd_new();
    cometd_loop loop;
    loop.start = test_start_loop;

    cometd_configure(h, COMETDOPT_URL, TEST_SERVER_URL);
    cometd_configure(h, COMETDOPT_LOOP, &loop);

    int code = cometd_connect(h);
    fail_unless(cometd_conn_is_status(h, COMETD_HANDSHAKE_SUCCESS));
    ck_assert_int_eq(ECOMETD_INIT_LOOP, code);

    cometd_destroy(h);
  }
}
END_TEST


Suite* make_cometd_json_stress_suite (void)
{
  Suite *s = suite_create("CometdJson");

  /* Integration tests that require cometd server dependency */
  TCase *tc = tcase_create ("Stress");

  //tcase_add_checked_fixture (tc, setup, teardown);
  tcase_set_timeout (tc, 10000);
  //tcase_add_test (tc, test_cometd_json_str2node_stress);
  //tcase_add_test (tc, test_cometd_handshake_stress);
  tcase_add_test (tc, test_cometd_failed_init_loop);
  suite_add_tcase (s, tc);

  return s;
}

int
main (void)
{
  g_type_init();

  int number_failed;

  SRunner *sr = srunner_create (make_cometd_json_stress_suite());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
