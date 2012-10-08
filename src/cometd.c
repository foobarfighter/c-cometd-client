#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include "cometd.h"

cometd_config *_cometd_config = NULL;

void
cometd_default_config(cometd_config* config){
  config->url = "";
  config->backoff_increment = DEFAULT_BACKOFF_INCREMENT;
  config->max_backoff       = DEFAULT_MAX_BACKOFF;
  config->max_network_delay = DEFAULT_MAX_NETWORK_DELAY;
  config->append_message_type_to_url = DEFAULT_APPEND_MESSAGE_TYPE;
}

cometd_config*
cometd_configure(cometd_config *config){
  if (config != NULL){
    _cometd_config = config;
  }

  return _cometd_config;
}

cometd*
cometd_new(){
  cometd* h = malloc(sizeof(cometd));

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_DISCONNECTED;

  h->conn = conn;
  return h;
}

int
cometd_init(const cometd* h){
  if (cometd_handshake(h, NULL))
    return 1;
    //return _error(h, "handshake failed: %s", _error_msg(h));

  if (cometd_connect(h, NULL))
    return 1;
    //return _error(h, "connect failed: %s", _error_msg(h));

  return 0;
}

int
cometd_handshake(const cometd* h, cometd_callback cb){
  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, _cometd_config->url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
  CURLcode res = curl_easy_perform(curl);


  int ret = 0;
  if (res != CURLE_OK){
    ret = 1;
    //_error(curl_easy_strerror(res));
  }
  printf("URL7: %s, %d\n", _cometd_config->url, ret);

  curl_easy_cleanup(curl);

  return ret;
}

int
cometd_connect(const cometd* h, cometd_callback cb){
  return 0;
}

//TODO: This should accept variable formatting args
//int
//_error(const cometd* h, const char* format, const char* value){
//  return 1;
//}
//
//const char* _error_msg(const cometd *h){
//  return "some error";
//}

//size_t
//cometd_recv(cometd_message_t* message);
//
//int
//cometd_subscribe(const char* channel, int(*handler)(cometd_message_t*));



void cometd_destroy(cometd* h){
  free(h->conn);
  free(h);
}
