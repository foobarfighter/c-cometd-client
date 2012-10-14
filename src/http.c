#include "http.h"
#include <curl/curl.h>

char*
http_json_post(const char *url, const char* data){
  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Content-Type: application/json");

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
  CURLcode res = curl_easy_perform(curl);

  int ret = 0;
  if (res != CURLE_OK){
    ret = 1;
    //_error(curl_easy_strerror(res));
  }

  curl_easy_cleanup(curl);

  return NULL;
}
