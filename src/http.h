#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>

char* http_json_post(const char* url, const char* data, int timeout);

#endif // HTTP_H
