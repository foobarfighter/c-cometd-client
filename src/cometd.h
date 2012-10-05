#ifndef COMETD_H
#define COMETD_H

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
  const cometd_conn* conn;
  const cometd_config* config;
} cometd;


void            cometd_default_config   (cometd_config* config);
cometd_config*  cometd_configure        (cometd_config *config);
cometd*         cometd_init             ();
void            cometd_destroy          (cometd* h);


#endif /* COMETD_H */
