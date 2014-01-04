#ifndef COMETD_TRANSPORT_LONG_POLLING_H
#define COMETD_TRANSPORT_LONG_POLLING_H

#include "../cometd.h"

#define COMETD_TRANSPORT_LONG_POLLING_NAME "long-polling"

JsonNode* cometd_transport_long_polling_send(const cometd* h, JsonNode* node);
JsonNode* cometd_transport_long_polling_recv(const cometd* h);

static const cometd_transport COMETD_TRANSPORT_LONG_POLLING = {
  COMETD_TRANSPORT_LONG_POLLING_NAME,
  &cometd_transport_long_polling_send,
  &cometd_transport_long_polling_recv
};

#endif  //COMETD_TRANSPORT_LONG_POLLING_H
