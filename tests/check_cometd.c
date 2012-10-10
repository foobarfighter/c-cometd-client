#include <stdlib.h>
#include <check.h>
#include "../src/cometd.h"
#include "../src/json.h"
#include "../tests/test_helper.h"

#define TEST_SERVER_URL "http://localhost:8089/cometd"

cometd_config* g_config   = NULL;
cometd*        g_instance = NULL;

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

int test_transport_send(JsonNode* node){ return 0; }
int test_transport_recv(JsonNode* node){ return 0; }

START_TEST (test_cometd_transport)
{
  g_config = (cometd_config*) malloc(sizeof(cometd_config));
  cometd_default_config(g_config);

  cometd_transport* transport = malloc(sizeof(cometd_transport));
  transport->name = "test-transport";
  transport->send = test_transport_send;
  transport->recv = test_transport_recv;

  cometd_register_transport(g_config, &transport);
  //g_config->transports;
  cometd_unregister_transport(g_config, "test-transport");

  free(transport);
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

START_TEST (test_cometd_create_handshake_req){
  g_instance = create_cometd();

  long seed = g_instance->conn->_msg_id_seed;

  JsonNode* msg = json_mkobject();
  cometd_create_handshake_req(g_instance, msg);

  double id = JSON_GET_DOUBLE(json_find_member(msg, COMETD_MSG_ID_FIELD));
  fail_unless(id == 1);

  //cometd_message_t message;
  //cometd_create_handshake_req(g_instance, &message);

  //long msg_id = cometd_msg_attr_get(&message, COMETD_MSG_ID_FIELD);
  //fail_unless(msg_id == 1);

  //fail_unless(message.id == g_instance->_msg_id_seed);
  //fail_unless(g_instance->_msg_id_seed == seed + 1);

  //fail_unless(message.version == COMETD_VERSION);
  //fail_unless(message.minimum_version == COMETD_MIN_SUPPORTED_VERSION);
  //fail_unless(message.channel == COMETD_CHANNEL_META_HANDSHAKE);

  //fail_unless(contains(message.supported_connection_types, "long-polling"));
  //fail_unless(contains(message.supported_connection_types, "callback-polling"));
  //fail_unless(same(message.advice, COMETD_HANDSHAKE_ADVICE));
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
  tcase_add_test (tc_unit, test_cometd_create_handshake_req);
  tcase_add_test (tc_unit, test_cometd_transport);
  suite_add_tcase (s, tc_unit);

  /* Integration tests that require cometd server dependency */
  TCase *tc_integration = tcase_create ("Client::Integration");
  tcase_add_checked_fixture (tc_integration, setup, teardown);
  tcase_add_test (tc_integration, test_cometd_successful_init);
  tcase_add_test (tc_integration, test_cometd_successful_handshake);
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
