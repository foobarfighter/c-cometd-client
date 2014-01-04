#include "cometd/ext.h"
#include <stdlib.h>

static void cometd_impl_ext_fire(GList* exts,
                                 cometd* h,
                                 JsonNode* n,
                                 gboolean incoming);

cometd_ext*
cometd_ext_new(void)
{
  return cometd_ext_malloc();
}

cometd_ext*
cometd_ext_malloc(void)
{
  cometd_ext* ext = malloc(sizeof(cometd_ext));
  ext->incoming = NULL;
  ext->outgoing = NULL;
  return ext;
}

void
cometd_ext_destroy(cometd_ext* ext)
{
  free(ext);
}

void
cometd_ext_fire_incoming(GList* exts, cometd* h, JsonNode* n)
{
  cometd_impl_ext_fire(exts, h, n, TRUE);
}

void
cometd_ext_fire_outgoing(GList* exts, cometd* h, JsonNode* n)
{
  cometd_impl_ext_fire(exts, h, n, FALSE);
}

void
cometd_ext_add(GList** exts, cometd_ext* ext)
{
  *exts = g_list_append(*exts, ext);
}

void
cometd_ext_remove(GList** exts, cometd_ext* ext)
{
  *exts = g_list_remove(*exts, ext);
  cometd_ext_destroy(ext);
}

void
cometd_impl_ext_fire(GList* exts, cometd* h, JsonNode* n, gboolean incoming)
{
  GList* iext;
  for (iext = exts; iext; iext = g_list_next(iext))
  {
    cometd_ext* ext = iext->data;

    if (incoming) {
      if (ext->incoming)
        ext->incoming(h, n);
    } else {
      if (ext->outgoing)
        ext->outgoing(h, n);
    }

  }
}
