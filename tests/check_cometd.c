#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <glib.h>
#include <signal.h>

#include "cometd.h"
#include "transport_long_polling.h"
#include "test_helper.h"

#define TEST_SERVER_URL "http://localhost:8089/cometd"

cometd_config* g_config   = NULL;
cometd*        g_instance = NULL;

int       test_transport_send(const cometd* h, JsonNode* node);
JsonNode* test_transport_recv(const cometd* h);

static const cometd_transport TEST_TRANSPORT = {
  "test-transport",
   test_transport_send,
   test_transport_recv
};

int test_transport_send(const cometd* h, JsonNode* node)
{
  return 0;
}

JsonNode* test_transport_recv(const cometd* h)
{
  return 0;
}

void setup (void)
{
}

void teardown (void)
{
  if (g_config != NULL){
    free(g_config);
    g_config = NULL;
  }
    
  if (g_instance != NULL){
    cometd_destroy(g_instance);
    g_instance = NULL;
  }
}

cometd*
create_cometd(){
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
  fail_if(cometd_find_transport(&config, "long-polling") == NULL);
}
END_TEST

START_TEST (test_cometd_transport)
{
  g_config = (cometd_config*) malloc(sizeof(cometd_config));
  cometd_default_config(g_config);

  // should have default transports + test-transport
  cometd_register_transport(g_config, &TEST_TRANSPORT);

  // default transports + test-transport
  fail_unless(g_list_length(g_config->transports) == 2);

  // should not be able to find a transport that doesn't exist
  cometd_transport* nullptr = cometd_find_transport(g_config, "0xdeadbeef");
  fail_unless(nullptr == NULL);

  // transport should be found by name
  cometd_transport* t = cometd_find_transport(g_config, "test-transport");
  fail_if(t == NULL);
  fail_unless(strcmp(t->name, "test-transport") == 0);

  // removing the transport should make it un-findable
  cometd_unregister_transport(g_config, t->name);
  fail_unless(cometd_find_transport(g_config, "test-transport") == NULL);
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

START_TEST (test_cometd_new_connect_message)
{
  g_instance = create_cometd();

  g_instance->conn->transport = &TEST_TRANSPORT;

  JsonNode* msg = cometd_new_connect_message(g_instance);
  JsonObject* obj = json_node_get_object(msg);
  const gchar* channel = json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
  fail_unless(strcmp(channel, COMETD_CHANNEL_META_CONNECT) == 0);
}
END_TEST


START_TEST (test_cometd_new_handshake_message){
  g_instance = create_cometd();

  long seed = g_instance->conn->_msg_id_seed;

  JsonNode* msg = cometd_new_handshake_message(g_instance);
  JsonObject* obj = json_node_get_object(msg);
  int id = json_object_get_int_member(obj, COMETD_MSG_ID_FIELD);

  json_node_free(msg);

  fail_unless(id == 1);
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

START_TEST (test_cometd_successful_handshake){
  g_instance = create_cometd();

  int code = cometd_handshake(g_instance, NULL);
  fail_unless(code == 0);
  fail_unless(strcmp(g_instance->conn->transport->name, "long-polling") == 0);
  fail_unless(g_instance->conn->client_id != NULL);
}
END_TEST

START_TEST (test_cometd_unsuccessful_handshake_without_advice)
{
  g_instance = create_cometd();

  ck_assert_int_eq(0, cometd_unregister_transport(g_instance->config, "long-polling"));
  ck_assert_int_eq(0, g_list_length(g_instance->config->transports));
  ck_assert_int_eq(0, cometd_register_transport(g_instance->config, &TEST_TRANSPORT));

  ck_assert_int_eq(0, cometd_handshake(g_instance, NULL));
}
END_TEST

START_TEST (test_cometd_unsuccessful_handshake_with_advice)
{
  //g_instance = create_cometd();

  //int code = cometd_handshake(g_instance, logger_handler)
}
END_TEST


static GCond cometd_init_cond;
static GMutex cometd_init_mutex;
static gboolean cometd_initialized = FALSE;

gpointer cometd_init_thread(gpointer data){
  cometd* instance = (cometd*) data;
  cometd_init(instance);
  fail_unless(g_instance->conn->state == COMETD_CONNECTED);

  g_mutex_lock(&cometd_init_mutex);
  cometd_initialized = TRUE;
  g_cond_signal(&cometd_init_cond);
  g_mutex_unlock(&cometd_init_mutex);
}

START_TEST (test_cometd_send_and_receive_message){
  g_instance = create_cometd();
  cometd_add_listener(g_instance, "/meta/**", inbox_handler);
  cometd_add_listener(g_instance, "/echo/message/test", inbox_handler);

  GThread* t = g_thread_new("cometd_init thread", &cometd_init_thread, g_instance);

  g_mutex_lock(&cometd_init_mutex);
  while (cometd_initialized == FALSE)
    g_cond_wait(&cometd_init_cond, &cometd_init_mutex);

  printf("this is where ill send messages\n");

  //cometd_send(g_instance, "/echo/message/test", create_message_from_json(json_str));
  //fail_unless(check_inbox_for_message(json_str, 10));
  //cometd_disconnect(g_instance);

  g_mutex_unlock(&cometd_init_mutex);

  gpointer ret = g_thread_join(t);

  fail();

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
  tcase_add_test (tc_unit, test_cometd_new_connect_message);
  tcase_add_test (tc_unit, test_cometd_new_handshake_message);
  tcase_add_test (tc_unit, test_cometd_transport);
  suite_add_tcase (s, tc_unit);

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Client::Integration");
  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_set_timeout (tc_integration, 15);
  //tcase_add_test (tc_integration, test_cometd_successful_init);
  tcase_add_test (tc_integration, test_cometd_successful_handshake);
  //tcase_add_test (tc_integration, test_cometd_unsuccessful_handshake_with_advice);
  //tcase_add_test (tc_integration, test_cometd_unsuccessful_handshake_without_advice);
  //tcase_add_test (tc_integration, test_cometd_send_and_receive_message);
  suite_add_tcase (s, tc_integration);

  return s;
}

int
main (void)
{
  signal(SIGSEGV, error_handler);

  int number_failed;
  Suite *s = cometd_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
