#pragma once

// Round 11 (owner H25 / safety clarification 2): each generation's facts for the pure
// HistoryConsistency check, built only from data already in the repository:
//  - Gen III: the record's origin game's native list (Names::isGen3NativeMetLocation) and its catch
//    templates with R2 evolution provenance (EncounterGuardrails::forGameSpeciesWithGen3Provenance);
//  - Gen IV / V: the origin game's bank-0 native list (Gen IV: isGen4NativeMetLocation; Gen V: the
//    origin's own name table), the special / trade / egg / event markers (SpecialMetLocationNotes)
//    and the wild catalogue with ancestry (EncounterContext45);
//  - Crystal: the native landmark list (ExactGameMetLocations) and the Crystal catch templates
//    with ancestry (EncounterContext45 Gen IV evolution edges cover every Gen I / II family).
// Gen I has no met data; Gold / Silver have no caught data. Used by the editors and by Review,
// only for an existing Pokemon whose species was changed; nothing here changes a record.

#include "UI/HistoryConsistency.h"
#include "UI/ExactGameMetLocations.h"
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Names/LocationNames.h"
#include "Names/SpeciesNames.h"
#include "Names/SpecialMetLocationNotes.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::HistoryConsistency {

// Generation III (PK3 origin 1-5; 0 is shown as "None" by every Gen III editor and is not judged).
[[nodiscard]] inline std::vector<std::string> gen3Flags(uint16_t species, uint8_t origin, uint16_t metLocation,
                                                       uint8_t metLevel, uint8_t level) {
    Facts facts;
    facts.species = Names::getSpeciesName(species);
    facts.metLocationRecorded = metLocation != 0;
    facts.originKnown = origin >= 1 && origin <= 5;
    facts.nativeToOrigin = Names::isGen3NativeMetLocation(origin, metLocation);
    facts.marker = metLocation >= 253 && metLocation <= 255;   // gift egg / in-game trade / fateful
    facts.catchDataAvailable = facts.originKnown;
    if (facts.originKnown && facts.metLocationRecorded)
        for (const auto& choice : PokeVault::Integration::EncounterGuardrails::forGameSpeciesWithGen3Provenance(
                 ExactGameMetLocations::gen3GameIdForOrigin(origin), species))
            if (choice.encounter.location == metLocation) { facts.listedCatch = true; break; }
    facts.metLevel = metLevel;
    facts.level = level;
    facts.levelsKnown = true;
    return flags(facts);
}

// Generation IV / V (PK4 origins 7, 8, 10-12; PK5 origins 20-23). Location 0 reads "(not recorded)".
[[nodiscard]] inline std::vector<std::string> gen45Flags(uint16_t species, uint8_t origin, uint16_t metLocation,
                                                        uint8_t metLevel, uint8_t level) {
    namespace Context = PokeVault::Integration::EncounterContext45;
    Facts facts;
    facts.species = Names::getSpeciesName(species);
    facts.metLocationRecorded = metLocation != 0;
    const uint8_t generation = Context::generation(origin);
    facts.marker = !Names::specialMetLocation(origin, metLocation).empty() ||
                   (generation == 4 && metLocation >= 2000) || (generation == 5 && metLocation >= 30000);
    if (generation == 4 && !facts.marker) {
        facts.originKnown = true;
        facts.nativeToOrigin = Names::isGen4NativeMetLocation(origin, metLocation);
    } else if (generation == 5 && !facts.marker) {
        const auto table = Names::getLocationTable(origin);
        facts.originKnown = table.names != nullptr && table.count != 0;
        facts.nativeToOrigin = metLocation < table.count && table.names && table.names[metLocation] &&
                               table.names[metLocation][0] != '\0';
    }
    facts.catchDataAvailable = !Context::gameId(origin).empty();
    if (facts.catchDataAvailable && facts.metLocationRecorded && !facts.marker) {
        const Context::Catalogue context(origin, species, metLocation, {}, {});
        facts.listedCatch = context.wildRow(metLocation) != nullptr;
    }
    facts.metLevel = metLevel;
    facts.level = level;
    facts.levelsKnown = true;
    return flags(facts);
}

// Crystal caught data (7-bit landmark; level code 0 unknown, 1 egg marker, else the met level).
[[nodiscard]] inline std::vector<std::string> crystalFlags(uint16_t species, std::string_view crystalSourceGameId,
                                                          uint8_t location, uint8_t levelCode, uint8_t level) {
    Facts facts;
    facts.species = Names::getSpeciesName(species);
    facts.metLocationRecorded = location != 0;
    facts.originKnown = true;
    facts.nativeToOrigin = ExactGameMetLocations::crystalNativeCaughtLocation(location);
    facts.marker = location == 0x7E || location == 0x7F;   // Gift / Event
    facts.catchDataAvailable = true;
    if (facts.metLocationRecorded && !facts.marker && species >= 1 && species <= 251) {
        const auto family = PokeVault::Integration::EncounterContext45::ancestry(4, species);
        for (uint16_t member = 1; member <= 251 && !facts.listedCatch; ++member) {
            if (!family[member]) continue;
            for (const auto& encounter :
                 PokeVault::Integration::EncounterGuardrails::forGameSpecies(crystalSourceGameId, member))
                if (encounter.location == location) { facts.listedCatch = true; break; }
        }
    }
    facts.metLevel = levelCode > 1 ? levelCode : 0;
    facts.level = level;
    facts.levelsKnown = true;
    return flags(facts);
}

} // namespace PokeBank::UIModel::HistoryConsistency
