#!/usr/bin/env python3
"""Dependency-free PBM conversion and integer nearest-neighbor contact sheets.
Optional: python3 tools/contact_sheet.py serial.log (extracts PBM_BEGIN/END).
PBM uses 1=black; exported firmware uses 1=lit, so output PNG inverts PBM.
"""
from pathlib import Path
import sys, struct, zlib, binascii
root=Path(__file__).resolve().parents[1]
out=root/'previews'
if len(sys.argv)>1:
    data=Path(sys.argv[1]).read_text()
    for i,part in enumerate(data.split('PBM_BEGIN')[1:]):
        (out/f'capture-{i}.pbm').write_text(part.split('PBM_END')[0].strip()+'\n')
def png(path,w,h,pixels):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',binascii.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+bytes(pixels[y*w:(y+1)*w]) for y in range(h))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,0,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
def load(path):
    data=path.read_text().split()
    assert data[:3]==['P1','128','64']
    return [255 if v=='1' else 0 for v in data[3:]]
for path in out.glob('*.pbm'):
    png(path.with_suffix('.png'),128,64,load(path))
files=sorted(out.glob('[0-9][0-9]-*-page1.pbm'))
for scale in (1,4):
    gap=8;w=4*(128*scale+gap)+gap;h=len(files)*(64*scale+gap)+gap
    pixels=bytearray([35])*(w*h)
    for row,first in enumerate(files):
        for col in range(4):
            data=load(Path(str(first).replace('page1','page'+str(col+1))))
            for y in range(64*scale):
                dest=(gap+row*(64*scale+gap)+y)*w+gap+col*(128*scale+gap)
                pixels[dest:dest+128*scale]=bytes(data[(y//scale)*128+x//scale] for x in range(128*scale))
    png(out/f'contact-sheet-{scale}x.png',w,h,pixels)
print('Wrote native PNGs and 1x / 4x contact sheets')
