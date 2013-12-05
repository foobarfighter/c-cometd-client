#ifndef COMETD_H
#define COMETD_H

#include <glib.h>
#include <json-glib/json-glib.h>
#include "cometd_msg.h"
#include "cometd_json.h"

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
#define COMETD_UNINITIALIZED          0x00000000
#define COMETD_DISCONNECTED           0x00000001
#define COMETD_HANDSHAKE_SUCCESS      0x00000002
#define COMETD_CONNECTED              0x00000004
#define COMETD_DISCONNECTING          0x00000008

// Message fields
#define COMETD_MSG_ID_FIELD           "id"
#define COMETD_MSG_DATA_FIELD         "data"
#define COMETD_MSG_CLIENT_ID_FIELD    "clientId"
#define COMETD_MSG_CHANNEL_FIELD      "channel"
#define COMETD_MSG_VERSION_FIELD      "version"
#define COMETD_MSG_MIN_VERSION_FIELD  "minimumVersion"
#define COMETD_MSG_ADVICE_FIELD       "advice"
#define COMETD_MSG_SUBSCRIPTION_FIELD "subscription"
#define COMETD_MSG_SUCCESSFUL_FIELD   "successful"

// Channels
#define COMETD_CHANNEL_META_HANDSHAKE   "/meta/handshake"
#define COMETD_CHANNEL_META_CONNECT     "/meta/connect"
#define COMETD_CHANNEL_META_SUBSCRIBE   "/meta/subscribe"
#define COMETD_CHANNEL_META_UNSUBSCRIBE "/meta/unsubscribe"
#define COMETD_CHANNEL_META_DISCONNECT  "/meta/disconnect"

// Configuration options
typedef enum {
  COMETDOPT_URL = 0,
  COMETDOPT_REQUEST_TIMEOUT,
  COMETDOPT_INIT_LOOPFUNC
} cometd_opt;

// Errors
#define COMETD_SUCCESS            0
#define ECOMETD_JSON_SERIALIZE    1
#define ECOMETD_JSON_DESERIALIZE  2
#define ECOMETD_HANDSHAKE         3
#define ECOMETD_INIT_LOOP         4
#define ECOMETD_UNKNOWN           5

// Other
#define COMETD_MAX_CLIENT_ID_LEN 128
#define COMETD_MAX_CHANNEL_LEN   512

// Forward declaration stuff
struct _cometd;
typedef struct _cometd cometd;

// Transport callback functions
typedef int       (*cometd_callback)(const cometd* h, JsonNode* message);
typedef JsonNode* (*cometd_send_callback)(const cometd* h, JsonNode* message);
typedef JsonNode* (*cometd_recv_callback)(const cometd* h);
typedef int       (*cometd_init_loopfunc)(const cometd* h);

typedef struct {
  char*                name;
  cometd_send_callback send;
  cometd_recv_callback recv;
} cometd_transport;

// connection configuration object
typedef struct {
  char*                url;
  long                 backoff_increment;
  long                 max_backoff;
  long                 max_network_delay;
  long                 request_timeout;
  int                  append_message_type_to_url;
  cometd_init_loopfunc init_loop_func;
  GList*      transports; 
} cometd_config;

typedef struct {
  long               state;
  long               _msg_id_seed;
  cometd_transport*  transport;
  GQueue*            inbox;
  GCond*             inbox_cond;
  GMutex*            inbox_mutex;
  GThread*           inbox_thread;
  GHashTable*        subscriptions;
  
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
  char channel[COMETD_MAX_CHANNEL_LEN];
  cometd_callback callback;
} cometd_subscription;


// configuration and lifecycle
cometd* cometd_new       (void);
void    cometd_destroy   (cometd* h);
/*
  This redefinition technique is borrowed from curl_easy_setopt
  to fake the compiler into enforcing the three parameter constraint
  while allowing the implementation to accept a va_list.
*/
#define cometd_configure(h, opt, value) cometd_configure(h, opt, value)

// message creation / serialization
JsonNode* cometd_new_connect_message  (const cometd* h);
JsonNode* cometd_new_handshake_message(const cometd* h);
JsonNode* cometd_new_subscribe_message(const cometd* h, const char* c);
JsonNode* cometd_new_unsubscribe_message(const cometd* h, const char* c);
JsonNode* cometd_new_publish_message(const cometd* h,
                                     const char* c,
                                     JsonNode* data);

// protocol
int         cometd_handshake    (const cometd* h, cometd_callback cb);
int         cometd_connect      (const cometd* h);
JsonNode*   cometd_recv         (const cometd* h);

int cometd_publish(const cometd* h,
                   const char* channel,
                   JsonNode* message);

cometd_subscription* cometd_subscribe(const cometd* h,
                                      char* channel,
                                      cometd_callback handler);
int cometd_unsubscribe(const cometd* h, cometd_subscription* s);

// transports
cometd_transport* cometd_current_transport(const cometd* h);

int cometd_register_transport(cometd_config* h,
                              const cometd_transport* transport);

int cometd_unregister_transport(cometd_config* h, const char* name);

cometd_transport* cometd_find_transport(const cometd_config* h,
                                        const char *name);

void cometd_destroy_transport(gpointer transport);

// events
cometd_subscription* cometd_add_listener(const cometd* h,
                                         char * channel,
                                         cometd_callback cb);

int cometd_listener_count(const cometd*, char* channel);

int cometd_remove_listener(const cometd* h,
                           cometd_subscription* subscription);

int cometd_fire_listeners(const cometd* h,
                          const char* channel,
                          JsonNode* message);

// processing
void cometd_process_payload  (const cometd* h, JsonNode* root);
void cometd_process_message(JsonArray *array, guint idx, JsonNode* node, gpointer data);
void cometd_process_handshake(const cometd* h, JsonNode* root);

// other
int               cometd_error(const cometd* h, int code, char* message);
gboolean          cometd_is_meta_channel(const char* channel);
cometd_error_st*  cometd_last_error(const cometd* h);
long              cometd_conn_status(const cometd* h);
void              cometd_conn_set_status(const cometd* h, long status);
long              cometd_conn_is_status(const cometd* h, long status);
void              cometd_conn_clear_status(const cometd* h);
char*             cometd_conn_client_id(const cometd* h);
void              cometd_conn_set_client_id(const cometd* h, const char *id);
void              cometd_conn_set_transport(const cometd* h, cometd_transport* t);
int               cometd_init_loop(const cometd* h);
void              cometd_listen(const cometd* h);

#endif /* COMETD_H */

