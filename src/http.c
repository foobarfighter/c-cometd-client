#include "http.h"
#include <stdlib.h>
#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    //exit(EXIT_FAILURE);
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
http_json_post(const char *url, const char* data){
  struct curl_slist *header_chunk = NULL;

  header_chunk = curl_slist_append(header_chunk, "Content-Type: application/json");

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

  struct MemoryStruct body_chunk;
  body_chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  body_chunk.size   = 0;          /* no data at this point */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body_chunk);

  CURLcode res = curl_easy_perform(curl);

  char* ret = (res == CURLE_OK) ? body_chunk.memory : NULL;

  curl_easy_cleanup(curl);

  return ret;
}
