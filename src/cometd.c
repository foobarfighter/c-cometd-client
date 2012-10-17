#include <stdlib.h>
#include <stddef.h>
#include "cometd.h"
#include "json.h"
#include "http.h"
#include "transport_long_polling.h"

void
cometd_default_config(cometd_config* config){
  config->url = "";
  config->backoff_increment = DEFAULT_BACKOFF_INCREMENT;
  config->max_backoff       = DEFAULT_MAX_BACKOFF;
  config->max_network_delay = DEFAULT_MAX_NETWORK_DELAY;
  config->append_message_type_to_url = DEFAULT_APPEND_MESSAGE_TYPE;
  config->transports = NULL;

  cometd_register_transport(config, &COMETD_TRANSPORT_LONG_POLLING);
}

cometd*
cometd_new(cometd_config* config){
  cometd* h = malloc(sizeof(cometd));

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_DISCONNECTED;
  conn->_msg_id_seed = 0;

  h->conn         = conn;
  h->config       = config;

  return h;
}

int
cometd_init(const cometd* h){
  if (cometd_handshake(h, NULL))
    return 1;
    //return _error(h, "handshake failed: %s", _error_msg(h));

  if (cometd_connect(h, NULL))
    return 1;
    //return _error(h, "connect failed: %s", _error_msg(h));

  return 0;
}

int
_negotiate_transport(const cometd* h, JsonNode* node){
  int code = 0;
  

  return code;
}

int
cometd_handshake(const cometd* h, cometd_callback cb){
  int code = 0;

  JsonNode* msg_handshake_req = json_mkobject();
  cometd_create_handshake_req(h, msg_handshake_req);

  const char* raw_response = http_json_post(h->config->url, json_stringify(msg_handshake_req, NULL));
  json_delete(msg_handshake_req);

  JsonNode* json_response = NULL;

  if (raw_response != NULL){
    json_response = json_decode(raw_response);
    printf("---------> raw_response: %s\n", raw_response);
    code = _negotiate_transport(h, json_response);
  } else {
    code = 1;
  }

  if (raw_response != NULL)
    free(raw_response);

  if (json_response != NULL)
    json_delete(json_response);

  return code;
}


int
cometd_connect(const cometd* h, cometd_callback cb){
  return 0;
}

void
cometd_destroy(cometd* h){
  //g_slist_free(h->transports);
  free(h->conn);
  free(h);
}

int
cometd_create_handshake_req(const cometd* h, JsonNode* root){
  long seed = ++(h->conn->_msg_id_seed);

  json_append_member(root, COMETD_MSG_ID_FIELD,          json_mknumber(seed));
  json_append_member(root, COMETD_MSG_CHANNEL_FIELD,     json_mkstring(COMETD_CHANNEL_META_HANDSHAKE));
  json_append_member(root, COMETD_MSG_VERSION_FIELD,     json_mkstring(COMETD_VERSION));
  json_append_member(root, COMETD_MSG_MIN_VERSION_FIELD, json_mkstring(COMETD_MIN_VERSION));

  // construct advice - TODO: these values should not be hardcoded
  JsonNode* advice = json_mkobject();
  json_append_member(advice, "timeout",  json_mknumber(60000));
  json_append_member(advice, "interval", json_mknumber(0));
  json_append_member(root, COMETD_MSG_ADVICE_FIELD, advice);

  // construct supported transports
  JsonNode* json_transports = json_mkarray();

  GList* entry = h->config->transports;
  while (entry){
    cometd_transport* t = entry->data;
    json_append_element(json_transports, json_mkstring(t->name));
    entry = g_slist_next(entry);
  }
  json_append_member(root, "supportedConnectionTypes", json_transports);

  // call extensions with message - TODO: implement extensions first

  return 0;
}

int
cometd_register_transport(cometd_config* h, const cometd_transport* transport){
  cometd_transport *t = g_new(cometd_transport, 1);

  t->name = transport->name;
  t->send = transport->send;
  t->recv = transport->recv;

  h->transports = g_slist_prepend(h->transports, t);

  return 0;
}

gint
_find_transport_by_name(gconstpointer a, gconstpointer b){
  const cometd_transport* t = (const cometd_transport*) a;
  return strcmp(t->name, b);
}

int
cometd_unregister_transport(cometd_config* h, const char* name){
  GList* t = g_slist_find_custom(h->transports, name, _find_transport_by_name);
  if (t == NULL) return NULL;

  cometd_transport* transport = (cometd_transport*) t->data;
  h->transports = g_slist_remove(h->transports, transport);
  g_free(transport);

  return 0;
}

cometd_transport*
cometd_find_transport(const cometd_config* h, const char *name){
  GList* t = g_slist_find_custom(h->transports, name, _find_transport_by_name);
  return (t == NULL) ? NULL : (cometd_transport*) t->data;

}
