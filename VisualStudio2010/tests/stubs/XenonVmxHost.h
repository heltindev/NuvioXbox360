// Independent scalar model for testing lane ordering/math; not Xbox instruction emulation.
#pragma once
#include <windows.h>
#include <cstring>
typedef unsigned char uint8_t;
static bool xb_host_corrupt=false; // Mutation control, used only by the host test.
struct __vector4 {unsigned char b[16];};
inline __vector4 __lvx(const void *p,int at) {__vector4 v;std::memcpy(v.b,(const char*)p+at,16);return v;}
inline __vector4 __lvlx(const void *p,int at) {
    const unsigned char *q=(const unsigned char*)p+at;__vector4 v={0};
    std::memcpy(v.b,q,16-((size_t)q&15));return v;
}
inline __vector4 __lvrx(const void *p,int at) {
    const unsigned char *q=(const unsigned char*)p+at;__vector4 v={0};int n=(int)((size_t)q&15);
    if(n)std::memcpy(v.b+16-n,q-n,n);return v;
}
inline void __stvlx(__vector4 v,void *p,int at) {
    unsigned char *q=(unsigned char*)p+at;std::memcpy(q,v.b,16-((size_t)q&15));
}
inline void __stvrx(__vector4 v,void *p,int at) {
    unsigned char *q=(unsigned char*)p+at;int n=(int)((size_t)q&15);
    if(n)std::memcpy(q-n,v.b+16-n,n);
}
inline __vector4 __vor(__vector4 a,__vector4 b) {for(int i=0;i<16;++i)a.b[i]|=b.b[i];return a;}
inline __vector4 __vspltisb(int n) {__vector4 v;std::memset(v.b,(unsigned char)n,16);return v;}
inline __vector4 xb_merge(__vector4 a,__vector4 b,int bytes,int low) {
    __vector4 v;int begin=low?8:0;
    for(int i=0;i<8/bytes;++i) {
        std::memcpy(v.b+i*bytes*2,a.b+begin+i*bytes,bytes);
        std::memcpy(v.b+i*bytes*2+bytes,b.b+begin+i*bytes,bytes);
    }return v;
}
inline __vector4 __vmrghb(__vector4 a,__vector4 b){return xb_merge(a,b,1,0);}
inline __vector4 __vmrglb(__vector4 a,__vector4 b){return xb_merge(a,b,1,1);}
inline __vector4 __vmrghh(__vector4 a,__vector4 b){return xb_merge(a,b,2,0);}
inline __vector4 __vmrglh(__vector4 a,__vector4 b){return xb_merge(a,b,2,1);}
inline float xb_float(__vector4 v,int lane){float n;std::memcpy(&n,v.b+lane*4,4);return n;}
inline int xb_int(__vector4 v,int lane){int n;std::memcpy(&n,v.b+lane*4,4);return n;}
inline void xb_float_at(__vector4 &v,int lane,float n){std::memcpy(v.b+lane*4,&n,4);}
inline void xb_int_at(__vector4 &v,int lane,int n){std::memcpy(v.b+lane*4,&n,4);}
inline __vector4 __vcfux(__vector4 v,int scale) {
    (void)scale;__vector4 out;
    for(int i=0;i<4;++i) {
        const unsigned char *b=v.b+i*4;
        unsigned n=(unsigned)b[0]<<24|(unsigned)b[1]<<16|(unsigned)b[2]<<8|b[3];
        xb_float_at(out,i,(float)n);
    }return out;
}
inline __vector4 __vmaddfp(__vector4 a,__vector4 b,__vector4 c) {
    __vector4 out;for(int i=0;i<4;++i)xb_float_at(out,i,xb_float(a,i)*xb_float(b,i)+xb_float(c,i)+(xb_host_corrupt?1.0f:0.0f));return out;
}
inline __vector4 __vctsxs(__vector4 v,int scale) {
    (void)scale;__vector4 out;for(int i=0;i<4;++i)xb_int_at(out,i,(int)xb_float(v,i));return out;
}
inline __vector4 __vsraw(__vector4 v,__vector4 shift) {
    __vector4 out;for(int i=0;i<4;++i)xb_int_at(out,i,xb_int(v,i)>>(xb_int(shift,i)&31));return out;
}
inline __vector4 __vpkswss(__vector4 a,__vector4 b) {
    __vector4 out;
    for(int i=0;i<8;++i) {
        int n=xb_int(i<4?a:b,i&3);if(n>32767)n=32767;if(n< -32768)n=-32768;
        out.b[2*i]=(unsigned char)((unsigned)n>>8);out.b[2*i+1]=(unsigned char)n;
    }return out;
}
inline __vector4 __vpkshus(__vector4 a,__vector4 b) {
    __vector4 out;
    for(int i=0;i<16;++i) {
        const __vector4 &v=i<8?a:b;int at=(i&7)*2;
        int n=(short)((unsigned)v.b[at]<<8|v.b[at+1]);if(n<0)n=0;if(n>255)n=255;
        out.b[i]=(unsigned char)n;
    }return out;
}
inline __vector4 __vsel(__vector4 a,__vector4 b,__vector4 mask) {
    for(int i=0;i<16;++i)a.b[i]=(a.b[i]&~mask.b[i])|(b.b[i]&mask.b[i]);return a;
}
typedef void (*h264_weight_func)(uint8_t*,int,int,int,int);
typedef void (*h264_biweight_func)(uint8_t*,uint8_t*,int,int,int,int,int);
struct H264DSPContext {h264_weight_func weight_h264_pixels_tab[10];h264_biweight_func biweight_h264_pixels_tab[10];};
