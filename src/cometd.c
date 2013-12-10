#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <json-glib/json-glib.h>

//#include "ev.h"
#include "cometd.h"
#include "cometd_json.h"
#include "http.h"
#include "transport_long_polling.h"

int  cometd_debug_handler (const cometd*, JsonNode*);
static void cometd_destroy_subscription(gpointer subscription);
static void cometd_impl_set_sys_s(const cometd* h);
static void cometd_impl_destroy_sys_s(const cometd* h);
static int cometd_impl_process_sync(const cometd* h, JsonNode* array);

const gchar*
cometd_get_channel(JsonObject* obj)
{
  g_assert(obj != NULL);

  return json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
}

cometd*
cometd_new(void)
{
  cometd* h = malloc(sizeof(cometd));

  // config
  cometd_config* config = malloc(sizeof(cometd_config));
  config->url = "";
  config->backoff_increment = DEFAULT_BACKOFF_INCREMENT;
  config->max_backoff       = DEFAULT_MAX_BACKOFF;
  config->max_network_delay = DEFAULT_MAX_NETWORK_DELAY;
  config->request_timeout   = DEFAULT_REQUEST_TIMEOUT;
  config->append_message_type_to_url = DEFAULT_APPEND_MESSAGE_TYPE;
  config->transports        = NULL;
  cometd_register_transport(config, &COMETD_TRANSPORT_LONG_POLLING);

  // connection
  GCond* cond = malloc(sizeof(GCond));
  GMutex* mutex = malloc(sizeof(GMutex));
  g_cond_init(cond);
  g_mutex_init(mutex);

  // run loop
  h->loop = cometd_loop_new(gthread, h);

  // inbox
  h->inbox = cometd_inbox_new(h->loop);

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_UNINITIALIZED;
  conn->transport = NULL;
  conn->_msg_id_seed = 0;

  conn->subscriptions = g_hash_table_new(g_str_hash, g_str_equal);

  h->conn = conn;

  // set internal handlers
  cometd_impl_set_sys_s(h);

  // error state
  cometd_error_st* error = malloc(sizeof(cometd_error_st));
  error->code = COMETD_SUCCESS;
  error->message = NULL;;
  
  h->config = config;
  h->last_error = error;

  return h;
}

void
cometd_impl_set_sys_s(const cometd* h)
{
  cometd_sys_s* handlers = &(h->sys_s); 

  handlers->handshake = cometd_add_listener(h, COMETD_CHANNEL_META_HANDSHAKE,
                                           cometd_process_handshake);
  handlers->connect = NULL;
  handlers->subscribe = NULL;
  handlers->unsubscribe = NULL;
  handlers->disconnect = NULL;
}

void
cometd_impl_destroy_sys_s(const cometd* h)
{
  cometd_sys_s* handlers = &(h->sys_s);
  cometd_remove_listener(h, handlers->handshake);
}

static void
cometd_destroy_subscription_list(gpointer key,
                                 gpointer value,
                                 gpointer userdata)
{
  g_list_free_full((GList*)value, free);
}

void
cometd_destroy(cometd* h)
{
  cometd_impl_destroy_sys_s(h);

  // config
  g_list_free_full(h->config->transports, cometd_destroy_transport);
  free(h->config);
 
  // connection
  g_hash_table_foreach(h->conn->subscriptions,
                       cometd_destroy_subscription_list,
                       h);

  g_hash_table_destroy(h->conn->subscriptions);

  free(h->conn);

  // error state
  free(h->last_error);

  // handle
  free(h);
}

#undef cometd_configure
int
cometd_configure(cometd* h, cometd_opt opt, ...)
{
  va_list value;
  va_start(value, opt);

  switch (opt)
  {
    case COMETDOPT_URL:
      h->config->url = va_arg(value, char*);
      break;
    case COMETDOPT_REQUEST_TIMEOUT:
      h->config->request_timeout = va_arg(value, long);
      break;
    case COMETDOPT_LOOP:
      cometd_loop_destroy(h->loop);
      h->loop = va_arg(value, cometd_loop*);
    default:
      return -1;
  }

  va_end(value);

  return 0;
}

