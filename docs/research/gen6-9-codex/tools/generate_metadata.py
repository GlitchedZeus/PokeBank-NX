#!/usr/bin/env python3
"""Extract factual constants and source hashes; never copies executable upstream code.
Only a small arithmetic grammar is accepted. A changed revision fails closed.
This is a source index, not an independently verified save specification.
"""
import argparse, ast, hashlib, json, pathlib, re, subprocess
PIN='641c26ba100225f3605e5a44b61b3b16e15a3774'
def arithmetic(expr, values):
    def visit(n):
        if isinstance(n,ast.Constant) and type(n.value) is int:return n.value
        if isinstance(n,ast.Name):return values[n.id]
        if isinstance(n,ast.BinOp) and isinstance(n.op,(ast.Add,ast.Sub,ast.Mult,ast.FloorDiv)):
            a,b=visit(n.left),visit(n.right)
            return {ast.Add:lambda:a+b,ast.Sub:lambda:a-b,ast.Mult:lambda:a*b,ast.FloorDiv:lambda:a//b}[type(n.op)]()
        raise ValueError('unsupported expression')
    return visit(ast.parse(expr.replace('/','//'),mode='eval').body)
def generate(root):
    sha=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
    if sha!=PIN:raise ValueError('PKHeX revision mismatch: '+sha)
    core=root/'PKHeX.Core'; sources={}
    def read(rel):
        p=core/rel; b=p.read_bytes();sources[rel]={'sha256':hashlib.sha256(b).hexdigest(),'url':f'https://github.com/kwsch/PKHeX/blob/{sha}/PKHeX.Core/{rel}'};return b.decode('utf-8-sig')
    sizes=[]; vals={}; pending=[]
    for i,line in enumerate(read('Saves/Util/SaveUtil.cs').splitlines(),1):
        m=re.search(r'const int (SIZE_G[6789]\w*)\s*=\s*([^;]+);',line)
        if m:pending.append((m[1],m[2],i))
    while pending:
        rest=[]
        for name,expr,line in pending:
            try: val=arithmetic(expr,vals)
            except (KeyError,ValueError):rest.append((name,expr,line));continue
            vals[name]=val;sizes.append({'symbol':name,'value':val,'hex':hex(val),'line':line,'source':'Saves/Util/SaveUtil.cs'})
        if len(rest)==len(pending):raise ValueError('Unresolved constants: '+str(rest))
        pending=rest
    tables={}
    for fam in ['6XY','6AO','7SM','7USUM','7b']:
        rel=f'Saves/Access/SaveBlockAccessor{fam}.cs';data=read(rel);blocks=[]
        for i,line in enumerate(data.splitlines(),1):
            m=re.search(r'new\(\w+,\s*(\d+),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)\)',line)
            if m:blocks.append({'id':int(m[1]),'offset':int(m[2],16),'length':int(m[3],16),'line':i})
        if not blocks:raise ValueError('No blocks: '+fam)
        tables[fam]={'source':rel,'blocks':blocks}
    keys={}
    for fam in ['8SWSH','8LA','9SV','9ZA']:
        rel=f'Saves/Access/SaveBlockAccessor{fam}.cs';data=read(rel);rows=[]
        for i,line in enumerate(data.splitlines(),1):
            m=re.search(r'const uint (K\w+)\s*=\s*(0x[0-9a-fA-F]+);',line)
            if m and re.search(r'Box|Party|Item|Status|Pokedex|Zukan|Team|Revision|Money|Time|Raid|Fused|Current',m[1]):rows.append({'symbol':m[1],'value':int(m[2],16),'line':i})
        keys[fam]={'source':rel,'keys':rows}
    bag_pouches={}
    for fam in ['6XY','6AO','7SM','7USUM','7b','8']:
        rel=f'Items/Bags/PlayerBag{fam}.cs';rows=[]
        for i,line in enumerate(read(rel).splitlines(),1):
            m=re.search(r'new\((0x[0-9A-Fa-f]+),\s*(\d+),\s*(?:(\d+),\s*)?info,\s*(\w+)\)',line)
            if m:rows.append({'pocket':m[4],'offset':int(m[1],16),'slots':int(m[2]) if m[3] else None,'default_max':int(m[3] or m[2]),'line':i})
        bag_pouches[fam]={'source':rel,'pouches':rows,'warning':'Defaults only: per-item exceptions and legal membership must also be applied.'}
    for fam,storage in [('8b','8BDSP'),('9','9SV'),('9a','9ZA')]:
        rel=f'Items/ItemStorage{storage}.cs';data=read(rel)
        part=data.split('public int GetMax(',1)[1].split('};',1)[0]
        caps={m[1]:int(m[2].replace('_','')) for m in re.finditer(r'InventoryType\.(\w+)\s*=>\s*([\d_]+)',part)}
        bag_pouches[fam]={'source':rel,'default_caps':caps,'warning':'Defaults only; PlayerBag overrides and legal membership required.'}
    fields={}
    for record in ['PK6','PK7','PB7','PK8','PB8','PA8','PK9','PA9']:
        rel=f'PKM/{record}.cs';data=read(rel);rows=[]
        for i,line in enumerate(data.splitlines(),1):
            m=re.search(r'public (?:override )?(?:\w+) (\w+)\s*\{ get =>.*?Data(?:\[|\.Slice\()(0x[0-9a-fA-F]+)',line)
            if m:rows.append({'property':m[1],'first_referenced_offset':int(m[2],16),'line':i})
        fields[record]={'source':rel,'warning':'Navigation index only: offsets can share bits; consult accessor for width/mask/encoding. Missing inherited fields are not absent.','properties':rows}
    # Hash a broad source inventory without importing game assets or data tables.
    for p in sorted(core.rglob('*')):
        if not p.is_file() or p.suffix!='.cs':continue
        rel=p.relative_to(core).as_posix()
        if any(s in rel for s in ['Legality/Verifiers/','Legality/Encounters/','Legality/LearnSource/','Legality/Breeding/','Legality/Evolutions/','PersonalInfo','PersonalTable','Items/Bags/PlayerBag','Items/ItemStorage','Saves/Encryption/','Saves/Blocks/','Saves/Substructures/Inventory/','PKM/Shared/','Saves/Substructures/Gen6/','Saves/Substructures/Gen7/','Saves/Substructures/Gen8/','Saves/Substructures/Gen9/']) or p.name in ['PokeCrypto.cs','Checksums.cs','GameVersion.cs','Experience.cs'] or re.fullmatch(r'SAV[6789].*\.cs',p.name):read(rel)
    return {'schema':1,'confidence':'Verified in PKHeX source only; not retail acceptance','repository':'kwsch/PKHeX','commit':sha,'save_sizes':sorted(sizes,key=lambda x:x['symbol']),'block_tables':tables,'sc_keys':keys,'record_field_index':fields,'bag_pouches':bag_pouches,'sources':sources}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('pkhex',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args();a.output.write_text(json.dumps(generate(a.pkhex),indent=2)+'\n')
