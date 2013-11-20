#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include <glib.h>
#include <signal.h>

#include "check_cometd.h"
#include "cometd.h"
#include "transport_long_polling.h"
#include "test_helper.h"

static cometd_config* g_config   = NULL;
static cometd*        g_instance = NULL;

int       test_transport_send(const cometd* h, JsonNode* node);
JsonNode* test_transport_recv(const cometd* h);

static cometd_transport TEST_TRANSPORT = {
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

static void setup (void)
{
}

static void teardown (void)
{
  if (g_instance != NULL){
    cometd_destroy(g_instance);
    g_instance = NULL;
  }

  if (g_config != NULL){
    free(g_config);
    g_config = NULL;
  }
}

static cometd*
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

  ck_assert_str_eq(actual_url, config.url);
  ck_assert_int_eq(DEFAULT_BACKOFF_INCREMENT, config.backoff_increment);
  ck_assert_int_eq(DEFAULT_REQUEST_TIMEOUT, config.request_timeout);
  fail_if(cometd_find_transport(&config, "long-polling") == NULL);

  cometd_destroy_config(&config);
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

  cometd_destroy_config(g_config);
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
  ck_assert_int_eq(COMETD_SUCCESS, h->last_error->code);
  cometd_destroy(h);
}
END_TEST

START_TEST (test_cometd_new_connect_message)
{
  g_instance = create_cometd();
  
  strcpy(g_instance->conn->client_id, "testid");
  g_instance->conn->transport = &TEST_TRANSPORT;

  JsonNode* msg = cometd_new_connect_message(g_instance);
  JsonObject* obj = json_node_get_object(msg);
  const gchar* channel = json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
  fail_unless(strcmp(channel, COMETD_CHANNEL_META_CONNECT) == 0);

  json_node_free(msg);
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

START_TEST (test_cometd_error)
{
  g_instance = create_cometd();

  char* message = "hey now";
  int code = 1234;
  int ret = cometd_error(g_instance, 1234, message);

  ck_assert_int_eq(code, ret);

  cometd_error_st* error = cometd_last_error(g_instance);

  ck_assert_int_eq(code, error->code);
  ck_assert_str_eq(message, error->message);
}
END_TEST


Suite* make_cometd_unit_suite (void)
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
  tcase_add_test (tc_unit, test_cometd_error);
  suite_add_tcase (s, tc_unit);

  return s;
}

