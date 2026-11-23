#include "stubs/XenonVmxHost.h"
#include <cassert>
#include <cstdio>
#define NUVIO_VMX_HOST_TEST
#include "../ThirdParty/x360-homebrew-libs/external/ffmpeg360/libavcodec/xbox360/h264_weight_xb.c"

static int clip(int n){return n<0?0:n>255?255:n;}
static void scalar(uint8_t *d,uint8_t *s,int stride,int w,int h,int denom,int wd,int ws,int offset,int bi) {
    int bias=bi?((offset+1)|1)*(1<<denom):offset*(1<<denom)+(denom?(1<<(denom-1)):0);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x) {
        int at=y*stride+x;
        d[at]=(uint8_t)clip((d[at]*wd+(bi?s[at]*ws:0)+bias)>>(denom+bi));
    }
}
#define C_WEIGHT(W,H) \
static void c_##W##x##H(uint8_t *d,int stride,int den,int weight,int off){scalar(d,NULL,stride,W,H,den,weight,0,off,0);} \
static void cb_##W##x##H(uint8_t *d,uint8_t *s,int stride,int den,int wd,int ws,int off){scalar(d,s,stride,W,H,den,wd,ws,off,1);}
C_WEIGHT(16,16) C_WEIGHT(16,8) C_WEIGHT(8,16) C_WEIGHT(8,8) C_WEIGHT(8,4)
int main(int argc,char **argv) {
    H264DSPContext c={0};
    const h264_weight_func original[5]={c_16x16,c_16x8,c_8x16,c_8x8,c_8x4};
    const h264_biweight_func bioriginal[5]={cb_16x16,cb_16x8,cb_8x16,cb_8x8,cb_8x4};
    for(int i=0;i<5;++i){c.weight_h264_pixels_tab[i]=original[i];c.biweight_h264_pixels_tab[i]=bioriginal[i];}
    if(argc>1&&!std::strcmp(argv[1],"--mismatch")) {
        xb_host_corrupt=true;
        ff_xb_h264_weight_init(&c,8);
        assert(ff_xb_h264_weight_status()>0&&c.weight_h264_pixels_tab[0]==original[0]);
        std::puts("PASS: deliberately corrupted vector arithmetic rejected; C fallback preserved");return 0;
    }
    ff_xb_h264_weight_init(&c,10);
    assert(ff_xb_h264_weight_status()==-2&&c.weight_h264_pixels_tab[0]==original[0]);
    ff_xb_h264_weight_init(&c,8);
    assert(ff_xb_h264_weight_status()==0&&c.weight_h264_pixels_tab[0]!=original[0]);
    unsigned seed=71431;
    uint8_t src[2048],actual[2048],expected[2048];
    for(int test=0;test<6000;++test) {
        int shape=test%5,denom=test%8,alignment=test%16;
        seed=seed*1664525u+1013904223u;int wd=(int)(seed%257)-128;
        seed=seed*1664525u+1013904223u;int ws=(int)(seed%257)-128;
        seed=seed*1664525u+1013904223u;int offset=(int)(seed%512)-256;
        for(int i=0;i<2048;++i) {
            seed=seed*1664525u+1013904223u;
            src[i]=(uint8_t)(seed>>24);expected[i]=actual[i]=(uint8_t)(seed>>16);
        }
        const int stride=48+(test%3)*16,start=64+alignment;
        if(test&1) {
            bioriginal[shape](expected+start,src+start+3,stride,denom,wd,ws,offset);
            c.biweight_h264_pixels_tab[shape](actual+start,src+start+3,stride,denom,wd,ws,offset);
        } else {
            original[shape](expected+start,stride,denom,wd,offset);
            c.weight_h264_pixels_tab[shape](actual+start,stride,denom,wd,offset);
        }
        assert(std::memcmp(actual,expected,sizeof(actual))==0);
    }
    std::puts("PASS: 6000 weighted/biweighted blocks, unaligned stores, guard bytes, 8-bit activation and 10-bit C fallback (scalar intrinsic model)");
    ff_xb_set_vmx_enabled(0);
    for(int i=0;i<5;++i)c.weight_h264_pixels_tab[i]=original[i];
    ff_xb_h264_weight_init(&c,8);
    assert(c.weight_h264_pixels_tab[0]==original[0]);
    ff_xb_set_vmx_enabled(1);
}
