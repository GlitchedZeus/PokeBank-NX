#!/usr/bin/env python3
from pathlib import Path

p = Path("tests/test_rby_readonly_bridge.cpp")
text = p.read_text()
old = '''    assert(trainer->items.size()==2);assert(trainer->items[0].size()==2);assert(trainer->items[1].size()==1);\n    assert(trainer->items[0][0].itemId==0x14&&trainer->items[0][0].count==3);\n    assert(trainer->items[0][1].itemId==0xC9&&trainer->items[0][1].count==2);\n    assert(trainer->items[1][0].itemId==0x2D&&trainer->items[1][0].count==1);\n'''
new = '''    // RBY still stores one real Bag plus PC Items, but the accepted classic inventory UI now\n    // presents six filtered views over those same bytes: Items, Medicines, Balls, Key Items,\n    // TM/HM, PC Items. This verifies presentation only; it does not fabricate new save pockets.\n    assert(trainer->items.size()==6);\n    assert(trainer->items[0].empty());\n    assert(trainer->items[1].size()==1);\n    assert(trainer->items[2].empty());\n    assert(trainer->items[3].empty());\n    assert(trainer->items[4].size()==1);\n    assert(trainer->items[5].size()==1);\n    assert(trainer->items[1][0].itemId==0x14&&trainer->items[1][0].count==3);\n    assert(trainer->items[4][0].itemId==0xC9&&trainer->items[4][0].count==2);\n    assert(trainer->items[5][0].itemId==0x2D&&trainer->items[5][0].count==1);\n'''
if old not in text:
    raise SystemExit("primary RBY inventory expectation anchor missing")
text = text.replace(old, new, 1)
old = '''        assert(t&&e.empty()&&t->sourceGameId()==id&&t->items.size()==2);\n        assert(t->items[0][0].itemId==0x14&&t->items[1][0].itemId==0x2D);\n'''
new = '''        assert(t&&e.empty()&&t->sourceGameId()==id&&t->items.size()==6);\n        assert(t->items[1][0].itemId==0x14&&t->items[4][0].itemId==0xC9&&t->items[5][0].itemId==0x2D);\n'''
if old not in text:
    raise SystemExit("Blue RBY inventory expectation anchor missing")
text = text.replace(old, new, 1)
old = '''    assert(yt&&ye.empty()&&yt->sourceGameId()=="yellow_gb"&&yt->items.size()==2);\n    assert(yt->items[0][0].itemId==0x14&&yt->items[1][0].itemId==0x2D);\n'''
new = '''    assert(yt&&ye.empty()&&yt->sourceGameId()=="yellow_gb"&&yt->items.size()==6);\n    assert(yt->items[1][0].itemId==0x14&&yt->items[4][0].itemId==0xC9&&yt->items[5][0].itemId==0x2D);\n'''
if old not in text:
    raise SystemExit("Yellow RBY inventory expectation anchor missing")
text = text.replace(old, new, 1)
p.write_text(text)
