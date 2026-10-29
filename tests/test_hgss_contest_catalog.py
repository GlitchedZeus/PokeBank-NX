#!/usr/bin/env python3
"""Protect real ordinary/contest routes while removing the inactive bank."""
import argparse
import pathlib
import re
import sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/legality_completion'))
from hgss_contest_repair import checked_native,native_bank,repaired
from hgss_swarm_repair import parse_array,render

p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True)
p.add_argument('--fetch',action='store_true');a=p.parse_args()
checked_native(a.native,a.fetch)
original=(ROOT/'src/Legality/Gen4LegalityData.cpp').read_text()
canonical,removed,_,_=repaired(original,a.native)
assert not removed and canonical==original,'Inactive contest bank remains or projection is not canonical'
bank,location,rate,fingerprints=native_bank(a.native)
areas=parse_array(original,'Area4 kAreas[]');slots=parse_array(original,'Slot4 kSlots[]')
ordinary=[row for row in areas if row[0] in (7,8) and row[1]==location and row[2]==0]
contest=[row for row in areas if row[0] in (7,8) and row[1]==location and row[2]==8]
assert len(ordinary)==2 and len(contest)==8

def with_placeholders(damage=False,duplicate=False):
    new_areas=[row[:] for row in areas];new_slots=[row[:] for row in slots]
    for version in [7,8]+([7] if duplicate else []):
        extra=[]
        for signature,count in fingerprints[version].items():
            extra.extend([list(signature)+[0,0,0,0] for _ in range(count)])
        if damage and version==7:extra[0][0]=493
        new_areas.append([version,location,0,rate,4,len(new_slots),len(extra)])
        new_slots.extend(extra)
    text=original
    for signature,rows in [('Area4 kAreas[]',new_areas),('Slot4 kSlots[]',new_slots)]:
        text,n=re.subn(r'const '+re.escape(signature)+r'\s*=\s*\{.*?\n\};',lambda _:render(signature,rows),text,count=1,flags=re.S)
        assert n==1
    return text

restored,removed,_,_=repaired(with_placeholders(),a.native)
assert restored==original and len(removed)==2
for adversarial in [with_placeholders(damage=True),with_placeholders(duplicate=True)]:
    try:repaired(adversarial,a.native)
    except AssertionError:pass
    else:raise AssertionError('Unexpected one-sided/duplicate bank drift must not be silently repaired')
print('HGSS contest catalog:2 ordinary and8 real contest areas protected;exact native bank removal,idempotence and unreviewed drift guards PASS')
