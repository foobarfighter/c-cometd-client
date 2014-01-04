#include "cometd/channel.h"

gboolean
cometd_channel_is_wildcard(const char* channel)
{
  g_assert(channel != NULL);
  return *(channel + strnlen(channel, COMETD_MAX_CHANNEL_LEN) - 1) == '*';
}

void
cometd_channel_matches_free(GList* matches)
{
  g_list_free_full(matches, g_free);
}

GList*
cometd_channel_matches(const char* channel)
{
  g_return_val_if_fail(channel != NULL, NULL);

  GList* channels  = NULL;
  char** parts     = g_strsplit(channel, "/", 0);
  guint  parts_len = g_strv_length(parts);

  if (parts_len > COMETD_MAX_CHANNEL_PARTS) 
    goto free_parts;

  // 254+2 because tmp needs space for (wildcard part + NULL)
  char* tmp[COMETD_MAX_CHANNEL_PARTS+2] = { NULL };

  guint i;
  for (i = 0; i < parts_len; i++)
  {
    tmp[i]   = parts[i];

    // add double wildcard
    if (i < parts_len - 1)
    {
      tmp[i+1] = "**";
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    // add base channel
    } else {
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    }
    
    // add single wildcard
    if (i == parts_len - 2)
    {
      tmp[i+1] = "*";
      channels = g_list_prepend(channels, g_strjoinv("/", tmp));
    }
  }

free_parts:
  g_strfreev(parts);
  return channels;
}
