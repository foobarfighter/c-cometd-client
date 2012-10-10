#ifndef COMETD_H
#define COMETD_H

#include "../deps/libev-4.11/ev.h"
#include "json.h"

// Version
#define COMETD_VERSION                "1.0"
#define COMETD_MIN_VERSION            "0.9"

// Defaults
#define DEFAULT_BACKOFF_INCREMENT     1000
#define DEFAULT_MAX_BACKOFF           1000
#define DEFAULT_MAX_NETWORK_DELAY     1000
#define DEFAULT_APPEND_MESSAGE_TYPE   1

// Connection state
#define COMETD_DISCONNECTED           0x00000000
#define COMETD_CONNECTED              0x00000001

// Message fields
#define COMETD_MSG_ID_FIELD           "id"
#define COMETD_MSG_CHANNEL_FIELD      "channel"
#define COMETD_MSG_VERSION_FIELD      "version"
#define COMETD_MSG_MIN_VERSION_FIELD  "minimumVersion"
#define COMETD_MSG_ADVICE_FIELD       "advice"

// Channels
#define COMETD_CHANNEL_META_HANDSHAKE   "/meta/handshake"
#define COMETD_CHANNEL_META_CONNECT     "/meta/connect"
#define COMETD_CHANNEL_META_SUBSCRIBE   "/meta/subscribe"
#define COMETD_CHANNEL_META_UNSUBSCRIBE "/meta/unsubscribe"
#define COMETD_CHANNEL_META_DISCONNECT  "/meta/disconnect"

typedef int (*cometd_callback)(JsonNode* message);

typedef struct {
  char*           name;
  cometd_callback send;
  cometd_callback recv;
} cometd_transport;

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
  int  state;
  long _msg_id_seed;
} cometd_conn;

// cometd handle
typedef struct {
  cometd_conn* conn;
  cometd_config* config;
} cometd;

//typedef struct {
//  long   id;
//  char*  version;
//  char*  minimum_version;
//  char*  channel;
//  char** supported_connection_types;
//}

#define JSON_GET_DOUBLE(node)  (node->number_);

// configuration and lifecycle
cometd*         cometd_new              (cometd_config* config);
void            cometd_default_config   (cometd_config* config);
void            cometd_destroy          (cometd* h);

// bayeux protocol
int cometd_handshake (const cometd* h, cometd_callback cb);
int cometd_connect   (const cometd* h, cometd_callback cb);

// transports
int cometd_register_transport(const cometd_config* h, const cometd_transport* transport);
int cometd_unregister_transport(const cometd_config* h, const char* name);

// message creation / serialization
int cometd_create_handshake_req(const cometd* h, JsonNode* message);

#endif /* COMETD_H */
