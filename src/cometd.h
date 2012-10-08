#ifndef COMETD_H
#define COMETD_H

#include "../deps/libev-4.11/ev.h"

#define NULL                          0

// Defaults
#define DEFAULT_BACKOFF_INCREMENT     1000
#define DEFAULT_MAX_BACKOFF           1000
#define DEFAULT_MAX_NETWORK_DELAY     1000
#define DEFAULT_APPEND_MESSAGE_TYPE   1

// Connection state
#define COMETD_DISCONNECTED           0x00000000
#define COMETD_CONNECTED              0x00000001

// connection configuration object
typedef struct {
  char* url;
  int   backoff_increment;
  int   max_backoff;
  int   max_network_delay;
  int   append_message_type_to_url;
} cometd_config;

// connection state object
typedef struct {
  int state;
} cometd_conn;

// cometd handle
typedef struct {
  cometd_conn* conn;
  cometd_config* config;
  //struct ev_loop* loop;
} cometd;

typedef struct {
  int successful;
} cometd_message_t;

typedef int (*cometd_callback)(cometd_message_t* msg);

// configuration and lifecycel
void            cometd_default_config   (cometd_config* config);
cometd_config*  cometd_configure        (cometd_config *config);
cometd*         cometd_new              ();
void            cometd_destroy          (cometd* h);

// bayeux protocol
int cometd_handshake (const cometd* h, cometd_callback cb);
int cometd_connect   (const cometd* h, cometd_callback cb);


#endif /* COMETD_H */
