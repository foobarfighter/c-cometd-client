#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <json-glib/json-glib.h>

#include "../deps/ev/ev.h"

#include "cometd.h"
#include "cometd_json.h"
#include "http.h"
#include "transport_long_polling.h"

int  cometd_debug_handler (const cometd*, JsonNode*);
void _process_payload     (JsonArray *array, guint idx, JsonNode* node, gpointer data);

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
  g_type_init();  // WTF

  cometd* h = malloc(sizeof(cometd));

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_DISCONNECTED;
  conn->_msg_id_seed = 0;

  cometd_error_st* error = malloc(sizeof(cometd_error_st));
  error->code = COMETD_SUCCESS;
  error->message = NULL;;

  h->conn         = conn;
  h->config       = config;
  h->last_error   = error;

  return h;
}

//TODO: Should this be some sort of macro? I suck at C
int cometd_debug_handler(const cometd* h, JsonNode* node){
  //printf("returning some data from somewhere: \n");
}

int
cometd_init(const cometd* h){
  //struct ev_loop *loop = malloc(ev_loop);

  if (cometd_handshake(h, cometd_debug_handler))
    return 1;
    //return _error(h, "handshake failed: %s", _error_msg(h));

  _cometd_run_loop(h);


  return 0;
}

int
_cometd_run_loop(const cometd* h){
  do {
    JsonNode* node = h->conn->transport->recv(h);

  } while (!(h->conn->state & (COMETD_DISCONNECTING | COMETD_DISCONNECTED)));
}


int
_negotiate_transport(const cometd* h, JsonObject* obj)
{
  int found = 0;

  JsonArray*  types = json_object_get_array_member(obj, "supportedConnectionTypes");

  if (!types || json_array_get_length(types) == 0) return 0;

  GList* client_entry      = h->config->transports;
  GList* server_entry_list = json_array_get_elements(types);

  // Loop through the client side transports
  while (client_entry){
    cometd_transport* transport = client_entry->data;

    GList* server_entry = server_entry_list;

    // Loop through the list of connection types supported by the server 
    while (server_entry){
      if (!strcmp(transport->name, json_node_get_string(server_entry->data))){
        h->conn->transport = transport;
        found = 1;
        break;
      }
      server_entry = g_list_next(server_entry);
    }

    client_entry = g_list_next(client_entry);

    if (found){ break; }
  }

  g_list_free(server_entry_list);

  // TODO: there has to be a better way to do this in C
  return (found == 1) ? 0 : 1;
}

char*
_extract_client_id(const cometd* h, JsonObject* node){
  return strncpy(
    h->conn->client_id,
    json_object_get_string_member(node, "clientId"),
    COMETD_MAX_CLIENT_ID_LEN
  );
}

int
cometd_handshake(const cometd* h, cometd_callback callback)
{
  JsonNode* handshake = cometd_new_handshake_message(h);
  gchar*    data      = cometd_json_node2str(handshake);

  if (data == NULL) {
    return cometd_error(h, COMETD_ERROR_JSON, "could not serialize json");
  }

  json_node_free(handshake);

  char* resp = http_json_post(h->config->url, data);
  g_free(data);

  if (resp != NULL) {
    JsonNode* payload = cometd_json_str2node(resp);
    free(resp);

    if (payload != NULL) {
      cometd_process_payload(h, payload);
      json_node_free(payload);
    } else {
      return cometd_error(h, COMETD_ERROR_JSON, "could not de-serialize json");
    }
  } else {
    return cometd_error(h, COMETD_ERROR_HANDSHAKE, "could not handshake");
  }
  
  return COMETD_SUCCESS;
}

int
cometd_error(const cometd* h, int code, char* message)
{
  printf("%d: %s\n", code, message);
  h->last_error->code = code;
  h->last_error->message = message;
  return code;
}

cometd_error_st*
cometd_last_error(const cometd* h)
{
  return h->last_error;
}

void
_process_message(JsonArray *array,guint idx, JsonNode* node, gpointer data)
{
  const cometd* h = (const cometd*) data;
  JsonObject* message = json_node_get_object(node);
  cometd_process_message(h, message);
}

void
cometd_process_message(const cometd* h, JsonObject* message)
{
  const gchar* channel = json_object_get_string_member(message,
                                    COMETD_MSG_CHANNEL_FIELD);

  //g_return_val_if_fail(channel != NULL, void);

  if (strcmp(channel, COMETD_CHANNEL_META_HANDSHAKE) == 0) {
    cometd_process_handshake(h, message);
  }
  //cometd_fire_handlers(h, message);
}

