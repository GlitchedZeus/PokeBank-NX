#!/usr/bin/env python3
"""Reconstruct distinct 160x144 Gen II mail stationery from original game art.

The source is the pinned pret/pokecrystal gfx/mail PNGs + mail.pal. This is
a SOURCE-BASED PRESENTATION RECONSTRUCTION, not a pixel-exact captured screen:
the writable text and holder species vary during play. Prefer original 160x144
screenshots when a verified archive source is available. No other-generation
mail icons, generic envelopes, Pillow, ImageMagick or arbitrary remote images.
"""
from __future__ import annotations
import re
import struct
import urllib.request
import urllib.parse
import zlib
from pathlib import Path

from png_asset_validation import validate_png

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "romfs" / "sprites" / "items"
CACHE = ROOT / "build-mail-source"
REF = "e058e4f50b3bbf7377e036b81c25a72c54656c5c"
BASE = "https://raw.githubusercontent.com/pret/pokecrystal/" + REF + "/gfx/mail/"
SOURCES = {
    "flower-mail": ("flower_1", "flower_2", "flower_mail_border"),
    "surf-mail": ("lapras", "wave", "surf_mail_border", "small_triangle"),
    "liteblue-mail": ("dratini", "litebluemail_border", "large_triangle"),
    "portraitmail": ("large_pokeball", "small_pokeball", "portraitmail_border"),
    "lovely-mail": ("poliwag", "large_heart", "small_heart", "lovely_mail_border"),
    "eon-mail": ("eevee", "large_circle", "eon_mail_border_1"),
    "morph-mail": ("ditto", "morph_mail_border", "morph_mail_divider"),
    "bluesky-mail": ("dragonite", "sentret", "cloud", "grass"),
    "music-mail": ("natu", "large_note", "small_note", "music_mail_border"),
    "mirage-mail": ("mew", "grass", "flower_1"),
}
PALS = [
    ((20,31,11),(31,19,0),(31,10,9)),      # Flower
    ((15,20,31),(30,26,0),(31,12,0)),      # Surf
    ((24,17,31),(30,26,0),(8,11,31)),      # Liteblue
    ((31,25,17),(31,18,4),(28,12,5)),      # Portrait
    ((19,26,31),(31,5,8),(31,9,31)),       # Lovely
    ((31,19,28),(31,21,0),(12,22,0)),      # Eon
    ((19,17,23),(30,26,0),(31,12,0)),      # Morph
    ((7,26,31),(26,26,27),(31,11,11)),     # Bluesky
    ((21,31,21),(30,26,0),(31,12,0)),      # Music
    ((7,26,31),(31,31,0),(0,21,0)),        # Mirage
]
ORDER = ("flower-mail","surf-mail","liteblue-mail","portraitmail",
         "lovely-mail","eon-mail","morph-mail","bluesky-mail",
         "music-mail","mirage-mail")

def png_chunks(data: bytes):
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise ValueError("not PNG")
    offset = 8
    while offset + 12 <= len(data):
        count = struct.unpack_from(">I",data,offset)[0]
        tag = data[offset+4:offset+8]
        payload = data[offset+8:offset+8+count]
        if len(payload) != count: raise ValueError("truncated PNG")
        yield tag,payload
        offset += count + 12
        if tag == b"IEND": break

