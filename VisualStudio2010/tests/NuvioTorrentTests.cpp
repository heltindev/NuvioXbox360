// Host integration test, using the public-domain protocol and an open Sintel sample.
#include "../src/NuvioTorrent.cpp"
#include <cassert>
namespace Http {
Response Request(const char*,const std::string&,const std::string&,const std::string&,unsigned long) {
    return Response(); // Integration fixtures use UDP tracker and native TCP peers.
}
}
int main(int argc,char **argv) {
    using namespace TorrentWire;
    assert(Sha1("abc",3)==HashBytes("a9993e364706816aba3e25717850c26c9cd0d89d"));
    assert(Sha1("",0)==HashBytes("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
    std::string many(1000000,'a');assert(Sha1(many.data(),many.size())==HashBytes("34aa973cd4c4daa4f61eeb2bdbad27316534016f"));
    assert(HashBytes("INVALID").empty());assert(HashBytes(std::string(40,'x')).empty());
    assert(HashBytes(std::string(32,'A'))==std::string(20,0));
    assert(Unescape("udp%3A%2F%2F127.0.0.1%3A19001")=="udp://127.0.0.1:19001");
    assert(Unescape("%GG").empty());
    const char *bad[]={"01:a","i-0e","i01e","ie","d1:ai1e1:ai2ee","9999999999999999999999:x","l", "i9223372036854775808e"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i){Value v;assert(!Parser(std::string(bad[i])).Parse(v));}
    Value v;assert(Parser(std::string("d1:ai0e1:bl1:xi-2eee")).Parse(v));
    assert(v.Get("a").number==0&&v.Get("b").list.size()==2);
    std::string prefix="d8:msg_typei1e5:piecei0e10:total_sizei3eeabc";size_t used=0;
    Value header;assert(Parser(prefix).Parse(header,&used)&&prefix.substr(used)=="abc");
    std::string error;NuvioTorrent invalid;assert(!invalid.Open("magnet:?xt=urn:btmh:bad",error));
    // Keep magnets without tracker lists discoverable, but never leak a
    // tracker-specific/private magnet to the public fallback trackers.
    std::vector<std::string> fallback;
    AddFallbackTrackers(fallback);
    assert(fallback.size()==4);
    assert(fallback[0].find("udp://")==0);
    std::vector<std::string> privateTrackers(1,"udp://127.0.0.1:19001/announce");
    AddFallbackTrackers(privateTrackers);
    assert(privateTrackers.size()==1);
    assert(privateTrackers[0]=="udp://127.0.0.1:19001/announce");
    assert(NuvioTorrentClearCache()); // host implementation does not touch disk
    std::puts("PASS: SHA1, bounded bencode, magnet parsing");
    if(argc<3)return 0;
    WSADATA wsa;assert(WSAStartup(MAKEWORD(2,2),&wsa)==0);
    std::FILE *f=std::fopen(argv[2],"rb");assert(f);std::fseek(f,0,SEEK_END);long size=std::ftell(f);std::rewind(f);
    std::vector<unsigned char> expected(size);assert(std::fread(&expected[0],1,size,f)==(size_t)size);std::fclose(f);
    NuvioTorrent torrent;
    if(!torrent.Open(argv[1],error)){std::fprintf(stderr,"Open: %s\n",error.c_str());return 1;}
    assert(torrent.Size()==size);
    std::vector<unsigned char> actual(size);int at=0;
    while(at<size){int n=torrent.Read(&actual[at],(std::min)(32768,(int)size-at));
        if(n<0){std::fprintf(stderr,"Read: %s\n",torrent.Error().c_str());if(argc>3)std::puts("PASS: corrupted data refused before delivery");return argc>3?0:2;}
        assert(n>0);at+=n;
    }
    if(argc>3){std::puts("FAIL: corrupt piece accepted");return 3;}
    assert(actual==expected);assert(torrent.Read(&actual[0],1)==0);
    const int offsets[]={17,262140,700000,4300000,0};
    for(int i=0;i<5;++i){int offset=offsets[i];assert(torrent.Seek(offset)==0);unsigned char bytes[333];int n=torrent.Read(bytes,sizeof(bytes));
        assert(n>0&&!std::memcmp(bytes,&expected[offset],n));}
    assert(torrent.Seek(-1)<0&&torrent.Seek(size+1)<0);
    torrent.Abort();assert(torrent.Read(&actual[0],1)<0);
    std::printf("PASS: UDP tracker, metadata, verified %ld bytes, seek and abort\n",size);
    return 0;
}
