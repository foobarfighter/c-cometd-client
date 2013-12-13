#include "check_cometd.h"

static GList* exts = NULL;
static long count = 0;

static void setup(void)
{
  exts = NULL;
  count = 0;
}

static void teardown(void)
{
  g_list_free_full(exts, (GDestroyNotify) cometd_ext_destroy);
}

static void count_callback(const cometd* h, JsonNode* n)
{
  count++;
}

START_TEST (test_cometd_ext)
{
  cometd_ext* ext = cometd_ext_new();

  cometd_ext_add(&exts, ext);
  fail_if(g_list_find(exts, ext) == NULL);
  cometd_ext_remove(&exts, ext);
  fail_unless(g_list_find(exts, ext) == NULL);
}
END_TEST

START_TEST (test_cometd_ext_fire_incoming)
{
  cometd_ext* ext = cometd_ext_new();
  ext->incoming = count_callback;
  cometd_ext_add(&exts, ext);
  cometd_ext_add(&exts, cometd_ext_new());

  cometd_ext_fire_incoming(exts, NULL, NULL);

  ck_assert_int_eq(1, count);
}
END_TEST

START_TEST (test_cometd_ext_fire_outgoing)
{
  cometd_ext* ext = cometd_ext_new();
  ext->outgoing = count_callback;
  cometd_ext_add(&exts, ext);
  cometd_ext_add(&exts, cometd_ext_new());

  cometd_ext_fire_outgoing(exts, NULL, NULL);

  ck_assert_int_eq(1, count);
}
END_TEST

Suite* make_ext_suite (void)
{
  Suite *s = suite_create ("cometd");

  TCase *tc_unit = tcase_create ("ext");
  tcase_add_checked_fixture (tc_unit, setup, teardown);
  tcase_add_test (tc_unit, test_cometd_ext);
  tcase_add_test (tc_unit, test_cometd_ext_fire_incoming);
  tcase_add_test (tc_unit, test_cometd_ext_fire_outgoing);
  suite_add_tcase (s, tc_unit);

  return s;
}
