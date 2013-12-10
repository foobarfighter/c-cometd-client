#ifndef COMETD_MSG_H
#define COMETD_MSG_H

#include "cometd.h"

gboolean cometd_msg_is_successful(JsonNode* node);
gboolean cometd_msg_has_data(JsonNode* node);
const gchar* cometd_msg_get_channel(JsonNode* node);

#endif // COMETD_MSG_H
