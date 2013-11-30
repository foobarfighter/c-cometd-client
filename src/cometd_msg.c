#include "cometd_msg.h"

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

