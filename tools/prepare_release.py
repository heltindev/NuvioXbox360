"""Build clean release archives and a source publication manifest; never include local credentials."""
import base64
import hashlib
import json
import os
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = 'v0.1.0-alpha.2'
EXCLUDED = {'.git', '.vs', 'bin', 'obj', 'build', 'Debug', 'Release', '__pycache__', 'dist', 'ffmpeg360-h264-xenon-main'}
BINARY_BUILD = {'.obj', '.lib', '.exe', '.dll', '.pdb', '.idb', '.ilk', '.xdb', '.tlog', '.pyc', '.log', '.xex'}
PRIVATE_FILES = {'nuvio.cfg', 'nuvio_session.dat', 'nuvio_session.dat.tmp',
                 'nuvio_home.cache', 'nuvio_search.cache', 'nuvio_last_torrent.meta'}

def source_files():
    for file in ROOT.rglob('*'):
        relative = file.relative_to(ROOT)
        if not file.is_file() or any(part in EXCLUDED for part in relative.parts):
            continue
        if file.suffix.lower() in BINARY_BUILD or file.name.lower() in PRIVATE_FILES or file.suffix.lower() in ('.cache', '.dat', '.meta'):
            continue
        if file.suffix == '.local' or file.suffix in ('.user','.suo','.sdf','.opensdf'):
            continue
        yield file

def publish(file):
    relative = file.relative_to(ROOT).as_posix()
    if '/ThirdParty/x360-homebrew-libs/' not in relative:
        return True
    vendor = relative.split('/ThirdParty/x360-homebrew-libs/')[1]
    if vendor.startswith('example/'):
        return False
    xenon_patches = {
        'external/ffmpeg360/libavcodec/h264dsp.c',
        'external/ffmpeg360/libavcodec/dsputil.c',
        'external/ffmpeg360/libavcodec/xbox360/h264_chroma_xb.c',
        'external/ffmpeg360/libavcodec/xbox360/h264_qpel_xb.c',
        'external/ffmpeg360/libavcodec/xbox360/h264_weight_xb.c',
        'external/ffmpeg360/libavcodec/xbox360/xenon_vmx_safe.h',
        'external/ffmpeg360/NOTICE-XENON.md',
        'external/ffmpeg360/COPYING.LGPLv2.1',
        'external/ffmpeg360/COPYING.LGPLv3',
    }
    return (vendor.split('/')[0] in ('https','video','guard','log') or
            '/' not in vendor or file.suffix in ('.vcxproj','.props') or
            vendor in xenon_patches or vendor in ('external/mbedtls/LICENSE','external/ffmpeg360/COPYING.GPLv2',
                       'external/ffmpeg360/COPYING.GPLv3','external/ffmpeg360/LICENSE'))

def main():
    if len(sys.argv)>1 and sys.argv[1]=='--manifest-chunk':
        text=(Path(os.environ['TEMP'])/'nuvio-publish-manifest.json').read_text()
        start=int(sys.argv[2]);print(json.dumps(text[start:start+24576],ensure_ascii=True));return
    dist=ROOT/'dist'
    dist.mkdir(exist_ok=True)
    files=list(source_files())
    with zipfile.ZipFile(dist/f'NUVIO-{VERSION}-source.zip','w',zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for file in files:
            archive.write(file, file.relative_to(ROOT).as_posix())
    xex=ROOT/'VisualStudio2010/Release/NUVIO.xex'
    assert xex.read_bytes()[:4]==b'XEX2'
    with zipfile.ZipFile(dist/f'NUVIO-{VERSION}-Xbox360.zip','w',zipfile.ZIP_DEFLATED) as archive:
        archive.write(xex,'NUVIO.xex')
        for name in ['nuvio.example.cfg','INSTALL-ON-XBOX.txt','docs/RELEASE_NOTES.md',
                     'docs/KNOWN_ISSUES.md','ROADMAP.md','NOTICE.md','LICENSE',
                     'VisualStudio2010/ThirdParty/simple360player/COPYING']:
            archive.write(ROOT/name,name)
    entries=[]
    for file in files:
        if not publish(file):
            continue
        raw=file.read_bytes()
        try:
            content=raw.decode('utf-8')
            entries.append({'path':file.relative_to(ROOT).as_posix(),'mode':'100644','type':'blob','content':content})
        except UnicodeDecodeError:
            # Keep original binary/legacy-encoded source bytes via the Git blob API.
            entries.append({'path':file.relative_to(ROOT).as_posix(),'encoding':'base64',
                            'content':base64.b64encode(raw).decode()})
    manifest=Path(os.environ['TEMP'])/'nuvio-publish-manifest.json'
    data=json.dumps(entries,ensure_ascii=True)
    manifest.write_text(data)
    checksums=[]
    for file in [xex]+list(dist.glob('NUVIO-'+VERSION+'-*.zip')):
        checksums.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.name)
    (dist/'SHA256SUMS.txt').write_text('\n'.join(checksums)+'\n')
    print(json.dumps({'source_files':len(files),'publish_files':len(entries),'manifest_chars':len(data),
                      'checksums':checksums,'packages':[f.name for f in dist.iterdir()]}))

if __name__=='__main__':
    main()
