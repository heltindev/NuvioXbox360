"""Loopback-only BEP 9/15 peer fixture. Supply an openly licensed MP4, never user data.
Usage: python torrent_fixture.py sample.mp4 [--corrupt]
Prints magnet and serves until terminated; no external traffic, no full-file cache in app.
"""
import hashlib
import socket
import struct
import sys
import threading
from pathlib import Path
from urllib.parse import quote

def bencode(value):
    if isinstance(value, int):
        return b'i' + str(value).encode() + b'e'
    if isinstance(value, str):
        value = value.encode()
    if isinstance(value, bytes):
        return str(len(value)).encode() + b':' + value
    if isinstance(value, list):
        return b'l' + b''.join(map(bencode, value)) + b'e'
    return b'd' + b''.join(bencode(k) + bencode(value[k]) for k in sorted(value)) + b'e'

def decode(data, pos=0):
    c = data[pos:pos+1]
    if c == b'i':
        end = data.index(b'e', pos)
        return int(data[pos+1:end]), end+1
    if c in (b'l', b'd'):
        result = [] if c == b'l' else {}
        pos += 1
        while data[pos:pos+1] != b'e':
            key, pos = decode(data, pos)
            if c == b'l':
                result.append(key)
            else:
                result[key], pos = decode(data, pos)
        return result, pos+1
    end = data.index(b':', pos)
    length = int(data[pos:end])
    return data[end+1:end+1+length], end+1+length

def receive(sock, n):
    result = b''
    while len(result) < n:
        part = sock.recv(n-len(result))
        if not part:
            raise EOFError()
        result += part
    return result

def main():
    data = Path(sys.argv[1]).read_bytes()
    original_size = len(data)
    multi = '--multi' in sys.argv
    if multi:
        data = b'P' * 12345 + data
    piece_length = 262144
    entry = {'name': 'sample.mp4', 'piece length': piece_length,
             'pieces': b''.join(hashlib.sha1(data[i:i+piece_length]).digest()
                               for i in range(0, len(data), piece_length))}
    if multi:
        entry['files'] = [{'length': 12345, 'path': ['readme.txt']},
                          {'length': original_size, 'path': ['sample.mp4']}]
    else:
        entry['length'] = len(data)
    info = bencode(entry)
    info_hash = hashlib.sha1(info).digest()
    corrupt = '--corrupt' in sys.argv
    def peer(sock):
        try:
            sock.settimeout(60)
            hello = receive(sock, 68)
            assert hello[28:48] == info_hash
            sock.sendall(b'\x13BitTorrent protocol' + b'\0'*5 + b'\x10\0\0' + info_hash + b'-FIX001-123456789012')
            def send(kind, payload=b''):
                sock.sendall(struct.pack('>I', len(payload)+1) + bytes([kind]) + payload)
            send(20, b'\0' + bencode({'m': {'ut_metadata': 3}, 'metadata_size': len(info)}))
            send(5, b'\xff' * ((len(data)+piece_length-1)//piece_length//8+1))
            client_extension = 1
            while True:
                size, = struct.unpack('>I', receive(sock, 4))
                if not size:
                    continue
                message = receive(sock, size)
                kind, payload = message[0], message[1:]
                if kind == 20 and payload[0] == 0:
                    client_extension = decode(payload[1:])[0][b'm'][b'ut_metadata']
                elif kind == 20 and payload[0] == 3:
                    request = decode(payload[1:])[0]
                    part = request[b'piece']
                    send(20, bytes([client_extension]) + bencode({'msg_type': 1, 'piece': part,
                        'total_size': len(info)}) + info[part*16384:(part+1)*16384])
                elif kind == 2:
                    send(1)
                elif kind == 6:
                    index, begin, length = struct.unpack('>III', payload)
                    assert 0 < length <= 16384
                    offset = index*piece_length + begin
                    block = data[offset:offset+length]
                    if corrupt:
                        block = bytes([block[0] ^ 1]) + block[1:]
                    send(7, struct.pack('>II', index, begin) + block)
        except (EOFError, OSError):
            pass
        finally:
            sock.close()
    def tcp():
        server = socket.socket()
        server.bind(('127.0.0.1', 19002))
        server.listen(16)
        while True:
            sock, _ = server.accept()
            threading.Thread(target=peer, args=(sock,), daemon=True).start()
    threading.Thread(target=tcp, daemon=True).start()
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(('127.0.0.1', 19001))
    magnet = 'magnet:?xt=urn:btih:' + info_hash.hex() + '&tr=' + quote('udp://127.0.0.1:19001/announce', safe='')
    if multi:
        magnet += '&so=1'
    Path(sys.argv[1]).with_suffix('.magnet.txt').write_text(magnet)
    print(magnet, flush=True)
    while True:
        request, address = udp.recvfrom(8192)
        action, transaction = struct.unpack('>II', request[8:16])
        if action == 0:
            reply = struct.pack('>IIQ', 0, transaction, 123456)
        elif action == 1:
            assert request[16:36] == info_hash
            reply = struct.pack('>IIIII', 1, transaction, 60, 0, 1) + socket.inet_aton('127.0.0.1') + struct.pack('>H', 19002)
        else:
            continue
        udp.sendto(reply, address)

if __name__ == '__main__':
    main()