int
cometd_connect(const cometd* h)
{
  int error_code = cometd_handshake(h, NULL);

  if (error_code != COMETD_SUCCESS){
    goto error;
  }

  if (cometd_loop_start(h->loop)){
    error_code = cometd_error(h, ECOMETD_INIT_LOOP,
                                 "could not initialize connection loop");
    goto error;
  }

  cometd_conn_set_status(h, COMETD_CONNECTED);

error:
  return error_code;
}

int
cometd_disconnect(const cometd* h, int wait_for_server)
{
  if (wait_for_server){
    // TODO
  } else {
    cometd_conn_set_status(h, COMETD_DISCONNECTED);
  } 
  cometd_loop_stop(h->loop);
}


//TODO: Should this be some sort of macro? I suck at C
int cometd_debug_handler(const cometd* h, JsonNode* node){
  //printf("returning some data from somewhere: \n");
}


/**
 * Reads JsonNodes that are received by the inbox thread.
 * If a handler wishes to to keep a JsonNode in memory
 * longer than the lifetime of a cometd_callback, then it
 * must increase the reference count to the node.
 */
void
cometd_listen(const cometd* h)
{
  while (cometd_conn_is_status(h, COMETD_DISCONNECTED) == FALSE &&
         cometd_conn_is_status(h, COMETD_UNINITIALIZED) == FALSE)
  {
    JsonNode* node;

    while (node = cometd_inbox_take(h->inbox))
    {
      JsonObject* msg = json_node_get_object(node);

      const gchar* channel = cometd_get_channel(msg);
      cometd_fire_listeners(h, channel, node);
      json_node_free(node);
    }
  }
}

int
cometd_publish(const cometd* h, const char* channel, JsonNode* message)
{
  int code = COMETD_SUCCESS;

  JsonNode* node = cometd_new_publish_message(h, channel, message);
  if (node == NULL)
    goto failed_node;

  code = cometd_transport_send(h, node);
  json_node_free(node);

failed_node:
  return code;
}

cometd_subscription*
cometd_subscribe(const cometd* h,
                 char* channel,
                 cometd_callback handler)
{
  int code = COMETD_SUCCESS;
  cometd_subscription* s = NULL;

  if (cometd_is_meta_channel(channel) == FALSE)
  {
    JsonNode* node = cometd_new_subscribe_message(h, channel);
    if (node != NULL)
    {
      code = cometd_transport_send(h, node);
      json_node_free(node);
    }
  }

  if (code == COMETD_SUCCESS)
    s = cometd_add_listener(h, channel, handler);

  return s;
}

