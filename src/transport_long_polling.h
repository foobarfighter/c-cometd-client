#ifndef COMETD_TRANSPORT_LONG_POLLING_H
#define COMETD_TRANSPORT_LONG_POLLING_H

#include "cometd.h"

#define COMETD_TRANSPORT_LONG_POLLING_NAME "long-polling"

int cometd_transport_long_polling_send(cometd* h, JsonNode* node);
int cometd_transport_long_polling_recv(cometd* h, JsonNode* node);

static const cometd_transport COMETD_TRANSPORT_LONG_POLLING = {
  COMETD_TRANSPORT_LONG_POLLING_NAME,
  cometd_transport_long_polling_send,
  cometd_transport_long_polling_recv
};

#endif  //COMETD_TRANSPORT_LONG_POLLING_H
