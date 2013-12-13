#ifndef COMETD_EXT_H
#define COMETD_EXT_H

#include "cometd.h"

typedef void (*CometdExtFunc)(const cometd* h, JsonNode* node);

struct _cometd_ext
{
  CometdExtFunc incoming;
  CometdExtFunc outgoing;
  void* internal;
};

cometd_ext* cometd_ext_new(void);
cometd_ext* cometd_ext_malloc(void);
void cometd_ext_destroy(cometd_ext* ext);
void cometd_ext_fire_incoming(GList* exts, cometd* h, JsonNode* n);
void cometd_ext_fire_outgoing(GList* exts, cometd* h, JsonNode* n);
void cometd_ext_add(GList** exts, cometd_ext* ext);
void cometd_ext_remove(GList** exts, cometd_ext* ext);


#endif  /* COMETD_EXT_H */
