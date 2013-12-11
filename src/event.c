#include "event.h"
#include <stdlib.h>
#include <string.h>

static void cometd_impl_destroy_slist(gpointer key, gpointer value, gpointer userdata);

GHashTable*
cometd_listener_new()
{
  return g_hash_table_new(g_str_hash, g_str_equal);
}

void
cometd_listener_destroy(GHashTable* subscriptions)
{
  g_hash_table_foreach(subscriptions,
                       cometd_impl_destroy_slist,
                       NULL);

  g_hash_table_destroy(subscriptions);
}

static void
cometd_impl_destroy_slist(gpointer key,
                          gpointer value,
                          gpointer userdata)
{
  g_list_free_full((GList*)value, free);
}

cometd_subscription*
cometd_add_listener(const cometd* h,
                    char * channel,
                    cometd_callback cb)
{
  return cometd_listener_add(h->subscriptions, channel, cb);
}

cometd_subscription*
cometd_listener_add(GHashTable* listeners,
                    char* channel,
                    cometd_callback cb)
{
  g_return_val_if_fail(listeners != NULL, NULL);
  int len                   = strlen(channel);

  g_return_val_if_fail(len > 0 && len < COMETD_MAX_CHANNEL_LEN - 1, NULL);

  cometd_subscription* s = malloc(sizeof(cometd_subscription));
  if (s == NULL)
    goto error;

  strncpy(s->channel, channel, len + 1);
  s->callback = cb;

  /*
    If the list isn't found then lookup will be NULL and prepend
    will create a brand new list for us.
  */
  GList* list = (GList*) g_hash_table_lookup(listeners, channel);

  /*
    Prepend to existing list or create brand new list.
  */
  list = g_list_append(list, s);

  /*
    We always need to update the value because the pointer
    to the list changes every time we prepend an element.
  */
  g_hash_table_insert(listeners, channel, list);

error:
  return s;
}

int
cometd_remove_listener(const cometd* h, cometd_subscription* s)
{
  g_return_val_if_fail(h->subscriptions != NULL, ECOMETD_UNKNOWN);

  return cometd_listener_remove(h->subscriptions, s);
}

int
cometd_listener_remove(GHashTable* listeners,
                       cometd_subscription* subscription)
{
  g_return_val_if_fail(subscription != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(subscription->channel != NULL, ECOMETD_UNKNOWN);

  char* channel = subscription->channel;

  GList* list = (GList*) g_hash_table_lookup(listeners, channel);

  list = g_list_remove(list, subscription);

  // We need to reset the pointer because it may have changed.
  g_hash_table_insert(listeners, channel, list);

  free(subscription);

  return COMETD_SUCCESS;
}

int
cometd_listener_count(const cometd* h, const char* channel)
{
  g_assert(h != NULL);
  g_assert(h->subscriptions != NULL);

  g_return_val_if_fail(channel != NULL, 0);
  
  GList* list;
  
  list = (GList*) g_hash_table_lookup(h->subscriptions, channel);
  
  return g_list_length(list);
}

int
cometd_fire_listeners(const cometd* h,
                      const char* channel,
                      JsonNode* message)
{
  return cometd_listener_fire(h->subscriptions, channel, h, message);
}

int
cometd_listener_fire(GHashTable* listeners,
                     const char* channel,
                     const cometd* h,
                     JsonNode* message)

{
  g_return_val_if_fail(listeners != NULL, ECOMETD_UNKNOWN);
  g_return_val_if_fail(channel != NULL, ECOMETD_UNKNOWN);

  GList* list = cometd_listener_get(listeners, channel);
  
  // If the list is NULL then, then there are no subscriptions.
  if (list == NULL) { return COMETD_SUCCESS; }

  GList* item;
  for (item = list; item; item = g_list_next(item))
  {
    cometd_subscription* s = (cometd_subscription*) item->data;
    printf("firing listener for channel: %s\n", s->channel);
    if (s->callback(h, message) != COMETD_SUCCESS) {
      goto error;
    }
  }

  g_list_free(list);

  return COMETD_SUCCESS;
error:
  return ECOMETD_UNKNOWN;
}

GList*
cometd_listener_get(GHashTable* map, const char* channel)
{
  g_assert(!cometd_channel_is_wildcard(channel));

  GList* subscriptions = NULL;
  GList* channels = cometd_channel_matches(channel);

  GList* c;
  for (c = channels; c; c = g_list_next(c))
  {
    GList* list = (GList*) g_hash_table_lookup(map, c->data);

    printf("channel match: %s\n", c->data);

    GList* s = NULL;
    for (s = list; s; s = g_list_next(s))
      subscriptions = g_list_prepend(subscriptions, s->data);
  }

  cometd_channel_matches_free(channels);

  return g_list_reverse(subscriptions);
}
