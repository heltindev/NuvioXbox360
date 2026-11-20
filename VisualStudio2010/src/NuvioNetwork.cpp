#include "NuvioNetwork.h"
#include "NuvioNativeBackend.h"

#include <xtl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static bool s_ready = false;

bool NuvioNetworkStartup()
{
    if (s_ready) return true;
    // The native HTTPS library owns XNet, DHCP, sockets and certificate validation.
    s_ready = NuvioNativeStartup();
    return s_ready;
}

void NuvioNetworkShutdown()
{
    if (!s_ready) return;
    NuvioNativeShutdown();
    s_ready = false;
}

void NuvioHttpFree(NuvioHttpData *result)
{
    if (!result) return;
    delete[] result->bytes;
    result->bytes = NULL;
    result->length = 0;
    result->status = 0;
}

bool NuvioHttpGet(const char *ip, int port, const char *path,
                  int maxBody, NuvioHttpData *result)
{
    // Existing UI request paths now resolve wholly inside the XEX. No PC/IP used.
    (void)ip;
    (void)port;
    return NuvioNativeGet(path, maxBody, result);
}
