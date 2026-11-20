# x360 homebrew libs

Three small libraries for Xbox 360 homebrew (XDK), taken from
[Multiplex 360](https://github.com/HG-ERIK/multiplex360) and
[Simple 360 Player](https://github.com/HG-ERIK/simple360player), where they run every day on RGH consoles.

| Folder | What it does |
|---|---|
| `https/` | HTTP and HTTPS client. TLS 1.2 with real certificate checks (mbedTLS), 32 common root CAs built in, chunked and ranged downloads, streaming reads. |
| `video/` | Fast FFmpeg video decoder: H.264 with frame threading on 4 hardware threads and VMX (AltiVec) motion compensation, plus Xvid, MPEG-2, WMV/VC-1 and the usual audio codecs. A small C API, so you never include FFmpeg headers. |
| `guard/` | Crash catcher. A crash on any thread, or a main loop stuck for 30 s, is logged and the console goes back to the dashboard instead of freezing. |
| `log/` | The log the others write to: debug output, a file next to your xex, and optionally UDP to your PC. |

## Building

Needs the Xbox 360 XDK (2.0.21256) and Visual Studio 2010 SP1.

```
.\build.ps1
```

The first run also builds FFmpeg (about 10 minutes). Libraries end up in `build\Release\lib`,
the example in `build\Release\example\example.xex`. To use a library, add its folder to your include
path and link its `.lib`:

| Library | Link | Also link |
|---|---|---|
| https | `https.lib`, `mbedtls.lib`, `log.lib` | `xnet.lib` |
| video | `video.lib`, `log.lib`, `guard.lib` (its threads run under the crash catcher) | the FFmpeg libs: `libavformat`, `libavcodec`, `libswscale`, `libavutil`, `zlib` from `external\ffmpeg360\vcproj\...\Xbox 360\Release` |
| guard | `guard.lib`, `log.lib` | |

`example\` uses all three; `example\example.vcxproj` is a working project setup to copy from.

## Using them

**Crash catcher.** Start it once, beat once per frame, and run your main loop under its filter:

```cpp
Log::Open( "game:\\mylog.txt" );
Guard_Start();
__try
{
    for( ;; ) { Guard_Beat(); /* frame */ }
}
__except( Guard_Filter( GetExceptionCode(), GetExceptionInformation(), "main" ) ) {}
```

Create threads with `Guard_CreateThread` instead of `CreateThread` so they are covered too.
Useful because xbdm keeps a crashed title frozen on screen, and DashLaunch's crash reboot doesn't
fire while xbdm is loaded.

**HTTPS.**

```cpp
std::string error, ip;
Http::Startup( error, ip );     // network, DHCP and TLS; once
Http::Response r = Http::Get( "https://example.com/data.json", "Accept: application/json\r\n" );
// r.status, r.body; r.error says why if status == 0

Http::Stream s;                 // big downloads, read as they arrive
s.Open( "https://example.com/big.bin", "", 0 /* start offset */, error );
char buf[65536];
int n;
while( ( n = s.Read( buf, sizeof( buf ) ) ) > 0 ) { /* ... */ }
```

Certificates are checked against the roots in `https/CaCerts.inc` and the host name. If the console's
clock was never set (year before 2025), only the certificate dates are skipped.
TLS is 1.2 only, with ECDHE key exchange. One Xenon thread manages about 8 Mbit/s of HTTPS, so use
plain http for big transfers on your own network.

**Video decoder.** Give it a byte source (file, HTTP stream, memory), then read packets and decode:

```cpp
FgSource src = { file, MyRead, MySeek, fileSize };
fg_init( MyLogFunction );
FgMedia* m = fg_open( &src, "movie.mkv", error, sizeof( error ) );
FgPacket* pkt;
int type;
while( ( type = fg_read_packet( m, &pkt ) ) > 0 )
{
    FgPicture pic;
    if( type == FG_PACKET_VIDEO && fg_decode_video( m, pkt, &pic ) > 0 )
        ;   // pic.plane[0..2] = Y, U, V (4:2:0), pic.pts in seconds
    fg_free_packet( pkt );
}
fg_close( m );
```

`video/ffglue.h` also has audio decoding (16-bit PCM), seeking, track lists, text subtitles and a
catch-up switch (`fg_set_fast`) for when decoding falls behind. Each VMX routine only switches on after
a bit-exact self-test against the plain C code (`fg_vmx_status`, `fg_vmx_qpel_status`).

Realistic limits: 720p H.264 plays in real time; 1080p H.264 does not (about 8-9 fps; the entropy
decoding can't be moved to the GPU).

## Licences

- Our code (`log`, `guard`, `https`, `video/ffglue.*`, `example`): MIT, see `LICENSE`.
- mbedTLS 2.16 (`external/mbedtls`): Apache-2.0.
- FFmpeg 0.7 from the FFPlay360 port (`external/ffmpeg360`): built with its GPL parts, so anything
  linking `video` is GPL v2 or later.

Not affiliated with Microsoft.
