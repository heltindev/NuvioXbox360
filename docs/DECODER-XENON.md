# H.264 Xenon integration

## Reference analysis

The supplied reference contains 2,976 files: 155 identical to the bundled tree,
1,008 different and 1,813 absent from the old tree. This inventory is not a
manual audit of every line. Detailed review focused on documentation, H.264/VMX
kernels, threading, verification and the player. Source fingerprints are in
`xenon-reference.json`. The supplied reference directory was left unchanged.

The reference uses FFmpeg 0.10.2 with GCC/devkitxenon/libxenon and ELF. NUVIO uses
a 0.7-family port, VS2010/XDK, COFF and XEX. APIs and internal layouts differ;
mixing headers or replacing a library would break the ABI. Compatible arithmetic
and checks were adapted while preserving the existing player API.

## Implemented changes

| Reference/problem | Adaptation |
| --- | --- |
| Safe multiplication in ppc/h264_altivec.c | New xbox360/h264_weight_xb.c for weighted/biweighted 16x16, 16x8, 8x16, 8x8 and 8x4 blocks |
| Xenon VMX restrictions | xenon_vmx_safe.h rejects 22 forbidden intrinsics at compile time |
| VMX audit script | COFF, .lib and PE executable-section audit without GNU objdump |
| VMX/C comparison | Install the new kernel only after its Xbox self-test; retain C on failure |
| Qpel/chroma | Keep XDK kernels with restrictions and 8-bit guards |
| Concurrent initialization | Thread-safe kernel self-tests |
| Old luma filter | Excluded; its output vectors were passed by value and lacked comparison with C; retain C filtering |
| Duplicate threading | Exclude w32thread.c in Release Xbox; retain xb_thread.c |
| Playback failures | Report decoder/read/thread creation failures instead of treating them as EOF |
| Delayed frames | Preserve draining; verifier also checks frames remaining at EOF |

The weight kernel retains 32-bit-range intermediates before shifting instead of
premature 16-bit saturation. Relevant integers are exactly representable in
float32. The optimization applies only to 8-bit samples.

Low-latency flags were not blindly enabled: clearing has_b_frames or forcing slice
threading can break films with B-frames. The app retains two frame-thread workers.

## Development validation

- Release Xbox libraries and NUVIO built with the XDK.
- 6,000 host blocks matched C, including alignment, extreme weights/offsets,
  stride and out-of-block writes. The scalar intrinsic model does not execute Xenon.
- Deliberate arithmetic mutation was rejected and C fallback retained.
- A negative compile test rejected __vmladduhm.
- libavcodec, libavutil and the linked PE had no forbidden opcode hits. The audit
  has positive controls for all 22 prohibited opcodes.
- Two synthetic CABAC 1280x720 clips, with/without B-frames, produced 300 frames
  each with host FFmpeg 62/PyAV; 600 visible-pixel Y/U/V MD5 references were generated.
- A separate XDK verifier built against the production libraries and passed its audit.

Historical UI7 hashes and measurements remain in `validation-xenon.json`; they
do not identify the newer UI8 XEX. Current build hashes are in `release-ui8.json`.
Old qemu results from the supplied reference do not validate this XDK adaptation.

## Hardware verifier

Build the app with `VisualStudio2010/BUILD-RELEASE.cmd`; current Settings displays UI8.
For pixel comparison, build the separate verifier with
`VisualStudio2010/build_decoder_verify.ps1` after building the decoder. It needs
PyAV and the original reference clips on the development PC.

Copy the entire `VisualStudio2010/Release/DecoderVerify` directory to Xbox and run
`NUVIO-DECODER-VERIFY.xex`. It needs no account/network and may show no picture
while decoding. Read `xenon-decode-results.txt` in that folder. Expected: eight
PASS lines (C/VMX, 1/2 threads, two clips), 300 frames per clip and TOTAL failures=0.
The chroma/qpel/weight fields report hardware self-test status; zero enables VMX.

Host checks: `VisualStudio2010/run_vmx_tests.bat` and
`python tools/test_audit_xenon_vmx.py`.

## Limits and open failure

The adapted decoder has not passed the current hardware verifier here. Pixel
equality, actual VMX use, long-run stability and performance require console
results. No FPS improvement is promised.

**The maintainer still reports video-loading failure.** Integration and host tests
do not fix or explain that failure; see [issue #4](https://github.com/heltindev/NuvioXbox360/issues/4).
Metadata retrieval precedes decoding and still needs available compatible peers.

This H.264 reference adds no HEVC, AV1, DRM, 1080p or torrent protocols. The native
1280x720 limit remains. LGPL notices and COPYING.LGPL files are preserved under
FFmpeg; the combined executable retains the existing GPL notices.
