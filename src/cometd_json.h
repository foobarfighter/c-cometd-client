#ifndef COMETD_JSON_H
#define COMETD_JSON_H

#include <glib.h>
#include <json-glib/json-glib.h>

gchar*    cometd_json_node2str(JsonNode* node);
JsonNode* cometd_json_str2node(char* str);

#endif  // COMETD_JSON_H
