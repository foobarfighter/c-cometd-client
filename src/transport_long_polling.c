#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "transport_long_polling.h"
#include "http.h"
#include "cometd.h"

const char* _lp_send(const cometd* h, JsonNode* node);
JsonNode*   _poll(const cometd* h);

int
cometd_transport_long_polling_send(const cometd* h, JsonNode* node)
{
  const char* raw_response = _lp_send(h, node);

  return 0;
}

JsonNode*
cometd_transport_long_polling_recv(const cometd* h){
  JsonNode* n = _poll(h);
  return n;
}

JsonNode*
_poll(const cometd* h){
  printf("=====polling\n");
  // Blocks
  const char* resp = _lp_send(h, cometd_new_connect_message(h));

  printf("=====polling2\n");
  //json_node_free(msg);

  JsonNode* ret = NULL;

  // Parse JSON data
  if (resp != NULL){
    JsonParser* parser = json_parser_new();  //TODO: should be static var
    printf("=====polling3\n");
    json_parser_load_from_data(parser, resp, strlen(resp), NULL);
    printf("=====polling4: %s\n", resp);
    ret = json_parser_get_root(parser);
    printf("=====polling5\n");
    g_object_unref(parser);
  }

  // This should probably get updated in the meta/connect handler
  //if (ret != NULL){
  //  h->conn->state = COMETD_CONNECTED;
  //}

  printf("=====polling6\n");

  return ret;
}

const char*
_lp_send(const cometd* h, JsonNode* node){
  gsize len = 0;

  JsonGenerator* gen = json_generator_new();
  json_generator_set_root(gen, node);
  gchar* data = json_generator_to_data(gen, &len);

  printf("=====_sending\n");

  return http_json_post(h->config->url, data, h->config->request_timeout);
}

