#ifndef COMETD_JSON_H
#define COMETD_JSON_H

#include "../cometd.h"

gchar*    cometd_json_node2str(JsonNode* node);
JsonNode* cometd_json_str2node(char* str);

#endif  // COMETD_JSON_H
