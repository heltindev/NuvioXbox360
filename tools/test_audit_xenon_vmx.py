"""Verify the opcode gate with positive controls and non-executable data."""
import struct
import unittest
from audit_xenon_vmx import VA,VX,audit,classify


def coff(code,data=b''):
    header=struct.pack('<HHIIIHH',0x1f2,2,0,0,0,0,0)
    text=b'.text\0\0\0'+struct.pack('<IIIIIIHHI',0,0,len(code),100,0,0,0,0,0x60000020)
    extra=b'.data\0\0\0'+struct.pack('<IIIIIIHHI',0,0,len(data),100+len(code),0,0,0,0,0xc0000040)
    return header+text+extra+code+data


class AuditTests(unittest.TestCase):
    def test_all_forbidden_instructions(self):
        for xo,name in list(VA.items())+list(VX.items()):
            word=(4<<26)|xo
            self.assertEqual(classify(word),name)
            self.assertEqual(audit(coff(struct.pack('>I',word)))['hits'][0]['opcode'],name)

    def test_data_is_not_disassembled(self):
        result=audit(coff(bytes.fromhex('60000000'),struct.pack('>I',(4<<26)|34)))
        self.assertEqual(result['words'],1)
        self.assertEqual(result['hits'],[])

    def test_pe(self):
        raw=b'MZ'+bytes(58)+struct.pack('<I',64)+b'PE\0\0'
        obj=bytearray(coff(bytes.fromhex('60000000')))
        struct.pack_into('<I',obj,40,168)
        result=audit(raw+obj)
        self.assertFalse(result['hits'])

    def test_invalid_input_fails_closed(self):
        for raw in [b'',b'XEX2',b'!<arch>\nwrong']:
            with self.assertRaises(ValueError):audit(raw)


if __name__=='__main__':
    unittest.main()
