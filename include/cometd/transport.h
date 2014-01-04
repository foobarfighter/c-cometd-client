#ifndef COMETD_TRANSPORT_H
#define COMETD_TRANSPORT_H

#include "../cometd.h"

struct _cometd_transport {
  char*                name;
  cometd_send_callback send;
  cometd_recv_callback recv;
};

cometd_transport* cometd_transport_negotiate(GList* registry, JsonNode* n);

#endif /* COMETD_TRANSPORT_H */
