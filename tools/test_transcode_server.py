"""Run with --ffmpeg PATH and --ffprobe PATH for real HEVC/HDR conversion tests."""
import argparse
import functools
import http.server
import json
from pathlib import Path
import subprocess
import tempfile
import threading
import time
import unittest
from urllib.error import HTTPError
from urllib.request import Request, urlopen
import transcode_server as service

FFMPEG = FFPROBE = None

class ServiceTests(unittest.TestCase):
    def test_validation(self):
        for url in ('file:///secret', 'concat:http://a', 'https://user:pass@host/a', 'http://host/\r\nbad'):
            with self.assertRaises(ValueError):
                service.http_url(url)
        with self.assertRaises(ValueError):
            service.provider_headers('Authorization: a\nInjected: x')
        self.assertEqual(service.provider_headers('Referer: https://provider.test/\r\n'),
                         'Referer: https://provider.test/\r\n')

    def test_hdr_command(self):
        args = service.conversion_args('ffmpeg', 'https://media.test/a', '', 'a.mp4', True)
        self.assertIn('tonemap=hable', args[args.index('-vf')+1])
        self.assertIn('aac', args)
        self.assertIn('+faststart', args)

    def test_real_http_conversion(self):
        if not FFMPEG or not FFPROBE:
            self.skipTest("Supply FFmpeg/FFprobe for integration test")
        with tempfile.TemporaryDirectory(prefix='nuvio-conversion-') as folder:
            root = Path(folder)
            for name, hdr in (('hevc', False), ('hdr', True)):
                command = [FFMPEG, '-v', 'error', '-y', '-f', 'lavfi', '-i', 'testsrc2=size=1920x1080:rate=24',
                    '-f', 'lavfi', '-i', 'sine=frequency=440:sample_rate=48000', '-t', '2',
                    '-c:v', 'libx265', '-preset', 'ultrafast', '-x265-params', 'pools=2:log-level=error',
                    '-c:a', 'pcm_s16le']
                if hdr:
                    command += ['-pix_fmt', 'yuv420p10le', '-color_primaries', 'bt2020',
                                '-color_trc', 'smpte2084', '-colorspace', 'bt2020nc']
                result = subprocess.run(command+[str(root/(name+'.mkv'))], capture_output=True, **service.HIDDEN)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace')[-500:])
            class Quiet(http.server.SimpleHTTPRequestHandler):
                def log_message(self, *_): pass
            source = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Quiet, directory=folder))
            host = http.server.ThreadingHTTPServer(('127.0.0.1', 0), service.Handler)
            host.jobs = service.Jobs(root/'cache', FFMPEG, FFPROBE, 'T'*40)
            for server in (source, host):
                threading.Thread(target=server.serve_forever, daemon=True).start()
            base = f'http://127.0.0.1:{host.server_port}'
            headers = {'Authorization': 'Bearer '+'T'*40}
            try:
                with self.assertRaises(HTTPError) as unauth:
                    urlopen(base+'/health')
                self.assertEqual(unauth.exception.code, 401)
                for name in ('hevc', 'hdr'):
                    body = json.dumps({'url': f'http://127.0.0.1:{source.server_port}/{name}.mkv'}).encode()
                    request = Request(base+'/v1/jobs', body, headers, method='POST')
                    key = json.load(urlopen(request))['id']
                    deadline = time.time()+90
                    while time.time() < deadline:
                        state = json.load(urlopen(Request(base+'/v1/jobs/'+key, headers=headers)))
                        if state['state'] in ('ready', 'failed'):
                            break
                        time.sleep(.2)
                    self.assertEqual(state['state'], 'ready', state)
                    url = base+'/v1/media/'+key+'.mp4'
                    media = urlopen(Request(url, headers=headers)).read()
                    partial = urlopen(Request(url, headers=dict(headers, Range='bytes=111-432')))
                    self.assertEqual(partial.status, 206)
                    self.assertEqual(partial.read(), media[111:433])
                    with self.assertRaises(HTTPError) as bounds:
                        urlopen(Request(url, headers=dict(headers, Range='bytes=999999999-')))
                    self.assertEqual(bounds.exception.code, 416)
                    info = json.loads(subprocess.check_output([FFPROBE, '-v', 'error', '-show_entries', 'stream=codec_name,width,height,pix_fmt,color_transfer,channels,duration',
                                            '-of', 'json', str(root/'cache'/(key+'.mp4'))], **service.HIDDEN))
                    video, audio = info['streams'][:2]
                    self.assertEqual(video['codec_name'], 'h264')
                    self.assertLessEqual(video['width'], 1280)
                    self.assertLessEqual(video['height'], 720)
                    self.assertEqual(video['pix_fmt'], 'yuv420p')
                    self.assertEqual(video['color_transfer'], 'bt709')
                    self.assertEqual(audio['codec_name'], 'aac')
                    self.assertEqual(audio['channels'], 2)
                    self.assertAlmostEqual(float(video['duration']), float(audio['duration']), delta=.12)
                    print(f'PASS: {name} over HTTP -> H.264 720p SDR + AAC; ranges, duration and auth')
            finally:
                for server in (host, source):
                    server.shutdown()
                    server.server_close()
                host.jobs.pool.shutdown(wait=True)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--ffmpeg')
    parser.add_argument('--ffprobe')
    args, extra = parser.parse_known_args()
    FFMPEG, FFPROBE = args.ffmpeg, args.ffprobe
    unittest.main(argv=['test_transcode_server']+extra)
