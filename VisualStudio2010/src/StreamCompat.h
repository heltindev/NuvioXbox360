// StreamCompat.h - title-based playback hints (portable C++03 / Xbox 360 XDK).
// Never treat title metadata as proof of the actual codec: the demuxer verifies it.
#ifndef NUVIO_STREAM_COMPAT_H
#define NUVIO_STREAM_COMPAT_H

#include <string>
#include <cctype>

enum StreamSupport {
    STREAM_OK = 0,        // Codec claimed in the title, not yet verified by decoder.
    STREAM_UNKNOWN,       // No reliable codec/resolution hint: attempt normal playback.
    STREAM_UNSUPPORTED    // Explicitly incompatible with native 720p FFmpeg player.
};

struct StreamCompat {
    StreamSupport support;
    const char *reason;
};

namespace StreamCompatImpl {
inline std::string Lower(const std::string &value) {
    std::string out(value);
    for (std::string::size_type i=0; i<out.size(); ++i)
        out[i]=(char)std::tolower((unsigned char)out[i]);
    return out;
}
inline bool HasToken(const std::string &haystack,const char *needle) {
    const std::string token(needle);
    if(token.empty())return false;
    std::string::size_type at=0;
    while((at=haystack.find(token,at))!=std::string::npos) {
        const bool left=at==0||!std::isalnum((unsigned char)haystack[at-1]);
        const std::string::size_type end=at+token.size();
        const bool right=end>=haystack.size()||!std::isalnum((unsigned char)haystack[end]);
        if(left&&right)return true;
        ++at;
    }
    return false;
}
inline StreamCompat Make(StreamSupport support,const char *reason) {
    StreamCompat r;r.support=support;r.reason=reason;return r;
}
}

// Call on the stream title/name/description; unknown formats remain selectable.
// The native player's max video size is 1280x720, even for H.264.
inline StreamCompat ClassifyStream(const std::string &title) {
    using namespace StreamCompatImpl;
    const std::string t=Lower(title);
    if(HasToken(t,"drm")||HasToken(t,"widevine")||HasToken(t,"playready"))
        return Make(STREAM_UNSUPPORTED,"Protecao DRM");
    if(HasToken(t,"hevc")||HasToken(t,"h265")||HasToken(t,"h.265")||
       HasToken(t,"h 265")||HasToken(t,"x265"))
        return Make(STREAM_UNSUPPORTED,"HEVC/H.265");
    if(HasToken(t,"av1")||HasToken(t,"av01"))
        return Make(STREAM_UNSUPPORTED,"AV1");
    if(HasToken(t,"vp9"))
        return Make(STREAM_UNSUPPORTED,"VP9");
    if(HasToken(t,"10bit")||HasToken(t,"10-bit")||HasToken(t,"10 bit")||
       HasToken(t,"10bits")||HasToken(t,"hi10p")||HasToken(t,"hi10")||
       HasToken(t,"main10"))
        return Make(STREAM_UNSUPPORTED,"Video 10-bit");
    if(HasToken(t,"2160p")||HasToken(t,"2160")||HasToken(t,"4k")||
       HasToken(t,"uhd")||HasToken(t,"4320p")||HasToken(t,"8k")||
       HasToken(t,"1440p")||HasToken(t,"1080p")||HasToken(t,"1080i")||
       HasToken(t,"1080")||HasToken(t,"fhd")||HasToken(t,"1920x1080")||
       HasToken(t,"2560x1440"))
        return Make(STREAM_UNSUPPORTED,"Resolucao acima de 720p");
    if(HasToken(t,"hdr")||HasToken(t,"hdr10")||HasToken(t,"hdr10+")||
       HasToken(t,"dolby vision")||HasToken(t,"dolbyvision")||HasToken(t,"dovi"))
        return Make(STREAM_UNSUPPORTED,"HDR/Dolby Vision");
    if(HasToken(t,"h264")||HasToken(t,"h.264")||HasToken(t,"x264")||
       HasToken(t,"avc")||HasToken(t,"xvid")||HasToken(t,"divx")||
       HasToken(t,"mpeg-2")||HasToken(t,"wmv"))
        return Make(STREAM_OK,"");
    return Make(STREAM_UNKNOWN,"");
}
inline const char *UnsupportedMessage() {
    return "Formato nao suportado no Xbox 360; escolha video 720p compativel.";
}
#endif // NUVIO_STREAM_COMPAT_H
