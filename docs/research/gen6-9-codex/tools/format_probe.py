#!/usr/bin/env python3
"""Bounded, read-only structural research probe; never authorizes export.
Constants: kwsch/PKHeX 641c26ba100225f3605e5a44b61b3b16e15a3774,
SaveUtil.SIZE_G8BDSP_0..3, SAV8BS.HashOffset/ChecksumsValid, Gem8Version.
No PK6/PK7/PK8 decoder is reimplemented here.
"""
import argparse, hashlib, json, pathlib, struct
BDSP={0xE9828:0x25,0xEDC20:0x2C,0xEED8C:0x32,0xEF0A4:0x34}
HASH_OFFSET=0xE9818
RECORDS={'PK6':(0xE8,0x104),'PK7':(0xE8,0x104),'PB7':(0x104,),
         'PK8':(0x148,0x158),'PB8':(0x148,0x158),'PA8':(0x168,0x178),
         'PK9':(0x148,0x158),'PA9':(0x148,0x158)}
def checked_range(length,offset,count,stride):
    if any(type(x) is not int or x<0 for x in (length,offset,count,stride)) or stride==0:return None
    if offset>length or count>(length-offset)//stride:return None
    return offset,offset+count*stride

def bdsp_integrity(data):
    """Integrity != authenticity. A digest-valid synthetic file isn't a valid game save."""
    if len(data) not in BDSP:return {'candidate':False,'reason':'unsupported length'}
    revision=struct.unpack_from('<I',data)[0]
    scratch=bytearray(data);stored=bytes(scratch[HASH_OFFSET:HASH_OFFSET+16]);scratch[HASH_OFFSET:HASH_OFFSET+16]=bytes(16)
    return {'candidate':True,'revision':revision,'revision_matches_length':revision==BDSP[len(data)],
            'md5_matches':hashlib.md5(scratch).digest()==stored,'game_identity':'not validated',
            'export_authorized':False,'retail_acceptance':'unknown'}
def main():
    p=argparse.ArgumentParser();p.add_argument('file',type=pathlib.Path);a=p.parse_args()
    # Exact-size allowlist before read: no whole-file read of arbitrarily large inputs.
    with a.file.open('rb') as f:
        import os
        size=os.fstat(f.fileno()).st_size
        if size not in BDSP:raise SystemExit('Unsupported size; not read')
        data=f.read(size+1)
        if len(data)!=size:raise SystemExit('File changed during read')
    print(json.dumps(bdsp_integrity(data),indent=2))
if __name__=='__main__':main()
