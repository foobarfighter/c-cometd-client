#include "cometd/json.h"
#include <string.h>

gchar*
cometd_json_node2str(JsonNode* node)
{
  JsonGenerator* gen = json_generator_new();
  json_generator_set_root(gen, node);
  gchar* data = json_generator_to_data(gen, NULL);

  g_object_unref(gen);

  return data;
}

JsonNode*
cometd_json_str2node(char* str)
{
  JsonParser* parser = json_parser_new();
  json_parser_load_from_data(parser, str, strlen(str), NULL);

  JsonNode* root = json_parser_get_root(parser);
  JsonNode* copy = json_node_copy(root);

  g_object_unref(parser);

  return copy;
}
