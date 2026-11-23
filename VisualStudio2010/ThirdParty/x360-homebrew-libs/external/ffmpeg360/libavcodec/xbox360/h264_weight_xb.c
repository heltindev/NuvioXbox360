/* H.264 weighted prediction, XDK/VMX128 adaptation of the Xenon-safe float
 * multiplication in ffmpeg360-h264-xenon-main/libavcodec/ppc/h264_altivec.c.
 * Original FFmpeg AltiVec work: Romain Dolbeau and FFmpeg contributors.
 * Xenon-safe reference and this adaptation: LGPL-2.1-or-later.
 *
 * Unlike the reference's s16 intermediate, keep sums in exact s32-range floats
 * until shifting: extreme legal biweights/offsets must not saturate early.
 * The largest intermediate is below 2^18, exactly representable in float32.
 * Only 8-bit samples; the C implementation remains installed for other depths.
 */
#ifndef NUVIO_VMX_HOST_TEST
#include <xtl.h>
#include <vectorintrinsics.h>
#include "libavcodec/h264dsp.h"
#endif
#include <string.h>
#include "xenon_vmx_safe.h"

#define XB_LOADU(p) __vor(__lvlx((p),0),__lvrx((p),16))

static __inline __vector4 xb_splat_float(float value)
{
    __declspec(align(16)) float lanes[4];
    lanes[0]=lanes[1]=lanes[2]=lanes[3]=value;
    return __lvx(lanes,0);
}

/* Four unsigned byte samples, expanded through u16/u32 to float. */
static __inline __vector4 xb_weight4(__vector4 bytes16,__vector4 zero,
    __vector4 weight,__vector4 offset,__vector4 shift,int low)
{
    __vector4 pixels=low?__vmrglh(zero,bytes16):__vmrghh(zero,bytes16);
    return __vsraw(__vctsxs(__vmaddfp(__vcfux(pixels,0),weight,offset),0),shift);
}

static __inline __vector4 xb_biweight4(__vector4 dst16,__vector4 src16,__vector4 zero,
    __vector4 weightd,__vector4 weights,__vector4 offset,__vector4 shift,int low)
{
    __vector4 d=low?__vmrglh(zero,dst16):__vmrghh(zero,dst16);
    __vector4 s=low?__vmrglh(zero,src16):__vmrghh(zero,src16);
    __vector4 sum=__vmaddfp(__vcfux(d,0),weightd,offset);
    sum=__vmaddfp(__vcfux(s,0),weights,sum);
    return __vsraw(__vctsxs(sum,0),shift);
}

static void xb_weight(uint8_t *dst,uint8_t *src,int stride,int width,int height,
    int denom,int weightd,int weights,int offset,int bipred)
{
    const __vector4 zero=__vspltisb(0);
    const __vector4 wd=xb_splat_float((float)weightd),ws=xb_splat_float((float)weights);
    __vector4 shift,off;
    __declspec(align(16)) unsigned shifts[4];
    static const __declspec(align(16)) uint8_t mask8[16]={255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0};
    int y;
    int bias=bipred?((offset+1)|1)*(1<<denom):offset*(1<<denom)+(denom?(1<<(denom-1)):0);
    shifts[0]=shifts[1]=shifts[2]=shifts[3]=denom+bipred;
    shift=__lvx(shifts,0);off=xb_splat_float((float)bias);
    for(y=0;y<height;++y) {
        __vector4 old=XB_LOADU(dst),s=bipred?XB_LOADU(src):old;
        __vector4 d0=__vmrghb(zero,old),d1=__vmrglb(zero,old);
        __vector4 s0=__vmrghb(zero,s),s1=__vmrglb(zero,s);
        __vector4 a,b,c,d,result;
        if(bipred) {
            a=xb_biweight4(d0,s0,zero,wd,ws,off,shift,0);
            b=xb_biweight4(d0,s0,zero,wd,ws,off,shift,1);
            c=width==16?xb_biweight4(d1,s1,zero,wd,ws,off,shift,0):a;
            d=width==16?xb_biweight4(d1,s1,zero,wd,ws,off,shift,1):b;
        } else {
            a=xb_weight4(d0,zero,wd,off,shift,0);b=xb_weight4(d0,zero,wd,off,shift,1);
            c=width==16?xb_weight4(d1,zero,wd,off,shift,0):a;
            d=width==16?xb_weight4(d1,zero,wd,off,shift,1):b;
        }
        result=__vpkshus(__vpkswss(a,b),__vpkswss(c,d));
        if(width==8)result=__vsel(old,result,__lvx(mask8,0));
        __stvlx(result,dst,0);__stvrx(result,dst,16);
        dst+=stride;if(bipred)src+=stride;
    }
}

