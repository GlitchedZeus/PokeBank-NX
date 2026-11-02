// Owner hardware 2026-11-01 (G69-HW-007): a real Legends: Z-A save reported "Battle teams: Unreadable (box
// changes refused)". PKHeX 6501f0ab TeamIndexes8 reads both team blocks as raw bytes whatever their
// SCTypeCode; the old readers also required an Object / Array<Byte> block. This test pins the shared
// reader (include/Integration/Gen8/Gen8TeamIndexes.h):
//   * every byte-carrying block shape PKHeX accepts is readable (Object, Array<Byte>, Array<Int16>, a single
//     Byte / UInt32 lock value) and yields the same members and lock bits;
//   * it still FAILS CLOSED, with a precise reason, for a missing block, a short index table, a lock block
//     with no data (Bool) and a pointer outside the title's boxes;
//   * end to end on Sword/Shield, Scarlet/Violet and Legends: Z-A saves whose team blocks use the real-save
//     shapes: the save opens, the team table is readable, a team member is never moved or released and a
//     free box Pokemon still moves.
// Evidence class: SYNTHETIC. The exact block shape in the owner's save is not known; the Trainer screen now
// names the reason if the table is still unreadable on hardware.

#include "Integration/Gen8/Gen8TeamIndexes.h"
#include "Integration/Modern/ModernSources.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"
#include "fixtures/gen9za/Gen9ZaSyntheticSave.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace Modern = PokeVault::Integration::Modern;
namespace G8 = PokeVault::Integration::Gen8;
namespace Swish = PokeVault::Integration::Gen8::Swish;
using Modern::SlotRef;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::vector<uint8_t> indexBytes(std::initializer_list<std::pair<size_t, uint16_t>> set) {
    std::vector<uint8_t> idx(72, 0xFF);
    for (const auto& [i, v] : set) {
        idx[2 * i] = static_cast<uint8_t>(v & 0xFF);
        idx[2 * i + 1] = static_cast<uint8_t>(v >> 8);
    }
    return idx;
}

void unitShapes() {
    const auto idx = indexBytes({{0, (0 << 8) | 1}, {7, (3 << 8) | 29}});   // team 1: box 1 slot 2; team 2: box 4 slot 30
    const std::vector<Swish::Block> indexShapes = {
        {0x33F39467, Swish::TypeCode::Object, Swish::TypeCode::None, idx},
        {0x33F39467, Swish::TypeCode::Array, Swish::TypeCode::Byte, idx},
        {0x33F39467, Swish::TypeCode::Array, Swish::TypeCode::Int16, idx},
    };
    const std::vector<Swish::Block> lockShapes = {
        {0x605EBC30, Swish::TypeCode::Object, Swish::TypeCode::None, {0x02, 0, 0, 0}},
        {0x605EBC30, Swish::TypeCode::Byte, Swish::TypeCode::None, {0x02}},
        {0x605EBC30, Swish::TypeCode::UInt32, Swish::TypeCode::None, {0x02, 0, 0, 0}},
    };
    for (const auto& i : indexShapes)
        for (const auto& l : lockShapes) {
            const std::string w = G8::describeBlock(i) + " / " + G8::describeBlock(l);
            const auto t = G8::readTeamIndexes(&i, &l, 32, 30);
            check(t.teams && t.diagnostic.empty(), w + ": readable");
            if (!t.teams) continue;
            check(t.teams->size() == 2, w + ": two members");
            check((*t.teams)[0].team == 0 && (*t.teams)[0].box == 0 && (*t.teams)[0].slot == 1 && !(*t.teams)[0].locked,
                  w + ": team 1 member unlocked");
            check((*t.teams)[1].team == 1 && (*t.teams)[1].box == 3 && (*t.teams)[1].slot == 29 && (*t.teams)[1].locked,
                  w + ": team 2 member locked (bit 1)");
        }

    const Swish::Block good{0x33F39467, Swish::TypeCode::Object, Swish::TypeCode::None, idx};
    const Swish::Block lock{0x605EBC30, Swish::TypeCode::Byte, Swish::TypeCode::None, {0}};
    const auto reason = [](const G8::TeamTable& t, const std::string& part) {
        return !t.teams && t.diagnostic.find(part) != std::string::npos;
    };
    check(reason(G8::readTeamIndexes(nullptr, &lock, 32, 30), "0x33F39467 missing"), "missing index block is unreadable, named");
    check(reason(G8::readTeamIndexes(&good, nullptr, 32, 30), "0x605EBC30 missing"), "missing lock block is unreadable, named");
    const Swish::Block shortIdx{0x33F39467, Swish::TypeCode::Array, Swish::TypeCode::Int16, std::vector<uint8_t>(70, 0xFF)};
    check(reason(G8::readTeamIndexes(&shortIdx, &lock, 32, 30), "Array<Int16>, 70 bytes; 72 bytes needed"),
          "a short index table is unreadable with its shape");
    const Swish::Block boolLock{0x605EBC30, Swish::TypeCode::Bool2, Swish::TypeCode::None, {}};
    check(reason(G8::readTeamIndexes(&good, &boolLock, 32, 30), "Bool2, 0 bytes; no lock byte"),
          "a lock block without data is unreadable with its shape");
    const Swish::Block farIdx{0x33F39467, Swish::TypeCode::Object, Swish::TypeCode::None, indexBytes({{12, (40 << 8) | 2}})};
    const auto far = G8::readTeamIndexes(&farIdx, &lock, 32, 30);
    check(reason(far, "team 3 pointer 0x2802 (box 41, slot 3) is outside 32 x 30"), "an out-of-range pointer names team/box/slot: " + far.diagnostic);
    const Swish::Block farSlot{0x33F39467, Swish::TypeCode::Object, Swish::TypeCode::None, indexBytes({{0, (0 << 8) | 30}})};
    check(!G8::readTeamIndexes(&farSlot, &lock, 32, 30).teams, "slot 31 of 30 is unreadable");
    check(G8::teamFactText(std::nullopt, "x").find("Unreadable: x (box changes refused)") == 0, "fact text carries the reason");
}

