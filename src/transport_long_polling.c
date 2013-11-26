#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "transport_long_polling.h"
#include "http.h"
#include "cometd.h"
#include "cometd_json.h"

static JsonNode* send(const cometd* h, JsonNode* node);

int
cometd_transport_long_polling_send(const cometd* h, JsonNode* node)
{
  JsonNode* n = send(h, node);

  if (n != NULL)
    json_node_free(n);

  return 0;
}

JsonNode*
cometd_transport_long_polling_recv(const cometd* h)
{
  JsonNode* ret = NULL;
  JsonNode* connect_message = cometd_new_connect_message(h); 
  if (connect_message == NULL)
    goto failed_connect_message;

  ret = send(h, connect_message);
  json_node_free(connect_message);

failed_connect_message:
  return ret;
}

JsonNode*
send(const cometd* h, JsonNode* node)
{
  JsonNode* ret = NULL;
  
  const char* data = cometd_json_node2str(node);
  if (data == NULL)
    goto failed_data;

  const char* resp = http_json_post(h->config->url,
                                    data,
                                    h->config->request_timeout);
  if (resp == NULL)
    goto failed_response;

  printf("raw response: %s\n", resp);
  ret = cometd_json_str2node(resp);

failed_response:
  free(data);
failed_data:
  return ret;
}
