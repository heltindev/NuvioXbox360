/* This compile MUST fail: verifies the Xenon forbidden-intrinsic gate. */
#include "../ThirdParty/x360-homebrew-libs/external/ffmpeg360/libavcodec/xbox360/xenon_vmx_safe.h"
int test_forbidden(void) {return __vmladduhm(0,0,0);}
