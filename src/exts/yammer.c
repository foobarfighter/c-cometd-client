#include "cometd/exts/yammer.h"

static void yammer_outgoing(const cometd* h, JsonNode* msg);

// FIXME: extensions are kind of broken... they need to work more like loops
static char* g_token = NULL;

cometd_ext*
cometd_ext_yammer_new(const char* token)
{
  cometd_ext* yammer = cometd_ext_new();
  yammer->outgoing = yammer_outgoing;
  g_token = token;
  return yammer;
}

void
yammer_outgoing(const cometd* h, JsonNode* msg)
{
  char* channel = cometd_msg_channel(msg);
  if (strcmp(channel, COMETD_CHANNEL_META_HANDSHAKE) == 0)
  {
    JsonObject* obj = json_node_get_object(msg);
    JsonObject* ext = json_object_new();
    json_object_set_string_member(ext, "auth", "oauth");
    json_object_set_string_member(ext, "token", g_token);
    json_object_set_boolean_member(ext, "push_message_bodies", TRUE);
    json_object_set_boolean_member(ext, "ack", FALSE);

    json_object_set_object_member(obj, "ext", ext);
  }
}
