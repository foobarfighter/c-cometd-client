#include <stdlib.h>
#include <pthread.h>
#include "cometd.h"
#include "../deps/libev-4.11/ev.h"

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
cometd_new(struct ev_loop *loop){
  cometd* h = malloc(sizeof(cometd));
  h->loop = loop;

  cometd_conn* conn = malloc(sizeof(cometd_conn));
  conn->state = COMETD_DISCONNECTED;

  h->conn = conn;
  return h;
}

void*
_cometd_init(void* handle){
  cometd* h = (cometd* h) handle;
  sleep(1);
  printf("=================================== in cometd_init %s\n", "hi");
  //cometd* h = (cometd*) handle;
  //sleep(1);
  //h->conn->state = COMETD_CONNECTED;
  //pthread_exit(NULL);
}

int
cometd_init(const cometd* h){
  pthread_t thread;

  printf("creating thread\n");
  if (pthread_create(&thread, NULL, _cometd_init, (void*) h)){
    return 1;
  } else {
    printf("created a thread\n");
  }

  return 0;
}


void cometd_destroy(cometd* h){
  free(h->conn);
  free(h);
}
