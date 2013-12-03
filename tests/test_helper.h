#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <glib.h>
#include <json-glib/json-glib.h>
#include "cometd.h"

#define TEST_SERVER_URL            "http://localhost:8089/cometd"
#define TEST_BAD_JSON_URL          "http://localhost:8090/bad_json"
#define TEST_LONG_REQUEST_URL      "http://localhost:8090/long_request"
#define TEST_ERROR_CODE_URL        "http://localhost:8090/echo_code?code=500"
#define TEST_SUCCESS_CODE_URL      "http://localhost:8090/echo_code?code=200"
#define TEST_LONG_REQUEST_TIMEOUT  20


// log stuff
int log_handler(const cometd* h, JsonNode* message);
int log_has_message(JsonNode* message);
guint log_size(void);
void log_clear(void);
guint wait_for_log_size(guint size);

// json matchers
gboolean json_node_equal(JsonNode* a, JsonNode* b, GList* excludes);

#endif // TEST_HELPER_H
