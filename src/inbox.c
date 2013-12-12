#include <stdlib.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include "cometd.h"

cometd_inbox*
cometd_inbox_new(cometd_loop* loop)
{
  cometd_inbox* inbox = malloc(sizeof(cometd_inbox));
  
  inbox->loop = loop;
  inbox->queue = g_queue_new();
  inbox->c = malloc(sizeof(GCond));
  inbox->m = malloc(sizeof(GMutex));
  g_cond_init(inbox->c);
  g_mutex_init(inbox->m);

  return inbox;
}

void
cometd_inbox_push(cometd_inbox* inbox, JsonNode* root)
{
  JsonArray* arr = json_node_get_array(root);
  GList* msgs = json_array_get_elements(arr);

  GList* msg;
  for (msg = msgs; msg; msg = g_list_next(msg))
    cometd_inbox_push_msg(inbox, msg->data);
}

void
cometd_inbox_push_msg(cometd_inbox* inbox, JsonNode* node)
{
  g_mutex_lock(inbox->m);

  JsonNode* save = json_node_copy(node);
  JsonObject* msg = json_node_get_object(save);

  const gchar* channel = json_object_get_string_member(msg,
                                    COMETD_MSG_CHANNEL_FIELD);

  g_queue_push_tail(inbox->queue, save);

  g_cond_signal(inbox->c);
  g_mutex_unlock(inbox->m);
}

JsonNode*
cometd_inbox_take(cometd_inbox* inbox)
{
  JsonNode* node = NULL;

  GTimeVal wait_timeout;
  GTimeVal now;

  GQueue* q = inbox->queue;
  GCond* c = inbox->c;
  GMutex* m = inbox->m;

  g_mutex_lock(m);

  g_get_current_time(&now);
  g_time_val_add(&now, 100000);  // 100ms

  while (g_queue_is_empty(q))
      if (!g_cond_timed_wait(c, m, &now))
        break;

  if (!g_queue_is_empty(q))
    node = (JsonNode*) g_queue_pop_head(q);

  g_mutex_unlock(m);

  return node;
}

void
cometd_inbox_destroy(cometd_inbox* inbox)
{
  g_mutex_lock(inbox->m);
  g_queue_free_full(inbox->queue, (GDestroyNotify) json_node_free);
  g_mutex_unlock(inbox->m);

  inbox->loop = NULL;
  g_cond_clear(inbox->c);
  free(inbox->c);
  g_mutex_clear(inbox->m);
  free(inbox->m);
  free(inbox);
}
