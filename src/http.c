#include "http.h"
#include <stdlib.h>
#include <stdio.h>
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
    printf("raw response:\n=========\n%s\n\n", ret);
  }

cleanup_curl:
  curl_easy_cleanup(curl);
cleanup_header_chunk:
  curl_slist_free_all(header_chunk);
curl_init_error:
  return ret;
}
