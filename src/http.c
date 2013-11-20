#include "http.h"
#include "http_parser.h"
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct _MemoryStruct {
  char *memory;
  size_t size;
} MemoryStruct;

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

static http_parser_settings settings_null =
{
  .on_message_begin = 0,
  .on_header_field = 0,
  .on_header_value = 0,
  .on_url = 0,
  .on_body = 0,
  .on_headers_complete = 0,
  .on_message_complete = 0
};

int
http_valid_response(const char* body, size_t size)
{
  http_parser parser;

  http_parser_init(&parser, HTTP_RESPONSE);
  http_parser_execute(&parser,
                      &settings_null,
                      body,
                      size);

  return parser.status_code >= 200 && parser.status_code <= 299;
}

/*
 * Returns a string of the http body, the buffer should be free'd by
 * the calling function.
 */
char*
http_json_post(const char *url, const char* data, int timeout){
  char* ret = NULL;

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

  if (body.memory == NULL)
    goto cleanup_curl;

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_HEADER, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &body);

  CURLcode res = curl_easy_perform(curl);

  if (res == CURLE_OK && http_valid_response(body.memory, body.size)){
    ret = body.memory;
  } else {
    free(body.memory);
  }

cleanup_curl:
  curl_easy_cleanup(curl);
curl_init_error:
  return ret;
}
