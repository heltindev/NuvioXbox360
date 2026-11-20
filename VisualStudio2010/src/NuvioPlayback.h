#ifndef NUVIO_PLAYBACK_H
#define NUVIO_PLAYBACK_H
#include <string>
struct NuvioPlaybackSource {
    std::string url, headers, name, prepareBody;
};
// Uses the selected source returned by the account's enabled addons.
// Native magnet is supported; optional authenticated conversion uses transcode_server.
bool NuvioNativePlaybackSource(int index,NuvioPlaybackSource *source);
#endif
