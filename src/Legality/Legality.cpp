/**
 * Legality.cpp - PKSE legality checker implementation (Layers 1+2, informational).
 *
 * See Legality.h. Every check reads the base Pokemon interface polymorphically;
 * per-gen behavior branches on the game group / capability flags, never on type
 * (RTTI is disabled). Optional fields that are unwired for a format read 0 and are
 * treated as "not applicable -> skip" so unsupported formats produce no false flags.
 */

#include "Legality/Legality.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Legality/Gen4WildEncounter.h"
#include "Legality/Gen4StaticEncounter.h"
#include "Legality/Gen4TradeEvidence.h"
#include "Legality/Gen1CatchRateEvidence.h"
#include "Legality/Gen1EncounterEvidence.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen4PidIvCorrelation.h"
#include "Legality/Gen4PokewalkerPid.h"
#include "Legality/Gen4CuteCharmPid.h"
#include "Legality/Gen4ChainShiny.h"
#include "Legality/Gen4MysteryGiftPid.h"
#include "Pokemon/Pokemon1ReadOnly.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "Pokemon/Pokemon.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/AbilityInfo.h"       // getAbilitySlots -> per-generation ability slots
#include "Pokemon/FormInfo.h"          // isFormGenderSpecific -> gender stored as the form index
#include "Pokemon/LearnsetTable.h"
#include "Trainer/Trainer.h"     // Trainer::getSpeciesName / getItemName (name-table sentinels)
#include "Names/MoveNames.h"     // Names::getMoveName
#include "Names/ItemNames.h"     // Names::getItemNameG3 (Gen 3 item id space)
#include "Integration/Gen3/Gen3LearnsetTable.h"

namespace Legality {

    namespace {
        void add(Report& r, Severity sev, std::string text,
                 CheckIdentifier identifier = CheckIdentifier::Misc) {
            r.issues.push_back(Issue{sev, std::move(text), identifier});
        }
        const char* statName(int i) {
            static const char* const kNames[6] = { "HP", "Atk", "Def", "Spe", "SpA", "SpD" };
            return kNames[i];
        }
        // Personal-table presence bit for a game group (0 if the group has no bit).
        uint8_t presenceBit(Enums::GameVersion g) {
            switch (g) {
                case Enums::GameVersion::GG:   return Pokemon::PERSONAL_GAME_GG;
                case Enums::GameVersion::SWSH: return Pokemon::PERSONAL_GAME_SWSH;
                case Enums::GameVersion::BDSP: return Pokemon::PERSONAL_GAME_BDSP;
                case Enums::GameVersion::PLA:  return Pokemon::PERSONAL_GAME_PLA;
                case Enums::GameVersion::SV:   return Pokemon::PERSONAL_GAME_SV;
                case Enums::GameVersion::ZA:   return Pokemon::PERSONAL_GAME_ZA;
                default:                       return 0;
            }
        }

        // PK3 is one binary entity format, but the five GBA games do not share one native
        // move pool. Keep the container/save identity separate from Pokemon::getGameGroup()
        // so generic format capabilities can stay FRLG without pretending a Ruby save is FRLG.
        bool exactGen1Source(std::string_view id,
                             PokeVault::Integration::Gen1::SourceGame& game) noexcept {
            using SourceGame = PokeVault::Integration::Gen1::SourceGame;
            if (id == "red_gb")    { game = SourceGame::Red;    return true; }
            if (id == "blue_gb")   { game = SourceGame::Blue;   return true; }
            if (id == "yellow_gb") { game = SourceGame::Yellow; return true; }
            return false;
        }

        bool exactGen2Source(std::string_view id,
                             PokeVault::Integration::Gen2::SourceGame& game) noexcept {
            using SourceGame = PokeVault::Integration::Gen2::SourceGame;
            if (id == "gold_gbc")    { game = SourceGame::Gold;    return true; }
            if (id == "silver_gbc")  { game = SourceGame::Silver;  return true; }
            if (id == "crystal_gbc") { game = SourceGame::Crystal; return true; }
            return false;
        }