def decode(data: bytes):
    width=height=depth=ctype=interlace=0
    palette=[]
    alpha=b""
    compressed=bytearray()
    for tag,payload in png_chunks(data):
        if tag == b"IHDR":
            width,height,depth,ctype,compression,filt,interlace = struct.unpack(">IIBBBBB",payload)
        elif tag == b"PLTE":
            palette=[tuple(payload[i:i+3]) for i in range(0,len(payload),3)]
        elif tag == b"tRNS": alpha=payload
        elif tag == b"IDAT": compressed.extend(payload)
    if interlace or depth not in (1,2,4,8) or ctype not in (0,2,3,4,6):
        raise ValueError("unsupported original game PNG encoding")
    channels={0:1,2:3,3:1,4:2,6:4}[ctype]
    rowlen=(width*channels*depth+7)//8
    stride=max(1,(channels*depth+7)//8)
    payload=zlib.decompress(compressed)
    prev=bytearray(rowlen)
    pos=0
    pixels=[]
    for y in range(height):
        f=payload[pos];pos+=1
        data_row=payload[pos:pos+rowlen];pos+=rowlen
        if len(data_row)!=rowlen: raise ValueError("truncated PNG row")
        row=bytearray(rowlen)
        for i,value in enumerate(data_row):
            left=row[i-stride] if i>=stride else 0
            up=prev[i]
            diag=prev[i-stride] if i>=stride else 0
            if f==0: p=0
            elif f==1:p=left
            elif f==2:p=up
            elif f==3:p=(left+up)//2
            elif f==4:
                predictor=left+up-diag
                ds=(abs(predictor-left),abs(predictor-up),abs(predictor-diag))
                p=(left,up,diag)[ds.index(min(ds))]
            else:raise ValueError("unknown PNG row filter")
            row[i]=(value+p)&255
        prev=row
        if depth==8:
            raw=list(row)
        else:
            mask=(1<<depth)-1
            raw=[(row[(n*depth)//8] >> (8-depth-((n*depth)%8)))&mask
                 for n in range(width*channels)]
        line=[]
        for x in range(width):
            samples=raw[x*channels:x*channels+channels]
            if ctype==3:
                index=samples[0]
                rr,gg,bb=palette[index]
                aa=alpha[index] if index<len(alpha) else 255
            elif ctype==0:
                rr=gg=bb=samples[0]*255//((1<<depth)-1); aa=255
            elif ctype==2:
                rr,gg,bb=samples; aa=255
            elif ctype==4:
                rr=gg=bb=samples[0];aa=samples[1]
            else:
                rr,gg,bb,aa=samples
            line.append((rr,gg,bb,aa))
        pixels.append(line)
    return width,height,pixels

def download(name: str):
    CACHE.mkdir(parents=True, exist_ok=True)
    dest=CACHE/(name+".png")
    if not dest.is_file():
        request=urllib.request.Request(BASE+name+".png",
                                       headers={"User-Agent":"PokeBank-NX-Original-Crystal-Mail/1.0"})
        with urllib.request.urlopen(request,timeout=25) as resp:
            dest.write_bytes(resp.read())
    valid,reason,_=validate_png(dest)
    if not valid:raise ValueError(name+": "+reason)
    return decode(dest.read_bytes())

def encode(pixels):
    height=len(pixels);width=len(pixels[0])
    def chunk(tag,data):
        return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)
    raw=b"".join(b"\x00"+bytes(c for px in row for c in px) for row in pixels)
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,6,0,0,0)) +
            chunk(b"IDAT",zlib.compress(raw,9))+chunk(b"IEND",b""))

