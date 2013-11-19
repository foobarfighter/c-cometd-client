#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>

char* http_json_post(const char* url, const char* data, int timeout);
int   http_valid_response(const char* body, size_t size);

#endif // HTTP_H
