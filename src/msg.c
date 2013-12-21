#include "msg.h"
#include <string.h>

/**
 * Takes a JsonNode and does its best to determine if
 * it's an ack payload with a successful ack message in
 * it.
 */
gboolean
cometd_msg_is_successful(JsonNode* node)
{
  JsonObject* msg = NULL;
  JsonArray* array = NULL;
  gboolean success = FALSE;

  switch (JSON_NODE_TYPE (node))
  {
    case JSON_NODE_ARRAY:
      array = json_node_get_array(node);
      msg = json_array_get_length(array) > 0 ?
        json_array_get_object_element(array, 0) : NULL;
      break;
    case JSON_NODE_OBJECT:
      msg = json_node_get_object(node);
      break;
  }

  if (msg != NULL && json_object_has_member(msg, COMETD_MSG_SUCCESSFUL_FIELD))
    success = json_object_get_boolean_member(msg, COMETD_MSG_SUCCESSFUL_FIELD);

  return success;
}

/**
 * Returns true if the JsonNode is an object and has a member
 * with the `data` attribute.
 */
gboolean
cometd_msg_has_data(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), FALSE);

  gboolean ret;
  JsonObject* obj = json_node_get_object(node);
  ret = json_object_has_member(obj, COMETD_MSG_DATA_FIELD);

  return ret;
}

/**
 * Returns the channel of the message as a new string
 */
char*
cometd_msg_channel(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  const char* channel;

  JsonObject* obj = json_node_get_object(node);
  channel = json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);

  return channel != NULL ? strdup(channel) : NULL;
}

/**
 * Returns the client id of the message as a new string
 */
char*
cometd_msg_client_id(JsonNode* node){
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  const char* client_id;

  JsonObject* obj = json_node_get_object(node);
  client_id = json_object_get_string_member(obj, COMETD_MSG_CLIENT_ID_FIELD);

  return strdup(client_id);
}

GList*
cometd_msg_supported_connection_types(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  JsonObject* obj = json_node_get_object(node);
  JsonArray* arr = json_object_get_array_member(obj,
                                                "supportedConnectionTypes");

  if (!arr)
    return NULL;

  GList *types = NULL, *ielem = NULL;
  GList* items = json_array_get_elements(arr);

  for (ielem = items; ielem; ielem = g_list_next(ielem))
    types = g_list_prepend(types, json_node_dup_string(ielem->data));

  types = g_list_reverse(types);

  g_list_free(items);

  return types;
}

void
cometd_msg_set_boolean_member(JsonNode* node, const char* member, gboolean val)
{
  g_assert(JSON_NODE_HOLDS_OBJECT (node));
  JsonObject* obj = json_node_get_object(node);
  json_object_set_boolean_member(obj, member, val);
}

/**
 * Returns a cometd_advice pointer that should be free'd by the caller.
 * Returns NULL if advice cannot be found.
 *
 * See also: http://svn.cometd.com/trunk/bayeux/bayeux.html#toc_32
 */
cometd_advice*
cometd_msg_advice(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  JsonObject* obj = json_node_get_object(node);
  cometd_advice* advice = NULL;
  JsonObject* advice_obj = NULL;

  if (json_object_has_member(obj, COMETD_MSG_ADVICE_FIELD))
    advice_obj = json_object_get_object_member(obj, COMETD_MSG_ADVICE_FIELD);

  if (!advice_obj)
    return NULL;

  const char* reconnect = json_object_get_string_member(advice_obj,
                                      "reconnect");
  const guint64 interval = json_object_get_int_member(advice_obj, "interval");

  if (reconnect || interval)
  {
    advice = cometd_advice_new();
    advice->interval = interval;

    if (strcmp("none", reconnect) == 0) {
      advice->reconnect = COMETD_RECONNECT_NONE;
    } else if (strcmp("handshake", reconnect) == 0) {
      advice->reconnect = COMETD_RECONNECT_HANDSHAKE;
    } else if (strcmp("retry", reconnect) == 0) {
      advice->reconnect = COMETD_RECONNECT_RETRY;
    }
  }

  return advice;
}

void
cometd_msg_set_advice(JsonNode* msg, const cometd_advice* advice)
{
  g_return_if_fail(JSON_NODE_HOLDS_OBJECT (msg));

  if (!advice) return;

  JsonObject* advice_obj = json_object_new();

  char* reconnect = NULL;
  switch (advice->reconnect)
  {
    case COMETD_RECONNECT_NONE:
      reconnect = "none";
      break;
    case COMETD_RECONNECT_RETRY:
      reconnect = "retry";
      break;
    case COMETD_RECONNECT_HANDSHAKE:
      reconnect = "handshake";
      break;
  } 

  json_object_set_string_member(advice_obj, "reconnect", reconnect);
  json_object_set_int_member(advice_obj, "interval", advice->interval);

  JsonObject* obj = json_node_get_object(msg);
  json_object_set_object_member(obj, COMETD_MSG_ADVICE_FIELD, advice_obj);
}

JsonNode*
cometd_msg_extract_connect(JsonNode* payload)
{
  JsonNode* connect = NULL;
  JsonArray* arr = json_node_get_array(payload);
  GList* msgs = json_array_get_elements(arr);

  gint i;
  gint index = -1;

  GList* item;

  for (i = 0, item = msgs; item; item = g_list_next(item), ++i)
  {
    JsonNode* msg = item->data;
    char* channel = cometd_msg_channel(msg);

    if (strcmp(COMETD_CHANNEL_META_CONNECT, channel) == 0)
      index = i;

    free(channel);
  }

  if (index > -1)
  {
    connect = json_array_dup_element(arr, index);
    json_array_remove_element(arr, index);
  }

  g_list_free(msgs);

  return connect;
}

JsonNode*
cometd_msg_wrap(JsonNode* msg)
{
  JsonNode* payload = json_node_new(JSON_NODE_ARRAY);
  JsonArray* arr = json_array_new();
  json_array_add_element(arr, msg);
  json_node_take_array(payload, arr);
  return payload;
}

JsonNode*
cometd_msg_connect_new(const cometd* h){
  JsonNode*   root = json_node_new(JSON_NODE_OBJECT);
  JsonObject* obj  = json_object_new();

  gint64 seed = ++(h->conn->msg_id_seed);
  char*  connection_type = cometd_current_transport(h)->name;

  json_object_set_int_member   (obj, COMETD_MSG_ID_FIELD,      seed);
  json_object_set_string_member(obj, COMETD_MSG_CHANNEL_FIELD, COMETD_CHANNEL_META_CONNECT);
  json_object_set_string_member(obj, "connectionType",         connection_type);
  json_object_set_string_member(obj, "clientId",               cometd_conn_client_id(h->conn));

  json_node_take_object(root, obj);

  return root;
}

JsonNode*
cometd_msg_bad_connect_new(const cometd* h)
{
  JsonNode* msg = cometd_msg_connect_new(h);
  cometd_msg_set_boolean_member(msg, COMETD_MSG_SUCCESSFUL_FIELD, FALSE);
  return msg;
}
