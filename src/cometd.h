#ifndef COMETD_H
#define COMETD_H

#include <glib.h>
#include <json-glib/json-glib.h>

// Version
#define COMETD_VERSION                "1.0"
#define COMETD_MIN_VERSION            "0.9"

// Defaults
#define DEFAULT_BACKOFF_INCREMENT     1000
#define DEFAULT_MAX_BACKOFF           1000
#define DEFAULT_MAX_NETWORK_DELAY     1000
#define DEFAULT_APPEND_MESSAGE_TYPE   1
#define DEFAULT_REQUEST_TIMEOUT       30000

// Connection state
#define COMETD_DISCONNECTED           0x00000000
#define COMETD_CONNECTED              0x00000001
#define COMETD_DISCONNECTING          0x00000010

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

// Errors
#define COMETD_SUCCESS          0
#define COMETD_ERROR_JSON       1
#define COMETD_ERROR_HANDSHAKE  2

// Other
#define COMETD_MAX_CLIENT_ID_LEN 128

// Forward declaration stuff
struct _cometd;
typedef struct _cometd cometd;

// Transport callback functions
typedef int       (*cometd_callback)(const cometd* h, JsonNode* message);
typedef JsonNode* (*cometd_recv_callback)(const cometd* h);

typedef struct {
  char*                name;
  cometd_callback      send;
  cometd_recv_callback recv;
} cometd_transport;

// connection configuration object
typedef struct {
  char*    url;
  int      backoff_increment;
  int      max_backoff;
  int      max_network_delay;
  int      request_timeout;
  int      append_message_type_to_url;
  GList*   transports; 
} cometd_config;

typedef struct {
  int                            state;
  long                           _msg_id_seed;
  cometd_transport*              transport;
  char client_id[COMETD_MAX_CLIENT_ID_LEN];
} cometd_conn;

typedef struct _cometd_error_st {
  int   code;
  char* message;
} cometd_error_st;

// cometd handle
struct _cometd {
  cometd_conn*     conn;
  cometd_config*   config;
  cometd_error_st* last_error;
};

typedef struct _cometd_subscription {
  long id;
} cometd_subscription;


// configuration and lifecycle
cometd*         cometd_new              (cometd_config* config);
void            cometd_default_config   (cometd_config* config);
void            cometd_destroy          (cometd* h);

// message creation / serialization
JsonNode* cometd_new_connect_message  (const cometd* h);
JsonNode* cometd_new_handshake_message(const cometd* h);

// bayeux protocol
int cometd_handshake    (const cometd* h, cometd_callback cb);
int cometd_connect      (const cometd* h, cometd_callback cb);

// transports
int               cometd_register_transport    (cometd_config* h, const cometd_transport* transport);
int               cometd_unregister_transport  (cometd_config* h, const char* name);
cometd_transport* cometd_find_transport        (const cometd_config* h, const char *name);
void              cometd_destroy_transport     (gpointer transport);

// events
cometd_subscription* cometd_add_listener(const cometd* h, const char * channel, cometd_callback cb);
//int                  cometd_remove_listener(const cometd* h, cometd_subscription* subscription);
//int                  cometd_fire_listeners(const cometd* h, const char* channel);

// processing
void cometd_process_payload  (const cometd* h, JsonNode* root);
void cometd_process_message  (const cometd* h, JsonObject* message);
void cometd_process_handshake(const cometd* h, JsonObject* message);

// other
int              cometd_error(const cometd* h, int code, char* message);
cometd_error_st* cometd_last_error(const cometd* h);

#endif /* COMETD_H */
