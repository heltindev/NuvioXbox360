// Production HTTP byte stream, with Windows sockets and no fake HTTP replies.
#include "../ThirdParty/x360-homebrew-libs/https/Http.cpp"
#include <cassert>
#include <vector>
namespace Log {void Write(const char*,...) {}}
namespace Tls {
bool Startup(std::string&){return true;}
Connection::Connection():m_impl(NULL){} Connection::~Connection(){}
bool Connection::Handshake(unsigned int,const std::string&,std::string&){return false;}
int Connection::Send(const char*,int){return -1;} int Connection::Recv(char*,int){return -1;} void Connection::Close(){}
}
int main(int argc,char **argv) {
    assert(argc==3);std::string error,ip;assert(Http::Startup(error,ip));
    std::FILE *file=std::fopen(argv[2],"rb");assert(file);std::fseek(file,0,SEEK_END);
    long size=std::ftell(file);std::rewind(file);std::vector<char> expected(size);
    assert(std::fread(&expected[0],1,size,file)==(size_t)size);std::fclose(file);
    const std::string base=argv[1];
    const char *paths[]={"/sample.mp4","/redirect","/chunked"};
    for(int i=0;i<3;++i) {
        Http::Stream stream;assert(stream.Open(base+paths[i],"",0,error));
        std::vector<char> actual;char block[32768];int n;
        while((n=stream.Read(block,sizeof(block)))>0)actual.insert(actual.end(),block,block+n);
        assert(n==0&&actual==expected);
    }
    Http::Stream seek;assert(seek.Open(base+"/sample.mp4","",123456,error));
    assert(seek.Status()==206&&seek.TotalSize()==size);char block[7000];int got=seek.Read(block,sizeof(block));
    assert(got>0&&!std::memcmp(block,&expected[123456],got));
    Http::Stream wrong;assert(!wrong.Open(base+"/no-range","",99,error));
    Http::Stream malformed;assert(!malformed.Open(base+"/wrong-range","",99,error));
    Http::Stream shortFile;assert(shortFile.Open(base+"/truncated","",0,error));
    while((got=shortFile.Read(block,sizeof(block)))>0){}
    assert(got<0);
    Http::Stream badChunk;assert(badChunk.Open(base+"/bad-chunk","",0,error));assert(badChunk.Read(block,sizeof(block))<0);
    std::puts("PASS: production HTTP MP4 bytes, redirect, chunked, Range seek, invalid ranges, truncated download");
    return 0;
}
