#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/LocationNames.h"
#include <cassert>
#include <iostream>
#include <set>
#include <tuple>

namespace C = PokeVault::Integration::EncounterContext45;
namespace Policy = PokeVault::Integration::MetLocationPolicy;
using PokeVault::Integration::Gen5::NativeMetLocations5;

int main() {
    const auto name = [](uint16_t species) { return std::to_string(species); };
    size_t rows = 0;
    std::set<std::string_view> games;
    for (const auto& row : C::kWild45) {
        ++rows;
        games.insert(row.sourceGameId);
        assert(row.species && row.species <= 649 && row.minLevel &&
               row.minLevel <= row.maxLevel && row.maxLevel <= 100);
    }
    assert(rows == 16837 && games.size() == 9);
    for (const uint8_t origin : {7,8,10,11,12,20,21,22,23}) {
        const auto gen = C::generation(origin);
        const auto pikachu = C::ancestry(gen, 25);
        assert(pikachu[25] && pikachu[172] && !pikachu[26]);
        const auto walrein = C::ancestry(gen, 365);
        assert(walrein[365] && walrein[364] && walrein[363] && !walrein[362]);
        // Actual native choices, unchanged by filtering. A stored unusual id is still selectable.
        std::vector<uint16_t> native;
        if (gen == 5) native.assign(NativeMetLocations5.begin(), NativeMetLocations5.end());
        else {
            const auto table = Names::getLocationTable(origin);
            for (size_t i=0; i<table.count; ++i)
                if (table.names[i] && table.names[i][0] &&
                    Names::isGen4NativeMetLocation(origin, static_cast<uint16_t>(i)))
                    native.push_back(static_cast<uint16_t>(i));
        }
        const auto before = native;
        for (const uint16_t species : {25,365}) {
            C::Catalogue context(origin, species, 65534, native, {});
            assert(native == before);
            assert(context.choices().front() == 65534 && context.choices(true).front() == 65534);
            assert(context.choices().size() < context.choices(true).size());
            // Round 12 (R12-004; updated deliberately): the default list (verified wild rows + the
            // trade / egg markers + "Other location (unverified)...") and the unverified list share
            // only the stored row 1; together they still offer every native id exactly once.
            assert(context.choices().back() == Policy::kUnverifiedListRow);
            std::set<uint16_t> distinct(context.choices(true).begin(), context.choices(true).end());
            assert(distinct.size() == context.choices(true).size());
            for (const auto id : context.choices()) {
                if (id == 65534 || id == Policy::kUnverifiedListRow) continue;
                assert(!distinct.count(id));
                distinct.insert(id);
            }
            assert(distinct.size() == native.size() + 1);
            for (const auto id : native) assert(distinct.count(id));
            for (const auto id : context.choices()) {
                if (id == 65534 || id == Policy::kUnverifiedListRow) continue;
                if (Policy::gen45DefaultMarker(gen, id)) {
                    assert(context.detail(id, name).find("Lv.") == std::string::npos);
                    continue;
                }
                const auto* row = context.wildRow(id);
                assert(row && row->levelReached && C::ancestry(gen, species)[row->encounter.species]);
                const auto text = context.detail(id, name);
                assert(text.find("Lv.") != std::string::npos && text.find("legal") == std::string::npos);
            }
            assert(context.detail(65534,name).find("unverified") != std::string::npos);
            std::cout << unsigned(origin) << " species " << species << ": "
                      << context.choices().size() << " context rows, " << native.size() << " native histories\n";
        }
    }
    // Real pre-evolution data: Walrein can carry an encounter obtained as Spheal.
    std::vector<uint16_t> all;
    for (uint16_t id=0; id<234; ++id) all.push_back(id);
    C::Catalogue walrein(11,365,65534,all,{});
    bool spheal = false;
    for (const auto id : walrein.choices())
        if (const auto* row = walrein.wildRow(id); row && row->encounter.species == 363) {
            assert(walrein.detail(id,name).find("via 363") != std::string::npos); spheal=true;
        }
    assert(spheal);
    // Eggs, hatched Pokémon, trades, events, gifts/static and transfers never become an
    // invalid verdict merely because this species-level wild subset lacks their history.
    for (const uint8_t origin : {10,21}) {
        const uint16_t current = 4;
        for (const C::History history : {C::History{0,true,false}, C::History{2000,false,false},
                                        C::History{0,false,true}}) {
            C::Catalogue context(origin,25,current,all,history);
            assert(context.choices().front() == current);
            assert(context.detail(current,name).find("unverified") != std::string::npos);
            assert(context.detail(current,name).find("Lv.") == std::string::npos);
        }
    }
    for (const auto& [origin, marker, kind] : {
            std::tuple<uint8_t,uint16_t,const char*>{10,55,"Transfer"}, {21,30001,"Transfer"},
            {21,30012,"Transfer"}, {10,2002,"Trade"}, {21,30003,"Trade"},
            {10,2001,"NPC"}, {21,30002,"NPC / egg"}, {10,2000,"Day-Care"},
            {21,60002,"Day-Care"}, {10,3001,"Event"}, {21,40001,"Event"}}) {
        const C::Catalogue context(origin,25,marker,all,{});
        assert(context.detail(marker,name).find(kind) != std::string::npos);
        assert(context.choices().front() == marker);
    }
    // Round 12 (R12-005): the owner's screenshots. A Lv. 5 Black 2 Nidorino is not offered the
    // Hidden Grotto Lv. 55-60 slot (via Nidoran) as verified, and a Lv. 5 Diamond Pidgeotto is not
    // offered Route 229 Lv. 51 (via Pidgey); both stay reachable in the unverified list, saying why.
    for (const auto& [origin, species, levelLow, levelHigh] :
         {std::tuple<uint8_t,uint16_t,uint8_t,uint8_t>{23,33,5,60}, {10,17,5,60}}) {
        std::vector<uint16_t> native;
        if (C::generation(origin) == 5) native.assign(NativeMetLocations5.begin(), NativeMetLocations5.end());
        else for (uint16_t id=0; id<=111; ++id) native.push_back(id);
        const C::Catalogue low(origin, species, native.front(), native, {}, levelLow);
        const C::Catalogue high(origin, species, native.front(), native, {}, levelHigh);
        size_t moved = 0;
        for (const auto id : high.choices()) {
            const auto* row = low.wildRow(id);
            if (!row || id == native.front() || low.verifiedEncounter(id)) continue;
            ++moved;
            // Not in the low-level default list; in its unverified list with the reason.
            assert(std::find(low.choices().begin(), low.choices().end(), id) == low.choices().end());
            assert(std::find(low.choices(true).begin(), low.choices(true).end(), id) != low.choices(true).end());
            assert(low.detail(id, name).find(Policy::kLevelBelowEncounter) != std::string::npos);
            assert(row->encounter.minLevel > levelLow);
        }
        assert(moved > 0);
        for (const auto id : low.choices())
            if (const auto* row = low.wildRow(id); row && id != native.front())
                assert(row->levelReached && low.detail(id, name).find(Policy::kLevelBelowEncounter) == std::string::npos);
        std::cout << unsigned(origin) << " species " << species << " Lv. " << unsigned(levelLow) << ": "
                  << moved << " wild rows above the level moved to the unverified list\n";
    }
    // The default list is never empty: a species with no wild row shows the stored row, the native
    // markers and the entry row, and says so in its header.
    {
        const C::Catalogue turtwig(10, 387, 0, all, {}, 5);
        assert(turtwig.verifiedCount() == 0 && turtwig.choices().back() == Policy::kUnverifiedListRow &&
               std::string(turtwig.scope(false)) == Policy::kMarkersOnlyScope);
        const C::Catalogue tepig(21, 498, 0, std::vector<uint16_t>(NativeMetLocations5.begin(), NativeMetLocations5.end()),
                                 {}, 5);
        assert(std::find(tepig.choices().begin(), tepig.choices().end(), 30002) != tepig.choices().end());
        assert(std::find(tepig.choices().begin(), tepig.choices().end(), 60003) != tepig.choices().end());
        assert(std::find(tepig.choices().begin(), tepig.choices().end(), 40001) == tepig.choices().end());
        assert(std::string(tepig.detail(Policy::kUnverifiedListRow, name)) == Policy::kUnverifiedListRowDetail);
        assert(C::unverifiedMetNote45(21, 498, 30002, 5).find("unverified") != std::string::npos);
        assert(C::unverifiedMetNote45(21, 498, 4, 5) == "Not a listed encounter; unverified");
    }
    assert(C::gameId(77).empty() && C::generation(1)==0); // group ids are not PKM origins
    assert(!C::ancestry(4,649)[649] && !C::ancestry(0,25)[25]);
    std::cout << "Gen IV/V location context: all nine titles, ancestry, native histories and unknown evidence PASS\n";
}
