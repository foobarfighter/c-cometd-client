#include "cometd.h"

cometd_config *_cometd_config = NULL;

void cometd_set_default_config(cometd_config* config){
  config->url = "";
  config->backoff_increment = DEFAULT_BACKOFF_INCREMENT;
  config->max_backoff       = DEFAULT_MAX_BACKOFF;
  config->max_network_delay = DEFAULT_MAX_NETWORK_DELAY;
  config->append_message_type_to_url = DEFAULT_APPEND_MESSAGE_TYPE;
}

cometd_config* cometd_configure(const cometd_config *config){
  if (config != NULL){
    _cometd_config = config;
  }

  return _cometd_config;
}

//int cometd_init(const &cometd_configuration config, const &properties props){
//  return 0;
//}
