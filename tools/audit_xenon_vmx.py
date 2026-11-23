"""Audit PPC executable sections in XDK COFF objects, .lib archives and PE images.

Adapted from the 22-opcode classifier in ffmpeg360-h264-xenon/verify/vmxaudit.py
(LGPL-2.1-or-later per that repository's NOTICE). Unlike its objdump wrapper,
this reads XDK files directly, never scanning data sections or encrypted XEXs.
"""
import argparse
import json
from pathlib import Path
import struct

VA = {32:'vmhaddshs',33:'vmhraddshs',34:'vmladduhm',36:'vmsumubm',37:'vmsummbm',
      38:'vmsumuhm',39:'vmsumuhs',40:'vmsumshm',41:'vmsumshs'}
VX = {8:'vmuloub',72:'vmulouh',264:'vmulosb',328:'vmulosh',520:'vmuleub',584:'vmuleuh',
      776:'vmulesb',840:'vmulesh',1544:'vsum4ubs',1800:'vsum4sbs',1608:'vsum4shs',
      1672:'vsum2sws',1928:'vsumsws'}


def classify(word):
    if word >> 26 != 4:
        return None
    return VA.get(word & 63) or VX.get(word & 2047)


def sections(raw):
    header = 0
    if raw[:2] == b'MZ':
        if len(raw) < 64:
            raise ValueError('Truncated DOS header')
        pe = struct.unpack_from('<I',raw,60)[0]
        if raw[pe:pe+4] != b'PE\0\0':
            raise ValueError('Invalid PE header')
        header = pe + 4
    if len(raw) < header + 20:
        raise ValueError('Truncated COFF header')
    machine,count,_,_,_,optional,_ = struct.unpack_from('<HHIIIHH',raw,header)
    if machine not in (0x1f0,0x1f2):
        raise ValueError('Expected PowerPC/Xbox COFF, got machine '+hex(machine))
    table = header + 20 + optional
    if not count or table+count*40 > len(raw):
        raise ValueError('Invalid COFF section table')
    for i in range(count):
        at = table + i*40
        name=raw[at:at+8].rstrip(b'\0').decode('ascii','replace')
        size,start=struct.unpack_from('<II',raw,at+16)
        flags=struct.unpack_from('<I',raw,at+36)[0]
        if not flags & 0x20 or not size:
            continue
        if not start or start+size > len(raw) or size % 4:
            raise ValueError('Invalid executable section '+name)
        yield name,raw[start:start+size]


def members(raw):
    if raw[:8] != b'!<arch>\n':
        yield '',raw
        return
    at=8
    names=b''
    while at<len(raw):
        if at+60>len(raw) or raw[at+58:at+60]!=b'`\n':
            raise ValueError('Invalid archive member header')
        name=raw[at:at+16].decode('ascii').strip()
        size=int(raw[at+48:at+58])
        data=raw[at+60:at+60+size]
        if len(data)!=size:
            raise ValueError('Truncated archive member')
        at+=60+size+(size&1)
        if name=='//':
            names=data
            continue
        if name in ('/','/SYM64/'):
            continue
        if name.startswith('/') and name[1:].isdigit():
            offset=int(name[1:])
            end=names.find(b'\0',offset)
            if end<0:end=names.find(b'/\n',offset)
            if end<0:raise ValueError('Invalid archive long name')
            name=names[offset:end].decode('utf-8','replace')
        yield name.rstrip('/'),data


def audit(raw,label='image'):
    hits=[]
    words=objects=vector_words=0
    for obj,data in members(raw):
        objects+=1
        for section,code in sections(data):
            for offset in range(0,len(code),4):
                word=struct.unpack_from('>I',code,offset)[0]
                words+=1
                vector_words+=word>>26==4
                bad=classify(word)
                if bad:
                    hits.append({'object':obj,'section':section,'offset':offset,'opcode':bad})
    if not words:
        raise ValueError('No executable instruction words audited')
    return {'file':label,'objects':objects,'words':words,'vmx_words':vector_words,'hits':hits}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files',nargs='+',type=Path)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    results=[]
    for path in args.files:
        result=audit(path.read_bytes(),str(path))
        results.append(result)
        print(f'{path.name}: {result["words"]} instruction words, {len(result["hits"])} forbidden opcode hits')
        for hit in result['hits'][:20]:print(hit)
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(results,indent=2),encoding='utf-8')
    return int(any(r['hits'] for r in results))


if __name__=='__main__':
    raise SystemExit(main())
