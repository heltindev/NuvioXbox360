# Licenses and provenance

Original NUVIO source retains the existing Apache-2.0 LICENSE. The combined player
executable must be distributed under GPL-3.0-compatible terms with corresponding
source because it contains FFPlayer GPL-3.0 and FFmpeg GPL-2.0-or-later components.
The complete GPL-3.0 text is in VisualStudio2010/ThirdParty/simple360player/COPYING.

FFPlayer.cpp/.h originate from HG-ERIK/simple360player, revision
5d235c1fd24c098ccc1b2463c1b4265bf2e7844c. Local changes include resolution limits,
byte-bounded queues, smaller buffers, two decoder workers, native torrent byte
input and explicit decode/read/thread failure reporting.

FFmpeg/video/guard originate from HG-ERIK/x360-homebrew-libs, revision
43a3969ec2a5103218a4d7120cceac7ea494e846. Source and license notices are under
ThirdParty/x360-homebrew-libs. ffglue adds packet size checks, queue accounting
and VMX control/status functions.

Other components retain their notices: mbedTLS (Apache-2.0), picojson (BSD),
stb and the QR encoder (MIT/public domain as specified in their files).
Proprietary Xbox XDK headers, tools, libraries and installers are not redistributed.

NuvioTorrent.cpp/.h independently implement public BEP 3/5/9/10/15 protocols.
tools/torrent_fixture.py uses Python's standard library and an authorized open
test sample; it is not a production media service.

The Xenon H.264 work adapts weighted-prediction arithmetic and opcode auditing
from the supplied ffmpeg360-h264-xenon-main reference (FFmpeg 0.10.2), under
LGPL-2.1-or-later. GCC sources do not replace the existing XDK ABI. See
docs/DECODER-XENON.md and
VisualStudio2010/ThirdParty/x360-homebrew-libs/external/ffmpeg360/NOTICE-XENON.md.

The NuvioMedia projects are protocol/design references. This is an independent,
unofficial client with no Microsoft or NuvioMedia affiliation.
