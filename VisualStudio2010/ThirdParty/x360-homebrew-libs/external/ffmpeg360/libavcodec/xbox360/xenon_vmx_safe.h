/* Xenon VMX128 safety gate, following ffmpeg360-h264-xenon's xenon_vmx.h.
 * LGPL-2.1-or-later, same terms as the FFmpeg files using this header.
 * Keep generic HAVE_ALTIVEC disabled for XDK; use only audited XDK intrinsics. */
#ifndef NUVIO_XENON_VMX_SAFE_H
#define NUVIO_XENON_VMX_SAFE_H
#define XENON_VMX_SAFE 1
/* A use must fail at compile time, rather than emit a trapping instruction. */
#define __vmhaddshs(...) NUVIO_FORBIDDEN_XENON_vmhaddshs
#define __vmhraddshs(...) NUVIO_FORBIDDEN_XENON_vmhraddshs
#define __vmladduhm(...) NUVIO_FORBIDDEN_XENON_vmladduhm
#define __vmsumubm(...) NUVIO_FORBIDDEN_XENON_vmsumubm
#define __vmsummbm(...) NUVIO_FORBIDDEN_XENON_vmsummbm
#define __vmsumuhm(...) NUVIO_FORBIDDEN_XENON_vmsumuhm
#define __vmsumuhs(...) NUVIO_FORBIDDEN_XENON_vmsumuhs
#define __vmsumshm(...) NUVIO_FORBIDDEN_XENON_vmsumshm
#define __vmsumshs(...) NUVIO_FORBIDDEN_XENON_vmsumshs
#define __vmuloub(...) NUVIO_FORBIDDEN_XENON_vmuloub
#define __vmulouh(...) NUVIO_FORBIDDEN_XENON_vmulouh
#define __vmulosb(...) NUVIO_FORBIDDEN_XENON_vmulosb
#define __vmulosh(...) NUVIO_FORBIDDEN_XENON_vmulosh
#define __vmuleub(...) NUVIO_FORBIDDEN_XENON_vmuleub
#define __vmuleuh(...) NUVIO_FORBIDDEN_XENON_vmuleuh
#define __vmulesb(...) NUVIO_FORBIDDEN_XENON_vmulesb
#define __vmulesh(...) NUVIO_FORBIDDEN_XENON_vmulesh
#define __vsum4ubs(...) NUVIO_FORBIDDEN_XENON_vsum4ubs
#define __vsum4sbs(...) NUVIO_FORBIDDEN_XENON_vsum4sbs
#define __vsum4shs(...) NUVIO_FORBIDDEN_XENON_vsum4shs
#define __vsum2sws(...) NUVIO_FORBIDDEN_XENON_vsum2sws
#define __vsumsws(...) NUVIO_FORBIDDEN_XENON_vsumsws
#endif
