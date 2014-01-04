#ifndef COMETD_INBOX_H
#define COMETD_INBOX_H

#include "../cometd.h"

struct _cometd_inbox {
  cometd_loop* loop;
  GQueue* queue;
  GMutex* m;
  GCond*  c;
};

cometd_inbox* cometd_inbox_new(cometd_loop* loop);
void cometd_inbox_push(cometd_inbox* inbox, JsonNode* root);
void cometd_inbox_push_msg(cometd_inbox* inbox, JsonNode* root);
JsonNode* cometd_inbox_take(cometd_inbox* inbox);
void cometd_inbox_destroy(cometd_inbox* inbox);

#endif /* COMETD_INBOX_H */
