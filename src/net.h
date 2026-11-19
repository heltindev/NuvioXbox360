#ifndef NUVIO_XEX_NET_H
#define NUVIO_XEX_NET_H

#include <stdbool.h>
#include <stddef.h>

bool xs_http_init(void);
void xs_http_shutdown(void);
bool xs_http_get(const char *url, char *buffer, size_t capacity,
                 char *error, size_t error_capacity);

#endif