void endToEnd(const std::string& id, const std::function<std::vector<uint8_t>(bool, bool)>& make) {
    for (int shape = 0; shape < 4; ++shape) {
        const bool int16Array = shape & 1, byteLock = shape & 2;
        const std::string w = id + (int16Array ? " Array<Int16> indexes" : " Object indexes") + (byteLock ? " + Byte lock" : " + Object lock");
        const auto bytes = make(int16Array, byteLock);
        auto opened = Modern::openSession(id, bytes);
        check(opened.ready(), w + ": opens: " + opened.diagnostic);
        if (!opened.ready()) continue;
        auto& s = *opened.session;
        bool readable = false;
        for (const auto& f : s.trainerFacts())
            if (f.label == "Battle teams") readable = f.value.find("box references") != std::string::npos;
        check(readable, w + ": Battle teams readable on the Trainer screen");
        const auto member = SlotRef::boxed(0, 1), free = SlotRef::boxed(0, 0);
        check(!s.moveSupport(member).available && !s.removeSupport(member).available, w + ": team member never moved/released");
        std::string error;
        const auto dest = s.firstEmptyBoxSlot();
        check(s.moveSupport(free).available && dest && s.move(free, *dest, error), w + ": a free box Pokemon moves: " + error);
    }
}
} // namespace

int main() {
    unitShapes();
    endToEnd("sword_switch", [](bool a, bool b) {
        Gen8Fixture::Options o;
        o.teamIndexesInt16Array = a;
        o.teamLocksByteValue = b;
        return Gen8Fixture::makeSave(o);
    });
    endToEnd("scarlet_switch", [](bool a, bool b) {
        Gen9SvFixture::Options o;
        o.teamIndexesInt16Array = a;
        o.teamLocksByteValue = b;
        return Gen9SvFixture::makeSave(o);
    });
    endToEnd("legends_za_switch", [](bool a, bool b) {
        Gen9ZaFixture::Options o;
        o.teamIndexesInt16Array = a;
        o.teamLocksByteValue = b;
        return Gen9ZaFixture::makeSave(o);
    });
    std::cout << "test_modern_team_indexes: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
