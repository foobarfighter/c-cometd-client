#ifndef COMETD_H
#define COMETD_H

// Version
#define COMETD_VERSION                "1.0"
#define COMETD_MIN_VERSION            "0.9"

// Defaults
#define DEFAULT_BACKOFF_INCREMENT     1000
#define DEFAULT_MAX_BACKOFF           30000
#define DEFAULT_MAX_NETWORK_DELAY     1000
#define DEFAULT_APPEND_MESSAGE_TYPE   1
#define DEFAULT_REQUEST_TIMEOUT       30000

// Connection state
#define COMETD_UNINITIALIZED          0x00000000
#define COMETD_DISCONNECTED           0x00000001
#define COMETD_HANDSHAKE_SUCCESS      0x00000002
#define COMETD_CONNECTED              0x00000004
#define COMETD_DISCONNECTING          0x00000008

// Channels
#define COMETD_CHANNEL_META_HANDSHAKE   "/meta/handshake"
#define COMETD_CHANNEL_META_CONNECT     "/meta/connect"
#define COMETD_CHANNEL_META_SUBSCRIBE   "/meta/subscribe"
#define COMETD_CHANNEL_META_UNSUBSCRIBE "/meta/unsubscribe"
#define COMETD_CHANNEL_META_DISCONNECT  "/meta/disconnect"

// Other
#define COMETD_MAX_CHANNEL_LEN   512
#define COMETD_MAX_CHANNEL_PARTS 254

#include <glib.h>
#include <json-glib/json-glib.h>

// Forward declaration stuff
struct _cometd;
struct _cometd_error_st;
struct _cometd_conn;
struct _cometd_advice;
struct _cometd_subscription;
struct _cometd_transport;
struct _cometd_ext;
typedef struct _cometd cometd;
typedef struct _cometd_error_st cometd_error_st;
typedef struct _cometd_conn cometd_conn;
typedef struct _cometd_advice cometd_advice;
typedef struct _cometd_subscription cometd_subscription;
typedef struct _cometd_transport cometd_transport;
typedef struct _cometd_ext cometd_ext;

// Transport callback functions
typedef int       (*cometd_callback)(const cometd* h, JsonNode* message);
typedef JsonNode* (*cometd_send_callback)(const cometd* h, JsonNode* message);
typedef JsonNode* (*cometd_recv_callback)(const cometd* h);

#include "error.h"
#include "conn.h"
#include "transport.h"
#include "channel.h"
#include "event.h"
#include "msg.h"
#include "json.h"
#include "loop.h"
#include "inbox.h"
#include "http.h"
#include "ext.h"

// Configuration options
typedef enum {
  COMETDOPT_URL = 0,
  COMETDOPT_REQUEST_TIMEOUT,
  COMETDOPT_LOOP,
  COMETDOPT_BACKOFF_INCREMENT,
  COMETDOPT_MAX_BACKOFF
} cometd_opt;

// connection configuration object
typedef struct {
  char*  url;
  long   backoff_increment;
  long   max_backoff;
  long   max_network_delay;
  long   request_timeout;
  int    append_message_type_to_url;
  GList* transports; 
} cometd_config;

// system handlers
typedef struct {
  cometd_subscription* handshake;
  cometd_subscription* connect;
  cometd_subscription* subscribe;
  cometd_subscription* unsubscribe;
  cometd_subscription* disconnect;
} cometd_sys_s;

// cometd handle
struct _cometd {
  cometd_conn*     conn;
  cometd_config*   config;
  cometd_error_st* last_error;
  cometd_loop*     loop;
  cometd_inbox*    inbox;
  GHashTable*      subscriptions;
  cometd_sys_s     sys_s;
  GList*           exts;
};


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
gboolean    cometd_should_handshake (const cometd* h);
long        cometd_get_backoff  (const cometd* h, long attempt);
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

// processing
int  cometd_process_handshake(const cometd* h, JsonNode* root);
int  cometd_process_msg(const cometd* h, JsonNode* msg);

// other
gboolean          cometd_is_meta_channel(const char* channel);
GHashTable*       cometd_conn_subscriptions(const cometd* h);
void              cometd_listen(const cometd* h);

#endif /* COMETD_H */

