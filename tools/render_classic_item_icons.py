#!/usr/bin/env python3
"""Reproducible, honest *custom* Gen II item illustrations.

Gold/Silver/Crystal did not ship separate per-item 32px bag sprites for these
historical names. Never claim these newly illustrated PNGs are ROM-extracted
Nintendo/GF artwork. They are small, individually recognizable illustrations
built from semantic item features. No network, fonts or imaging packages needed.
"""
from __future__ import annotations
import argparse
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "romfs" / "sprites" / "items"
SIZE = 32
KEYS = (
    "silver-leaf", "gold-leaf", "pink-bow", "polkadot-bow",
    "gs-ball", "egg-ticket", "normal-box", "gorgeous-box",
    "brick-piece",
)
CLEAR = (0, 0, 0, 0)
INK = (40, 42, 59, 255)
PALE = (255, 248, 230, 255)

class Icon:
    def __init__(self):
        self.p = [[CLEAR for _ in range(SIZE)] for _ in range(SIZE)]
    def dot(self, x, y, c):
        if 0 <= x < SIZE and 0 <= y < SIZE:
            self.p[y][x] = c
    def rect(self, a, b, c, d, color):
        for y in range(max(0,b), min(SIZE,d+1)):
            for x in range(max(0,a), min(SIZE,c+1)):
                self.p[y][x] = color
    def line(self, x0, y0, x1, y1, color):
        dx, dy = abs(x1-x0), -abs(y1-y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx + dy
        while True:
            self.dot(x0, y0, color)
            if x0 == x1 and y0 == y1: break
            e = 2*err
            if e >= dy: err += dy; x0 += sx
            if e <= dx: err += dx; y0 += sy
    def poly(self, pts, color):
        # Exact integer even-odd interior, then sharp 1px border.
        for y in range(SIZE):
            for x in range(SIZE):
                hit = False
                for i in range(len(pts)):
                    x1, y1 = pts[i]
                    x2, y2 = pts[(i+1) % len(pts)]
                    if (y1 > y) != (y2 > y) and x < (x2-x1)*(y-y1)/(y2-y1)+x1:
                        hit = not hit
                if hit: self.dot(x, y, color)
    def ellipse(self, x0, y0, x1, y1, color):
        rx = max(1, (x1-x0)/2)
        ry = max(1, (y1-y0)/2)
        cx, cy = (x0+x1)/2, (y0+y1)/2
        for y in range(max(0,y0), min(SIZE,y1+1)):
            for x in range(max(0,x0), min(SIZE,x1+1)):
                if ((x-cx)/rx)**2 + ((y-cy)/ry)**2 <= 1:
                    self.dot(x, y, color)
    def stroke(self, pts, color=INK):
        for a,b in zip(pts,pts[1:]+pts[:1]):
            self.line(a[0],a[1],b[0],b[1],color)
    def png(self) -> bytes:
        rows = b"".join(b"\x00" + bytes(c for pixel in row for c in pixel)
                        for row in self.p)
        def chunk(tag, blob):
            return (struct.pack(">I",len(blob)) + tag + blob
                    + struct.pack(">I",zlib.crc32(tag+blob)&0xffffffff))
        return (b"\x89PNG\r\n\x1a\n"
                + chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 6, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))

def leaf(c, gold=False):
    base = (216,158,48,255) if gold else (144,168,183,255)
    shade = (157,91,26,255) if gold else (81,107,129,255)
    glint = (255,235,136,255) if gold else (226,244,250,255)
    pts=[(6,21),(4,17),(7,11),(13,7),(23,4),(27,5),
         (26,13),(22,21),(17,25),(10,26)]
    c.poly(pts,base);c.stroke(pts)
    c.line(8,27,23,9,shade);c.line(15,19,10,13,shade)
    c.line(20,14,22,8,shade);c.line(11,22,6,20,shade)
    c.line(10,13,18,8,glint);c.dot(22,7,glint)

def bow(c, spots=False):
    dark=(136,45,100,255);pink=(239,118,183,255)
    hi=(255,200,228,255);tie=(255,221,111,255)
    left=[(3,10),(14,13),(14,21),(4,25),(6,18)]
    right=[(17,13),(28,9),(29,24),(18,21)]
    for pts in (left,right):
        c.poly(pts,dark);c.stroke(pts)
    c.poly([(5,12),(13,15),(12,19),(7,22)],pink)
    c.poly([(19,15),(27,12),(26,22),(19,19)],pink)
    c.line(6,12,11,15,hi);c.line(22,15,27,12,hi)
    c.rect(13,14,19,21,dark);c.rect(15,15,17,19,tie)
    if spots:
        for x,y in ((7,15),(10,19),(8,21),(24,15),(22,19),(26,21)):
            c.ellipse(x,y,x+2,y+2,PALE)

