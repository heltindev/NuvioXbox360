#ifndef NUVIO_TORRENT_H
#define NUVIO_TORRENT_H
#include <string>
// Bounded IPv4 BitTorrent v1 byte source for FFmpeg. The owner thread opens,
// reads and seeks; Abort may be called by the render thread.
class NuvioTorrent {
public:
    NuvioTorrent();
    ~NuvioTorrent();
    bool Open(const std::string &magnet,std::string &error);
    int Read(unsigned char *bytes,int count);
    int Seek(__int64 offset);
    void Abort();
    __int64 Size() const;
    const char *Status() const;
    // Preparation stages, not percentage of the whole movie downloaded.
    int StartupPercent() const;
    const std::string &Filename() const;
    const std::string &Error() const;
private:
    struct Impl;
    Impl *m;
    NuvioTorrent(const NuvioTorrent &);
    NuvioTorrent &operator=(const NuvioTorrent &);
};
// Clears the on-console metadata cache; in-memory piece buffers end with playback.
bool NuvioTorrentClearCache();
#endif
