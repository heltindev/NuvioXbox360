#ifndef NUVIO_NATIVE_BACKEND_H
#define NUVIO_NATIVE_BACKEND_H

struct NuvioHttpData;

// All requests run on the Xbox 360's network stack: no PC gateway is needed.
bool NuvioNativeStartup();
void NuvioNativeShutdown();
bool NuvioNativeGet(const char *path, int maxBody, NuvioHttpData *result);
const char *NuvioNativeLastError();

#endif
