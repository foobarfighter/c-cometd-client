#include "msg.h"

/**
 * Takes a JsonNode and does its best to determine if
 * it's an ack payload with a successful ack message in
 * it.
 */
gboolean
cometd_msg_is_successful(JsonNode* node)
{
  JsonObject* msg;
  JsonArray* array;
  gboolean success = FALSE;

  switch (JSON_NODE_TYPE (node))
  {
    case JSON_NODE_ARRAY:
      array = json_node_get_array(node);
      msg = json_array_get_length(array) > 0 ?
        json_array_get_object_element(array, 0) : NULL;
      break;
  }

  if (msg != NULL)
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
const gchar*
cometd_msg_get_channel(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);
  JsonObject* obj = json_node_get_object(node);
  return json_object_get_string_member(obj, COMETD_MSG_CHANNEL_FIELD);
}

/**
 * Returns the client id of the message as a new string
 */
gchar*
cometd_msg_client_id(JsonNode* node){
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  const gchar* client_id;

  JsonObject* obj = json_node_get_object(node);
  client_id = json_object_get_string_member(obj, COMETD_MSG_CLIENT_ID_FIELD);

  return g_strdup(client_id);
}

GList*
cometd_msg_supported_connection_types(JsonNode* node)
{
  g_return_val_if_fail(JSON_NODE_HOLDS_OBJECT (node), NULL);

  JsonObject* obj = json_node_get_object(node);
  JsonArray* arr = json_object_get_array_member(obj, "supportedConnectionTypes");

  if (!arr)
    return NULL;

  GList *types = NULL, *ielem = NULL;
  GList* items = json_array_get_elements(arr);

  for (ielem = items; ielem; ielem = g_list_next(ielem))
    types = g_list_prepend(types, json_node_get_string(ielem->data));

  types = g_list_reverse(types);

  return types;
}
