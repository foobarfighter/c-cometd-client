#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <glib.h>
#include <json-glib/json-glib.h>
#include "cometd.h"

int inbox_handler(const cometd* h, JsonNode* node);
void await(int result);
void error_handler(int sig);

#endif // TEST_HELPER_H
