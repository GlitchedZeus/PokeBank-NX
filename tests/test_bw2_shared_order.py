#!/usr/bin/env python3
"""A violated necessary condition may explain a disagreement; IDs cannot."""
import contextlib
import io
import os
import pathlib
import random
import sys
import tempfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/legality_completion'))
sys.path.insert(0,str(ROOT/'tools/pkhex_gen34_diff'))
from bw2_shared_order import necessary_order_proof, index
import hgss_placeholders as gate

os.environ.pop('CURRENT_RELEASE_REPLAY_REPORT',None)
records={line.split()[0]:bytes.fromhex(line.split()[2]) for line in
         (ROOT/'tests/fixtures/gen15_session2/gen5_joint_breeding.txt').read_text().splitlines()}
good=records['pidgey-native-shared-order']
bad=records['pidgey-reversed-shared-order']
enc='EncounterEgg5:16'

def checksum(raw):
    raw[6:8]=(sum(int.from_bytes(raw[i:i+2],'little') for i in range(8,136,2))&65535).to_bytes(2,'little')
    return bytes(raw)

assert necessary_order_proof(bad.hex(),enc)['basis']=='native-bw2-shared-learnset-order'
assert necessary_order_proof(good.hex(),enc) is None
assert necessary_order_proof(bad.hex(),'EncounterGift5:16') is None
assert necessary_order_proof('gg'*136,enc) is None
assert necessary_order_proof(bad[:135].hex(),enc) is None
# Do not generalize evidence to BW, event/form/traded/hatched eggs, or damaged input.
for offset,value in ((0x17,1),(0x17,8),(0x5f,21),(0x40,1),(0x40,8),(0x7e,0),(0x80,1),(0x3b,bad[0x3b]&~64)):
    changed=bytearray(bad);changed[offset]=value
    assert necessary_order_proof(checksum(changed).hex(),enc) is None,(offset,value)
damaged=bytearray(bad);damaged[6]^=1
assert necessary_order_proof(damaged.hex(),enc) is None
# Simulated false Invalid on a real positive still fails the comparison gate.
# The negative's explanation is unchanged when its record ID changes.
with tempfile.TemporaryDirectory() as temp:
    path=pathlib.Path(temp)/'joined.txt'
    path.write_text(''.join(f'{name} {raw.hex()} VALID {enc} ; original\t{name} INVALID ; E: move order\n'
                            for name,raw in [('positive',good),('renamed-negative',bad)]))
    out,err=io.StringIO(),io.StringIO()
    with contextlib.redirect_stdout(out),contextlib.redirect_stderr(err):gate.gen5(path)
    assert out.getvalue().startswith('positive ') and 'renamed-negative' not in out.getvalue()
    assert 'native-bw2-shared-learnset-order): 1' in err.getvalue()

# Independently construct legal forward insertions from all indexed species.
# Every actual child order must survive, including alternate-source and overflow
# cases; the checker cannot simply exempt all eggs or reject reversed numbers.
rng=random.Random(2026102866)
for _ in range(5000):
    species=rng.randrange(1,650);entry=index()[str(species)]
    pool=list(dict.fromkeys(entry['levels']+entry['egg']+entry['machines']))
    dad=rng.sample(pool,min(len(pool),rng.randrange(1,5)))
    mom=rng.sample(pool,min(len(pool),rng.randrange(1,5)))
    result=[]
    def append(move):
        if move not in result:
            if len(result)==4:result.pop(0)
            result.append(move)
    for move in entry['initial']:append(move)
    for move in dad:
        if move in entry['egg']:append(move)
    for move in entry['levels']:
        if move in dad and move in mom:append(move)
    for move in dad:
        if move in entry['machines']:append(move)
    raw=bytearray(good);raw[8:10]=species.to_bytes(2,'little')
    for i,move in enumerate((result+[0]*4)[:4]):raw[0x28+2*i:0x2a+2*i]=move.to_bytes(2,'little')
    assert necessary_order_proof(checksum(raw).hex(),f'EncounterEgg5:{species}') is None,(species,result)
print('BW2 necessary-order proof: native pair, positive/identity/domain/checksum guards and 5000 forward births PASS')
