#include "transport_long_polling.h"
#include "http.h"

int
cometd_transport_long_polling_send(cometd* h, JsonNode* node)
{
  gsize len = 0;

  JsonGenerator* gen = json_generator_new();
  json_generator_set_root(gen, node);
  gchar* data = json_generator_to_data(gen, &len);

  printf("\n\n\nSENDING THE SHITS\n\n");
  const char* raw_response = http_json_post(h->config->url, data);

  printf("\n\n\nSENT THE SHITS\n\n");
  return 0;
}

int
cometd_transport_long_polling_recv(cometd* h, JsonNode* node){
  return 0;
}
