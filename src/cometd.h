#ifndef COMETD_H
#define COMETD_H

#define NULL                          0

#define DEFAULT_BACKOFF_INCREMENT     1000
#define DEFAULT_MAX_BACKOFF           1000
#define DEFAULT_MAX_NETWORK_DELAY     1000
#define DEFAULT_APPEND_MESSAGE_TYPE   1

typedef struct {
  char* url;
  int   backoff_increment;
  int   max_backoff;
  int   max_network_delay;
  int   append_message_type_to_url;
} cometd_config;


void            cometd_set_default_config (cometd_config* config);
cometd_config*  cometd_configure          (const cometd_config *config);


#endif /* COMETD_H */
