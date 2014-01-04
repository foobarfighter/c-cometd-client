#ifndef COMETD_ERROR_H
#define COMETD_ERROR_H

#include "../cometd.h"

typedef enum {
  COMETD_SUCCESS = 0,
  ECOMETD_JSON_SERIALIZE,
  ECOMETD_JSON_DESERIALIZE,
  ECOMETD_INIT_LOOP,
  ECOMETD_UNKNOWN,
  ECOMETD_NO_TRANSPORT,
  ECOMETD_UNSUCCESSFUL,
  ECOMETD_SEND
} cometd_code;

struct _cometd_error_st {
  int   code;
  char* message;
};

cometd_error_st* cometd_error_new(void);
int cometd_error(const cometd* h, int code, char* message);
cometd_error_st* cometd_last_error(const cometd* h);
void cometd_error_destroy(cometd_error_st* error);

#endif