#define XB_WEIGHT(W,H) \
static void xb_weight_##W##x##H(uint8_t *p,int stride,int denom,int weight,int offset) \
{xb_weight(p,NULL,stride,W,H,denom,weight,0,offset,0);} \
static void xb_biweight_##W##x##H(uint8_t *d,uint8_t *s,int stride,int denom,int wd,int ws,int offset) \
{xb_weight(d,s,stride,W,H,denom,wd,ws,offset,1);}
XB_WEIGHT(16,16)
XB_WEIGHT(16,8)
XB_WEIGHT(8,16)
XB_WEIGHT(8,8)
XB_WEIGHT(8,4)

static h264_weight_func xb_weights[5]={xb_weight_16x16,xb_weight_16x8,xb_weight_8x16,xb_weight_8x8,xb_weight_8x4};
static h264_biweight_func xb_biweights[5]={xb_biweight_16x16,xb_biweight_16x8,xb_biweight_8x16,xb_biweight_8x8,xb_biweight_8x4};
static volatile LONG xb_weight_verdict=-2;
volatile LONG ff_xb_vmx_enabled=1;
void ff_xb_set_vmx_enabled(int enabled) {InterlockedExchange(&ff_xb_vmx_enabled,enabled?1:0);}

static int xb_test_weights(const H264DSPContext *c)
{
    enum {SIZE=64*20};
    static __declspec(align(16)) uint8_t src[SIZE],expected[SIZE],actual[SIZE];
    static const int values[8]={-128,-127,-1,0,1,64,127,128};
    static const int offsets[8]={-256,-128,-1,0,1,63,127,255};
    unsigned seed=12345;
    int pattern,shape,test,alignment,i,bad=0;
    for(pattern=0;pattern<4;++pattern)for(shape=0;shape<5;++shape)
        for(test=0;test<16;++test)for(alignment=0;alignment<4;++alignment) {
            const int start=64+alignment*3;
            const int wd=values[test&7],ws=values[(test*3+pattern)&7];
            const int offset=offsets[(test+pattern)&7],denom=test&7;
            for(i=0;i<SIZE;++i) {
                seed=seed*1664525u+1013904223u;
                src[i]=(uint8_t)(pattern==0?0:pattern==1?255:seed>>24);
                expected[i]=actual[i]=(uint8_t)(pattern==0?255:pattern==1?0:seed>>16);
            }
            c->weight_h264_pixels_tab[shape](expected+start,64,denom,wd,offset);
            xb_weights[shape](actual+start,64,denom,wd,offset);
            if(memcmp(expected,actual,SIZE))++bad;
            memcpy(expected,src,SIZE);memcpy(actual,src,SIZE);
            c->biweight_h264_pixels_tab[shape](expected+start,src+start+5,64,denom,wd,ws,offset);
            xb_biweights[shape](actual+start,src+start+5,64,denom,wd,ws,offset);
            if(memcmp(expected,actual,SIZE))++bad;
        }
    return bad;
}

int ff_xb_h264_weight_status(void) {return (int)xb_weight_verdict;}

void ff_xb_h264_weight_init(H264DSPContext *c,int bit_depth)
{
    int i;
    LONG verdict;
    if(bit_depth!=8||!ff_xb_vmx_enabled)return;
    verdict=InterlockedCompareExchange(&xb_weight_verdict,-3,-2);
    if(verdict==-2)InterlockedExchange(&xb_weight_verdict,xb_test_weights(c));
    else while(xb_weight_verdict==-3)Sleep(0);
    if(xb_weight_verdict!=0)return; /* Any mismatch keeps the original C functions. */
    for(i=0;i<5;++i) {
        c->weight_h264_pixels_tab[i]=xb_weights[i];
        c->biweight_h264_pixels_tab[i]=xb_biweights[i];
    }
}
