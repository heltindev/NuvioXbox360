"""Loopback-only real byte-stream fixture for NuvioHttpTests. Supply an open MP4."""
import http.server
from pathlib import Path
import re
import sys

DATA = b''
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_): pass
    def do_GET(self):
        if self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/sample.mp4')
            self.end_headers()
            return
        chunked = self.path in ('/chunked','/bad-chunk')
        start, end = 0, len(DATA)-1
        interval = self.headers.get('Range')
        if interval and self.path != '/no-range':
            match = re.fullmatch(r'bytes=(\d+)-', interval)
            start = int(match[1]) if match else 0
            self.send_response(206)
            reported = 0 if self.path == '/wrong-range' else start
            self.send_header('Content-Range', f'bytes {reported}-{end}/{len(DATA)}')
        else:
            self.send_response(200)
        self.send_header('Content-Type', 'video/mp4')
        if chunked:
            self.send_header('Transfer-Encoding', 'chunked')
        else:
            self.send_header('Content-Length', str(len(DATA)-start))
        self.end_headers()
        try:
            if self.path == '/bad-chunk':
                self.wfile.write(b'NOTHEX\r\n')
            elif chunked:
                for at in range(0,len(DATA),8192):
                    part=DATA[at:at+8192]
                    self.wfile.write(f'{len(part):x}\r\n'.encode()+part+b'\r\n')
                self.wfile.write(b'0\r\n\r\n')
            else:
                self.wfile.write(DATA[start:1000] if self.path == '/truncated' else DATA[start:])
        except (BrokenPipeError,ConnectionResetError): pass

if __name__ == '__main__':
    DATA=Path(sys.argv[1]).read_bytes()
    server=http.server.ThreadingHTTPServer(('127.0.0.1',19003),Handler)
    server.serve_forever()
