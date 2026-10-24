#!/usr/bin/env python3
"""Regenerate BW/B2W2 MoveBits rows from pinned PokeAPI CSVs.
PIN: 168b1e89467054cda2e7df43ccebbb69b459497a
usage: python3 tools/gen_gen5_move_compatibility.py /path/to/pinned/data/v2/csv
Prints two 650-row tables; do not network-fetch during application builds.
"""
import csv,pathlib,sys,hashlib
def rows(root,name):
    with (root/name).open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))
def main():
    if len(sys.argv)==4 and sys.argv[2]=="--forms":
        return forms(pathlib.Path(sys.argv[1]),pathlib.Path(sys.argv[3]))
    if len(sys.argv)!=2:raise SystemExit(__doc__)
    root=pathlib.Path(sys.argv[1])
    ids={int(r["id"]):int(r["species_id"]) for r in rows(root,"pokemon.csv")
         if r["is_default"]=="1" and int(r["species_id"])<=649}
    parents={int(r["id"]):int(r["evolves_from_species_id"] or 0)
         for r in rows(root,"pokemon_species.csv") if int(r["id"])<=649}
    pools={g:[set() for _ in range(650)] for g in (11,14)}
    for r in rows(root,"pokemon_moves.csv"):
        species=ids.get(int(r["pokemon_id"]));group=int(r["version_group_id"])
        move=int(r["move_id"]);method=int(r["pokemon_move_method_id"])
        if species and group in pools and 1<=move<=559 and method in (1,2,3,4,6) and not (group==11 and move==555 and method==4):
            pools[group][species].add(move)
    for group in (11,14):
        for species in range(1,650):
            p=parents.get(species,0);seen=set()
            while p and p not in seen and len(seen)<24:
                seen.add(p);pools[group][species].update(pools[group][p])
                p=parents.get(p,0)
    for group,name in ((11,"BlackWhite"),(14,"Black2White2")):
        print("//",name)
        for species in range(650):
            words=[0]*9
            for move in pools[group][species]:words[move//64]|=1<<(move%64)
            print("    MoveBits{{"+", ".join(f"0x{x:016x}ULL" for x in words)+"}}, //",species)

def bits(moves):
    words=[0]*9
    for move in moves:words[move//64]|=1<<(move%64)
    return "MoveBits{{"+", ".join(f"0x{x:016x}ULL" for x in words)+"}}"

def forms(root,pkhex):
    """Additional native-form rows. Existing default-species tables stay reproducible.

    usage: gen_gen5_move_compatibility.py CSV_DIR --forms PKHEX_CHECKOUT
    PKHeX 6042efa241a9925ce57e645329ea7f976deabdd3 supplies exact native
    form counts (not modern regional/mega forms). PokeAPI form_order - 1 maps
    the pre-Gen-VI records to these native forms, including White/Black Kyurem.
    Missing form learnsets are refused, never replaced by another form's pool.
    """
    pokemon={int(r['id']):r for r in rows(root,'pokemon.csv')}
    defaults={int(r['species_id']):int(r['id']) for r in pokemon.values() if r['is_default']=='1'}
    parents={int(r['id']):int(r['evolves_from_species_id'] or 0) for r in rows(root,'pokemon_species.csv')}
    pools={g:{} for g in (8,9,10,11,14)}
    for r in rows(root,'pokemon_moves.csv'):
        group=int(r['version_group_id']);move=int(r['move_id']);p=int(r['pokemon_id'])
        if group in pools and 1<=move<=559 and int(r['pokemon_move_method_id']) in (1,2,3,4,6) and not (group==11 and move==555 and r['pokemon_move_method_id']=='4'):
            pools[group].setdefault(p,set()).add(move)
    personal=[]
    for game,size in (('bw',0x3c),('b2w2',0x4c)):
        path=pkhex/'PKHeX.Core/Resources/byte/personal'/('personal_'+game)
        data=path.read_bytes()
        personal.append([max(1,data[s*size+0x20]) for s in range(650)])
        print('// PKHeX',game,'SHA256',hashlib.sha256(data).hexdigest())
    counts=[(s,personal[0][s],personal[1][s]) for s in range(1,650) if max(personal[0][s],personal[1][s])>1]
    print('inline constexpr std::array<FormCountRow,'+str(len(counts))+'> FormCounts{{')
    for s,bw,b2 in counts:print(f'    {{{s},{bw},{b2}}},')
    print('}};')
    def moves(p,group):
        result=set(pools[group].get(p,()))
        species=int(pokemon[p]['species_id']);parent=parents.get(species,0);seen=set()
        while parent and parent not in seen:
            seen.add(parent);result.update(pools[group].get(defaults[parent],()))
            parent=parents.get(parent,0)
        return result
    mappings={}
    for r in rows(root,'pokemon_forms.csv'):
        p=int(r['pokemon_id']);s=int(pokemon[p]['species_id']);form=int(r['form_order'])-1
        if s>649 or form<=0 or form>=personal[1][s] or p==defaults.get(s):continue
        assert (s,form) not in mappings,(s,form)
        mappings[s,form]=p
    print('inline constexpr std::array<FormMoveRow,'+str(len(mappings))+'> FormMoves{{')
    rotom={1:315,2:56,3:59,4:403,5:437}
    for (s,form),p in sorted(mappings.items()):
        groups=[]
        for group in (11,14):
            pool=moves(p,group)
            if form>=personal[group==14][s]:assert not pool,(s,form,group)
            else:assert pool,(s,form,group)
            if s==479:pool.add(rotom[form]) # PKHeX LearnSource5 enhanced form tutor.
            groups.append(pool)
        old=set().union(*(moves(p,g) for g in (8,9,10))) if s<=493 else set()
        if s==479:old.add(rotom[form])
        print(f'    {{{s},{form},'+', '.join(bits(pool) for pool in [*groups,old])+'}, // '+pokemon[p]['identifier'])
    print('}};')
if __name__=="__main__":main()
