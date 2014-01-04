#include "cometd/error.h"
#include <stdlib.h>

cometd_error_st*
cometd_error_new(void)
{
  cometd_error_st* error = malloc(sizeof(cometd_error_st));
  error->code = COMETD_SUCCESS;
  error->message = NULL;
  return error;
}

int
cometd_error(const cometd* h, int code, char* message)
{
  h->last_error->code = code;
  h->last_error->message = message;
  return code;
}

cometd_error_st*
cometd_last_error(const cometd* h)
{
  return h->last_error;
}

void
cometd_error_destroy(cometd_error_st* error)
{
  free(error);
}
