#include "check_cometd.h"
#include <stdlib.h>

int
main (void)
{
  g_type_init();

  int number_failed;

  SRunner *sr = srunner_create (make_cometd_integration_suite());
  srunner_add_suite (sr, make_cometd_unit_suite());
  srunner_add_suite (sr, make_cometd_msg_suite());
  srunner_add_suite (sr, make_http_integration_suite ());
  srunner_add_suite (sr, make_test_helper_suite ());
  srunner_add_suite (sr, make_cometd_event_suite ());

  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
