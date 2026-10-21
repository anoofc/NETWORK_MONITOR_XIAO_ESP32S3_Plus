#!/usr/bin/env python3
"""Compile the firmware renderer against pinned U8g2 C sources; export real PBMs."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'tools/.build'
BUILD.mkdir(exist_ok=True)
lib=ROOT/'.pio/libdeps/xiao_ssd1306/U8g2/src/clib'
if not lib.exists():
    raise SystemExit('Run pio run -e xiao_ssd1306 first to install pinned U8g2 2.36.12')
objects=[]
for source in sorted(lib.glob('*.c')):
    obj=BUILD/(source.stem+'.o')
    if not obj.exists() or obj.stat().st_mtime<source.stat().st_mtime:
        subprocess.run(['cc','-O2','-w','-ffunction-sections','-fdata-sections','-I'+str(lib),'-c',str(source),'-o',str(obj)],check=True)
    objects.append(str(obj))
subprocess.run(['c++','-std=c++11','-O2','-I'+str(lib),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'include'),str(ROOT/'tools/preview.cpp'),str(ROOT/'src/OledRenderer.cpp'),*objects,'-o',str(BUILD/'preview')],check=True)
subprocess.run([str(BUILD/'preview'),str(ROOT/'previews')],check=True)
for frame in (ROOT/'previews').glob('[0-9][0-9]-*.pbm'):
    pixels=frame.read_text().split()[3:]
    assert len(pixels)==128*64, frame
    assert all(pixels[y*128+x]=='0' for y in range(64) for x in (0,1,2,3,124,125,126,127)), frame
print('All 44 framebuffers retain four-pixel horizontal margins')
subprocess.run(['python3',str(ROOT/'tools/contact_sheet.py')],check=True)
