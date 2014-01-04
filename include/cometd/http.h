#ifndef COMETD_HTTP_H
#define COMETD_HTTP_H

#include "../cometd.h"

char* http_json_post(const char* url, const char* data, int timeout);
JsonNode* http_post_msg(const cometd* h, JsonNode* msg);

#endif // COMETD_HTTP_H