int
cometd_unsubscribe(const cometd* h, cometd_subscription* s)
{
  g_return_val_if_fail(s != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(s->channel != NULL, ECOMETD_UNKNOWN);

  const char* channel = s->channel;
  int code = ECOMETD_UNKNOWN;
  JsonNode* node;

  /*
    We can't unsubscribe from a remote meta channel and we only
    want to remotely unsubscribe if this is the last remaining
    listener.
  */
  if (cometd_is_meta_channel(channel) == FALSE &&
      cometd_listener_count(h, channel) == 1)
  {
    JsonNode* node = cometd_new_unsubscribe_message(h, channel);

    if (node != NULL)
    {
      code = cometd_transport_send(h, node);
      json_node_free(node);
    }
  }

  // don't remove the local listener if the remote call failed
  if (code != COMETD_SUCCESS)
    code = cometd_remove_listener(h, s);

  return code;
}

JsonNode*
cometd_recv(const cometd* h)
{
  cometd_transport* t = cometd_current_transport(h);
  JsonNode* node = t->recv(h);
  return node;
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
        cometd_conn_set_transport(h, transport);
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
  cometd_conn_set_client_id(h, json_object_get_string_member(node, "clientId"));
  return h->conn->client_id;
}

int
cometd_handshake(const cometd* h, cometd_callback cb)
{
  JsonNode* handshake   = cometd_new_handshake_message(h);
  gchar*    data        = cometd_json_node2str(handshake);
  int       error_code  = COMETD_SUCCESS;

  if (data == NULL){
    error_code = cometd_error(h, ECOMETD_JSON_SERIALIZE, "could not serialize json");
    goto free_handshake;
  }
  
  char* resp = http_json_post(h->config->url, data, h->config->request_timeout);

  if (resp == NULL){
    error_code = cometd_error(h, ECOMETD_HANDSHAKE, "could not handshake");
    goto free_data;
  }

  JsonNode* payload = cometd_json_str2node(resp);

  if (payload == NULL){
    error_code = cometd_error(h, ECOMETD_JSON_DESERIALIZE, "could not de-serialize json");
    goto free_resp;
  }

  error_code = cometd_impl_process_sync(h, payload);

  // TODO
  if (!cometd_conn_is_status(h, COMETD_HANDSHAKE_SUCCESS)){
    // backoff
    // restart handshake
  }
free_payload:
  json_node_free(payload);
free_resp:
  free(resp);
free_data:
  g_free(data);
free_handshake:
  json_node_free(handshake);

  return error_code;
}

long
cometd_conn_status(const cometd* h)
{
  g_return_val_if_fail(h != NULL, COMETD_UNINITIALIZED);
  g_return_val_if_fail(h->conn != NULL, COMETD_UNINITIALIZED);

  return h->conn->state;
}

long
cometd_conn_is_status(const cometd* h, long status)
{
  long actual = cometd_conn_status(h);
  if (status == COMETD_UNINITIALIZED)
    return actual == COMETD_UNINITIALIZED;

  return actual & status;
}

void
cometd_conn_set_status(const cometd* h, long status)
{
  assert(h != NULL);
  assert(h->conn != NULL);

  h->conn->state = cometd_conn_status(h) | status;
}

char*
cometd_conn_client_id(const cometd* h)
{
  g_return_val_if_fail(h != NULL, NULL);
  g_return_val_if_fail(h->conn != NULL, NULL);

  gboolean has_id = cometd_conn_is_status(h, COMETD_HANDSHAKE_SUCCESS);

  return has_id ? h->conn->client_id : NULL;
}

void
cometd_conn_set_client_id(const cometd* h, const char* id)
{
  g_assert(id != NULL);
  g_assert(strnlen(id, COMETD_MAX_CLIENT_ID_LEN) < COMETD_MAX_CLIENT_ID_LEN);

  strncpy(h->conn->client_id, id, COMETD_MAX_CLIENT_ID_LEN - 1);
}

void
cometd_conn_set_transport(const cometd* h, cometd_transport* t)
{
  g_assert(h != NULL);
  g_assert(h->conn != NULL);
  g_assert(t != NULL);

  h->conn->transport = t;
}

void
cometd_conn_clear_status(const cometd* h)
{
  assert(h != NULL);
  assert(h->conn != NULL);

  h->conn->state = COMETD_UNINITIALIZED;
}

gboolean
cometd_is_meta_channel(const char* channel)
{
  g_return_val_if_fail(channel != NULL, FALSE);
  return strncmp(channel, "/meta", 5) == 0 ? TRUE : FALSE;
}

GHashTable* cometd_conn_subscriptions(const cometd* h)
{
  g_assert(h != NULL);
  g_assert(h->conn != NULL);
  g_assert(h->conn->subscriptions != NULL);

  return h->conn->subscriptions;
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

/**
 * Processes a cometd payload.
 */

int
cometd_impl_process_sync(const cometd* h, JsonNode* root)
{
  JsonArray* arr = json_node_get_array(root);
  GList* msgs = json_array_get_elements(arr);

  GList* item;
  for (item = msgs; item; item = g_list_next(item))
  {
    const JsonNode* msg = item->data;
    const char* channel = cometd_msg_get_channel(msg);
    cometd_fire_listeners(h, channel, msg);
  }
  g_list_free(msgs);

  // TODO: What happens if cometd_fire_listeners blows up?
  return COMETD_SUCCESS;
}

int
cometd_process_handshake(const cometd* h, JsonNode* root)
{
  JsonObject* message = json_node_get_object(root);

  _negotiate_transport(h, message);
  _extract_client_id(h, message);
  cometd_conn_set_status(h, COMETD_HANDSHAKE_SUCCESS);

  return COMETD_SUCCESS;
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
  json_object_set_string_member(obj, "clientId",               cometd_conn_client_id(h));

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

JsonNode*
cometd_new_publish_message(const cometd* h,
                           const char* channel,
                           JsonNode* data)
{
  gint64 seed = ++(h->conn->_msg_id_seed);

  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  json_object_set_int_member(obj, COMETD_MSG_ID_FIELD, seed);

  json_object_set_string_member(obj,
                                COMETD_MSG_CHANNEL_FIELD,
                                channel);

  json_object_set_string_member(obj,
                                COMETD_MSG_CLIENT_ID_FIELD,
                                cometd_conn_client_id(h));

  json_object_set_member(obj,
                         COMETD_MSG_DATA_FIELD,
                         json_node_copy(data));

  json_node_take_object(root, obj);

  return root;
}

JsonNode*
cometd_new_subscribe_message(const cometd* h, const char* channel)
{
  
  gint64 seed = ++(h->conn->_msg_id_seed);

  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  json_object_set_int_member(obj, COMETD_MSG_ID_FIELD, seed);

  json_object_set_string_member(obj,
                                COMETD_MSG_CHANNEL_FIELD,
                                COMETD_CHANNEL_META_SUBSCRIBE);

  json_object_set_string_member(obj,
                                COMETD_MSG_CLIENT_ID_FIELD,
                                cometd_conn_client_id(h));

  json_object_set_string_member(obj,
                                COMETD_MSG_SUBSCRIPTION_FIELD,
                                channel);

  json_node_take_object(root, obj);

  return root;
}

JsonNode*
cometd_new_unsubscribe_message(const cometd* h, const char* channel)
{
  
  gint64 seed = ++(h->conn->_msg_id_seed);

  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  json_object_set_int_member(obj, COMETD_MSG_ID_FIELD, seed);

  json_object_set_string_member(obj,
                                COMETD_MSG_CHANNEL_FIELD,
                                COMETD_CHANNEL_META_UNSUBSCRIBE);

  json_object_set_string_member(obj,
                                COMETD_MSG_CLIENT_ID_FIELD,
                                cometd_conn_client_id(h));

  json_object_set_string_member(obj,
                                COMETD_MSG_SUBSCRIPTION_FIELD,
                                channel);

  json_node_take_object(root, obj);

  return root;
}

int
cometd_transport_send(const cometd* h, JsonNode* msg)
{
  int code = COMETD_SUCCESS;

  cometd_transport* t = cometd_current_transport(h);
  g_return_val_if_fail(t != NULL, ECOMETD_UNKNOWN);

  JsonNode* payload = t->send(h, msg);

  if (payload != NULL) {
    code = cometd_msg_is_successful(payload) ?
              COMETD_SUCCESS : ECOMETD_UNKNOWN;

    cometd_impl_process_sync(h, payload);
    json_node_free(payload);
  }
  
  return code;
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

static gint
find_transport_by_name(gconstpointer a, gconstpointer b){
  const cometd_transport* t = (const cometd_transport*) a;
  return strcmp(t->name, b);
}

int
cometd_unregister_transport(cometd_config* h, const char* name){
  GList* t = g_list_find_custom(h->transports, name, find_transport_by_name);
  if (t == NULL) return 0;

  cometd_transport* transport = (cometd_transport*) t->data;
  h->transports = g_list_remove(h->transports, transport);
  g_free(transport);

  return 0;
}

cometd_transport*
cometd_find_transport(const cometd_config* h, const char *name)
{
  GList* t = g_list_find_custom(h->transports,
                                name,
                                find_transport_by_name);

  return (t == NULL) ? NULL : (cometd_transport*) t->data;
}

void
cometd_destroy_transport(gpointer transport)
{
  g_free(transport);
}

cometd_subscription*
cometd_add_listener(const cometd* h,
                    char * channel,
                    cometd_callback cb)
{
  g_return_val_if_fail(h->conn != NULL, NULL);
  g_return_val_if_fail(h->conn->subscriptions != NULL, NULL);

  GHashTable* subscriptions = h->conn->subscriptions;
  int len                   = strlen(channel);

  g_return_val_if_fail(len > 0 && len < COMETD_MAX_CHANNEL_LEN - 1, NULL);

  cometd_subscription* s = malloc(sizeof(cometd_subscription));
  if (s == NULL)
    goto error;

  strncpy(s->channel, channel, len + 1);
  s->callback = cb;

  /*
    If the list isn't found then lookup will be NULL and prepend
    will create a brand new list for us.
  */
  GList* list = (GList*) g_hash_table_lookup(subscriptions, channel);

  /*
    Prepend to existing list or create brand new list.
  */
  list = g_list_prepend(list, s);

  /*
    We always need to update the value because the pointer
    to the list changes every time we prepend an element.
  */
  g_hash_table_insert(subscriptions, channel, list);

error:
  return s;
}

int
cometd_remove_listener(const cometd* h,
                       cometd_subscription* subscription)
{
  g_return_val_if_fail(h->conn != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(h->conn->subscriptions != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(subscription != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(subscription->channel != NULL, ECOMETD_UNKNOWN);

  GHashTable* subscriptions = h->conn->subscriptions;
  char* channel = subscription->channel;

  GList* list = (GList*) g_hash_table_lookup(subscriptions, channel);

  list = g_list_remove(list, subscription);

  // We need to reset the pointer because it may have changed.
  g_hash_table_insert(subscriptions, channel, list);

  free(subscription);

  return COMETD_SUCCESS;
}

int
cometd_listener_count(const cometd* h, const char* channel)
{
  g_assert(h != NULL);
  g_assert(h->conn != NULL);
  g_assert(h->conn->subscriptions != NULL);

  g_return_val_if_fail(channel != NULL, 0);
  
  GList* list;
  
  list = (GList*) g_hash_table_lookup(h->conn->subscriptions, channel);
  
  return g_list_length(list);
}

int
cometd_fire_listeners(const cometd* h,
                      const char* channel,
                      JsonNode* message)
{
  g_return_val_if_fail(h->conn != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(h->conn->subscriptions != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(channel != NULL, ECOMETD_UNKNOWN);

  GList* list = cometd_channel_subscriptions(h, channel);
  
  // If the list is NULL then, then there are no subscriptions.
  if (list == NULL) { return COMETD_SUCCESS; }

  GList* item;
  for (item = list; item; item = g_list_next(item))
  {
    cometd_subscription* s = (cometd_subscription*) item->data;
    if (s->callback(h, message) != COMETD_SUCCESS) {
      goto error;
    }
  }

  g_list_free(list);

  return COMETD_SUCCESS;
error:
  return ECOMETD_UNKNOWN;
}

gboolean
cometd_channel_is_wildcard(const char* channel)
{
  g_assert(channel != NULL);
  return *(channel + strnlen(channel, COMETD_MAX_CHANNEL_LEN) - 1) == '*';
}


GList*
cometd_channel_subscriptions(const cometd* h, const char* channel)
{
  g_assert(!cometd_channel_is_wildcard(channel));

  GList* subscriptions = NULL;
  GHashTable* map = cometd_conn_subscriptions(h);
  GList* channels = cometd_channel_matches(channel);

  GList* c;
  for (c = channels; c; c = g_list_next(c))
  {
    GList* list = (GList*) g_hash_table_lookup(map, c->data);

    GList* s = NULL;
    for (s = list; s; s = g_list_next(s))
      subscriptions = g_list_prepend(subscriptions, s->data);
  }

  cometd_channel_matches_free(channels);

  return subscriptions;
}

void
cometd_channel_matches_free(GList* matches)
{
  g_list_free_full(matches, g_free);
}

GList* cometd_channel_matches(const char* channel)
{
  g_return_val_if_fail(channel != NULL, NULL);

  GList* channels  = NULL;
  char** parts     = g_strsplit(channel, "/", 0);
  guint  parts_len = g_strv_length(parts);

  if (parts_len > COMETD_MAX_CHANNEL_PARTS) 
    goto free_parts;

  // 254+2 because tmp needs space for (wildcard part + NULL)
  char* tmp[COMETD_MAX_CHANNEL_PARTS+2] = { NULL };

  guint i;
  for (i = 0; i < parts_len; i++)
  {
    tmp[i]   = parts[i];

    // add double wildcard
    if (i < parts_len - 1)
    {
      tmp[i+1] = "**";
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    // add base channel
    } else {
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    }
    
    // add single wildcard
    if (i == parts_len - 2)
    {
      tmp[i+1] = "*";
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    }
  }

free_parts:
  g_strfreev(parts);
  return channels;
}

