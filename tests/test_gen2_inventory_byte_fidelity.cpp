// Q07 (independent QA 2026-10-27): Gen II Bag edits change only what the game's own list routines
// change (pret pokecrystal 5c877ef5, engine/items/items.asm), through both staged writers:
// StagedEditor::stageItemQuantity and stageInventoryQuantity. A same-value edit or a revert is a
// byte-for-byte no-op, and stale bytes after a list's 0xFF terminator are never zero-filled.
#include "Integration/Gen2/Gen2StagedEditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {
struct L {
    VersionFamily family;
    std::size_t money, idx, names, party, current, end, sum, sum2, gender;
    std::size_t tm, items, key, balls, pc;
};
constexpr std::size_t N = static_cast<std::size_t>(-1);
constexpr L GS{VersionFamily::GoldSilver,0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,0x7E6D,N,
               0x23E6,0x241F,0x2449,0x2464,0x247E};
constexpr L C {VersionFamily::Crystal,0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,0x1F0D,0x3E3D,
               0x23E7,0x2420,0x244A,0x2465,0x247F};

std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(){return listLen(20,32,11)+2;}
std::size_t boxStart(std::size_t b){return b<7?0x4000+b*stride():0x6000+(b-7)*stride();}
void emptyList(std::vector<uint8_t>& d,std::size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void name(std::vector<uint8_t>& d,std::size_t ofs,std::size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),len,0x50);
    d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,std::size_t ofs,std::size_t cap,std::size_t bodySize,std::size_t str,bool party){
    constexpr uint8_t species=25;
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const std::size_t body=ofs+1+(cap+1);
    d[body]=species;d[body+1]=1;d[body+2]=33;d[body+3]=45;
    d[body+6]=0x12;d[body+7]=0x34;d[body+9]=0x10;
    for(int i=0;i<5;i++){d[body+11+2*i]=0;d[body+12+2*i]=static_cast<uint8_t>(i+1);}
    d[body+21]=0x7A;d[body+22]=0xAA;d[body+23]=10;d[body+24]=20;
    d[body+27]=123;d[body+28]=0x21;d[body+31]=15;
    if(party){
        auto be=[&](std::size_t o,uint16_t v){d[body+o]=static_cast<uint8_t>(v>>8);d[body+o+1]=static_cast<uint8_t>(v);};
        d[body+32]=0x08;be(34,35);be(36,40);be(38,30);be(40,20);be(42,25);be(44,26);be(46,27);
    }
    const std::size_t ot=body+cap*bodySize;
    const std::size_t nick=ot+cap*str;
    name(d,ot,str,'O');name(d,nick,str,'M');
}
void pairPocket(std::vector<uint8_t>& d,std::size_t ofs,std::initializer_list<std::pair<uint8_t,uint8_t>> entries){
    d[ofs]=static_cast<uint8_t>(entries.size());
    std::size_t p=ofs+1;
    for(const auto& [id,qty]:entries){d[p++]=id;d[p++]=qty;}
    d[p]=0xFF;
}
void inventory(std::vector<uint8_t>& d,const L& l){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(l.tm),57,0);
    pairPocket(d,l.items,{{kPotionItemId,2}});
    d[l.key]=0;d[l.key+1]=0xFF;
    pairPocket(d,l.balls,{{kPokeBallItemId,5}});
    d[l.pc]=0;d[l.pc+1]=0xFF;
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l){
    std::vector<uint8_t>d(0x8000);
    d[0x2009]=0x12;d[0x200A]=0x34;name(d,0x200B,11,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;
    d[l.idx]=0;if(l.gender!=N)d[l.gender]=1;
    for(std::size_t b=0;b<14;b++){emptyList(d,boxStart(b));name(d,l.names+b*9,9,static_cast<char>('A'+b));}
    pokemonList(d,boxStart(0),20,32,11,false);
    emptyList(d,l.current);pokemonList(d,l.party,6,48,11,true);
    inventory(d,l);checksum(d,l);
    return d;
}

// Distinctive stale bytes after the Items and Key Items terminators, as a played save carries.
std::vector<uint8_t> staleFixture(const L& l) {
    auto d = fixture(l);
    for (std::size_t i = l.items + 4; i < l.items + 1 + 2 * 20 + 1; ++i) d[i] = 0xAB;
    d[l.key] = 1; d[l.key + 1] = 7; d[l.key + 2] = 0xFF;  // Key Items: one entry (Bicycle)
    for (std::size_t i = l.key + 3; i < l.key + 1 + 25 + 1; ++i) d[i] = 0xCD;
    checksum(d, l);
    return d;
}

std::vector<std::size_t> diff(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < a.size(); ++i) if (a[i] != b[i]) out.push_back(i);
    return out;
}
void assertWithin(std::span<const uint8_t> a, std::span<const uint8_t> b, std::size_t lo, std::size_t hi) {
    for (const auto at : diff(a, b)) {
        if (at < lo || at >= hi) { std::cerr << "unexpected change at 0x" << std::hex << at << "\n"; assert(false); }
    }
}
std::vector<uint8_t> copyOf(std::span<const uint8_t> s) { return {s.begin(), s.end()}; }

using Stage = bool (StagedEditor::*)(InventoryPocket, uint8_t, uint8_t, std::string&);