        bool exactGen3Source(std::string_view id,
                             PokeVault::Integration::Gen3::SourceGame& game) noexcept {
            using SourceGame = PokeVault::Integration::Gen3::SourceGame;
            if (id == "ruby_gba")      { game = SourceGame::RubyGBA;      return true; }
            if (id == "sapphire_gba")  { game = SourceGame::SapphireGBA;  return true; }
            if (id == "emerald_gba")   { game = SourceGame::EmeraldGBA;   return true; }
            if (id == "firered_gba")   { game = SourceGame::FireRedGBA;   return true; }
            if (id == "leafgreen_gba") { game = SourceGame::LeafGreenGBA; return true; }
            return false;
        }
    }

    Report analyze(const Pokemon::Pokemon& pk, const Context& context) {
        const auto originGroup = context.originGroup;
        const auto exactSourceGameId = context.exactSourceGameId;
        Report r;
        PokeVault::Integration::Gen1::SourceGame gen1Source =
            PokeVault::Integration::Gen1::SourceGame::Red;
        PokeVault::Integration::Gen2::SourceGame gen2Source =
            PokeVault::Integration::Gen2::SourceGame::Gold;
        PokeVault::Integration::Gen3::SourceGame gen3Source =
            PokeVault::Integration::Gen3::SourceGame::FireRedGBA;
        const bool hasExactGen1Source = exactGen1Source(exactSourceGameId, gen1Source);
        const bool hasExactGen2Source = exactGen2Source(exactSourceGameId, gen2Source);
        const bool hasExactGen3Source = exactGen3Source(exactSourceGameId, gen3Source);
        const uint16_t species = pk.speciesID();
        if (species == 0) return r;  // empty slot — nothing to validate

        const auto* sourceProfile = sourceGameProfile(exactSourceGameId);
        const uint8_t exactGeneration = sourceProfile ? sourceProfile->generation : 0;

        if (sourceProfile && exactGeneration == 1 &&
            pk.getGameGroup() == Pokemon::Pokemon1ReadOnly::kReadOnlyGameGroup) {
            const auto& gen1 = static_cast<const Pokemon::Pokemon1ReadOnly&>(pk);
            const uint8_t catchRate = gen1.strictRecord().catchRate;
            using Evidence = Gen1CatchRate::Evidence;
            const auto evidence = Gen1CatchRate::classify(
                exactSourceGameId, gen1.speciesID(), catchRate);
            if (evidence == Evidence::NativeSpeciesRate) {
                add(r, Severity::Info,
                    "PK1 catch-rate byte matches the current species in this exact Generation I game",
                    CheckIdentifier::Encounter);
            } else if (evidence == Evidence::Gen1SpeciesOrPreEvolutionRate) {
                add(r, Severity::Info,
                    "PK1 catch-rate byte matches R/B/Y species or pre-evolution provenance",
                    CheckIdentifier::Encounter);
            } else if (evidence == Evidence::AmbiguousGen1OrTimeCapsuleHeldItem) {
                add(r, Severity::Info,
                    "PK1 catch-rate byte matches Gen I species/pre-evolution provenance and a valid Gen II held-item byte; Time Capsule history is indeterminate",
                    CheckIdentifier::Encounter);
            } else if (evidence == Evidence::PossibleTimeCapsuleHeldItem) {
                add(r, Severity::Info,
                    "PK1 catch-rate byte is compatible with a Generation II held item after Time Capsule tradeback",
                    CheckIdentifier::Encounter);
            } else {
                add(r, Severity::Info,
                    "PK1 catch-rate provenance is unresolved; event/Stadium evidence is not complete",
                    CheckIdentifier::Encounter);
            }

            const bool tradeOT = gen1.strictRecord().originalTrainer == "*";
            const auto trade = Gen1Encounter::matchTrade(
                exactSourceGameId, gen1.speciesID(), gen1.level(), catchRate, tradeOT);
            if (trade.matched) {
                add(r, Severity::Info,
                    trade.nativeToContainer
                        ? "PK1 data is compatible with a released native in-game trade template"
                        : "PK1 data is compatible with a released R/B/Y in-game trade template from another link-compatible version",
                    CheckIdentifier::Encounter);
            } else {
                const auto fixed = Gen1Encounter::matchStatic(
                    exactSourceGameId, gen1.speciesID(), gen1.level(), catchRate);
                if (fixed.matched) {
                    add(r, Severity::Info,
                        fixed.nativeToContainer
                            ? "PK1 data is compatible with a released native static/gift/Game Corner template"
                            : "PK1 data is compatible with a released R/B/Y static/gift/Game Corner template from another link-compatible version",
                        CheckIdentifier::Encounter);
                }
            }
        }
        if (sourceProfile) {
            r.coverage.sourceGame = CoverageLevel::Complete;
            r.coverage.encounter = sourceProfile->encounterCoverage;
            // Exact-game move pools exist, but event/tradeback chronology is not complete yet.
            r.coverage.moves = CoverageLevel::Partial;
            // Internal mechanics are intentionally still partial until DV/Stat-Exp and
            // generation-specific PID/RNG rules have dedicated verifiers.
            r.coverage.internal = CoverageLevel::Partial;
            if (exactGeneration == 3 || exactGeneration == 4)
                r.coverage.pidRng = CoverageLevel::Partial;
        }

        if (sourceProfile && exactGeneration == 3) {
            const std::array<uint8_t, 6> ivs{
                pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()
            };
            const auto correlation =
                Gen3PidIv::analyze(pk.pid(), ivs, species == 201);
            if (correlation.matched()) {
                add(r, Severity::Info,
                    "PID/IV spread matches Gen III " +
                    std::string(Gen3PidIv::methodName(correlation.method)),
                    CheckIdentifier::PidRng);
            } else if (Gen3PidIv::isRoamerSpecies(species)) {
                const auto roamer = Gen3PidIv::analyzeRoamer(pk.pid(), ivs);
                if (roamer.matched()) {
                    add(r, Severity::Info,
                        "PID/IV spread matches the Gen III truncated-roamer Method 1 class used by Ruby/Sapphire and FireRed/LeafGreen roamers",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "No handheld Method 1/2/3/4 or truncated-roamer PID/IV match; special/event/GC RNG classes are not fully covered",
                        CheckIdentifier::PidRng);
                }
            } else {
                add(r, Severity::Info,
                    "No handheld Method 1/2/3/4 PID/IV match; special/event/GC RNG classes are not fully covered",
                    CheckIdentifier::PidRng);
            }
        } else if (sourceProfile && exactGeneration == 4) {
            const bool isHgss =
                exactSourceGameId == "heartgold_nds" || exactSourceGameId == "soulsilver_nds";
            if (isHgss && pk.metLocation() == 233) {
                const uint8_t genderRatio =
                    Pokemon::getPersonalInfo(species, pk.form()).genderRatio;
                if (Gen4PokewalkerPid::matches(
                        pk.pid(), pk.id32(), pk.nature(), pk.gender(), genderRatio)) {
                    add(r, Severity::Info,
                        "PID matches the Generation IV PokeWalker trainer/nature/gender formula",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "PokeWalker met location detected, but PID correlation is unresolved against the current species ratio; evolution/course provenance remains incomplete",
                        CheckIdentifier::PidRng);
                }
            } else {
                const std::array<uint8_t, 6> ivs{
                    pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()
                };
                const auto correlation = Gen4PidIv::analyze(pk.pid(), ivs);
                if (correlation.matched()) {
                    add(r, Severity::Info,
                        "PID/IV spread matches normal Gen IV Method 1; wild lead-frame correlation is still partial",
                        CheckIdentifier::PidRng);
                } else {
                    const auto identity =
                        Gen4CuteCharmPid::remapEncounterIdentity(
                            species, pk.gender(), pk.pid());
                    const uint8_t ratio =
                        Pokemon::getPersonalInfo(identity.species, 0).genderRatio;
                    const uint8_t cuteGender = identity.deriveGenderFromPid
                        ? Gen4CuteCharmPid::genderFromPid(pk.pid(), ratio)
                        : identity.gender;
                    if (Gen4CuteCharmPid::matchesSurface(
                            pk.pid(), cuteGender, ratio)) {
                        add(r, Severity::Info,
                            "PID has a valid Gen IV Cute Charm buffered form; Method J/K lead-frame and encounter-slot correlation remain incomplete",
                            CheckIdentifier::PidRng);
                    } else {
                        const auto chain =
                            Gen4ChainShiny::analyze(pk.pid(), pk.id32(), ivs);
                        if (chain.matched) {
                            add(r, Severity::Info,
                                "PID/IV/trainer IDs match the Gen IV Poké Radar Chain Shiny RNG class; radar-capable encounter-slot provenance remains incomplete",
                                CheckIdentifier::PidRng);
                        } else {
                            const auto gift =
                                Gen4MysteryGiftPid::analyze(pk.pid(), ivs);
                            if (gift.matched) {
                                add(r, Severity::Info,
                                    "PID/IV spread matches a Gen IV Mystery Gift anti-shiny ARNG reroll class; exact event-template provenance remains incomplete",
                                    CheckIdentifier::PidRng);
                            } else {
                                add(r, Severity::Info,
                                    "No normal Gen IV Method-1, Cute Charm, Chain Shiny, or Mystery Gift anti-shiny RNG match; other event RNG classes remain incomplete",
                                    CheckIdentifier::PidRng);
                            }
                        }
                    }
                }
            }
        }

        const bool hasStatNature =
            originGroup == Enums::GameVersion::SWSH ||
            originGroup == Enums::GameVersion::BDSP ||
            originGroup == Enums::GameVersion::PLA ||
            originGroup == Enums::GameVersion::SV ||
            originGroup == Enums::GameVersion::ZA;

        // Species/form personal data (abilities / gender ratio / per-game presence) for the L2 checks.
        const Pokemon::PersonalInfo& pi = Pokemon::getPersonalInfo(species, pk.form());

        // ---- L1: species id known (name-table sentinel = "Unknown") ----
        if (std::string(Trainer::getSpeciesName(species)) == "Unknown")
            add(r, Severity::Invalid, "Unknown species id " + std::to_string(species),
                CheckIdentifier::Species);

        if (sourceProfile && species > sourceProfile->maxSpecies) {
            add(r, Severity::Invalid,
                "Species " + std::to_string(species) +
                " cannot exist in a Generation " + std::to_string(sourceProfile->generation) +
                " save (maximum species id " + std::to_string(sourceProfile->maxSpecies) + ")",
                CheckIdentifier::SourceGame);
        }

        for (int slot = 0; sourceProfile && slot < 4; ++slot) {
            const uint16_t moveId = pk.move(slot);
            if (moveId > sourceProfile->maxMove) {
                add(r, Severity::Invalid,
                    "Move id " + std::to_string(moveId) +
                    " cannot exist in a Generation " +
                    std::to_string(sourceProfile->generation) +
                    " save (maximum move id " + std::to_string(sourceProfile->maxMove) + ")",
                    CheckIdentifier::Moves);
            }
        }

        // ---- L1: nature range (+ stat/mint nature on Gen8+) ----
        if (pk.nature() > 24)
            add(r, Severity::Invalid, "Nature out of range (" + std::to_string(pk.nature()) + ")");
        if (hasStatNature && pk.statNature() > 24)
            add(r, Severity::Invalid, "Stat nature out of range (" + std::to_string(pk.statNature()) + ")");

        // ---- L1: EVs ----
        const uint8_t ev[6] = { pk.evHP(), pk.evATK(), pk.evDEF(), pk.evSPE(), pk.evSPA(), pk.evSPD() };
        int evTotal = 0;
        for (int i = 0; i < 6; ++i) evTotal += ev[i];

        if (pk.hasAwakeningValues()) {
            // Let's Go has no EV training -- it replaced it with Awakening Values. Nothing in the game
            // writes these bytes and the stat formula has no EV term (PKHeX PB7.LoadStats), so on a
            // legitimate Pokemon they are all 0.
            //
            // The 252/510 caps are the wrong question here, not merely a differently-worded one: they
            // would pass a fully trained 510 spread as perfectly legal while a lone stray 4 slipped by
            // in silence. What matters is non-zero at all, so that is what is checked.
            //
            // Reported as a WARNING, not Invalid. PKHeX has this exact rule ("Cannot receive EVs.") but
            // deliberately leaves it disabled, so calling it illegal outright would be a stronger claim
            // than the reference is willing to make. It is still worth surfacing -- the value cannot come
            // from playing the game -- and it is harmless in itself, since no stat reads it.
            if (evTotal != 0) {
                std::string which;
                for (int i = 0; i < 6; ++i) {
                    if (ev[i] == 0) continue;
                    if (!which.empty()) which += ", ";
                    which += std::string(statName(i)) + " " + std::to_string(ev[i]);
                }
                add(r, Severity::Warning,
                    "Let's Go has no EVs, but this Pokemon carries " + which +
                    " -- Awakening Values are its stat training, and these bytes affect nothing");
            }
        } else {
            // Everywhere else: <=252 per stat, <=510 total.
            for (int i = 0; i < 6; ++i) {
                if (ev[i] > 252)
                    add(r, Severity::Invalid, std::string(statName(i)) + " EV over 252 (" + std::to_string(ev[i]) + ")");
            }
            if (evTotal > 510)
                add(r, Severity::Invalid, "EV total over 510 (" + std::to_string(evTotal) + ")");
        }

        // ---- L1: AVs (Let's Go only, <=200 each) ----
        if (pk.hasAwakeningValues()) {
            const uint8_t av[6] = { pk.avHP(), pk.avATK(), pk.avDEF(), pk.avSPE(), pk.avSPA(), pk.avSPD() };
            for (int i = 0; i < 6; ++i)
                if (av[i] > 200)
                    add(r, Severity::Invalid, std::string(statName(i)) + " AV over 200 (" + std::to_string(av[i]) + ")");
        }

        // ---- L1/L2: ability slot valid + ability id legal for the species/form ----
        if (!sourceProfile || exactGeneration >= 3) {
            const uint8_t an = pk.abilityNumber();
            if (an != 1 && an != 2 && an != 4)
                add(r, Severity::Warning, "Unusual ability slot (" + std::to_string(an) + ")");
            // Slots are per-generation: Gen 3 has two of them and its pair differs from the modern
            // table for 101 species, so checking a Gen 3 mon against the modern one both misses real
            // problems and invents fake ones. PKHeX encodes "single ability" as ability2 == ability1,
            // so that set collapses naturally.
            const Pokemon::AbilitySlots slots =
                Pokemon::getAbilitySlots(pk.speciesID(), pk.form(), pk.getGameGroup());
            if (!Pokemon::isAbilityLegal(slots, pk.ability()))
                add(r, Severity::Invalid, "Ability not legal for this species");
        }

        // ---- L1: moves are known ids + no duplicates ----
        {
            const uint16_t mv[4] = { pk.move(0), pk.move(1), pk.move(2), pk.move(3) };
            for (int i = 0; i < 4; ++i)
                if (mv[i] != 0 && std::string(Names::getMoveName(mv[i])) == "-")
                    add(r, Severity::Invalid, "Unknown move in slot " + std::to_string(i + 1));
            for (int i = 0; i < 4; ++i)
                for (int j = i + 1; j < 4; ++j)
                    if (mv[i] != 0 && mv[i] == mv[j]) {
                        add(r, Severity::Invalid, "Duplicate move: " + std::string(Names::getMoveName(mv[i])));
                        break;  // report each duplicated move once
                    }
        }

        // ---- L2: move learnability ----
        // The pool is that game's own learn methods unioned with the species' whole pre-evolution chain,
        // so an inherited move no longer false-flags. Still a Warning rather than Invalid: the pool can't
        // express *when* a move was legal (move tutors that came and went, event moves, trade-backs).
        // All seven games have a table now, but keep the nullptr guard honest -- if a group ever lacks one,
        // getLearnableBits() returns nullptr meaning "unknown", which must not be reported as illegal.
        if (hasExactGen1Source) {
            for (int i = 0; i < 4; ++i) {
                const uint16_t moveId = pk.move(i);
                if (moveId != 0 &&
                    !PokeVault::Integration::Gen1::MoveCompatibility::canLearnMove(
                        gen1Source, species, moveId)) {
                    add(r, Severity::Warning,
                        "Move is outside the audited move pool for this exact Gen I game: " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                }
            }
        } else if (hasExactGen2Source) {
            for (int i = 0; i < 4; ++i) {
                const uint16_t moveId = pk.move(i);
                if (moveId != 0 &&
                    !PokeVault::Integration::Gen2::MoveCompatibility::canLearnMove(
                        gen2Source, species, moveId)) {
                    add(r, Severity::Warning,
                        "Move is outside the audited native/Time Capsule pool for this exact Gen II game: " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                }
            }
        } else if (hasExactGen3Source) {
            const std::array<uint16_t, 4> sourceMoves{
                pk.move(0), pk.move(1), pk.move(2), pk.move(3)
            };
            for (const uint16_t moveId : sourceMoves) {
                if (moveId == 0) continue;
                const auto availability =
                    PokeVault::Integration::Gen3::Learnset::classify(
                        gen3Source, species, moveId, sourceMoves);
                using Availability = PokeVault::Integration::Gen3::Learnset::Availability;
                if (availability == Availability::Transfer) {
                    add(r, Severity::Warning,
                        "Move is not native to this exact Gen III game (transfer required): " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                } else if (availability == Availability::Preserved) {
                    add(r, Severity::Warning,
                        "Move is preserved but not in the audited Gen III native/transfer pool: " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                } else if (availability == Availability::Invalid) {
                    add(r, Severity::Warning,
                        "Move may not be learnable: " + std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                }
            }
        } else if (sourceProfile && exactGeneration == 4) {
            using namespace PokeVault::Integration::Gen4MoveCompatibility;
            for (int i = 0; i < 4; ++i) {
                const uint16_t moveId = pk.move(i);
                if (moveId == 0) continue;
                const auto availability =
                    classify(exactSourceGameId, species, pk.form(), moveId, true);
                if (availability == Availability::Transfer) {
                    add(r, Severity::Info,
                        "Move requires another Generation IV game/source: " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                } else if (availability == Availability::Preserved ||
                           availability == Availability::Invalid) {
                    add(r, Severity::Warning,
                        "Move is outside the audited direct/transfer pool for this exact Gen IV game: " +
                        std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
                }
            }
        } else if (Pokemon::getLearnableBits(species, pk.form(), originGroup) != nullptr) {
            for (int i = 0; i < 4; ++i) {
                const uint16_t moveId = pk.move(i);
                if (moveId != 0 && !Pokemon::isLearnable(species, pk.form(), originGroup, moveId))
                    add(r, Severity::Warning,
                        "Move may not be learnable: " + std::string(Names::getMoveName(moveId)),
                        CheckIdentifier::Moves);
            }
        }

        // ---- L1: held item is a known id (skip when none) ----
        // Resolve through the id space the mon's own game uses -- a Gen 3 held item checked against
        // the modern table would either name the wrong item or be reported "unknown" when it is fine.
        if (pk.heldItem() != 0 && (!sourceProfile || exactGeneration >= 3)) {
            const char* heldName = (hasExactGen3Source || originGroup == Enums::GameVersion::FRLG)
                                 ? Names::getItemNameG3(pk.heldItem())
                                 : Trainer::getItemName(pk.heldItem());
            if (std::string(heldName) == "???")
                add(r, Severity::Warning, "Unknown held item id " + std::to_string(pk.heldItem()));
        }

        // ---- L1: ball / language ranges (skip when unwired == 0) ----
        if (pk.ball() != 0 && pk.ball() > 37)
            add(r, Severity::Invalid, "Ball id out of range (" + std::to_string(pk.ball()) + ")");
        if (pk.language() != 0 && (pk.language() > 10 || pk.language() == 6))
            add(r, Severity::Invalid, "Invalid language id (" + std::to_string(pk.language()) + ")");

        // ---- L2: level <-> EXP (EXP-derived level is authoritative; box mons read level() == 0) ----
        const uint8_t expLevel = Pokemon::getLevelFromExp(pk.exp(), Pokemon::getGrowthRate(species));
        if (expLevel < 1 || expLevel > 100)
            add(r, Severity::Invalid, "EXP maps to invalid level (" + std::to_string(expLevel) + ")");
        if (pk.level() != 0 && pk.level() != expLevel)
            add(r, Severity::Invalid, "Level " + std::to_string(pk.level()) +
                                      " does not match EXP (expected " + std::to_string(expLevel) + ")");

        // ---- L2: met level <= current level ----
        {
            const uint8_t effLevel = pk.level() != 0 ? pk.level() : expLevel;
            if (pk.metLevel() != 0 && pk.metLevel() > effLevel)
                add(r, Severity::Invalid, "Met level " + std::to_string(pk.metLevel()) +
                                          " above current level " + std::to_string(effLevel));
        }

        // ---- L3 foundation: exact encounter evidence where audited tables exist ----
        if (sourceProfile && sourceProfile->encounterCoverage != CoverageLevel::None &&
            pk.metLocation() != 0 && pk.metLevel() != 0) {
            using namespace PokeVault::Integration::EncounterGuardrails;
            if (exactGeneration == 3) {
                const auto candidates =
                    forGameSpeciesWithGen3Provenance(exactSourceGameId, species);
                if (!candidates.empty()) {
                    const bool match = std::any_of(
                        candidates.begin(), candidates.end(), [&](const auto& choice) {
                            return choice.encounter.location == pk.metLocation() &&
                                   choice.encounter.containsLevel(pk.metLevel());
                        });
                    if (!match)
                        add(r, Severity::Warning,
                            "Met location/level does not match the audited encounter table for " +
                            std::string(exactSourceGameId),
                            CheckIdentifier::Encounter);
                }
            } else if (exactGeneration == 4) {
                // Gen IV wild-slot evidence is complete for the five retail games, but the overall
                // encounter layer remains Partial until static/gift/trade/event templates are added.
                // A positive match is useful evidence. A non-match is intentionally NOT an error yet,
                // because the Pokemon may come from one of those still-unimported encounter classes.
                if (Legality::Gen4Wild::matches(
                        exactSourceGameId, species, pk.metLocation(), pk.metLevel(), pk.form())) {
                    add(r, Severity::Info,
                        "Met data matches an audited Generation IV wild encounter slot",
                        CheckIdentifier::Encounter);
                } else if (Legality::Gen4Static::matches(
                               exactSourceGameId, species, pk.metLocation(), pk.metLevel(),
                               pk.form(), pk.eggLocation())) {
                    add(r, Severity::Info,
                        "Met data matches an audited Generation IV static/gift encounter",
                        CheckIdentifier::Encounter);
                } else if (Legality::Gen4Trade::matches(
                               exactSourceGameId, species, pk.pid(), pk.id32(),
                               pk.gender(), pk.otGender(),
                               std::array<uint8_t, 6>{
                                   pk.ivHP(), pk.ivATK(), pk.ivDEF(),
                                   pk.ivSPE(), pk.ivSPA(), pk.ivSPD()},
                               pk.metLocation(), pk.metLevel())) {
                    add(r, Severity::Info,
                        "Trainer/PID/IV/met data matches an audited Generation IV in-game trade",
                        CheckIdentifier::Encounter);
                } else if (Legality::Gen4Wild::hasSpecies(exactSourceGameId, species) ||
                           Legality::Gen4Static::hasSpecies(exactSourceGameId, species) ||
                           Legality::Gen4Trade::hasSpecies(exactSourceGameId, species)) {
                    add(r, Severity::Info,
                        "No matching Gen IV wild/static/gift/trade evidence; external event/PokeWalker evidence is incomplete",
                        CheckIdentifier::Encounter);
                }
            } else {
                const auto candidates = forGameSpecies(exactSourceGameId, species);
                if (!candidates.empty()) {
                    const bool match = std::any_of(
                        candidates.begin(), candidates.end(), [&](const auto& encounter) {
                            return encounter.location == pk.metLocation() &&
                                   encounter.containsLevel(pk.metLevel());
                        });
                    if (!match)
                        add(r, Severity::Warning,
                            "Met location/level does not match the audited encounter table for " +
                            std::string(exactSourceGameId),
                            CheckIdentifier::Encounter);
                }
            }
        }

        // ---- L2: gender vs the species gender ratio (255 genderless / 254 female-only / 0 male-only) ----
        if (!sourceProfile || exactGeneration >= 2) {
            const uint8_t g = pk.gender();
            if (g > 2)
                add(r, Severity::Invalid, "Gender value out of range (" + std::to_string(g) + ")");
            // Meowstic, Indeedee, Basculegion and Oinkologne keep the gender in the form index, so
            // the per-form ratio read above describes the FORM: reporting "male-only species" about
            // a Meowstic would be a false statement about a species that is plainly dual-gender.
            // What can actually be wrong is the pairing -- PKHeX checks the same invariant, from the
            // form side, as `(form & 1) != gender`.
            else if (Pokemon::isFormGenderSpecific(species)) {
                if (g > 1)
                    add(r, Severity::Invalid, "Gendered species marked genderless");
                else if ((pk.form() & 1) != g)
                    add(r, Severity::Invalid, "Form " + std::to_string(pk.form()) + " is the " +
                                              ((pk.form() & 1) ? "female" : "male") +
                                              " form but the gender is " + (g ? "female" : "male"));
            }
            else if (pi.genderRatio == 255) {
                if (g != 2) add(r, Severity::Invalid, "Genderless species but a gender is set");
            } else if (g == 2)
                add(r, Severity::Invalid, "Gendered species marked genderless");
            else if (pi.genderRatio == 254 && g != 1)
                add(r, Severity::Invalid, "Female-only species set to male");
            else if (pi.genderRatio == 0 && g != 0)
                add(r, Severity::Invalid, "Male-only species set to female");
        }

        // ---- L2: species/form is obtainable in this game (per the personal presence bitmask) ----
        {
            const uint8_t bit = presenceBit(originGroup);
            if (bit != 0 && (pi.presence & bit) == 0)
                add(r, Severity::Invalid, "Species/form not obtainable in this game");
        }

        // ---- L2: OT presence (a non-egg should have an OT name + nonzero trainer id) ----
        const std::u16string otNameStr = pk.otName();
        if (!pk.isEgg()) {
            if (otNameStr.empty())
                add(r, Severity::Warning, "Empty OT name");
            if (pk.id32() == 0)
                add(r, Severity::Warning, "Trainer ID is zero");
        }

        // ---- L1: name lengths (<=12 UTF-16 chars) ----
        if (pk.nickname().size() > 12)
            add(r, Severity::Warning, "Nickname longer than 12 characters");
        if (otNameStr.size() > 12)
            add(r, Severity::Warning, "OT name longer than 12 characters");

        // ---- Checksum: a strong regression guard on our own editor (flags corruption / mid-edit) ----
        if (!pk.checksumValid())
            add(r, Severity::Warning, "Stored checksum is invalid");

        return r;
    }

    Report analyze(const Pokemon::Pokemon& pk, Enums::GameVersion originGroup,
                   std::string_view exactSourceGameId) {
        return analyze(pk, Context{originGroup, exactSourceGameId});
    }
}
