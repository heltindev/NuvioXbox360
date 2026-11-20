#ifndef NUVIO_NETWORK_H
#define NUVIO_NETWORK_H

// Xbox 360 XNet/Winsock. The local Python bridge fetches modern HTTPS
// metadata and JPG posters; XEX receives plain HTTP from a trusted local PC.
struct NuvioHttpData {
    unsigned char *bytes;
    int length;
    int status;
};

bool NuvioNetworkStartup();
void NuvioNetworkShutdown();
bool NuvioHttpGet(const char *ip, int port, const char *path,
                  int maxBody, NuvioHttpData *result);
void NuvioHttpFree(NuvioHttpData *result);

#endif
