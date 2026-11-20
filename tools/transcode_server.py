"""Authenticated conversion worker for Nuvio Xbox 360 (Python 3.10+, FFmpeg).

Converts to a complete seekable MP4 before playback. One job at a time; disk
cache is bounded. No shell commands, local-file inputs, or URLs in request logs.
"""
import argparse
import concurrent.futures
import hashlib
import hmac
import http.server
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import threading
import time
from urllib.parse import parse_qs, quote, urlsplit

HIDDEN = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}

def http_url(url):
    value = urlsplit(url)
    if value.scheme not in ('http', 'https') or not value.hostname or value.username or value.password:
        raise ValueError('Only HTTP/HTTPS media sources are supported')
    if any(c in url for c in '\r\n\x00') or len(url) > 16384:
        raise ValueError('Invalid source URL')
    return url

def provider_headers(raw):
    if not isinstance(raw, str) or len(raw) > 8192:
        raise ValueError('Invalid provider headers')
    for line in raw.split('\r\n'):
        if not line:
            continue
        name, separator, value = line.partition(':')
        if not separator or not re.fullmatch('[A-Za-z0-9-]{1,64}', name) or '\n' in value or '\r' in value:
            raise ValueError('Invalid provider headers')
        if name.lower() in ('host', 'range', 'connection', 'content-length'):
            raise ValueError('Conflicting provider header')
    return raw

def conversion_args(ffmpeg, url, headers, output, hdr=False):
    filters = []
    if hdr:
        filters += ['zscale=t=linear:npl=100', 'format=gbrpf32le', 'tonemap=hable:desat=0.5',
                    'zscale=p=bt709:t=bt709:m=bt709:r=limited']
    filters += ["scale=w='min(1280,iw)':h='min(720,ih)':force_original_aspect_ratio=decrease:force_divisible_by=2",
                'setsar=1', 'fps=24', 'format=yuv420p']
    command = [ffmpeg, '-hide_banner', '-nostdin', '-v', 'error', '-y',
               '-protocol_whitelist', 'http,https,tcp,tls,crypto', '-rw_timeout', '15000000']
    if headers:
        command += ['-headers', headers]
    return command + ['-i', url, '-map', '0:v:0', '-map', '0:a:0?', '-sn', '-dn',
        '-vf', ','.join(filters), '-c:v', 'libx264', '-preset', 'veryfast', '-crf', '23',
        '-profile:v', 'main', '-level:v', '3.1', '-refs', '2', '-bf', '2', '-g', '48',
        '-maxrate', '2500k', '-bufsize', '5000k', '-threads', '4',
        '-c:a', 'aac', '-ac', '2', '-ar', '48000', '-b:a', '128k',
        '-af', 'aresample=async=1:first_pts=0', '-color_primaries', 'bt709',
        '-color_trc', 'bt709', '-colorspace', 'bt709', '-movflags', '+faststart',
        '-max_muxing_queue_size', '1024', '-fs', str(4*1024**3), '-f', 'mp4', str(output)]

