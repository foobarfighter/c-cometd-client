#ifndef COMETD_EVENT_H
#define COMETD_EVENT_H

#include "../cometd.h"

struct _cometd_subscription {
  char channel[COMETD_MAX_CHANNEL_LEN];
  cometd_callback callback;
};

// events
GHashTable* cometd_listener_new(void);
void cometd_listener_destroy(GHashTable* subscriptions);

cometd_subscription* cometd_listener_add(GHashTable* listeners,
                                         char * channel,
                                         cometd_callback cb);

int cometd_listener_fire(GHashTable* listeners,
                         const char* channel,
                         const cometd* h,
                         JsonNode* n);

int cometd_listener_remove(GHashTable* listeners, cometd_subscription* s);

GList* cometd_listener_get(GHashTable* listeners, const char* channel);

cometd_subscription* cometd_add_listener
    (const cometd* h, char * channel, cometd_callback cb);

int cometd_listener_count
    (const cometd* h, const char* channel);

int cometd_remove_listener
    (const cometd* h, cometd_subscription* subscription);

int cometd_fire_listeners
    (const cometd* h, const char* channel, JsonNode* message);

#endif  /* COMETD_EVENT_H */