void
cometd_process_payload(const cometd* h, JsonNode* root)
{
  JsonArray* messages = json_node_get_array(root);
  json_array_foreach_element(messages, _process_message, (cometd*) h);
}

void
cometd_process_handshake(const cometd* h, JsonObject* message)
{
  _negotiate_transport(h, message);
  _extract_client_id(h, message);
}

cometd_transport*
cometd_current_transport(const cometd* h){
  g_return_val_if_fail(h != NULL, NULL);
  g_return_val_if_fail(h->conn != NULL, NULL);
  
  return h->conn->transport;
}

JsonNode*
cometd_new_connect_message(const cometd* h){
  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  gint64 seed = ++(h->conn->_msg_id_seed);
  char*  connection_type = cometd_current_transport(h)->name;

  json_object_set_int_member   (obj, COMETD_MSG_ID_FIELD,      seed);
  json_object_set_string_member(obj, COMETD_MSG_CHANNEL_FIELD, COMETD_CHANNEL_META_CONNECT);
  json_object_set_string_member(obj, "connectionType",         connection_type);
  json_object_set_string_member(obj, "clientId",               h->conn->client_id);

  json_node_take_object(root, obj);

  return root;
}

JsonNode*
cometd_new_handshake_message(const cometd* h)
{
  gint64 seed = ++(h->conn->_msg_id_seed);

  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  json_object_set_int_member   (obj, COMETD_MSG_ID_FIELD,          seed);
  json_object_set_string_member(obj, COMETD_MSG_CHANNEL_FIELD,     COMETD_CHANNEL_META_HANDSHAKE);
  json_object_set_string_member(obj, COMETD_MSG_VERSION_FIELD,     COMETD_VERSION);
  json_object_set_string_member(obj, COMETD_MSG_MIN_VERSION_FIELD, COMETD_MIN_VERSION);

  // construct advice - TODO: these values should not be hardcoded
  JsonObject* advice = json_object_new();
  json_object_set_int_member(advice, "timeout",  60000);
  json_object_set_int_member(advice, "interval", 0);
  json_object_set_object_member(obj, COMETD_MSG_ADVICE_FIELD, advice);

  // construct supported transports
  JsonArray* json_transports = json_array_new();

  GList* entry = h->config->transports;
  while (entry){
    cometd_transport* t = entry->data;
    json_array_add_string_element(json_transports, t->name);
    entry = g_list_next(entry);
  }
  json_object_set_array_member(obj, "supportedConnectionTypes", json_transports);

  // call extensions with message - TODO: implement extensions first
  json_node_take_object(root, obj);

  return root;
}

int
cometd_transport_send(const cometd* h, JsonNode* msg){
  cometd_transport* t = cometd_current_transport(h);

  g_return_val_if_fail(t != NULL, 1);

  return t->send(h, msg);
}

int
cometd_connect(const cometd* h, cometd_callback cb){
  JsonNode* msg = cometd_new_connect_message(h);

  int ret = 0;
  if (ret = cometd_transport_send(h, msg)){
    return ret;
  }

  h->conn->state = COMETD_CONNECTED;
  return ret;
}


void
cometd_destroy(cometd* h){
  //g_slist_free(h->transports);
  free(h->conn);
  free(h);
}

int
cometd_register_transport(cometd_config* h, const cometd_transport* transport){
  cometd_transport *t = g_new(cometd_transport, 1);

  t->name = transport->name;
  t->send = transport->send;
  t->recv = transport->recv;

  h->transports = g_list_prepend(h->transports, t);

  return 0;
}

gint
_find_transport_by_name(gconstpointer a, gconstpointer b){
  const cometd_transport* t = (const cometd_transport*) a;
  return strcmp(t->name, b);
}

int
cometd_unregister_transport(cometd_config* h, const char* name){
  GList* t = g_list_find_custom(h->transports, name, _find_transport_by_name);
  if (t == NULL) return 0;

  cometd_transport* transport = (cometd_transport*) t->data;
  h->transports = g_list_remove(h->transports, transport);
  g_free(transport);

  return 0;
}

cometd_transport*
cometd_find_transport(const cometd_config* h, const char *name){
  GList* t = g_list_find_custom(h->transports, name, _find_transport_by_name);
  return (t == NULL) ? NULL : (cometd_transport*) t->data;

}

cometd_subscription*
cometd_add_listener(const cometd* h, const char * channel, cometd_callback cb){
  return 0;
}
