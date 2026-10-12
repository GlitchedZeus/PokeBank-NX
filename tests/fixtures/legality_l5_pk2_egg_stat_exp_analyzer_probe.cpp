// L5 Stage 3: actual Legality::analyze Gen II native stat-experience egg probe.
// This must be compiled with FULL PokeBank NX analyzer sources, not the oracle model.
// Baseline reproduces absent reference rule; after-fix is coordinator acceptance.
// Synthetic wrappers only; no cartridge assets, saving, or mutation.
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace {
bool hasEggStatExpInvalid(const Legality::Report& report) {
    for (const auto& issue:report.issues) {
        if (issue.identifier == Legality::CheckIdentifier::Egg &&
            issue.severity == Legality::Severity::Invalid &&
            issue.text.find("stat experience") != std::string::npos)
            return true;
    }
    return false;
}
bool sourceConflict(const Legality::Report& report) {
    for (const auto& issue:report.issues)
        if (issue.identifier == Legality::CheckIdentifier::SourceGame &&
            issue.severity == Legality::Severity::Warning &&
            issue.text.find("source-game label conflicts") != std::string::npos)
            return true;
    return false;
}
PokeVault::Integration::Gen2::PokemonRecord specimen(
    uint16_t species, bool egg, uint16_t statExp) {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species = species;
    rec.level = 5;
    rec.experience = 135;
    rec.isEgg = egg;
    rec.trainerId = 1234;
    rec.moves = {33,0,0,0};
    rec.pp = {35,0,0,0};
    rec.originalTrainer = "L5";
    rec.nickname = "REFERENCE";
    rec.dvs[1] = 15;
    rec.statExperience = {statExp,0,0,0,0};
    rec.rawBodySize = rec.rawBody.size();
    return rec;
}
}
int main(int argc, char** argv) {
    if (argc != 2 || (std::string_view(argv[1]) != "--baseline" &&
                       std::string_view(argv[1]) != "--after-fix")) {
        std::cerr << "usage: l5_pk2_egg_stat_exp_probe --baseline|--after-fix\n";
        return 2;
    }
    const bool afterFix = std::string_view(argv[1]) == "--after-fix";
    const std::array<std::string_view,5> sources = {
        "", "gold_gbc", "crystal_gbc", "firered_gba", "unknown_unrecognized"
    };
    unsigned contexts = 0, eggNonzero = 0;
    for (uint16_t species : {1u,29u,81u,201u})
        for (uint16_t value : {0u,1u,2560u,25600u,65535u})
            for (bool egg : {false,true})
                for (size_t source=0;source<sources.size();++source) {
                    const auto record = specimen(species,egg,value);
                    const Pokemon::Pokemon2ReadOnly pk(record);
                    const auto originalBody = pk.strictRecord().rawBody;
                    assert(pk.isEgg() == egg);
                    assert(pk.statExpHP() == value);
                    assert(pk.evHP() == 0); // Neutral modern EV getter: the gap.
                    const auto report=Legality::analyze(
                        pk, pk.getGameGroup(), sources[source]);
                    const bool expected = afterFix && egg && value != 0;
                    assert(hasEggStatExpInvalid(report) == expected);
                    if (source == 3) assert(sourceConflict(report));
                    else assert(!sourceConflict(report));
                    assert(pk.strictRecord().rawBody == originalBody);
                    ++contexts;
                    if (egg && value != 0) ++eggNonzero;
                }
    assert(contexts == 200 && eggNonzero == 80);
    std::cout << "L5 actual native PK2 stat-experience egg analyzer: "
              << contexts << " synthetic contexts, " << eggNonzero
              << " reference egg-invalid candidates, "
              << (afterFix ? "post-fix" : "current baseline") << " PASS\n";
}
