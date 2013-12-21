#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "transport_long_polling.h"
#include "http.h"
#include "cometd.h"
#include "json.h"

static JsonNode* send(const cometd* h, JsonNode* node);

JsonNode*
cometd_transport_long_polling_send(const cometd* h, JsonNode* node)
{
  return send(h, node);
}

JsonNode*
cometd_transport_long_polling_recv(const cometd* h)
{
  JsonNode* ret = NULL;
  JsonNode* connect_message = cometd_msg_wrap(cometd_msg_connect_new(h)); 
  
  if (connect_message == NULL)
    goto failed_connect_message;

  ret = send(h, connect_message);
  json_node_free(connect_message);

failed_connect_message:
  return ret;
}

static JsonNode*
send(const cometd* h, JsonNode* node)
{
  JsonNode* ret = NULL;
  
  char* data = cometd_json_node2str(node);
  if (data == NULL)
    goto failed_data;

  char* resp = http_json_post(h->config->url,
                              data,
                              h->config->request_timeout);
  if (resp == NULL)
    goto failed_response;

  ret = cometd_json_str2node(resp);

  free(resp);
failed_response:
  free(data);
failed_data:
  return ret;
}