void pairList(const L& l, SourceGame game, Stage stage) {
    const auto source = staleFixture(l);
    auto parsed = parse(source, game);
    assert(parsed);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor && error.empty());
    auto& e = *editor;

    // Same quantity, and removing an absent item: byte-identical, nothing pending.
    assert((e.*stage)(InventoryPocket::Items, kPotionItemId, 2, error));
    assert(copyOf(e.stagedBytes()) == source && !e.hasPendingChanges());
    assert((e.*stage)(InventoryPocket::Items, 0x0A, 0, error));  // Burn Heal: absent
    assert(copyOf(e.stagedBytes()) == source && !e.hasPendingChanges());

    // Quantity change: exactly one byte, the entry's quantity.
    assert((e.*stage)(InventoryPocket::Items, kPotionItemId, 7, error));
    auto d = diff(source, e.stagedBytes());
    assert(d.size() == 1 && d[0] == l.items + 2);
    // Repeat: no further change. Revert: identical, nothing pending.
    assert((e.*stage)(InventoryPocket::Items, kPotionItemId, 7, error));
    assert(diff(source, e.stagedBytes()).size() == 1);
    assert((e.*stage)(InventoryPocket::Items, kPotionItemId, 2, error));
    assert(copyOf(e.stagedBytes()) == source && !e.hasPendingChanges());

    // Add: entry at the old terminator, new terminator after it, count + 1. Nothing past that.
    const uint8_t antidote = 0x09;
    assert((e.*stage)(InventoryPocket::Items, antidote, 3, error));
    const auto added = copyOf(e.stagedBytes());
    assertWithin(source, added, l.items, l.items + 1 + 2 * 2 + 1);
    assert(added[l.items] == 2 && added[l.items + 3] == antidote && added[l.items + 4] == 3 &&
           added[l.items + 5] == 0xFF);
    assert(added[l.items + 6] == 0xAB);  // stale tail untouched

    // Remove the first entry: the following entry and terminator shift down; the stale bytes
    // beyond the new terminator are kept, exactly as RemoveItemFromPocket leaves them.
    assert((e.*stage)(InventoryPocket::Items, kPotionItemId, 0, error));
    const auto removed = copyOf(e.stagedBytes());
    assertWithin(source, removed, l.items, l.items + 1 + 2 * 2 + 1);
    assert(removed[l.items] == 1 && removed[l.items + 1] == antidote && removed[l.items + 2] == 3 &&
           removed[l.items + 3] == 0xFF);
    // Stale, as RemoveItemFromPocket leaves it: the old last quantity and the old terminator.
    assert(removed[l.items + 4] == 3 && removed[l.items + 5] == 0xFF);
    assert(removed[l.items + 6] == 0xAB);

    // Serialize -> strict reparse -> same-value edit on the reparsed copy is a no-op.
    const auto out = e.finalizedBytes(error);
    assert(!out.empty() && error.empty());
    auto reparsed = parse(out, game);
    assert(reparsed);
    auto again = StagedEditor::create(*reparsed.save, error);
    assert(again);
    assert(((*again).*stage)(InventoryPocket::Items, antidote, 3, error));
    assert(copyOf(again->stagedBytes()) == out);
    assert(parse(source, game));  // the source buffer was never written
}

void keyList(const L& l, SourceGame game) {
    const auto source = staleFixture(l);
    auto parsed = parse(source, game);
    assert(parsed);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor);
    // Present key item set "to 1" again: nothing changes.
    assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems, 7, 1, error));
    assert(copyOf(editor->stagedBytes()) == source && !editor->hasPendingChanges());
    // Add one: count, entry, terminator only; stale tail kept.
    assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems, 54, 1, error));
    const auto added = copyOf(editor->stagedBytes());
    assertWithin(source, added, l.key, l.key + 4);
    assert(added[l.key] == 2 && added[l.key + 2] == 54 && added[l.key + 3] == 0xFF && added[l.key + 4] == 0xCD);
    // Remove the first: shift down, stale byte after the new terminator kept.
    assert(editor->stageInventoryQuantity(InventoryPocket::KeyItems, 7, 0, error));
    const auto removed = copyOf(editor->stagedBytes());
    assert(removed[l.key] == 1 && removed[l.key + 1] == 54 && removed[l.key + 2] == 0xFF);
    assert(removed[l.key + 3] == 0xFF && removed[l.key + 4] == 0xCD);
    // The Balls list right after Key Items is untouched throughout.
    assert(std::equal(source.begin() + static_cast<std::ptrdiff_t>(l.balls),
                      source.begin() + static_cast<std::ptrdiff_t>(l.balls + 4), removed.begin() + static_cast<std::ptrdiff_t>(l.balls)));
}
}  // namespace

int main() {
    pairList(GS, SourceGame::Gold, &StagedEditor::stageItemQuantity);
    pairList(GS, SourceGame::Gold, &StagedEditor::stageInventoryQuantity);
    pairList(C, SourceGame::Crystal, &StagedEditor::stageItemQuantity);
    pairList(C, SourceGame::Crystal, &StagedEditor::stageInventoryQuantity);
    keyList(GS, SourceGame::Gold);
    keyList(C, SourceGame::Crystal);
    std::cout << "Gen II inventory byte fidelity (both writers; no-op, 1-byte quantity, native add/remove): PASS\n";
}