def rgb(values):
    return tuple(min(255,c*255//31) for c in values)+(255,)

def assemble(key: str):
    if key not in SOURCES:raise KeyError(key)
    bg,shade,ink=[rgb(x) for x in PALS[ORDER.index(key)]]
    W,H=160,144
    canvas=[[bg]*W for _ in range(H)]
    def line(x1,y1,x2,y2,color):
        dx=abs(x2-x1);dy=-abs(y2-y1)
        sx=1 if x1<x2 else -1;sy=1 if y1<y2 else -1
        er=dx+dy
        while True:
            if 0<=x1<W and 0<=y1<H:canvas[y1][x1]=color
            if (x1,y1)==(x2,y2):break
            e2=2*er
            if e2>=dy:er+=dy;x1+=sx
            if e2<=dx:er+=dx;y1+=sy
    # Letter-sized frame: reserve the main two-line message area as the original
    # Gen II stationery did. Borders, accents and motifs are pinned game source.
    for margin,color in ((0,ink),(2,shade),(7,ink)):
        line(margin,margin,W-1-margin,margin,color)
        line(margin,H-1-margin,W-1-margin,H-1-margin,color)
        line(margin,margin,margin,H-1-margin,color)
        line(W-1-margin,margin,W-1-margin,H-1-margin,color)
    line(16,111,143,111,shade)
    line(16,114,143,114,shade)
    def stamp(name,x,y,scale=1,variant=0):
        width,height,image=download(name)
        color = (shade,ink)[variant%2]
        total=0
        for cy,row in enumerate(image):
            for cx,(r,g,b,a) in enumerate(row):
                # The source original 1bpp graphics contain uncolored mask
                # pixels. Use the matching .pal stationery palette.
                if a<96 or (r+g+b)>500:continue
                for dy in range(scale):
                    for dx in range(scale):
                        xx=x+cx*scale+dx;yy=y+cy*scale+dy
                        if 9<=xx<W-9 and 9<=yy<H-9:
                            canvas[yy][xx]=color
                            total+=1
        if not total:raise ValueError(name+" source graphic contained no drawable pixels")
    src=SOURCES[key]
    # Preserve recognisable native graphics rather than decorating with a
    # later-generation letter/item picture. Type-dependent layout motifs vary.
    main=src[0]
    width,height,_=download(main)
    scale=1 if max(width,height)>39 else 2
    mainwidth=width*scale;mainheight=height*scale
    stamp(main,max(16,139-mainwidth),max(18,104-mainheight),scale,1)
    if len(src)>1:
        second=src[1]
        w2,h2,_=download(second)
        scale2=1 if max(w2,h2)>25 else 2
        stamp(second,13,18,scale2,0)
    if len(src)>2:
        repeat=src[2]
        for x,y in ((16,120),(110,18)):
            stamp(repeat,x,y,1,0)
    for x in range(16,145,16):
        if x%32==16:canvas[121][x]=ink
    return encode(canvas)

# Actual 10 finished 160x144 Game Boy Color mail screens, preserved in a
# fixed Pokémon Crystal sprite sheet by FrenchOrange/The Spriters Resource.
# The raw sheet has two six-column rows, with two Portraitmail variations
# (Japanese / international). We crop the international Portraitmail.
# This is authentic game screenshot art; unlike our fallback reconstruction,
# these are not synthesized layouts. Original Pokémon imagery © Nintendo/TPC.
REAL_MAIL_SHEET = (
    "https://www.spriters-resource.com/media/assets/49/52408.png"
    "?updated=1755473031"
)
MAIL_SHEET_SIZE = (1184, 344)
MAIL_TILE = (160, 144)
MAIL_CROP = {
    "bluesky-mail": (8,24), "eon-mail": (176,24),
    "flower-mail": (344,24), "liteblue-mail": (512,24),
    "lovely-mail": (680,24), "mirage-mail": (848,24),
    "morph-mail": (8,192), "music-mail": (176,192),
    "portraitmail": (512,192), "surf-mail": (848,192)
}

def recover_real_mail_sheet(keys, out=OUT):
    """Prefer pixel-for-pixel original screenshot crops over reconstructions."""
    if not keys: return {}
    out.mkdir(parents=True,exist_ok=True)
    cache_file=CACHE / "crystal-mail-sheet-52408.png"
    cache_file.parent.mkdir(parents=True,exist_ok=True)
    if not cache_file.is_file():
        # Original resource blocks GitHub Actions (HTTP403) occasionally.
        # Two standards-compliant public image-proxy routes provide the same
        # immutable source when hotlinking fails. Every route is strictly
        # checked against the fixed dimensions before caching and cropping.
        # These mirrors are NOT alternative artwork or generated scenes.
        targets = (
            REAL_MAIL_SHEET,
            "https://wsrv.nl/?url=" + urllib.parse.quote(REAL_MAIL_SHEET, safe=""),
            "https://images.weserv.nl/?url=" +
                urllib.parse.quote(REAL_MAIL_SHEET, safe=""),
            "https://corsproxy.io/?url=" +
                urllib.parse.quote(REAL_MAIL_SHEET, safe=""),
        )
        failures = []
        for index, url in enumerate(targets):
            try:
                request=urllib.request.Request(url,
                    headers={"User-Agent":"Mozilla/5.0 (PokeBank-NX-art-recovery)",
                             "Accept":"image/png,image/*;q=0.8",
                             "Referer":"https://www.spriters-resource.com/"})
                with urllib.request.urlopen(request,timeout=20) as source:
                    payload=source.read(2_000_001)
                if len(payload)>2_000_000:
                    raise ValueError("Unexpectedly huge source image")
                w,h,_=decode(payload)
                if (w,h)!=MAIL_SHEET_SIZE:
                    raise ValueError("Source proxy did not return the pinned sheet")
                cache_file.write_bytes(payload)
                print("Authentic Pokémon Crystal mail screenshot source " +
                      str(index+1) + "/" + str(len(targets)) + ": VERIFIED")
                break
            except (OSError, ValueError) as exc:
                failures.append(str(index+1) + ":" + type(exc).__name__)
        if not cache_file.is_file():
            raise OSError("Original Crystal mail image blocked on all 4 routes: "+
                          ", ".join(failures))
    width,height,pixels=decode(cache_file.read_bytes())
    if (width,height)!=MAIL_SHEET_SIZE:
        raise ValueError("Authentic Crystal sheet changed source dimensions")
    recovered={}
    for key in keys:
        x,y=MAIL_CROP[key]
        tile=[row[x:x+160] for row in pixels[y:y+144]]
        if len(tile)!=144 or any(len(row)!=160 for row in tile):
            raise ValueError("Source sheet unexpectedly truncated: "+key)
        # Reject blank crops rather than silently bundling the wrong rectangle.
        if len(set(c for row in tile for c in row))<4:
            raise ValueError("Mail screenshot crop is not an actual stationery: "+key)
        target=out/(key+".png")
        target.write_bytes(encode(tile))
        valid,reason,size=validate_png(target)
        if not valid or tuple(size)!=MAIL_TILE:
            raise ValueError("Authentic screenshot asset invalid: "+key+": "+reason)
        recovered[key]="AUTHENTIC Crystal screenshot"
    return recovered

def render_original_mail_icons(keys=ORDER, out=OUT):
    """32x32 mail-list icons from each matching Crystal original graphics tile.

    Mail stationery in Gen II is a *screen*, not a native item minisprite.
    These are recognisable UI-sized extracts of authentic Crystal motifs,
    distinct from the full stationery screenshot/reconstruction.
    """
    out.mkdir(parents=True, exist_ok=True)
    results={}
    for key in keys:
        name=SOURCES[key][0]
        width,height,src=download(name)
        if width <= 0 or height <= 0:
            raise RuntimeError("Invalid pinned mail icon motif: "+name)
        backdrop,shade,ink=[rgb(x) for x in PALS[ORDER.index(key)]]
        canvas=[[ (0,0,0,0) for _ in range(32)] for _ in range(32)]
        for y in range(2,30):
            for x in range(2,30):
                canvas[y][x]=backdrop
        for i in range(2,30):
            canvas[2][i]=shade
            canvas[29][i]=shade
            canvas[i][2]=shade
            canvas[i][29]=shade
        scale=min(26/width, 26/height, 3.0)
        w=max(1,int(width*scale))
        h=max(1,int(height*scale))
        ox=(32-w)//2
        oy=(32-h)//2
        painted=0
        for dy in range(h):
            for dx in range(w):
                px=src[min(height-1,int(dy/scale))][min(width-1,int(dx/scale))]
                r,g,b,a=px
                if a<96 or r+g+b>500: continue
                canvas[oy+dy][ox+dx]=ink
                painted+=1
        if painted==0:
            raise RuntimeError("Original Crystal mail motif has no visible ink: "+name)
        path=out/("gsc-mail-icon-"+key+".png")
        path.write_bytes(encode(canvas))
        valid,reason,dims=validate_png(path)
        if not valid or tuple(dims)!=(32,32):
            raise RuntimeError("Invalid 32px Crystal mail icon: "+key+": "+reason)
        results[key]="32x32 original-game motif illustration"
    return results


def render_unavailable_exact_mail(keys, out=OUT):
    out.mkdir(parents=True,exist_ok=True)
    rendered={}
    for key in keys:
        target=out/(key+".png")
        payload=assemble(key)
        target.write_bytes(payload)
        valid,reason,dims=validate_png(target)
        if not valid or tuple(dims)!=(160,144):
            raise RuntimeError("Crystal stationery reconstruction invalid for "+key+": "+reason)
        rendered[key]="Crystal-source reconstructed background"
    return rendered
