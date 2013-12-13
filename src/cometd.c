#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <json-glib/json-glib.h>

//#include "ev.h"
#include "cometd.h"
#include "json.h"
#include "http.h"
#include "transport_long_polling.h"



int  cometd_debug_handler (const cometd*, JsonNode*);
static void cometd_destroy_subscription(gpointer subscription);
static void cometd_impl_set_sys_s(const cometd* h);
static void cometd_impl_destroy_sys_s(const cometd* h);
static int cometd_impl_process_sync(const cometd* h, JsonNode* array);
static int cometd_impl_handshake(const cometd* h, cometd_callback cb);
static long cometd_impl_compute_backoff(const cometd_config* config, long attempt);

const gchar*
cometd_get_channel(JsonObject* obj)
{
  g_assert(obj != NULL);

  return json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
}

static gboolean g_types_initialized = FALSE;
static void cometd_types_init(void)
{
  if (!g_types_initialized) {
    g_type_init();
    g_types_initialized = TRUE; 
  }
}

cometd*
cometd_new(void)
{
  // we need to initialize glib :-/
  cometd_types_init();

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

  h->conn = cometd_conn_new();                // connection
  h->loop = cometd_loop_new(gthread, h);      // run loop
  h->inbox = cometd_inbox_new(h->loop);       // inbox
  h->subscriptions = cometd_listener_new();   // events

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
 
  cometd_listener_destroy(h->subscriptions);
  cometd_conn_destroy(h->conn);
  // cometd_loop_destroy(h->loop);
  cometd_inbox_destroy(h->inbox);

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
      break;

    case COMETDOPT_BACKOFF_INCREMENT:
      h->config->backoff_increment = va_arg(value, long);
      break;

    case COMETDOPT_MAX_BACKOFF:
      h->config->max_backoff = va_arg(value, long);
      break;
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

  cometd_conn_set_status(h->conn, COMETD_CONNECTED);

error:
  return error_code;
}

int
cometd_disconnect(const cometd* h, int wait_for_server)
{
  if (wait_for_server){
    // TODO
  } else {
    cometd_conn_set_status(h->conn, COMETD_DISCONNECTED);
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
  while (cometd_conn_is_status(h->conn, COMETD_DISCONNECTED) == FALSE &&
         cometd_conn_is_status(h->conn, COMETD_UNINITIALIZED) == FALSE)
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
cometd_handshake(const cometd* h, cometd_callback cb)
{
  cometd_loop* loop = h->loop;
  cometd_conn* conn = h->conn;

  int code = ECOMETD_UNKNOWN;
  long backoff = 0;

  guint attempt;
  for (attempt = 1; cometd_should_handshake(h); ++attempt)
  {
    cometd_loop_wait(loop, backoff);

    code  = cometd_impl_handshake(h, cb);
    backoff = cometd_get_backoff(h, attempt);

    if (backoff == -1)
      break;
  }

  return code;
}

long
cometd_get_backoff(const cometd* h, long attempt)
{
  long backoff;

  cometd_config* config = h->config;
  cometd_advice* advice = h->conn->advice;

  if (!advice)
    backoff = cometd_impl_compute_backoff(config, attempt);
  else if (cometd_advice_is_handshake(advice))
    backoff = advice->interval;
  else if (cometd_advice_is_none(advice))
    backoff = -1;

  return backoff;
}

long
cometd_impl_compute_backoff(const cometd_config* config, long attempt)
{
  if (!config->backoff_increment)
    return -1;

  long backoff = attempt * config->backoff_increment;

  return (config->max_backoff && backoff < config->max_backoff) ? backoff : -1;
}

gboolean
cometd_should_handshake(const cometd* h)
{
  cometd_conn* conn = h->conn;
  
  if (cometd_conn_is_status(conn, COMETD_HANDSHAKE_SUCCESS) ||
      cometd_conn_is_status(conn, COMETD_CONNECTED))
  {
    return FALSE;
  }

  return conn->advice == NULL || cometd_advice_is_handshake(conn->advice);
}

int
cometd_impl_handshake(const cometd* h, cometd_callback cb)
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

  if (!cometd_current_transport(h)){
    error_code = cometd_error(h, ECOMETD_NO_TRANSPORT, "could not find acceptable transport");
    goto free_payload;
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

int
cometd_process_handshake(const cometd* h, JsonNode* msg)
{
  cometd_conn* conn = h->conn;
  cometd_config* config = h->config;

  cometd_transport* t = cometd_transport_negotiate(config->transports, msg);

  int code = COMETD_SUCCESS;
  
  if (t) {
    gchar* client_id = cometd_msg_client_id(msg);
    cometd_conn_set_transport(conn, t);
    cometd_conn_set_client_id(conn, client_id);
    cometd_conn_set_status(conn, COMETD_HANDSHAKE_SUCCESS);
    g_free(client_id);
  } else {
    code = ECOMETD_NO_TRANSPORT;
  }
  cometd_conn_take_advice(conn, cometd_msg_advice(msg));

  return code;
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
  g_assert(h->subscriptions != NULL);

  return h->subscriptions;
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
    char* channel = cometd_msg_channel(msg);
    cometd_fire_listeners(h, channel, msg);
    free(channel);
  }
  g_list_free(msgs);

  // TODO: What happens if cometd_fire_listeners blows up?
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
  json_object_set_string_member(obj, "clientId",               cometd_conn_client_id(h->conn));

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
                                cometd_conn_client_id(h->conn));

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
                                cometd_conn_client_id(h->conn));

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
                                cometd_conn_client_id(h->conn));

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
