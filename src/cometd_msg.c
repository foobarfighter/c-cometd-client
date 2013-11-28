#include "cometd_msg.h"

/**
 * Take a json
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
