// On-console bit-exact test of the production FFmpeg glue, C vs guarded VMX.
// Reads local synthetic H.264 clips and desktop-generated visible-plane MD5s.
// No accounts, HTTP or torrent activity. Results: game:\xenon-decode-results.txt.
#define __STDC_CONSTANT_MACROS
#include <xtl.h>
#include <cstdio>
#include <cstring>
#include "ffglue.h"
extern "C" {
#include "libavutil/md5.h"
#include "libavutil/mem.h"
}

static int ReadFileBytes(void *opaque,unsigned char *bytes,int count)
{return (int)std::fread(bytes,1,count,(std::FILE*)opaque);}
static int SeekFile(void *opaque,__int64 at)
{return at<0||at>0x7fffffff?-1:std::fseek((std::FILE*)opaque,(long)at,SEEK_SET);}

static void Digest(const FgPicture &pic,char hex[33])
{
    AVMD5 *md5=(AVMD5*)av_malloc(get_av_md5_size());
    unsigned char digest[16];
    av_md5_init(md5);
    for(int p=0;p<3;++p) {
        int w=p?(pic.width+1)/2:pic.width,h=p?(pic.height+1)/2:pic.height;
        for(int y=0;y<h;++y)av_md5_update(md5,pic.plane[p]+y*pic.pitch[p],w);
    }
    av_md5_final(md5,digest);av_free(md5);
    for(int i=0;i<16;++i)std::sprintf(hex+i*2,"%02x",digest[i]);
}

static bool CheckFrame(const FgPicture &pic,std::FILE *expected,std::FILE *log,int number)
{
    char actual[33],wanted[33];Digest(pic,actual);
    if(std::fscanf(expected,"%32s",wanted)!=1||std::strcmp(actual,wanted)) {
        std::fprintf(log,"FAIL frame %d actual=%s\n",number,actual);return false;
    }
    return true;
}

static bool Run(const char *name,int threads,int vmx,std::FILE *log)
{
    char path[220],error[128]="";
    std::sprintf(path,"game:\\%s.h264",name);
    std::FILE *file=std::fopen(path,"rb");
    std::sprintf(path,"game:\\%s.expected.md5",name);
    std::FILE *expected=std::fopen(path,"rb");
    if(!file||!expected) {
        std::fprintf(log,"FAIL missing local fixture %s\n",name);
        if(file)std::fclose(file);if(expected)std::fclose(expected);return false;
    }
    std::fseek(file,0,SEEK_END);long size=std::ftell(file);std::rewind(file);
    FgSource source;source.opaque=file;source.read=ReadFileBytes;source.seek=SeekFile;source.size=size;
    fg_set_decode_threads(threads);fg_set_vmx_enabled(vmx);
    FgMedia *media=fg_open(&source,name,error,sizeof(error));
    if(!media) {
        std::fprintf(log,"FAIL open %s: %s\n",name,error);
        std::fclose(file);std::fclose(expected);return false;
    }
    int frames=0,packets=0,drained=0;bool ok=true;
    DWORD started=GetTickCount();
    while(ok) {
        if(GetTickCount()-started>120000){std::fprintf(log,"FAIL timeout\n");ok=false;break;}
        FgPacket *packet=NULL;int type=fg_read_packet(media,&packet);
        if(type<=0){fg_free_packet(packet);if(type<0)ok=false;break;}
        if(type==FG_PACKET_VIDEO) {
            ++packets;FgPicture pic;int got=fg_decode_video(media,packet,&pic);
            if(got<0)ok=false;
            else if(got>0)ok=CheckFrame(pic,expected,log,frames++);
        }
        fg_free_packet(packet);
    }
    // Drain delayed B-frames and frame-thread buffers separately from demux EOF.
    while(ok) {
        FgPicture pic;int got=fg_decode_video(media,NULL,&pic);
        if(got<0){ok=false;break;}if(!got)break;
        ++drained;ok=CheckFrame(pic,expected,log,frames++);
        if(drained>32){ok=false;break;}
    }
    char extra[33];
    if(std::fscanf(expected,"%32s",extra)==1||frames!=300)ok=false;
    std::fprintf(log,"%s %s threads=%d vmx=%d frames=%d packets=%d drained=%d chroma=%d qpel=%d weight=%d ms=%lu\n",
        ok?"PASS":"FAIL",name,threads,vmx,frames,packets,drained,fg_vmx_status(),fg_vmx_qpel_status(),fg_vmx_weight_status(),GetTickCount()-started);
    std::fflush(log);fg_close(media);std::fclose(file);std::fclose(expected);return ok;
}

int main()
{
    std::FILE *log=std::fopen("game:\\xenon-decode-results.txt","wb");
    if(!log)return 1;fg_init(NULL);int failures=0;
    const char *names[]={"main-cabac-bf0","main-cabac-bf3"};
    for(int vmx=0;vmx<2;++vmx)for(int threads=1;threads<=2;++threads)for(int i=0;i<2;++i)
        if(!Run(names[i],threads,vmx,log))++failures;
    std::fprintf(log,"TOTAL failures=%d\n",failures);std::fclose(log);
    return failures?2:0;
}
