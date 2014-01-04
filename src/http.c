#include "cometd/http.h"
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct _MemoryStruct {
  char *memory;
  size_t size;
} MemoryStruct;

static size_t
WriteMemoryCallback(void *contents,
                    size_t size,
                    size_t nmemb,
                    void *userp)
{
  size_t realsize = size * nmemb;
  MemoryStruct *mem = (MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    // printf("not enough memory (realloc returned NULL)\n");
    // TODO: how should I handle this?
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

/*
 * Returns a string of the http body, the buffer should be free'd by
 * the calling function.
 */
char*
http_json_post(const char *url, const char* data, int timeout){
  char* ret = NULL;
  unsigned int http_error = 1;

  struct curl_slist *header_chunk = NULL;

  header_chunk = curl_slist_append(header_chunk,
                                   "Content-Type: application/json");

  CURL* curl = curl_easy_init();

  if (curl == NULL)
    goto curl_init_error;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);

  MemoryStruct body;
  body.memory = malloc(1);
  body.size   = 0;

  // couldn't malloc
  if (body.memory == NULL)
    goto cleanup_curl;

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &body);

  CURLcode res = curl_easy_perform(curl);

  // make sure that the request/response went ok
  if (res == CURLE_OK){
    long http_code;

    // validate the http response
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 200 && http_code <= 299){
      http_error = 0;
    }
  }

  if (http_error){
    free(body.memory);
  } else {
    ret = body.memory;
  }

cleanup_curl:
  curl_easy_cleanup(curl);
cleanup_header_chunk:
  curl_slist_free_all(header_chunk);
curl_init_error:
  return ret;
}

JsonNode*
http_post_msg(const cometd* h, JsonNode* msg)
{
  JsonNode* payload = NULL;
  gchar*    data = cometd_json_node2str(msg);
  int       code = COMETD_SUCCESS;

  if (data == NULL){
    code = cometd_error(h, ECOMETD_JSON_SERIALIZE, "could not serialize json");
    goto failed_serialization;
  }
  
  char* resp = http_json_post(h->config->url, data, h->config->request_timeout);

  if (resp == NULL){
    code = cometd_error(h, ECOMETD_SEND, "could not post");
    goto failed_post;
  }

  payload = cometd_json_str2node(resp);

  if (payload == NULL){
    code = cometd_error(h, ECOMETD_JSON_DESERIALIZE, "could not de-serialize json");
  }

  free(resp);

failed_post:
  g_free(data);

failed_serialization:
  return payload;
}