class Jobs:
    def __init__(self, cache, ffmpeg, ffprobe, token, torrent_server='', limit=6*1024**3):
        self.cache = Path(cache).resolve()
        self.cache.mkdir(parents=True, exist_ok=True)
        self.ffmpeg, self.ffprobe, self.token = ffmpeg, ffprobe, token
        self.torrent_server = torrent_server.rstrip('/')
        self.limit = limit
        self.jobs = {}
        self.lock = threading.Lock()
        self.pool = concurrent.futures.ThreadPoolExecutor(max_workers=1)

    def create(self, body):
        url = body.get('url', '')
        if not isinstance(url, str):
            raise ValueError('Invalid source')
        headers = provider_headers(body.get('headers', ''))
        if url.startswith('magnet:?'):
            if not self.torrent_server:
                raise ValueError('Magnet conversion requires --torrent-server; native torrent playback is separate')
            query = parse_qs(urlsplit(url).query)
            index = int(body.get('file_index', query.get('so', ['0'])[0]))
            if index < 0 or index > 2047:
                raise ValueError('Invalid torrent file index')
            url = self.torrent_server + '/stream/nuvio?link=' + quote(url, safe='') + '&index=' + str(index+1) + '&play'
        http_url(url)
        # Hash includes provider headers, so private sources never share cached jobs.
        key = hashlib.sha256((url+'\0'+headers).encode()).hexdigest()[:32]
        with self.lock:
            existing = self.jobs.get(key)
            if existing and existing['state'] != 'failed':
                return key
            if sum(j['state'] in ('queued', 'converting') for j in self.jobs.values()) >= 4:
                raise ValueError('Conversion queue is full')
            self.jobs[key] = {'state': 'queued', 'error': '', 'updated': time.time()}
        self.pool.submit(self.convert, key, url, headers)
        return key

    def cleanup(self):
        files = sorted(self.cache.glob('*.mp4'), key=lambda p: p.stat().st_mtime)
        total = sum(p.stat().st_size for p in files)
        for file in files:
            if total < self.limit - 4*1024**3 and time.time()-file.stat().st_mtime < 5400:
                continue
            size = file.stat().st_size
            try:
                file.unlink()
                total -= size
                with self.lock:
                    self.jobs.pop(file.stem, None)
            except OSError:
                pass # A currently playing file may be locked on Windows.

    def convert(self, key, url, headers):
        output = self.cache / (key + '.mp4')
        partial = self.cache / (key + '.partial')
        try:
            self.cleanup()
            if shutil.disk_usage(self.cache).free < 4*1024**3:
                raise ValueError('At least 4 GiB of free cache space is required')
            with self.lock:
                self.jobs[key]['state'] = 'converting'
            command = [self.ffprobe, '-v', 'error', '-protocol_whitelist', 'http,https,tcp,tls,crypto',
                       '-rw_timeout', '15000000']
            if headers:
                command += ['-headers', headers]
            probe = subprocess.run(command + ['-show_entries', 'stream=codec_type,color_transfer:format=duration', '-of', 'json', url],
                capture_output=True, timeout=40, **HIDDEN)
            if probe.returncode:
                raise ValueError('Unable to inspect the media source')
            info = json.loads(probe.stdout)
            video = next(s for s in info['streams'] if s.get('codec_type') == 'video')
            hdr = video.get('color_transfer') in ('smpte2084', 'arib-std-b67')
            # Duration limit prevents a capped output from being mistaken for a complete film.
            duration = float(info.get('format', {}).get('duration', 0))
            if duration <= 0 or duration > 10800:
                raise ValueError('Conversion requires a finite video no longer than 3 hours')
            result = subprocess.run(conversion_args(self.ffmpeg, url, headers, partial, hdr),
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=7200, **HIDDEN)
            if result.returncode or not partial.exists() or partial.stat().st_size >= 4*1024**3:
                raise ValueError('Conversion failed; verify codecs, HDR filters and source connectivity')
            partial.replace(output)
            with self.lock:
                self.jobs[key].update(state='ready', duration=duration, updated=time.time())
        except Exception as exc:
            partial.unlink(missing_ok=True)
            with self.lock:
                self.jobs[key].update(state='failed', error=str(exc) if isinstance(exc, ValueError)
                                     else 'Conversion worker failed', updated=time.time())

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def authorized(self):
        if hmac.compare_digest(self.headers.get('Authorization', ''), 'Bearer '+self.server.jobs.token):
            return True
        self.respond(401, {'error': 'Unauthorized'})
        return False

    def respond(self, status, body):
        data = json.dumps(body).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(data)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        if not self.authorized():
            return
        if self.path != '/v1/jobs':
            return self.respond(404, {'error': 'Not found'})
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if not 0 < length <= 32768:
                raise ValueError('Invalid request size')
            body = json.loads(self.rfile.read(length))
            if not isinstance(body, dict):
                raise ValueError('Invalid request')
            key = self.server.jobs.create(body)
            self.respond(201, {'id': key})
        except (ValueError, TypeError, KeyError) as exc:
            self.respond(400, {'error': str(exc)})

    def do_GET(self):
        if not self.authorized():
            return
        if self.path == '/health':
            return self.respond(200, {'service': 'nuvio-transcoder', 'version': '0.1.0-alpha.2'})
        match = re.fullmatch(r'/v1/(jobs|media)/([0-9a-f]{32})(\.mp4)?', self.path)
        if not match:
            return self.respond(404, {'error': 'Not found'})
        kind, key, _ = match.groups()
        with self.server.jobs.lock:
            state = dict(self.server.jobs.jobs.get(key, {}))
        if not state:
            return self.respond(404, {'error': 'Unknown job'})
        if kind == 'jobs':
            return self.respond(200, state)
        if state['state'] == 'failed':
            return self.respond(422, {'error': state['error']})
        if state['state'] != 'ready':
            return self.respond(503, {'state': state['state']})
        path = self.server.jobs.cache / (key+'.mp4')
        try:
            media = path.open('rb')
        except FileNotFoundError:
            return self.respond(410, {'error': 'Cached conversion expired; select the source again'})
        with media:
            size = os.fstat(media.fileno()).st_size
            start, end = 0, size-1
            partial = self.headers.get('Range')
            if partial:
                bounds = re.fullmatch(r'bytes=(\d+)-(\d*)', partial)
                if not bounds:
                    return self.respond(416, {'error': 'Unsupported byte range'})
                start = int(bounds[1])
                end = min(int(bounds[2]) if bounds[2] else size-1, size-1)
                if start >= size or start > end:
                    return self.respond(416, {'error': 'Invalid byte range'})
            self.send_response(206 if partial else 200)
            self.send_header('Content-Type', 'video/mp4')
            self.send_header('Accept-Ranges', 'bytes')
            self.send_header('Content-Length', str(end-start+1))
            if partial:
                self.send_header('Content-Range', f'bytes {start}-{end}/{size}')
            self.end_headers()
            media.seek(start)
            remaining = end-start+1
            try:
                while remaining:
                    chunk = media.read(min(65536, remaining))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    remaining -= len(chunk)
            except (BrokenPipeError, ConnectionResetError):
                pass

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=8787)
    parser.add_argument('--ffmpeg', default='ffmpeg')
    parser.add_argument('--ffprobe', default='ffprobe')
    parser.add_argument('--cache', default=str(Path.home()/'.nuvio-transcodes'))
    parser.add_argument('--token-file', required=True)
    parser.add_argument('--torrent-server', default='')
    args = parser.parse_args()
    token = Path(args.token_file).read_text().strip()
    if len(token) < 32 or not re.fullmatch('[A-Za-z0-9_-]+', token):
        parser.error('token-file must contain at least 32 safe random characters')
    if args.torrent_server:
        http_url(args.torrent_server)
    server = http.server.ThreadingHTTPServer((args.host, args.port), Handler)
    server.daemon_threads = True
    server.jobs = Jobs(args.cache, args.ffmpeg, args.ffprobe, token, args.torrent_server)
    print(f'Nuvio conversion service listening on {args.host}:{args.port}', flush=True)
    try:
        server.serve_forever()
    finally:
        server.server_close()

if __name__ == '__main__':
    main()
