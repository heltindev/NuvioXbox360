// Independent C++98 implementation of BEP 3/5/9/10/15, for Xbox 360.
// Limits: IPv4, v1 SHA-1 torrents, 64 discovered peers, 1 MiB metadata,
// 4 MiB per piece and two verified pieces in RAM. No whole-file download.
#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif
#include "NuvioTorrent.h"
#include "Http.h"
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace TorrentWire {
typedef unsigned int U32;
static U32 Rol(U32 n,int bits) {return (n<<bits)|(n>>(32-bits));}
static std::string Sha1(const void *input,size_t size) {
    const unsigned char *data=(const unsigned char*)input;
    U32 h[5]={0x67452301,0xEFCDAB89,0x98BADCFE,0x10325476,0xC3D2E1F0};
    const size_t blocks=(size+9+63)/64;
    for(size_t block=0;block<blocks;++block) {
        unsigned char b[64]={0};
        for(int i=0;i<64;++i) {
            size_t at=block*64+i;
            if(at<size)b[i]=data[at];
            else if(at==size)b[i]=0x80;
        }
        if(block+1==blocks) {
            unsigned __int64 bits=(unsigned __int64)size*8;
            for(int i=0;i<8;++i)b[63-i]=(unsigned char)(bits>>(8*i));
        }
        U32 w[80];
        for(int i=0;i<16;++i)w[i]=((U32)b[i*4]<<24)|((U32)b[i*4+1]<<16)|((U32)b[i*4+2]<<8)|b[i*4+3];
        for(int i=16;i<80;++i)w[i]=Rol(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        U32 a=h[0],c=h[2],d=h[3],e=h[4],v=h[1];
        for(int i=0;i<80;++i) {
            U32 f,k;
            if(i<20){f=(v&c)|(~v&d);k=0x5A827999;}
            else if(i<40){f=v^c^d;k=0x6ED9EBA1;}
            else if(i<60){f=(v&c)|(v&d)|(c&d);k=0x8F1BBCDC;}
            else {f=v^c^d;k=0xCA62C1D6;}
            U32 t=Rol(a,5)+f+e+k+w[i];e=d;d=c;c=Rol(v,30);v=a;a=t;
        }
        h[0]+=a;h[1]+=v;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    std::string out(20,0);
    for(int i=0;i<20;++i)out[i]=(char)(h[i/4]>>(24-8*(i%4)));
    return out;
}
static U32 Read32(const char *p) {
    const unsigned char *b=(const unsigned char*)p;
    return (U32)b[0]<<24|(U32)b[1]<<16|(U32)b[2]<<8|b[3];
}
static void Put32(std::string &s,U32 n) {
    s+=(char)(n>>24);s+=(char)(n>>16);s+=(char)(n>>8);s+=(char)n;
}
static int Hex(char c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static std::string Unescape(const std::string &s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='%') {
            if(i+2>=s.size()||Hex(s[i+1])<0||Hex(s[i+2])<0)return "";
            out+=(char)(Hex(s[i+1])*16+Hex(s[i+2]));i+=2;
        } else out+=s[i]=='+'?' ':s[i];
    }
    return out;
}
static std::string Escape(const std::string &s) {
    const char *hex="0123456789ABCDEF";std::string out;
    for(size_t i=0;i<s.size();++i) {unsigned char c=s[i];out+='%';out+=hex[c>>4];out+=hex[c&15];}
    return out;
}
static std::string HashBytes(const std::string &s) {
    std::string out;
    if(s.size()==40)for(size_t i=0;i<40;i+=2) {
        int a=Hex(s[i]),b=Hex(s[i+1]);if(a<0||b<0)return "";out+=(char)(a*16+b);
    } else if(s.size()==32) {
        U32 buffer=0;int bits=0;
        for(size_t i=0;i<s.size();++i) {
            char c=s[i];if(c>='a'&&c<='z')c-='a'-'A';
            int n=c>='A'&&c<='Z'?c-'A':c>='2'&&c<='7'?c-'2'+26:-1;
            if(n<0)return "";buffer=(buffer<<5)|n;bits+=5;
            if(bits>=8){bits-=8;out+=(char)(buffer>>bits);}
        }
    }
    return out.size()==20?out:std::string();
}
struct Value {
    char type;__int64 number;std::string text;
    std::vector<Value> list;std::map<std::string,Value> dict;
    Value():type('?'),number(0){}
    const Value &Get(const char *key) const {
        static const Value nullValue;
        std::map<std::string,Value>::const_iterator it=dict.find(key);
        return it==dict.end()?nullValue:it->second;
    }
};
class Parser {
    const std::string &s;size_t p;int nodes;
    bool Read(Value &v,int depth) {
        if(depth>16||++nodes>20000||p>=s.size())return false;
        char c=s[p++];
        if(c=='i') {
            bool negative=false;if(p<s.size()&&s[p]=='-'){negative=true;++p;}
            size_t begin=p;unsigned __int64 n=0;
            while(p<s.size()&&s[p]>='0'&&s[p]<='9') {
                if(n>900000000000000000ULL)return false;
                n=n*10+(s[p++]-'0');
            }
            if(p==begin||p>=s.size()||s[p++]!='e'||(p-begin>2&&s[begin]=='0')||(negative&&!n))return false;
            v.type='i';v.number=negative?-(__int64)n:(__int64)n;return true;
        }
        if(c>='0'&&c<='9') {
            size_t n=c-'0',digits=1;
            while(p<s.size()&&s[p]>='0'&&s[p]<='9') {if(n>1024*1024)return false;n=n*10+(s[p++]-'0');++digits;}
            if((c=='0'&&digits>1)||p>=s.size()||s[p++]!=':'||n>s.size()-p||n>1024*1024)return false;
            v.type='s';v.text.assign(s,p,n);p+=n;return true;
        }
        if(c!='d'&&c!='l')return false;
        v.type=c;
        while(p<s.size()&&s[p]!='e') {
            if(c=='l') {Value item;if(!Read(item,depth+1))return false;v.list.push_back(item);}
            else {
                Value key,item;
                if(!Read(key,depth+1)||key.type!='s'||key.text.size()>512||!Read(item,depth+1)||
                    v.dict.find(key.text)!=v.dict.end())return false;
                v.dict[key.text]=item;
            }
        }
        if(p>=s.size())return false;++p;return true;
    }
public:
    Parser(const std::string &input):s(input),p(0),nodes(0){}
    bool Parse(Value &v,size_t *used=NULL) {
        if(s.size()>1100000||!Read(v,0))return false;
        if(used)*used=p;
        return used||p==s.size();
    }
};
static std::string BString(const std::string &s) {char n[24];std::sprintf(n,"%u:",(unsigned)s.size());return n+s;}
// Public fallback trackers from NuvioTV; keep private/custom trackers isolated.
static void AddFallbackTrackers(std::vector<std::string> &trackers) {
    if(!trackers.empty())return;
    const char *defaults[]={
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://tracker.torrent.eu.org:451/announce",
        "udp://tracker.publictracker.xyz:6969/announce",
        "udp://open.stealth.si:80/announce"
    };
    for(size_t i=0;i<sizeof(defaults)/sizeof(defaults[0]);++i)trackers.push_back(defaults[i]);
}
struct Endpoint {
    U32 ip;int port;std::string id;bool visited;
    Endpoint():ip(0),port(0),visited(false){}
    bool operator==(const Endpoint &other)const{return ip==other.ip&&port==other.port;}
};
static void CompactPeers(const std::string &s,std::vector<Endpoint> &out) {
    if(s.size()%6)return;
    for(size_t i=0;i+6<=s.size()&&out.size()<64;i+=6) {
        Endpoint e;std::memcpy(&e.ip,s.data()+i,4);
        e.port=((unsigned char)s[i+4]<<8)|(unsigned char)s[i+5];
        if(e.ip&&e.ip!=0xFFFFFFFF&&e.port&&std::find(out.begin(),out.end(),e)==out.end())out.push_back(e);
    }
}
static bool Resolve(const std::string &host,U32 &ip) {
#ifdef _XBOX
    unsigned long result=0;std::string error;
    if(!Http::ResolveIPv4(host,&result,error))return false;ip=(U32)result;return true;
#else
    ip=inet_addr(host.c_str());if(ip!=INADDR_NONE)return true;
    hostent *entry=gethostbyname(host.c_str());
    if(!entry||entry->h_length!=4)return false;std::memcpy(&ip,entry->h_addr_list[0],4);return true;
#endif
}
static void Random(unsigned char *out,int n) {
#ifdef _XBOX
    XNetRandom(out,n);
#else
    for(int i=0;i<n;++i)out[i]=(unsigned char)(std::rand()&255);
#endif
}
static sockaddr_in Address(const Endpoint &e) {
    sockaddr_in a;std::memset(&a,0,sizeof(a));a.sin_family=AF_INET;a.sin_addr.s_addr=e.ip;a.sin_port=htons((u_short)e.port);return a;
}
}

using namespace TorrentWire;
struct NuvioTorrent::Impl {
    volatile LONG quit,phase;
    SOCKET peer,control;
    CRITICAL_SECTION lock;
    std::string hash,peerId,metadata,pieceHashes,name,error,bitfield;
    std::vector<std::string> trackers;
    std::vector<Endpoint> peers;
    __int64 position,fileOffset,fileSize,totalSize;
    int pieceLength,selectedFile,peerIndex,metadataId,nextPeer;
    size_t nextMetadataPeer;
    int metadataRejected;
    bool choked,supportsExtension;
    unsigned clock;
    struct Cache {int piece;unsigned age;std::vector<unsigned char> bytes;Cache():piece(-1),age(0){}};
    Cache cache[2];
    Impl():quit(0),phase(0),peer(INVALID_SOCKET),control(INVALID_SOCKET),position(0),fileOffset(0),
        fileSize(0),totalSize(0),pieceLength(0),selectedFile(-1),peerIndex(-1),metadataId(0),nextPeer(0),nextMetadataPeer(0),metadataRejected(0),choked(true),supportsExtension(false),clock(0) {
        InitializeCriticalSection(&lock);
    }
    ~Impl(){Close(peer);Close(control);DeleteCriticalSection(&lock);}
    bool Fail(const char *message){error=message;phase=6;return false;}
    // Bound the on-disk cache to one verified metadata record (max 1 MiB).
    // It is stored on the Xbox itself, never on a PC or remote service.
    bool LoadCachedMetadata() {
#ifdef _XBOX
        std::FILE *file=std::fopen("game:\\nuvio_last_torrent.meta","rb");
        if(!file)return false;
        char header[24];
        bool valid=std::fread(header,1,sizeof(header),file)==sizeof(header)&&
            std::memcmp(header,"NVT1",4)==0&&std::memcmp(header+4,hash.data(),20)==0;
        std::string bytes;
        if(valid) {
            char block[16384];size_t got=0;
            while((got=std::fread(block,1,sizeof(block),file))>0) {
                if(bytes.size()+got>1024*1024){valid=false;break;}
                bytes.append(block,got);
            }
            if(std::ferror(file))valid=false;
        }
        std::fclose(file);
        if(valid&&!bytes.empty()&&Sha1(bytes.data(),bytes.size())==hash) {
            metadata.swap(bytes);return true;
        }
#endif
        return false;
    }
    void SaveCachedMetadata() {
#ifdef _XBOX
        if(metadata.empty()||metadata.size()>1024*1024)return;
        std::FILE *file=std::fopen("game:\\nuvio_last_torrent.meta","wb");
        if(!file)return;
        std::fwrite("NVT1",1,4,file);
        std::fwrite(hash.data(),1,20,file);
        std::fwrite(metadata.data(),1,metadata.size(),file);
        std::fclose(file);
#endif
    }
    SOCKET NewSocket(int type) {
        SOCKET s=socket(AF_INET,type,type==SOCK_STREAM?IPPROTO_TCP:IPPROTO_UDP);
        if(s==INVALID_SOCKET)return s;
#ifdef _XBOX
        BOOL insecure=TRUE;setsockopt(s,SOL_SOCKET,0x5801,(const char*)&insecure,sizeof(insecure));
#endif
        u_long yes=1;ioctlsocket(s,FIONBIO,&yes);return s;
    }
    void Set(SOCKET &target,SOCKET value) {EnterCriticalSection(&lock);target=value;LeaveCriticalSection(&lock);}
    void Close(SOCKET &target) {
        EnterCriticalSection(&lock);SOCKET s=target;target=INVALID_SOCKET;LeaveCriticalSection(&lock);
        if(s!=INVALID_SOCKET)closesocket(s);
    }
    bool Wait(SOCKET socket,bool writing,DWORD deadline) {
        while(!quit&&(LONG)(deadline-GetTickCount())>0) {
            fd_set f,failed;FD_ZERO(&f);FD_SET(socket,&f);FD_ZERO(&failed);FD_SET(socket,&failed);timeval tv={0,200000};
            int n=select(0,writing?NULL:&f,writing?&f:NULL,&failed,&tv);
            if(FD_ISSET(socket,&failed))return false;
            if(n>0)return true;if(n==SOCKET_ERROR)return false;
        }
        return false;
    }
    bool Send(SOCKET socket,const char *bytes,int count,DWORD deadline) {
        while(count>0) {
            if(!Wait(socket,true,deadline))return false;
            int n=send(socket,bytes,count,0);
            if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;
            if(n<=0)return false;bytes+=n;count-=n;
        }return true;
    }
    bool Receive(SOCKET socket,char *bytes,int count,DWORD deadline) {
        while(count>0) {
            if(!Wait(socket,false,deadline))return false;
            int n=recv(socket,bytes,count,0);
            if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;
            if(n<=0)return false;bytes+=n;count-=n;
        }return true;
    }
    bool Datagram(const Endpoint &dest,const std::string &request,std::string &reply,int timeout) {
        if(quit)return false;
        Close(control);Set(control,NewSocket(SOCK_DGRAM));
        if(control==INVALID_SOCKET)return false;
        sockaddr_in to=Address(dest);
        if(sendto(control,request.data(),(int)request.size(),0,(sockaddr*)&to,sizeof(to))!=(int)request.size())return false;
        const DWORD deadline=GetTickCount()+timeout;
        while(Wait(control,false,deadline)) {
            char data[8192];sockaddr_in from;int length=sizeof(from);
            int got=recvfrom(control,data,sizeof(data),0,(sockaddr*)&from,&length);
            if(got>0&&from.sin_addr.s_addr==dest.ip&&from.sin_port==to.sin_port) {
                reply.assign(data,got);Close(control);return true;
            }
        }
        Close(control);return false;
    }
    bool Tracker(const std::string &url) {
        if(url.find("udp://")==0) {
            size_t slash=url.find('/',6);std::string authority=url.substr(6,slash==std::string::npos?std::string::npos:slash-6);
            size_t colon=authority.find(':');Endpoint dest;
            if(colon==std::string::npos||!Resolve(authority.substr(0,colon),dest.ip))return false;
            dest.port=std::atoi(authority.c_str()+colon+1);if(dest.port<1||dest.port>65535)return false;
            unsigned char random[4];Random(random,4);U32 tx=Read32((char*)random);
            std::string request,reply;Put32(request,0x417);Put32(request,0x27101980);Put32(request,0);Put32(request,tx);
            if(!Datagram(dest,request,reply,2000)||reply.size()<16||Read32(reply.data())!=0||Read32(reply.data()+4)!=tx)return false;
            request=reply.substr(8,8);Put32(request,1);Put32(request,++tx);request+=hash+peerId;
            Put32(request,0);Put32(request,0);Put32(request,0);Put32(request,1); // downloaded/left
            Put32(request,0);Put32(request,0);Put32(request,2);Put32(request,0);Put32(request,tx^0x13579);
            Put32(request,32);request+=(char)0xC8;request+=(char)0xD5; // advertised port 51413, outbound-only
            if(!Datagram(dest,request,reply,2500)||reply.size()<20||Read32(reply.data())!=1||Read32(reply.data()+4)!=tx)return false;
            CompactPeers(reply.substr(20),peers);return !peers.empty();
        }
        if(url.find("https://")!=0&&url.find("http://")!=0)return false;
        std::string query=url+(url.find('?')==std::string::npos?"?":"&")+"info_hash="+Escape(hash)+
            "&peer_id="+Escape(peerId)+"&port=51413&uploaded=0&downloaded=0&left=1&compact=1&event=started&numwant=32";
        Http::Response reply=Http::Get(query,"User-Agent: Nuvio/1.0\r\n",5000);
        Value body;if(reply.status!=200||!Parser(reply.body).Parse(body))return false;
        const Value &p=body.Get("peers");
        if(p.type=='s')CompactPeers(p.text,peers);
        else if(p.type=='l')for(size_t i=0;i<p.list.size()&&peers.size()<64;++i) {
            Endpoint e;e.port=(int)p.list[i].Get("port").number;
            if(e.port>0&&e.port<65536&&Resolve(p.list[i].Get("ip").text,e.ip)&&std::find(peers.begin(),peers.end(),e)==peers.end())peers.push_back(e);
        }
        return !peers.empty();
    }
    void Dht(size_t targetPeers) {
        phase=1;std::vector<Endpoint> nodes;
        const char *hosts[]={"dht.transmissionbt.com","router.bittorrent.com","dht.libtorrent.org"};
        for(int i=0;i<3&&!quit;++i){Endpoint e;if(Resolve(hosts[i],e.ip)){e.port=i==2?25401:6881;nodes.push_back(e);}}
        for(int query=0;query<32&&!quit&&peers.size()<targetPeers;++query) {
            int best=-1;
            for(size_t i=0;i<nodes.size();++i)if(!nodes[i].visited) {
                if(best<0||nodes[i].id.empty())best=(int)i;
                else if(!nodes[best].id.empty()) {
                    for(int b=0;b<20;++b) {
                        int a=(unsigned char)nodes[i].id[b]^(unsigned char)hash[b];
                        int c=(unsigned char)nodes[best].id[b]^(unsigned char)hash[b];
                        if(a<c)best=(int)i;if(a!=c)break;
                    }
                }
            }
            if(best<0)break;Endpoint current=nodes[best];nodes[best].visited=true;
            char tx[2]={(char)query,(char)(query^0x69)};std::string transaction(tx,2);
            std::string request="d1:ad2:id"+BString(peerId)+"9:info_hash"+BString(hash)+"e1:q9:get_peers2:roi1e1:t"+BString(transaction)+"1:y1:qe",reply;
            if(!Datagram(current,request,reply,1300))continue;
            Value body;if(!Parser(reply).Parse(body)||body.Get("t").text!=transaction||body.Get("y").text!="r")continue;
            const Value &r=body.Get("r"),&values=r.Get("values");
            for(size_t i=0;i<values.list.size();++i)CompactPeers(values.list[i].text,peers);
            const std::string &compact=r.Get("nodes").text;
            if(compact.size()%26)continue;
            for(size_t i=0;i+26<=compact.size()&&nodes.size()<80;i+=26) {
                Endpoint e;e.id=compact.substr(i,20);std::memcpy(&e.ip,compact.data()+i+20,4);
                e.port=((unsigned char)compact[i+24]<<8)|(unsigned char)compact[i+25];
                if(e.ip&&e.port&&std::find(nodes.begin(),nodes.end(),e)==nodes.end())nodes.push_back(e);
            }
        }
    }
    bool Message(int id,const std::string &payload,DWORD deadline) {
        std::string wire;Put32(wire,(U32)payload.size()+1);wire+=(char)id;wire+=payload;
        return Send(peer,wire.data(),(int)wire.size(),deadline);
    }
    bool ReadMessage(int &id,std::string &payload,DWORD deadline) {
        while(!quit) {
            char header[4];if(!Receive(peer,header,4,deadline))return false;
            U32 n=Read32(header);if(!n)continue;if(n>128*1024)return false;
            char kind;if(!Receive(peer,&kind,1,deadline))return false;
            id=(unsigned char)kind;payload.resize(n-1);
            if(n>1&&!Receive(peer,&payload[0],n-1,deadline))return false;
            if(id==0)choked=true;if(id==1)choked=false;
            if(id==5)bitfield=payload;
            if(id==20&&payload.size()>1&&(unsigned char)payload[0]==2) {
                Value pex;if(Parser(payload.substr(1)).Parse(pex))CompactPeers(pex.Get("added").text,peers);
            }
            return true;
        }return false;
    }
    bool ConnectPeer(int index,int timeout=4500) {
        Close(peer);Set(peer,NewSocket(SOCK_STREAM));peerIndex=-1;metadataId=0;choked=true;supportsExtension=false;bitfield.clear();
        if(peer==INVALID_SOCKET||index<0||index>=(int)peers.size())return false;
        sockaddr_in address=Address(peers[index]);int result=connect(peer,(sockaddr*)&address,sizeof(address));
        if(result==SOCKET_ERROR&&WSAGetLastError()!=WSAEWOULDBLOCK&&WSAGetLastError()!=WSAEINPROGRESS)return false;
        DWORD deadline=GetTickCount()+timeout;if(!Wait(peer,true,deadline))return false;
#ifndef _XBOX
        int socketError=0,length=sizeof(socketError);getsockopt(peer,SOL_SOCKET,SO_ERROR,(char*)&socketError,&length);
        if(socketError)return false;
#endif
        std::string handshake;handshake+=(char)19;handshake+="BitTorrent protocol";handshake.append(8,0);handshake[25]=0x10;
        handshake+=hash+peerId;char response[68];
        if(!Send(peer,handshake.data(),68,deadline)||!Receive(peer,response,68,deadline)||
            (unsigned char)response[0]!=19||std::memcmp(response+1,"BitTorrent protocol",19)||std::memcmp(response+28,hash.data(),20))return false;
        peerIndex=index;
        supportsExtension=(response[25]&0x10)!=0;
        if(supportsExtension) {
            std::string ext(1,0);ext+="d1:md11:ut_metadatai1e6:ut_pexi2ee1:v8:Nuvio360e";
            if(!Message(20,ext,deadline))return false;
        }
        return true;
    }
    bool Metadata(size_t firstPeer) {
        phase=2;
        DWORD totalDeadline=GetTickCount()+45000;
        for(size_t p=firstPeer;p<peers.size()&&!quit&&(LONG)(totalDeadline-GetTickCount())>0;++p) {
            nextMetadataPeer=p+1;
            if(!ConnectPeer((int)p,2500)||!supportsExtension)continue;
            DWORD deadline=GetTickCount()+8000;
            if((LONG)(deadline-totalDeadline)>0)deadline=totalDeadline;
            int metadataSize=0;
            while(!quit&&(!metadataId||!metadataSize)&&(LONG)(deadline-GetTickCount())>0) {
                int id;std::string payload;
                if(!ReadMessage(id,payload,deadline))break;
                if(id==20&&payload.size()>1&&payload[0]==0) {
                    Value ext;
                    if(Parser(payload.substr(1)).Parse(ext)) {
                        const Value &idValue=ext.Get("m").Get("ut_metadata"),&sizeValue=ext.Get("metadata_size");
                        if(idValue.type=='i'&&idValue.number>=0&&idValue.number<=255)metadataId=(int)idValue.number;
                        if(sizeValue.type=='i'&&sizeValue.number>0&&sizeValue.number<=1024*1024)metadataSize=(int)sizeValue.number;
                    }
                }
            }
            if(metadataId<1||metadataId>255||metadataSize<1||metadataSize>1024*1024)continue;
            // BEP 9: bounded pipeline, accept any requested block order and ignore unknown types.
            const int parts=(metadataSize+16383)/16384;
            std::string collected(metadataSize,0);
            std::vector<unsigned char> sent(parts,0),done(parts,0);
            int received=0,inflight=0;bool ok=true;
            deadline=GetTickCount()+6000;
            while(received<parts&&!quit&&(LONG)(totalDeadline-GetTickCount())>0) {
                DWORD ioDeadline=(LONG)(deadline-totalDeadline)>0?totalDeadline:deadline;
                for(int part=0;part<parts&&inflight<4;++part)if(!sent[part]) {
                    char dict[70];std::sprintf(dict,"d8:msg_typei0e5:piecei%dee",part);
                    std::string request(1,(char)metadataId);request+=dict;
                    if(!Message(20,request,ioDeadline)){ok=false;break;}
                    sent[part]=1;++inflight;
                }
                if(!ok)break;
                int id;std::string payload;
                if(!ReadMessage(id,payload,ioDeadline)){ok=false;break;}
                if(id!=20||payload.size()<2||(unsigned char)payload[0]!=1)continue;
                std::string data=payload.substr(1);Value reply;size_t used=0;
                if(!Parser(data).Parse(reply,&used)||reply.type!='d')continue;
                const Value &type=reply.Get("msg_type"),&piece=reply.Get("piece");
                if(type.type!='i'||piece.type!='i'||piece.number<0||piece.number>=parts)continue;
                const int part=(int)piece.number;
                if(!sent[part])continue;
                if(type.number==2){++metadataRejected;ok=false;break;}
                if(type.number!=1)continue;
                int want=(std::min)(16384,metadataSize-part*16384);
                if(reply.Get("total_size").type!='i'||reply.Get("total_size").number!=metadataSize||
                    (int)(data.size()-used)!=want){ok=false;break;}
                if(!done[part]) {
                    std::memcpy(&collected[part*16384],data.data()+used,want);
                    done[part]=1;++received;--inflight;deadline=GetTickCount()+6000;
                }
            }
            if(ok&&received==parts&&Sha1(collected.data(),collected.size())==hash) {
                metadata.swap(collected);return true;
            }
        }
        return false; // Caller can discover and retry new peers before reporting error.
    }
    bool FileInfo() {
        Value info;if(!Parser(metadata).Parse(info)||info.type!='d')return Fail("Metadados torrent invalidos.");
        __int64 length=info.Get("piece length").number;
        if(length<16384||length>4*1024*1024||(length&(length-1)))return Fail("Tamanho de parte do torrent nao suportado (maximo 4 MiB).");
        pieceLength=(int)length;pieceHashes=info.Get("pieces").text;
        const Value &files=info.Get("files");
        if(files.type=='l') {
            if(files.list.empty()||files.list.size()>2048)return Fail("Numero de arquivos do torrent nao suportado.");
            __int64 largest=-1;
            for(size_t i=0;i<files.list.size();++i) {
                const Value &file=files.list[i];__int64 size=file.Get("length").number;
                if(size<0||size>1000000000000LL||totalSize>1000000000000LL-size)return Fail("Tamanho do torrent invalido.");
                const Value &path=file.Get("path.utf-8").type=='l'?file.Get("path.utf-8"):file.Get("path");
                std::string filename=path.list.empty()?"video":path.list.back().text;
                std::string lower=filename;for(size_t c=0;c<lower.size();++c)if(lower[c]>='A'&&lower[c]<='Z')lower[c]+='a'-'A';
                bool video=false;
                const char *ext[]={".mp4",".mkv",".avi",".wmv",".m4v",".mov",".mpg",".mpeg",".ts",".m2ts",".webm",
                    ".flv",".3gp",".3g2",".vob",".divx",".asf",".ogv",".mts",".m2t",".f4v"};
                for(size_t e=0;e<sizeof(ext)/sizeof(ext[0]);++e) {
                    size_t n=std::strlen(ext[e]);
                    if(lower.size()>=n&&lower.compare(lower.size()-n,n,ext[e])==0){video=true;break;}
                }
                if((selectedFile>=0&&(int)i==selectedFile)||(selectedFile<0&&video&&size>largest)) {
                    fileOffset=totalSize;fileSize=size;name=filename;largest=size;
                }
                totalSize+=size;
            }
            if(fileSize<=0)return Fail("Arquivo de video nao encontrado no torrent.");
        } else {
            totalSize=fileSize=info.Get("length").number;name=info.Get("name.utf-8").text;
            if(name.empty())name=info.Get("name").text;
            if(selectedFile>0||fileSize<=0||fileSize>1000000000000LL)return Fail("Arquivo do torrent invalido.");
        }
        if(pieceHashes.empty()||pieceHashes.size()%20||pieceHashes.size()/20!=(size_t)((totalSize+pieceLength-1)/pieceLength))
            return Fail("Tabela de verificacao do torrent invalida.");
        if(name.size()>255)name.resize(255);
        phase=3;return true;
    }
    bool Interested() {
        DWORD deadline=GetTickCount()+10000;
        if(!Message(2,"",deadline))return false;
        while(choked&&!quit) {
            int id;std::string payload;if(!ReadMessage(id,payload,deadline))return false;
        }
        return !quit;
    }
    bool Download(int piece,std::vector<unsigned char> &bytes) {
        int amount=(int)((std::min)((__int64)pieceLength,totalSize-(__int64)piece*pieceLength));
        if(amount<=0)return false;bytes.resize(amount);
        const int blocks=(amount+16383)/16384;
        std::vector<unsigned char> sent(blocks,0),done(blocks,0);
        int inflight=0,received=0;DWORD deadline=GetTickCount()+25000;
        while(received<blocks&&!quit) {
            for(int b=0;b<blocks&&inflight<8;++b)if(!sent[b]) {
                std::string request;Put32(request,piece);Put32(request,b*16384);Put32(request,(std::min)(16384,amount-b*16384));
                if(!Message(6,request,deadline))return false;sent[b]=1;++inflight;
            }
            int id;std::string payload;if(!ReadMessage(id,payload,deadline))return false;
            if(id==0)return false;
            if(id!=7||payload.size()<8)continue;
            U32 index=Read32(payload.data()),begin=Read32(payload.data()+4);
            if(index!=(U32)piece||begin%16384||begin>=(U32)amount)continue;
            int b=begin/16384,want=(std::min)(16384,amount-(int)begin);
            if((int)payload.size()!=8+want||!sent[b])return false;
            if(!done[b]) {
                std::memcpy(&bytes[begin],payload.data()+8,want);done[b]=1;++received;--inflight;
            }
        }
        return !quit&&Sha1(&bytes[0],bytes.size())==pieceHashes.substr((size_t)piece*20,20);
    }
    Cache *Piece(int piece) {
        for(int i=0;i<2;++i)if(cache[i].piece==piece){cache[i].age=++clock;return &cache[i];}
        int slot=cache[0].piece<0?0:cache[1].piece<0?1:cache[0].age<cache[1].age?0:1;
        Cache &item=cache[slot];item.piece=-1;phase=4;
        const int attempts=(std::min)(16,(int)peers.size()+1);
        for(int attempt=0;attempt<attempts&&!quit;++attempt) {
            if(peerIndex<0) {
                if(peers.empty())break;
                int candidate=nextPeer++%(int)peers.size();
                if(!ConnectPeer(candidate))continue;
            }
            if((!choked||Interested())&&Download(piece,item.bytes)) {
                item.piece=piece;item.age=++clock;phase=5;return &item;
            }
            Close(peer);peerIndex=-1;
        }
        Fail(quit?"Torrent cancelado.":"Peers indisponiveis ou parte do torrent sem verificacao valida.");
        return NULL;
    }
    bool Magnet(const std::string &magnet) {
        if(magnet.find("magnet:?")!=0||magnet.size()>16384)return Fail("Magnet invalido.");
        size_t at=8;int params=0;
        while(at<magnet.size()&&params++<80) {
            size_t end=magnet.find('&',at);if(end==std::string::npos)end=magnet.size();
            std::string pair=magnet.substr(at,end-at);size_t equals=pair.find('=');
            if(equals!=std::string::npos) {
                std::string key=pair.substr(0,equals),value=Unescape(pair.substr(equals+1));
                if(key=="xt"&&value.find("urn:btih:")==0)hash=HashBytes(value.substr(9));
                if(key=="tr"&&trackers.size()<12&&value.size()<2048)trackers.push_back(value);
                if(key=="so") {char *stop=NULL;long n=std::strtol(value.c_str(),&stop,10);if(stop==value.c_str()||*stop||n<0||n>2047)return Fail("Indice de arquivo invalido.");selectedFile=(int)n;}
                if(key=="x.pe"&&peers.size()<64) {
                    size_t colon=value.find(':');Endpoint e;
                    if(colon!=std::string::npos&&Resolve(value.substr(0,colon),e.ip)) {
                        e.port=std::atoi(value.c_str()+colon+1);if(e.port>0&&e.port<65536)peers.push_back(e);
                    }
                }
            }
            at=end+1;
        }
        if(hash.size()!=20)return Fail("Somente magnet BitTorrent v1 e suportado.");
        AddFallbackTrackers(trackers);
        return true;
    }
};

NuvioTorrent::NuvioTorrent():m(new Impl){}
NuvioTorrent::~NuvioTorrent(){Abort();delete m;}
bool NuvioTorrent::Open(const std::string &magnet,std::string &error) {
    if(!m->Magnet(magnet)){error=m->error;return false;}
    unsigned char random[20];Random(random,20);m->peerId.assign((char*)random,20);m->peerId.replace(0,8,"-NV3601-");
    m->phase=1;
    // Collect peers from multiple trackers: the first can contain only peers
    // that use uTP, encryption or lack ut_metadata support.
    for(size_t i=0;i<m->trackers.size()&&i<8&&!m->quit&&m->peers.size()<24;++i)
        m->Tracker(m->trackers[i]);
    // Try discovered/explicit peers first; do not delay working TCP peers behind a DHT crawl.
    if(m->peers.empty()&&!m->quit)m->Dht(24);
    bool fromCache=!m->peers.empty()&&m->LoadCachedMetadata();
    bool ready=fromCache||(!m->peers.empty()&&m->Metadata(0));
    if(!ready&&!m->quit&&m->nextMetadataPeer<m->peers.size())ready=m->Metadata(m->nextMetadataPeer);
    if(!ready&&!m->quit) {
        size_t firstNew=m->peers.size();
        m->Dht(64);
        if(m->peers.size()>firstNew)ready=m->Metadata(firstNew);
    }
    if(!ready) {
        m->Fail(m->quit?"Torrent cancelado.":m->peers.empty()?
            "Nenhum peer encontrado. Verifique trackers, DHT e seeders.":
            m->metadataRejected?"Peers ainda sem metadados completos. Aguarde seeders ou escolha outro link.":
            "Peers TCP nao enviaram metadados. Tente outro link com seeders ou um link HTTP.");
        error=m->error;return false;
    }
    if(!m->FileInfo()){error=m->error;return false;}
    if(!fromCache)m->SaveCachedMetadata();
    return true;
}
int NuvioTorrent::Read(unsigned char *bytes,int count) {
    if(!bytes||count<=0||m->quit)return -1;if(m->position>=m->fileSize)return 0;
    __int64 absolute=m->fileOffset+m->position;
    int piece=(int)(absolute/m->pieceLength),within=(int)(absolute%m->pieceLength);
    Impl::Cache *cache=m->Piece(piece);if(!cache)return -1;
    int n=(int)((std::min)((__int64)count,(std::min)((__int64)cache->bytes.size()-within,m->fileSize-m->position)));
    if(n<=0)return -1;std::memcpy(bytes,&cache->bytes[within],n);m->position+=n;return n;
}
int NuvioTorrent::Seek(__int64 offset) {
    if(offset<0||offset>m->fileSize||m->quit)return -1;m->position=offset;return 0;
}
void NuvioTorrent::Abort() {
    InterlockedExchange(&m->quit,1);EnterCriticalSection(&m->lock);
    if(m->peer!=INVALID_SOCKET)shutdown(m->peer,SD_BOTH);
    if(m->control!=INVALID_SOCKET)shutdown(m->control,SD_BOTH);
    LeaveCriticalSection(&m->lock);
}
__int64 NuvioTorrent::Size()const{return m->fileSize;}
const std::string &NuvioTorrent::Filename()const{return m->name;}
const std::string &NuvioTorrent::Error()const{return m->error;}
bool NuvioTorrentClearCache() {
#ifdef _XBOX
    if(DeleteFileA("game:\\nuvio_last_torrent.meta"))return true;
    DWORD status=GetLastError();
    return status==ERROR_FILE_NOT_FOUND||status==ERROR_PATH_NOT_FOUND;
#else
    // Host tests have no Xbox game: mount or persistent torrent cache.
    return true;
#endif
}
int NuvioTorrent::StartupPercent() const {
    // Stages are coarse because verified pieces are fetched on demand, not as a full download.
    static const int progress[]={0,12,32,50,70,84,0};
    const int stage=(int)m->phase;
    return stage>=0&&stage<7?progress[stage]:0;
}
const char *NuvioTorrent::Status()const{
    static const char *status[]={"Preparando torrent...","Buscando peers/tracker/DHT...","Obtendo e verificando metadados...",
        "Torrent pronto; preparando video...","Carregando partes verificadas...","Reproduzindo torrent...","Falha no torrent."};
    int phase=(int)m->phase;return status[phase>=0&&phase<7?phase:0];
}
