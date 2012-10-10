#include <stdlib.h>
#include <stddef.h>
//#include <pthread.h>
#include <curl/curl.h>
#include "cometd.h"
#include "json.h"

void
cometd_default_config(cometd_config* config){
  config->url = "";
  config->backoff_increment = DEFAULT_BACKOFF_INCREMENT;
  config->max_backoff       = DEFAULT_MAX_BACKOFF;
  config->max_network_delay = DEFAULT_MAX_NETWORK_DELAY;
  config->append_message_type_to_url = DEFAULT_APPEND_MESSAGE_TYPE;
}

cometd*
cometd_new(cometd_config* config){
  cometd* h = malloc(sizeof(cometd));

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_DISCONNECTED;
  conn->_msg_id_seed = 0;

  h->conn         = conn;
  h->config       = config;

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
  JsonNode* message = json_mkobject();
  cometd_create_handshake_req(h, message);

  const char* data = json_stringify(message, NULL);

  struct curl_slist *chunk = NULL;
  chunk = curl_slist_append(chunk, "Content-Type: application/json");

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, h->config->url);
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
  json_delete(message);

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

int cometd_create_handshake_req(const cometd* h, JsonNode* root){
  long seed = ++(h->conn->_msg_id_seed);

  json_append_member(root, COMETD_MSG_ID_FIELD,          json_mknumber(seed));
  json_append_member(root, COMETD_MSG_CHANNEL_FIELD,     json_mkstring(COMETD_CHANNEL_META_HANDSHAKE));
  json_append_member(root, COMETD_MSG_VERSION_FIELD,     json_mkstring(COMETD_VERSION));
  json_append_member(root, COMETD_MSG_MIN_VERSION_FIELD, json_mkstring(COMETD_MIN_VERSION));

  // construct advice - TODO: these values should not be hardcoded
  JsonNode* advice = json_mkobject();
  json_append_member(advice, "timeout",  json_mknumber(60000));
  json_append_member(advice, "interval", json_mknumber(0));
  json_append_member(root, COMETD_MSG_ADVICE_FIELD, advice);

  // construct transports - TODO: implement this more fully when the concept of a transport is introduced
  JsonNode* transports = json_mkarray();
  json_append_element(transports, json_mkstring("long-polling"));
  json_append_member(root, "supportedConnectionTypes", transports);

  // call extensions with message - TODO: implement extensions first

  return 0;
}

int
cometd_msg_attr_set(cometd_message_t* message, int property, ...){
  switch (property){
  }
}

void*
cometd_msg_attr_get(cometd_message_t* message, int property){
  int hi;
  return &hi;
}
