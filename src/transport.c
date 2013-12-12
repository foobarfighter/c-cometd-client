#include <string.h>
#include "transport.h"

cometd_transport*
cometd_transport_negotiate(GList* registry, JsonNode* n)
{
  cometd_transport* transport = NULL;
  GList *iitem, *itran;

  GList* types = cometd_msg_supported_connection_types(n);

  for (iitem = types; iitem; iitem = g_list_next(iitem))
  {
    for (itran = registry; itran; itran = g_list_next(itran))
    {
      cometd_transport* t = itran->data;
      if (strcmp(t->name, iitem->data) == 0)
      {
        transport = t;
        break;
      }
    }
  }

  g_list_free(types);

  return transport;
}