def ball(c):
    c.ellipse(3,3,28,28,INK)
    c.ellipse(5,5,26,26,(241,239,235,255))
    c.rect(6,9,25,15,(230,70,86,255))
    c.rect(5,15,26,17,INK)
    c.ellipse(12,12,20,20,INK)
    c.ellipse(14,14,18,18,(255,218,103,255))
    c.line(9,8,13,6,(255,171,178,255))
    c.line(19,23,23,20,(202,194,182,255))
    # The GS monogram is part of this custom image, not a copied cartridge icon.
    for x,y in ((15,15),(16,15),(17,15),(15,16),(15,17),(16,17),(17,17)):
        c.dot(x,y,(120,62,36,255))

def ticket(c):
    ink=(126,80,83,255);paper=(249,223,169,255)
    c.rect(4,7,27,24,INK)
    c.rect(6,8,25,22,paper)
    for x in (9,14,19,24):c.dot(x,24,(219,170,111,255))
    c.ellipse(16,10,23,19,(244,237,215,255))
    c.ellipse(18,11,21,14,(194,165,217,255))
    c.dot(18,18,(151,121,154,255))
    c.rect(7,11,12,12,ink);c.rect(7,15,12,16,ink)
    c.line(7,19,12,19,ink)

def gift(c, fancy=False):
    if fancy:
        base=(111,72,165,255);light=(186,136,242,255)
        ribbon=(239,193,69,255);side=(73,46,128,255)
    else:
        base=(192,139,86,255);light=(232,190,133,255)
        ribbon=(225,91,55,255);side=(143,91,55,255)
    c.poly([(4,11),(16,8),(28,12),(16,16)],light)
    c.stroke([(4,11),(16,8),(28,12),(16,16)])
    c.poly([(4,12),(16,16),(16,28),(4,23)],base)
    c.poly([(16,16),(28,12),(28,23),(16,28)],side)
    c.stroke([(4,12),(16,16),(16,28),(4,23)])
    c.stroke([(16,16),(28,12),(28,23),(16,28)])
    c.poly([(13,9),(17,9),(20,14),(16,16)],ribbon)
    c.rect(14,16,17,27,ribbon)
    c.line(6,18,13,20,(248,215,168,255) if not fancy else (219,183,252,255))
    if fancy:
        c.ellipse(12,4,19,9,(235,187,62,255))
        c.ellipse(14,5,17,8,(116,227,243,255))
    else:
        c.line(11,6,15,10,ribbon);c.line(22,6,17,10,ribbon)

def brick(c):
    clay=(192,89,67,255);edge=(130,56,48,255);top=(242,143,100,255)
    c.poly([(3,14),(13,6),(27,11),(17,19)],top)
    c.poly([(3,14),(17,19),(17,27),(3,21)],clay)
    c.poly([(17,19),(27,11),(27,21),(17,27)],edge)
    c.stroke([(3,14),(13,6),(27,11),(27,21),(17,27),(3,21)])
    c.line(10,10,23,14,(255,189,141,255))
    c.line(9,17,9,24,edge);c.line(5,21,16,24,(235,126,96,255))

def make(name):
    c=Icon()
    if name=="silver-leaf":leaf(c)
    elif name=="gold-leaf":leaf(c,True)
    elif name=="pink-bow":bow(c)
    elif name=="polkadot-bow":bow(c,True)
    elif name=="gs-ball":ball(c)
    elif name=="egg-ticket":ticket(c)
    elif name=="normal-box":gift(c)
    elif name=="gorgeous-box":gift(c,True)
    elif name=="brick-piece":brick(c)
    else:raise ValueError(name)
    return c.png()

def build(out=OUT):
    out.mkdir(parents=True,exist_ok=True)
    for name in KEYS:
        path=out / (name+".png")
        payload=make(name)
        if not path.is_file() or path.read_bytes()!=payload:
            path.write_bytes(payload)
    return len(KEYS)

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",type=Path,default=OUT)
    args=parser.parse_args()
    print(f"Custom distinct GSC historical item illustrations: {build(args.output)}")
