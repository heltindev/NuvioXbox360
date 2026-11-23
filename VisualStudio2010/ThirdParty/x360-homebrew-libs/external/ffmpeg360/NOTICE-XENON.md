# Xenon H.264 reference integration

The NUVIO XDK decoder retains its existing FFmpeg version and ABI. The weighted-prediction arithmetic and 22-opcode safety classifier were adapted from the user-supplied ffmpeg360-h264-xenon-main reference (FFmpeg 0.10.2). See docs/DECODER-XENON.md and docs/xenon-reference.json in the NUVIO source tree for mapping and source fingerprints.

The new XDK weight implementation retains 32-bit-range intermediates rather than copying the reference s16 saturation behavior. Generic GCC AltiVec code was not enabled. The original project's license terms below continue to apply to the adapted work. Existing FFmpeg LGPL/GPL notices remain unchanged.

--- Original reference notice ---

# NOTICE — license and attribution

## FFmpeg

This repository contains the source of **FFmpeg 0.10.2** with modifications. FFmpeg
is free software licensed under the **GNU Lesser General Public License version 2.1
or later** (LGPL v2.1+); some optional components are GPL. The complete, unmodified
license texts ship with the tree:

- `COPYING.LGPLv2.1`, `COPYING.LGPLv3` — the LGPL
- `COPYING.GPLv2`, `COPYING.GPLv3` — the GPL
- `LICENSE` — FFmpeg's summary of which parts are under which license
- `CREDITS` — upstream authors

Upstream project: https://ffmpeg.org/ — the original 0.10.2 release can be obtained
from the FFmpeg git history (tag `n0.10.2`).

## Modifications in this tree

The changes under `libavcodec/ppc/` (the Xenon-safe AltiVec rewrite), the
`pthread.c` frame-threading edits, `libavcodec/xc_prof.h`, and the `verify/`
tooling are distributed under the **same license as the file they modify** (LGPL
v2.1+ for the libavcodec/libavutil sources). They carry no additional restrictions.

A single consolidated source diff is also kept separately as
`the source patch` in the project this was extracted from; here the changes are
already applied to the tree.

## Attribution for the Xenon instruction-subset work

The list of AltiVec instructions that the Xbox 360 Xenon VMX128 does **not**
implement, and the decision to rewrite rather than emulate them, rests on three
independent public sources that agree:

- **Free60 project** — `gcc-9.2.0.diff`, which disables the same instruction
  patterns with `TARGET_ALTIVEC && 0`.
- **Xenia** Xbox 360 emulator — marks the same instructions `XEINSTRNOTIMPLEMENTED`.
- **G. Bobic**, gcc@gcc.gnu.org, 2007-08-21 — enumerated the missing instructions.

The unofficial **Wolf3s** `ffmpeg-xbox` fork is referenced as prior art for an
AltiVec H.264 path on the 360; it is **not** used or included here.

## No warranty

Provided as-is, with no warranty, under the terms of the LGPL/GPL as applicable.
See *What is and isn't verified* in `README.md` — notably, this has not yet been
run on real Xbox 360 hardware.
