"""Exercise native torrent code against loopback fixtures, without external peers."""
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'VisualStudio2010/obj/TorrentIntegration'
EXE = ROOT / 'VisualStudio2010/obj/NativeTests/NuvioTorrentTests.exe'


def main():
    WORK.mkdir(exist_ok=True)
    payload = bytes(range(256)) * 18000
    scenarios = [
        ('standard', []),
        ('multi', ['--multi']),
        ('metadata', ['--large-metadata', '--pipeline-metadata', '--split-handshake', '--reject-peer']),
        ('corrupt', ['--corrupt']),
    ]
    for name, options in scenarios:
        sample = WORK / (name + '.bin')
        sample.write_bytes(payload)
        magnet_file = sample.with_suffix('.magnet.txt')
        if magnet_file.exists():
            magnet_file.unlink()
        with (WORK / (name + '.log')).open('w') as log:
            fixture = subprocess.Popen([sys.executable, str(ROOT / 'tools/torrent_fixture.py'), str(sample)] + options,
                stdout=log, stderr=subprocess.STDOUT,
                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            try:
                end = time.monotonic() + 5
                while not magnet_file.exists():
                    if fixture.poll() is not None or time.monotonic() > end:
                        raise RuntimeError('Fixture failed to start: ' + name)
                    time.sleep(.05)
                command = [str(EXE), magnet_file.read_text(), str(sample)]
                if name == 'corrupt':
                    command.append('expect-corruption')
                result = subprocess.run(command, capture_output=True, text=True, timeout=55,
                    creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                print(name + ': ' + result.stdout.strip(), flush=True)
                if result.returncode:
                    raise RuntimeError(name + ': ' + result.stderr)
            finally:
                fixture.terminate()
                fixture.wait(timeout=5)
    print('PASS: metadata pipeline, split handshake, rejection fallback, SHA-1, seek and corruption checks', flush=True)


if __name__ == '__main__':
    main()
