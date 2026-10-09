#include "Legality/Gen3EggEventRibbonEvidence.h"
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
#include "Legality/Gen4SpecialBallEvidence.h"
#include "Legality/Gen4StaticEncounter.h"
#include "Legality/Gen4TradeEvidence.h"
#include "Legality/Gen1CatchRateEvidence.h"
#include "Legality/Gen1EncounterEvidence.h"
#include "Legality/Gen12TimeCapsuleEvidence.h"
#include "Legality/Gen2StaticEncounter.h"
#include "Legality/Gen2TradeEvidence.h"
#include "Legality/Gen2WildEncounter.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3SafariOriginEvidence.h"
#include "Legality/Gen3WondercardEggEventTemplate.h"
#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3ColoEReaderShadowEvidence.h"
#include "Legality/Gen3GameCubeFixedEncounterEvidence.h"
#include "Legality/Gen3GameCubeShadowEvidence.h"
#include "Legality/Gen3XdPokeSpotEvidence.h"
#include "Legality/Gen3XdShadowEncounter.h"
#include "Legality/Gen3XdShadowTeamLock.h"
#include "Legality/Gen3ChannelPidIvCorrelation.h"
#include "Legality/Gen3ChannelEventTemplate.h"
#include "Legality/Gen3BacdPidIvCorrelation.h"
#include "Legality/Gen3WishmkrEventTemplate.h"
#include "Legality/Gen3BerryFixEventTemplate.h"
#include "Legality/Gen3NegaiBoshiEventTemplate.h"
#include "Legality/Gen3PokeParkEggEventTemplate.h"
#include "Legality/Gen3PcjpFifthEggEventTemplate.h"
#include "Legality/Gen3PokemonBoxEggEventTemplate.h"
#include "Legality/Gen3PcjpMachineEventTemplate.h"
#include "Legality/Gen3PcnyEventTemplate.h"
#include "Legality/Gen3BacdRaEventTemplate.h"
#include "Legality/Gen3MystryMewEvidence.h"
#include "Legality/Gen4PidIvCorrelation.h"
#include "Legality/Gen4PokewalkerPid.h"
#include "Legality/Gen4PokewalkerEncounter.h"
#include "Legality/Gen4CuteCharmPid.h"
#include "Legality/Gen4ChainShiny.h"
#include "Legality/Gen4MysteryGiftPid.h"
#include "Legality/Gen4RangerManaphy.h"
#include "Legality/Gen4EventTemplate.h"
#include "Legality/Gen4WildRngCorrelation.h"
#include "Legality/Gen4LeadReportingEvidence.h"
#include "Legality/Gen34EggState.h"
#include "Legality/Gen34EggBallEvidence.h"
#include "Legality/Gen34EggMoveEvidence.h"
#include "Legality/Gen34LanguageEvidence.h"
#include "Legality/Gen34BallDomainEvidence.h"
#include "Legality/Gen4TransferEvidence.h"
#include "Legality/Gen4ReleaseEvidence.h"
#include "Legality/Gen4FormEvidence.h"
#include "Legality/Gen4HatchLocationEvidence.h"
#include "Legality/Gen4OriginEvidence.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

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

        void addGen3PidEvidence(Report& r, const Pokemon::Pokemon& pk,
                                uint16_t species) {
            const std::array<uint8_t, 6> ivs{
                pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()
            };
            if (pk.originGame() == 15 && pk.otGender() != 0) {
                add(r, Severity::Invalid,
                    "Pokemon Colosseum/XD-origin PK3 cannot have a female OT gender",
                    CheckIdentifier::Trainer);
            }

            const auto correlation =
                Gen3PidIv::analyze(pk.pid(), ivs, species == 201);
            if (correlation.matched()) {
                add(r, Severity::Info,
                    "PID/IV spread matches Gen III " +
                    std::string(Gen3PidIv::methodName(correlation.method)),
                    CheckIdentifier::PidRng);

                if (Gen3WondercardEggEvent::matches(
                        {
                            species, pk.originGame(), pk.language(),
                            pk.metLevel(), pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(),
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
                        },
                        correlation)) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Handheld Method 1/2/4 PID/IV correlation, fateful unhatched egg fields and event moves match a pinned Gen III Wondercard egg template; post-hatch reconstruction remains incomplete",
                        CheckIdentifier::EventGift);
                }
                return;
            }

            if (Gen3PidIv::isRoamerSpecies(species)) {
                const auto roamer = Gen3PidIv::analyzeRoamer(pk.pid(), ivs);
                if (roamer.matched()) {
                    add(r, Severity::Info,
                        "PID/IV spread matches the Gen III truncated-roamer Method 1 class used by Ruby/Sapphire and FireRed/LeafGreen roamers",
                        CheckIdentifier::PidRng);
                    return;
                }
            }

            // GameCube-origin PK3 records use source-family-specific rules. PK3 stores
            // Colosseum and XD with the same origin value (15), so persistent encounter
            // identity must choose the family before selecting RNG semantics.
            const bool gen3Shiny = pk.isShiny(pk.id32(), {});

            // Japanese Colosseum e-Reader shadows are a special fixed-zero-IV path
            // and do not use the normal CXD PID/IV correlation.
            const auto eReaderShadow = Gen3ColoEReaderShadowEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(),
                pk.metLevel(), pk.metLocation(), pk.isEgg(),
                pk.isFatefulEncounter(), pk.pid(), ivs
            });
            if (eReaderShadow.identityMatched) {
                if (eReaderShadow.matched) {
                    add(r, Severity::Info,
                        "PID and fixed-zero-IV history match the Japanese Pokemon Colosseum e-Reader shadow generation path",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Japanese Pokemon Colosseum e-Reader shadow identity and recursive prior-team history match pinned source data",
                        CheckIdentifier::Encounter);
                } else if (eReaderShadow.searchLimited) {
                    add(r, Severity::Info,
                        "Pinned Japanese Colosseum e-Reader shadow identity matches, but bounded recursive team-history search reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        "Pinned Japanese Colosseum e-Reader shadow identity matches, but its PID/team-history provenance was not proven; legality remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            const auto gameCubeShadow = Gen3GameCubeShadowEvidence::analyze({
                species, pk.originGame(), pk.metLevel(), pk.metLocation(),
                pk.ball(), pk.isEgg(), pk.isFatefulEncounter(), gen3Shiny,
                pk.tid16(), pk.sid16(), pk.pid(), ivs
            });
            if (gameCubeShadow.identityMatched) {
                using Family = Gen3GameCubeShadowEvidence::Family;
                using History = Gen3GameCubeShadowEvidence::HistoryResult;
                const bool isXd = gameCubeShadow.family == Family::XD;

                if (gameCubeShadow.correlation.matched) {
                    std::string rngDetail = isXd
                        ? "PID/IV spread matches the Pokemon XD "
                        : "PID/IV spread matches the standard Pokemon Colosseum ";
                    if (isXd && gameCubeShadow.correlation.antiShiny())
                        rngDetail += "CXDAnti target-PID reroll class required by the pinned shadow identity";
                    else
                        rngDetail += "XDRNG class required by the pinned shadow identity";
                    add(r, Severity::Info, std::move(rngDetail), CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity matches, but neither standard CXD nor trainer-aware CXDAnti PID/IV provenance was proven"
                            : "Pinned Pokemon Colosseum shadow identity matches, but the required standard CXD PID/IV provenance was not proven",
                        CheckIdentifier::PidRng);
                    return;
                }

                if (gameCubeShadow.history == History::Matched) {
                    if (isXd) {
                        add(r, Severity::Info,
                            gameCubeShadow.xdRebattleLocation
                                ? "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data through a source-supported Miror B. rebattle location"
                                : "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data",
                            CheckIdentifier::Encounter);
                    } else {
                        std::string detail =
                            "Pokemon Colosseum shadow identity and recursive prior-team history match pinned source data";
                        if (gameCubeShadow.identityCandidateCount > 1)
                            detail += " through one of " +
                                      std::to_string(gameCubeShadow.identityCandidateCount) +
                                      " source-compatible encounter histories";
                        add(r, Severity::Info, std::move(detail), CheckIdentifier::Encounter);
                    }
                } else if (gameCubeShadow.history == History::SearchLimit) {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity and PID/IV evidence match, but bounded recursive team/anti-shiny history search reached its safety limit; provenance remains incomplete"
                            : "Pinned Pokemon Colosseum shadow identity and PID/IV evidence match, but bounded recursive prior-team search reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        isXd
                            ? "Pinned Pokemon XD shadow identity and PID/IV evidence match, but no recursive team/anti-shiny history was proven; provenance remains incomplete"
                            : "Pinned Pokemon Colosseum shadow identity and PID/IV evidence match, but no recursive prior-team history was proven; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            // Direct GameCube starters, gifts and trades have template-specific
            // trainer/language/fateful rules and, for starters, a different XDRNG
            // call sequence from ordinary CXD encounters.
            const std::u16string gameCubeOtName = pk.otName();
            const std::u16string gameCubeNickname = pk.nickname();
            const auto fixedGameCube = Gen3GameCubeFixedEncounterEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(), pk.gender(),
                pk.tid16(), pk.sid16(), pk.metLevel(),
                static_cast<uint8_t>(pk.metLocation()), pk.ball(), pk.isEgg(),
                pk.isFatefulEncounter(), gen3Shiny, pk.pid(), ivs,
                gameCubeOtName, gameCubeNickname
            });
            if (fixedGameCube.identityMatched) {
                if (fixedGameCube.rngMatched) {
                    add(r, Severity::Info,
                        "PID/IV and trainer evidence match the source-specific GameCube starter/gift/trade RNG policy",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Persistent fields match a pinned direct Pokemon Colosseum/XD starter, gift or in-game trade template",
                        CheckIdentifier::Encounter);
                } else if (fixedGameCube.searchLimited) {
                    add(r, Severity::Info,
                        "Pinned direct GameCube starter/gift/trade identity matches, but bounded RNG reconstruction reached its safety limit; provenance remains incomplete",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "Pinned direct GameCube starter/gift/trade identity matches, but its required RNG correlation was not proven; provenance remains incomplete",
                        CheckIdentifier::PidRng);
                }
                return;
            }

            // XD Poke Spot encounters generate PID activation and IV/level animation
            // histories separately, so they must not be forced through ordinary CXD.
            const auto pokeSpot = Gen3XdPokeSpotEvidence::analyze({
                species, pk.originGame(), pk.language(), pk.otGender(),
                pk.metLevel(), static_cast<uint8_t>(pk.metLocation()),
                pk.isEgg(), pk.isFatefulEncounter(), true, pk.pid(), ivs
            });
            if (pokeSpot.identityMatched) {
                if (pokeSpot.matched()) {
                    add(r, Severity::Info,
                        "PID activation and IV/level animation histories match the Pokemon XD Poke Spot RNG path",
                        CheckIdentifier::PidRng);
                    add(r, Severity::Info,
                        "Species/location/level match a pinned Pokemon XD Poke Spot slot",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        "Pinned Pokemon XD Poke Spot identity matches, but the separate PID activation and IV/level histories were not both proven; provenance remains incomplete",
                        CheckIdentifier::Encounter);
                }
                return;
            }

            // Generic standard CXD correlation remains useful when no exact GameCube
            // encounter family above can be established. CXDAnti is intentionally not
            // attempted generically because that reroll policy is XD-family-specific.
            const auto cxd = Gen3CxdPidIv::analyze(pk.pid(), ivs);
            if (cxd.matched) {
                add(r, Severity::Info,
                    "PID/IV spread matches the standard Pokemon Colosseum/XD XDRNG class",
                    CheckIdentifier::PidRng);
                add(r, Severity::Info,
                    "Standard GameCube RNG evidence is present, but no pinned direct shadow/fixed/Poke Spot identity was proven; exact encounter provenance remains incomplete",
                    CheckIdentifier::Encounter);
                return;
            }

            const auto channel = Gen3ChannelPidIv::analyze(
                pk.pid(), ivs, pk.sid16(), pk.originGame(), pk.otGender());
            if (channel.matched) {
                add(r, Severity::Info,
                    "PID/IV spread matches the Pokemon Channel Jirachi XDRNG class",
                    CheckIdentifier::PidRng);

                if (Gen3ChannelEvent::matchesTemplate(
                        species, pk.tid16(), pk.originGame(), pk.language(),
                        pk.metLevel(), pk.metLocation(), pk.ball(), pk.isEgg(),
                        pk.isFatefulEncounter(), pk.otName())) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Persistent fields match the pinned Pokemon Channel Jirachi distribution template (TID/OT/origin/met/ball); SID and OT gender are also consistent with the Channel RNG",
                        CheckIdentifier::EventGift);
                } else {
                    add(r, Severity::Info,
                        "Channel-like PID/IV correlation is present, but the pinned Channel Jirachi distribution fields do not all match; exact event provenance remains incomplete",
                        CheckIdentifier::EventGift);
                }
                return;
            }

            const auto bacd = Gen3BacdPidIv::analyzeWithTrainer(
                pk.pid(), ivs, pk.tid16(), pk.sid16());
            if (bacd.matched()) {
                add(r, Severity::Info,
                    "PID/IV spread matches the Gen III " +
                    std::string(Gen3BacdPidIv::variantName(bacd.variant)) +
                    (bacd.restrictedSeed ? " restricted-seed" : "") +
                    " event RNG class",
                    CheckIdentifier::PidRng);

                const bool mystryMew =
                    Gen3MystryMew::matches(
                        {
                            species, pk.tid16(), pk.sid16(), pk.originGame(),
                            pk.language(), pk.otGender(), pk.metLevel(),
                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
                const bool wishmkr =
                    bacd.variant == Gen3BacdPidIv::Variant::Regular &&
                    bacd.restrictedSeed &&
                    Gen3WishmkrEvent::matchesTemplate(
                        species, pk.tid16(), pk.sid16(), pk.originGame(),
                        pk.language(), pk.otGender(), pk.metLevel(),
                        pk.metLocation(), pk.ball(), pk.isEgg(),
                        pk.isFatefulEncounter(), pk.otName());
                const bool berryFix =
                    bacd.variant == Gen3BacdPidIv::Variant::ForceShiny &&
                    Gen3BerryFixEvent::validOriginSeed(bacd.originSeed) &&
                    Gen3BerryFixEvent::matchesTemplate(
                        species, pk.tid16(), pk.sid16(), pk.originGame(),
                        pk.language(), pk.otGender(), pk.metLevel(),
                        pk.metLocation(), pk.ball(), pk.isEgg(),
                        pk.isFatefulEncounter(), pk.otName());
                const auto restrictedAntiEvent =
                    Gen3BacdRaEvent::match(
                        {
                            species, pk.tid16(), pk.sid16(), pk.originGame(),
                            pk.language(), pk.otGender(), pk.metLevel(),
                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.ribbonCountry(), pk.ribbonChampionBattle(),
                            pk.ribbonChampionRegional(), pk.ribbonChampionNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
                const Gen3NegaiBoshiEvent::Candidate negaiCandidate{
                    species, pk.tid16(), pk.sid16(), pk.originGame(),
                    pk.language(), pk.otGender(), pk.metLevel(),
                    pk.metLocation(), pk.ball(), pk.isEgg(),
                    pk.isFatefulEncounter(),
                    pk.isShiny(pk.id32(), {}), pk.otName()
                };
                const bool negaiForceAnti =
                    Gen3NegaiBoshiEvent::matchesForceAntiShiny(
                        negaiCandidate, bacd);
                const bool negaiTable2 =
                    Gen3NegaiBoshiEvent::matchesRestrictedTable2(
                        negaiCandidate, bacd);
                const bool pokeParkEgg =
                    Gen3PokeParkEggEvent::matches(
                        {
                            species, pk.tid16(), pk.sid16(), pk.originGame(),
                            pk.language(), pk.otGender(), pk.metLevel(), pk.metLocation(),
                            pk.ball(), pk.isEgg(), pk.isFatefulEncounter(),
                            pk.otName()
                        },
                        bacd);
                const bool pcjpFifthEgg =
                    Gen3PcjpFifthEggEvent::matches(
                        {
                            species, pk.originGame(), pk.language(),
                            pk.otGender(), pk.metLevel(), pk.metLocation(),
                            pk.ball(), pk.isEgg(), pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName(),
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
                        },
                        bacd);
                const bool pokemonBoxEgg =
                    Gen3PokemonBoxEggEvent::matches(
                        {
                            species, pk.originGame(), pk.language(), pk.otGender(),
                            pk.metLevel(), pk.metLocation(), pk.ball(),
                            pk.isEgg(), pk.isFatefulEncounter(), pk.otName(),
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
                        },
                        bacd);
                const bool pcjpMachine =
                    Gen3PcjpMachineEvent::matches(
                        {
                            species, pk.tid16(), pk.sid16(),
                            pk.originGame(), pk.language(), pk.otGender(),
                            pk.metLevel(), pk.metLocation(), pk.ball(),
                            pk.isEgg(), pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
                const bool pcnyMachine =
                    Gen3PcnyEvent::matches(
                        {
                            species, pk.tid16(), pk.sid16(),
                            pk.originGame(), pk.language(),
                            pk.metLevel(), pk.metLocation(), pk.ball(),
                            pk.isEgg(), pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
                if (mystryMew) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Regular BA-CD RNG, released MYSTRY seed provenance, and persistent fields match the pinned MYSTRY Mew distribution template",
                        CheckIdentifier::EventGift);
                } else if (wishmkr) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Restricted BA-CD RNG and persistent fields match the pinned WISHMKR Jirachi distribution template",
                        CheckIdentifier::EventGift);
                } else if (berryFix) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Forced-shiny BA-CD RNG, RTC-derived seed range, and persistent fields match a pinned Berry Fix Zigzagoon distribution template",
                        CheckIdentifier::EventGift);
                } else if (negaiTable2) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Restricted BACD_TA table seed and persistent fields match the pinned Japanese Negai Boshi Jirachi distribution template",
                        CheckIdentifier::EventGift);
                } else if (negaiForceAnti) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Forced anti-shiny BA-CD RNG and persistent fields match the pinned Japanese Negai Boshi Jirachi distribution template",
                        CheckIdentifier::EventGift);
                } else if (pokeParkEgg) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Restricted regular BA-CD RNG and persistent egg fields match a pinned PokePark DS Download event egg template; post-hatch trainer/location history is intentionally not inferred",
                        CheckIdentifier::EventGift);
                } else if (pcjpFifthEgg) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Weighted BACD_TA/BACD_TS table seed, fixed unhatched egg fields and event moves match a pinned Pokemon Center Japan 5th Anniversary egg template; post-hatch reconstruction remains incomplete",
                        CheckIdentifier::EventGift);
                } else if (pokemonBoxEgg) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Unrestricted regular BA-CD RNG, fixed unhatched egg fields and event moves match a pinned Pokemon Box recipient egg template; post-hatch reconstruction remains incomplete",
                        CheckIdentifier::EventGift);
                } else if (pcjpMachine) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Regular BA-CD RNG, distribution TID/species/city OT fields and recovered OT gender match a pinned PCJP machine gift template",
                        CheckIdentifier::EventGift);
                } else if (pcnyMachine) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Forced anti-shiny BA-CD RNG plus species/level/distribution OT fields match a pinned PCNY machine gift row",
                        CheckIdentifier::EventGift);
                } else if (restrictedAntiEvent.matched) {
                    r.coverage.eventGift = CoverageLevel::Partial;
                    add(r, Severity::Info,
                        "Restricted anti-shiny BA-CD RNG and persistent fields match a pinned Gen III BACD_R_A distribution template",
                        CheckIdentifier::EventGift);
                } else {
                    add(r, Severity::Info,
                        "BA-CD event RNG evidence is present, but no supported exact Gen III distribution template was proven; distribution provenance remains incomplete",
                        CheckIdentifier::EventGift);
                }
                return;
            }

            add(r, Severity::Info,
                Gen3PidIv::isRoamerSpecies(species)
                    ? "No handheld Method 1/2/3/4, truncated-roamer, standard Colosseum/XD, Channel Jirachi, or regular/anti-shiny BA-CD PID/IV match; different-OT/restricted-template event variants remain incomplete"
                    : "No handheld Method 1/2/3/4, standard Colosseum/XD, Channel Jirachi, or regular/anti-shiny BA-CD PID/IV match; different-OT/restricted-template event variants remain incomplete",
                CheckIdentifier::PidRng);
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
        const auto storedGen4OriginKind = exactGeneration == 4
            ? Gen4Origin::kind(pk.originGame())
            : Gen4Origin::Kind::Unknown;
        const std::string_view gen4EncounterGameId = exactGeneration == 4
            ? Gen4Origin::exactRetailGameId(pk.originGame())
            : std::string_view{};
        std::string_view eggEvidenceGameId = exactSourceGameId;
        if (exactGeneration == 4) {
            if (!gen4EncounterGameId.empty()) {
                eggEvidenceGameId = gen4EncounterGameId;
            } else if (storedGen4OriginKind == Gen4Origin::Kind::Gen3Handheld) {
                eggEvidenceGameId = Gen4Origin::exactGen3GameId(pk.originGame());
            } else {
                eggEvidenceGameId = {};
            }
        }

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
                    CheckIdentifier::Transfer);
            } else if (evidence == Evidence::PossibleTimeCapsuleHeldItem) {
                add(r, Severity::Info,
                    "PK1 catch-rate byte is compatible with a Generation II held item after Time Capsule tradeback",
                    CheckIdentifier::Transfer);
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
        if (sourceProfile && exactGeneration == 2 &&
            pk.getGameGroup() == Pokemon::Pokemon2ReadOnly::kReadOnlyGameGroup) {
            const auto& gen2 = static_cast<const Pokemon::Pokemon2ReadOnly&>(pk);
            const std::array<uint16_t, 4> moves{
                gen2.move(0), gen2.move(1), gen2.move(2), gen2.move(3)
            };
            if (Gen12TimeCapsule::couldOriginateGen1(
                    gen2.speciesID(), gen2.isEgg(), gen2.caughtData())) {
                add(r, Severity::Info,
                    "PK2 structure is compatible with a possible Generation I origin through Time Capsule; this does not prove the transfer occurred",
                    CheckIdentifier::Transfer);
            }
            if (Gen12TimeCapsule::canCurrentlyTradeToGen1(
                    gen2.speciesID(), gen2.isEgg(), moves)) {
                add(r, Severity::Info,
                    "Current PK2 species/moves are compatible with a Gen II -> Gen I Time Capsule trade",
                    CheckIdentifier::Transfer);
            }

            if (Gen2Static::matches(
                    exactSourceGameId, gen2.speciesID(), gen2.level(),
                    gen2.caughtData(), gen2.isEgg(),
                    gen2.isShiny(gen2.id32(), {}))) {
                add(r, Severity::Info,
                    "PK2 data is compatible with a pinned Generation II static/gift encounter",
                    CheckIdentifier::Encounter);
            } else if (Gen2Static::hasSpecies(exactSourceGameId, gen2.speciesID())) {
                add(r, Severity::Info,
                    "No matching Gen II static/gift evidence; wild/trade/event provenance remains incomplete",
                    CheckIdentifier::Encounter);
            }

            if (Gen2Trade::matches(
                    exactSourceGameId, gen2.speciesID(), gen2.level(), gen2.tid16(),
                    std::array<uint8_t,5>{gen2.dvHP(), gen2.dvATK(), gen2.dvDEF(),
                                          gen2.dvSPE(), gen2.dvSpecial()},
                    gen2.caughtData())) {
                add(r, Severity::Info,
                    "PK2 trainer/DV/caught-data is compatible with a pinned Generation II in-game trade; nickname/OT language proof remains incomplete",
                    CheckIdentifier::Encounter);
            }
            if (gen2.caughtData() != 0 && exactSourceGameId == "crystal_gbc") {
                if (Gen2Wild::matchesCrystalCaughtData(gen2.speciesID(), gen2.caughtData())) {
                    add(r, Severity::Info,
                        "PK2 caught-data location/level/time matches a pinned Crystal wild encounter slot",
                        CheckIdentifier::Encounter);
                } else if (Gen2Wild::hasSpecies(exactSourceGameId, gen2.speciesID())) {
                    add(r, Severity::Info,
                        "Species has Crystal wild-slot evidence, but the stored caught-data fields do not directly match a pinned slot; evolution/trade/headbutt provenance remains incomplete",
                        CheckIdentifier::Encounter);
                }
            } else if (gen2.caughtData() == 0 &&
                       (exactSourceGameId == "gold_gbc" || exactSourceGameId == "silver_gbc") &&
                       Gen2Wild::hasSpecies(exactSourceGameId, gen2.speciesID())) {
                add(r, Severity::Info,
                    "Species has pinned Gold/Silver wild-slot provenance; those formats do not retain Crystal-style met location/level/time",
                    CheckIdentifier::Encounter);
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
            if (exactGeneration == 3 || exactGeneration == 4) {
                r.coverage.pidRng = CoverageLevel::Partial;
                r.coverage.eggBreeding = CoverageLevel::Partial;
            }
            // Gen I/III containers cannot carry an earlier-generation entity history.
            // Gen II Time Capsule history is only partially recoverable from PK1/PK2 data.
            if (exactGeneration == 1 || exactGeneration == 3)
                r.coverage.transfer = CoverageLevel::Complete;
            else if (exactGeneration == 2)
                r.coverage.transfer = CoverageLevel::Partial;
            else if (exactGeneration == 4)
                r.coverage.transfer = CoverageLevel::Complete;
        }

        // Native Gen III Safari capture history is positive source affinity
        // only. Use the PK3 stored origin (not necessarily the current
        // Ruby/Emerald/FRLG save container, which may hold traded Pokémon).
        // Evolution is included only via the pinned pre-evolution table;
        // unknown encounter/gift/egg/transfer alternatives stay Incomplete.
        if (sourceProfile && exactGeneration == 3 &&
            pk.ball() == Gen3Safari::kSafariBall && !pk.isEgg()) {
            const std::string_view storedOrigin =
                Gen4Origin::exactGen3GameId(pk.originGame());
            const auto safari = Gen3SafariOrigin::wildHistoryEvidence(
                storedOrigin, species, pk.metLocation(), pk.metLevel(),
                pk.form());
            if (safari.resolution ==
                    Gen3SafariOrigin::Resolution::RequiredBallKnown &&
                safari.requiredBall == Gen3Safari::kSafariBall) {
                add(r, Severity::Info,
                    safari.evolved
                        ? "Safari Ball has a compatible Generation III Safari Zone capture via a pre-evolution from the stored origin game; competing history remains incomplete"
                        : "Safari Ball has a compatible direct Generation III Safari Zone capture source in the stored origin game; competing history remains incomplete",
                    CheckIdentifier::Items);
            }
        }

        bool matchedGen4EventTemplate = false;
        const uint64_t* directGen4StaticRow = nullptr;
        auto directGen4StaticPidCategory = Gen4Static::PidCategory::None;
        bool exactGen4FormValid = true;
        if (sourceProfile && exactGeneration == 4) {
            exactGen4FormValid =
                Gen4Form::isFormValid(exactSourceGameId, species, pk.form());
            if (!exactGen4FormValid) {
                add(r, Severity::Invalid,
                    "Form " + std::to_string(pk.form()) +
                    " is not available in this exact Generation IV game",
                    CheckIdentifier::Species);
            } else if (!gen4EncounterGameId.empty()) {
                directGen4StaticRow = Gen4Static::findMatch(
                    gen4EncounterGameId, species, pk.metLocation(), pk.metLevel(),
                    pk.form(), pk.eggLocation(), pk.ball(),
                    pk.gender(), pk.nature(),
                    pk.isShiny(pk.id32(), {}), pk.isFatefulEncounter());
            }
            if (directGen4StaticRow) {
                directGen4StaticPidCategory =
                    Gen4Static::pidCategoryForRow(*directGen4StaticRow);
                if (Gen4Release::staticEncounterUnreleased(
                        species, pk.language())) {
                    add(r, Severity::Invalid,
                        "Generation IV Shaymin static encounter was never released for Korean-language games",
                        CheckIdentifier::Encounter);
                }
            }

            const bool rangerManaphy = Gen4RangerManaphy::matches({
                species,
                pk.language(),
                pk.gender(),
                pk.isEgg(),
                pk.eggLocation(),
                pk.metLocation(),
                pk.ball(),
                pk.isFatefulEncounter()
            });
            if (rangerManaphy) {
                r.coverage.eventGift = CoverageLevel::Partial;
                const std::array<uint8_t, 6> rangerIvs{
                    pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()
                };
                const auto rangerPid = Gen4RangerManaphy::analyzePidIv(
                    {
                        species,
                        pk.language(),
                        pk.gender(),
                        pk.isEgg(),
                        pk.eggLocation(),
                        pk.metLocation(),
                        pk.ball(),
                        pk.isFatefulEncounter()
                    },
                    pk.pid(), rangerIvs, pk.tid16(), pk.sid16());
                if (rangerPid.matched()) {
                    add(r, Severity::Info,
                        "PK4 has positive Pokémon Ranger Manaphy PGT provenance and " +
                        std::string(Gen4RangerManaphy::pidEvidenceName(rangerPid.evidence)) +
                        " PID/IV evidence",
                        CheckIdentifier::EventGift);
                } else {
                    add(r, Severity::Info,
                        "PK4 has positive Pokémon Ranger Manaphy PGT structural provenance; exact PID/recipient history remains incomplete",
                        CheckIdentifier::EventGift);
                }
            }

            Gen4EventTemplate::MatchResult eventMatch{};
            if (exactGen4FormValid) {
                eventMatch = Gen4EventTemplate::matchEvolutionLine({
                    species,
                    pk.tid16(),
                    pk.sid16(),
                    pk.pid(),
                    pk.metLevel(),
                    pk.metLocation(),
                    pk.ball(),
                    pk.form(),
                    pk.gender(),
                    pk.language(),
                    pk.originGame(),
                    pk.otGender(),
                    pk.isFatefulEncounter()
                });
            }
            if (eventMatch.matched) {
                matchedGen4EventTemplate = true;
                r.coverage.eventGift = CoverageLevel::Partial;
                std::string eventDetail = eventMatch.evolved
                    ? "PK4 persistent invariant fields match a pinned Gen IV WC4/PCD Pokémon template after evolution from source species " +
                      std::to_string(eventMatch.sourceSpecies)
                    : "PK4 invariant fields match a pinned Gen IV WC4/PCD Pokémon template";
                if (eventMatch.cardId)
                    eventDetail += " (card " + std::to_string(eventMatch.cardId) + ")";
                eventDetail += eventMatch.evolved
                    ? "; exact native event provenance survives evolution, while OT text, ribbons, dates, form-specific source evolution, and trash-byte evidence remain incomplete"
                    : "; exact native met location is proven, while OT text, ribbons, dates, and trash-byte evidence remain incomplete";
                add(r, Severity::Info, std::move(eventDetail),
                    CheckIdentifier::EventGift);
            }

            const int originGeneration = Enums::getVersionGeneration(pk.originGame());
            if (originGeneration == 0) {
                add(r, Severity::Warning,
                    "PK4 origin game could not be mapped to a known generation",
                    CheckIdentifier::Transfer);
                r.coverage.transfer = CoverageLevel::Partial;
            } else if (originGeneration == 3) {
                const auto transfer = Gen4Transfer::classifyStoredFields(
                    3, pk.gen4MetLocationDP(), pk.gen4MetLocationExtended(), pk.isEgg());
                if (transfer == Gen4Transfer::Evidence::PalParkDiamondPearlFields ||
                    transfer == Gen4Transfer::Evidence::PalParkPtHgssFields) {
                    if (!Gen4Transfer::validPalParkBallFields(
                            transfer, pk.gen4BallDPPt(), pk.gen4BallHGSS())) {
                        add(r, Severity::Invalid,
                            "Gen III -> IV Pal Park split-ball fields are inconsistent with a Generation III source ball",
                            CheckIdentifier::Items);
                    } else {
                        add(r, Severity::Info,
                            std::string(Gen4Transfer::evidenceName(transfer)) +
                            "; split-ball fields are compatible with a Generation III source ball, while exact Pt/HGSS trash-byte provenance remains incomplete",
                            CheckIdentifier::Transfer);
                    }
                    r.coverage.transfer = CoverageLevel::Partial;
                } else if (Gen4Transfer::invalid(transfer)) {
                    add(r, Severity::Invalid,
                        std::string(Gen4Transfer::evidenceName(transfer)),
                        CheckIdentifier::Transfer);
                }
            } else if (originGeneration != 4) {
                add(r, Severity::Invalid,
                    "Origin generation " + std::to_string(originGeneration) +
                    " cannot be stored directly in a retail Generation IV PK4",
                    CheckIdentifier::Transfer);
            }
        }

        if (sourceProfile && (exactGeneration == 3 || exactGeneration == 4)) {
            const auto eggState = Gen34EggState::analyze(
                exactGeneration, pk.isEgg(), pk.eggLocation(), pk.metLevel());
            if (eggState.invalid()) {
                add(r, Severity::Invalid,
                    std::string(Gen34EggState::evidenceName(eggState.evidence)),
                    CheckIdentifier::Egg);
            } else if (eggState.applies()) {
                add(r, Severity::Info,
                    exactGeneration == 3
                        ? "PK3 unhatched egg state has the native met-level-0 structure; exact hatch-location and inherited-move evidence remain incomplete"
                        : "PK4 egg-origin state has native met-level-0 structure and egg-location evidence; hatch-location evidence is checked separately for hatched records and inherited-move evidence remains incomplete",
                    CheckIdentifier::Egg);
                // A native Gen III/IV egg-origin Poké Ball is compatible;
                // this is INFO, not a complete competing-history proof.
                const bool provenNativeEggOrigin = exactGeneration == 3
                    ? !Gen4Origin::exactGen3GameId(pk.originGame()).empty()
                    : Gen4Origin::isNativeRetailGen4(pk.originGame());
                if (provenNativeEggOrigin &&
                    Gen34EggBallEvidence::analyze(
                        exactGeneration, pk.isEgg(),
                        pk.eggLocation(), pk.metLevel(), pk.ball()) ==
                        Gen34EggBallEvidence::Kind::NativeEggPokeBall) {
                    add(r, Severity::Info,
                        "Native Generation III/IV egg origin has a compatible Poke Ball; ball inheritance is unavailable before Generation VI and alternative gift/transfer provenance remains incomplete",
                        CheckIdentifier::Items);
                }
            }

            if (exactGeneration == 4 && eggState.eggOrigin &&
                !eggState.invalid() && !pk.isEgg()) {
                if (Gen4Release::eggHatchLocationUnreleased(
                        pk.language(), pk.metLocation())) {
                    add(r, Severity::Invalid,
                        "Korean Generation IV egg-origin Pokemon cannot use Seabreak Path or Flower Paradise as a hatch location because the enabling Shaymin event was never distributed there",
                        CheckIdentifier::Egg);
                } else if (!Gen4Hatch::isValidHatchedEgg(
                               pk.originGame(), pk.eggLocation(),
                               pk.metLocation())) {
                    add(r, Severity::Invalid,
                        "Hatch location is not valid for this Generation IV egg origin",
                        CheckIdentifier::Egg);
                } else {
                    add(r, Severity::Info,
                        pk.eggLocation() == Gen4Hatch::LinkTrade4
                            ? "PK4 traded-egg hatch location is valid in a Generation IV game"
                            : "PK4 hatch location is valid for its stored Generation IV origin game",
                        CheckIdentifier::Egg);
                }
            }

            for (int slotIndex = 0; slotIndex < 4; ++slotIndex) {
                const uint16_t moveId = pk.move(slotIndex);
                const auto eggMove = eggEvidenceGameId.empty()
                    ? Gen34EggMove::MoveResult{}
                    : Gen34EggMove::classify(eggEvidenceGameId, species, moveId);
                if (!eggMove.matched())
                    continue;

                std::string detail =
                    "Move has positive " +
                    std::string(Gen34EggMove::evidenceName(eggMove.evidence)) +
                    " evidence for this exact generation/game: " +
                    std::string(Names::getMoveName(moveId));
                if (eggMove.evidence == Gen34EggMove::MoveEvidence::PreEvolutionEggMove)
                    detail += " (ancestor species " +
                              std::to_string(eggMove.sourceSpecies) + ")";
                add(r, Severity::Info, std::move(detail), CheckIdentifier::Egg);
            }
        }

        if (sourceProfile && exactGeneration == 3) {
            // Use the exact source container, never inferred transferred origin.
            const auto eggRibbons = Gen3EggEventRibbon::evaluate({
                pk.isEgg(), pk.ribbonEarth(), pk.ribbonNational(), pk.ribbonCountry(),
                pk.ribbonChampionBattle(), pk.ribbonChampionRegional(), pk.ribbonChampionNational()
            });
            if (eggRibbons.status == Gen3EggEventRibbon::Status::Invalid) {
                for (size_t i = 0; i < eggRibbons.contradictions.size(); ++i) {
                    if (eggRibbons.contradictions[i])
                        add(r, Severity::Invalid,
                            "Unhatched Gen III egg cannot have the " +
                                std::string(Gen3EggEventRibbon::kNames[i]) + " Ribbon",
                            CheckIdentifier::Egg);
                }
            }
            addGen3PidEvidence(r, pk, species);
        } else if (sourceProfile && exactGeneration == 4 &&
                   storedGen4OriginKind == Gen4Origin::Kind::Gen4Retail) {
            const bool isHgss =
                gen4EncounterGameId == "heartgold_nds" || gen4EncounterGameId == "soulsilver_nds";
            const std::array<uint8_t, 6> ivs{
                pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()
            };

            if (isHgss && pk.metLocation() == 233) {
                const std::array<uint16_t, 4> currentMoves{
                    pk.move(0), pk.move(1), pk.move(2), pk.move(3)
                };
                const auto walkerEncounter =
                    Gen4PokewalkerEncounter::matchEvolutionLine(
                        species, pk.metLevel(), pk.gender(), currentMoves);
                const uint16_t walkerSourceSpecies = walkerEncounter.matched
                    ? walkerEncounter.sourceSpecies
                    : Gen4PokewalkerEncounter::sourceSpeciesInEvolutionLine(species);

                if (walkerEncounter.matched) {
                    std::string detail =
                        "Species/level/gender/moves are compatible with pinned PokeWalker course " +
                        std::to_string(walkerEncounter.course) + " slot " +
                        std::to_string(walkerEncounter.slot);
                    if (walkerEncounter.evolved)
                        detail += " via captured ancestor species " +
                                  std::to_string(walkerEncounter.sourceSpecies);
                    add(r, Severity::Info, std::move(detail),
                        CheckIdentifier::Encounter);
                } else if (walkerSourceSpecies != 0) {
                    add(r, Severity::Info,
                        walkerSourceSpecies == species
                            ? "PokeWalker species is known, but exact course-slot evidence is unresolved after current level/gender/move checks"
                            : "PokeWalker met location and evolution line reach a pinned course species, but exact course-slot evidence is unresolved after current level/gender/move checks",
                        CheckIdentifier::Encounter);
                } else {
                    add(r, Severity::Info,
                        "PokeWalker met location detected; no direct or pre-evolution course species was proven",
                        CheckIdentifier::Encounter);
                }

                const uint16_t pidSpecies =
                    walkerSourceSpecies != 0 ? walkerSourceSpecies : species;
                const uint8_t genderRatio =
                    Pokemon::getPersonalInfo(pidSpecies, 0).genderRatio;
                if (Gen4PokewalkerPid::matches(
                        pk.pid(), pk.id32(), pk.nature(), pk.gender(), genderRatio)) {
                    add(r, Severity::Info,
                        pidSpecies == species
                            ? "PID matches the Generation IV PokeWalker trainer/nature/gender formula"
                            : "PID matches the Generation IV PokeWalker trainer/nature/gender formula using the captured ancestor species ratio",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        pidSpecies == species
                            ? "PokeWalker met location detected, but PID correlation is unresolved against the current species ratio"
                            : "PokeWalker evolution provenance was found, but PID correlation is unresolved against the captured ancestor species ratio",
                        CheckIdentifier::PidRng);
                }
            } else if (directGen4StaticPidCategory == Gen4Static::PidCategory::Pokewalker) {
                const uint8_t genderRatio =
                    Pokemon::getPersonalInfo(species, pk.form()).genderRatio;
                if (Gen4PokewalkerPid::matches(
                        pk.pid(), pk.id32(), pk.nature(), pk.gender(), genderRatio)) {
                    add(r, Severity::Info,
                        "Audited Gen IV static identity and PID match the required Pokewalker trainer/nature/gender formula",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "Audited Gen IV static identity requires the Pokewalker PID class, but that PID correlation was not proven; legality remains incomplete",
                        CheckIdentifier::PidRng);
                }
            } else if (directGen4StaticPidCategory == Gen4Static::PidCategory::ChainShiny) {
                const auto chain =
                    Gen4ChainShiny::analyze(pk.pid(), pk.id32(), ivs);
                if (chain.matched) {
                    add(r, Severity::Info,
                        "Audited forced-shiny Gen IV static identity and PID/IV/trainer IDs match the Chain Shiny RNG class",
                        CheckIdentifier::PidRng);
                } else {
                    add(r, Severity::Info,
                        "Audited forced-shiny Gen IV static identity requires the Chain Shiny RNG class, but the PID/IV correlation was not proven; legality remains incomplete",
                        CheckIdentifier::PidRng);
                }
            } else if (directGen4StaticPidCategory == Gen4Static::PidCategory::Method1OrCuteCharm) {
                const auto correlation = Gen4PidIv::analyze(pk.pid(), ivs);
                if (correlation.matched()) {
                    add(r, Severity::Info,
                        "Audited Gen IV static/gift identity and PID/IV spread match normal Method 1",
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
                            "Audited Gen IV static/gift identity has a compatible Cute Charm buffered PID surface; exact lead-frame eligibility remains incomplete",
                            CheckIdentifier::PidRng);
                    } else {
                        add(r, Severity::Info,
                            "Audited Gen IV static/gift identity matched, but Method 1 or compatible Cute Charm PID correlation was not proven; legality remains incomplete",
                            CheckIdentifier::PidRng);
                    }
                }
            } else {
                const auto correlation = Gen4PidIv::analyze(pk.pid(), ivs);
                if (correlation.matched()) {
                    const auto wildRng = Gen4WildRng::analyzeSupported(
                        gen4EncounterGameId, species, pk.metLocation(), pk.metLevel(),
                        pk.form(), pk.id32(), correlation.originSeed, pk.pid());
                    if (wildRng.matched()) {
                        add(r, Severity::Info,
                            Gen4LeadReporting::describeWildResult(
                                wildRng.leadHistoryMask,
                                Gen4WildRng::methodName(wildRng.method)),
                            CheckIdentifier::PidRng);
                    } else {
                        add(r, Severity::Info,
                            "PID/IV spread matches normal Gen IV Method 1; current supported Method J/K wild-slot correlation was not proven for this encounter, and additional lead/method branches remain incomplete",
                            CheckIdentifier::PidRng);
                    }
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
                            const auto radar =
                                Gen4Wild::radarEvidence(
                                    gen4EncounterGameId, species,
                                    pk.metLocation(), pk.metLevel(), pk.form());
                            if (radar.matched) {
                                std::string detail =
                                    "PID/IV/trainer IDs match the Gen IV Poké Radar Chain Shiny RNG class and a Radar-capable wild encounter";
                                if (radar.evolved)
                                    detail += " via captured ancestor species " +
                                              std::to_string(radar.sourceSpecies);
                                add(r, Severity::Info, std::move(detail),
                                    CheckIdentifier::PidRng);
                            } else {
                                add(r, Severity::Info,
                                    "PID/IV/trainer IDs match the Gen IV Poké Radar Chain Shiny RNG class, but no Radar-capable direct or base-form ancestor wild encounter row was proven; event/form-specific evolution provenance remains incomplete",
                                    CheckIdentifier::PidRng);
                            }
                        } else {
                            const auto gift =
                                Gen4MysteryGiftPid::analyze(pk.pid(), ivs);
                            if (gift.matched) {
                                r.coverage.eventGift = CoverageLevel::Partial;
                                add(r, Severity::Info,
                                    matchedGen4EventTemplate
                                        ? "PID/IV spread matches a Gen IV Mystery Gift anti-shiny ARNG reroll class and the invariant fields match a pinned WC4/PCD template; remaining distribution-history evidence is incomplete"
                                        : "PID/IV spread matches a Gen IV Mystery Gift anti-shiny ARNG reroll class; exact event-template provenance remains incomplete",
                                    CheckIdentifier::PidRng);
                                if (!matchedGen4EventTemplate) {
                                    add(r, Severity::Info,
                                        "Generation IV gift-generation evidence is present, but no pinned direct WC4/PCD template candidate matched; evolved-event and exact distribution provenance remain incomplete",
                                        CheckIdentifier::EventGift);
                                }
                            } else {
                                add(r, Severity::Info,
                                    "No normal Gen IV Method-1, Cute Charm, Chain Shiny, or Mystery Gift anti-shiny RNG match; other event RNG classes remain incomplete",
                                    CheckIdentifier::PidRng);
                            }
                        }
                    }
                }
            }
        } else if (sourceProfile && exactGeneration == 4 &&
                   Gen4Origin::isPalParkOrigin(pk.originGame())) {
            add(r, Severity::Info,
                "Pal Park preserves the Generation III PID/IV relationship; native Generation IV PID methods are not applicable",
                CheckIdentifier::Transfer);
            addGen3PidEvidence(r, pk, species);
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
            if (!Pokemon::isAbilityLegal(slots, pk.ability())) {
                add(r, Severity::Invalid, "Ability not legal for this species");
            } else if (sourceProfile && exactGeneration == 4 &&
                       storedGen4OriginKind == Gen4Origin::Kind::Gen4Retail) {
                const uint8_t slotIndex = static_cast<uint8_t>(pk.pid() & 1u);
                uint16_t expectedAbility = slots.slot[slotIndex];
                if (expectedAbility == 0)
                    expectedAbility = slots.slot[0];
                if (expectedAbility != 0 && pk.ability() != expectedAbility) {
                    add(r, Severity::Invalid,
                        "Ability does not match the Generation IV PID-selected native slot",
                        CheckIdentifier::Ability);
                }
            }
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
        const uint8_t ball = pk.ball();
        if (sourceProfile && (exactGeneration == 3 || exactGeneration == 4)) {
            if (Gen34BallDomain::isInvalid(exactGeneration, ball)) {
                add(r, Severity::Invalid,
                    "Ball id " + std::to_string(ball) +
                    " cannot exist in Generation " + std::to_string(exactGeneration),
                    CheckIdentifier::Items);
            }
        } else if (ball != 0 && ball > 37) {
            // Preserve the historical generic-format fallback where exact source identity
            // is unavailable. Zero remains the project's unwired/unknown sentinel.
            add(r, Severity::Invalid,
                "Ball id out of range (" + std::to_string(ball) + ")");
        }

        const uint8_t language = pk.language();
        if (sourceProfile && (exactGeneration == 3 || exactGeneration == 4)) {
            if (Gen34Language::isInvalid(exactGeneration, language)) {
                add(r, Severity::Invalid,
                    "Language id " + std::to_string(language) +
                    " cannot exist in Generation " + std::to_string(exactGeneration),
                    CheckIdentifier::Trainer);
            }
        } else if (language != 0 && (language > 10 || language == 6)) {
            // Preserve the historical generic-format fallback where exact source identity
            // is unavailable. Zero remains the project's unwired/unknown sentinel.
            add(r, Severity::Invalid,
                "Invalid language id (" + std::to_string(language) + ")");
        }

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
            (exactGeneration == 4 || (pk.metLocation() != 0 && pk.metLevel() != 0))) {
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
                // Encounter provenance follows the Pokemon's stored origin version, not the
                // current save container. A D-origin PK4 traded into Platinum must still be
                // checked against Diamond encounter data. Pal Park/PBR origins are separate.
                if (!gen4EncounterGameId.empty()) {
                    // Nincada captured in the HGSS Bug-Catching Contest may
                    // evolve into Shedinja. PKHeX permits Sport or Poké Ball
                    // for this history. Shedinja itself has no wild slot;
                    // keep this positive origin note outside direct wild
                    // matching, with no hard-Invalid on nonmatches.
                    if (!pk.isEgg() && species == 292 &&
                        Gen4SpecialBallEvidence::analyzeSupported(
                            gen4EncounterGameId, species,
                            pk.metLocation(), pk.metLevel(),
                            pk.form(), pk.ball()) ==
                            Gen4SpecialBallEvidence::Affinity::ShedinjaBugContest) {
                        add(r, Severity::Info,
                            "Shedinja's Sport or Poke Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest Nincada pre-evolution origin; competing history remains incomplete",
                            CheckIdentifier::Items);
                    }
                    if (pk.metLevel() != 0 && Legality::Gen4Wild::matchesWithTrainerId(
                            gen4EncounterGameId, species, pk.metLocation(), pk.metLevel(),
                            pk.form(), pk.id32())) {
                        add(r, Severity::Info,
                            "Met data matches an audited Generation IV wild encounter slot for the stored origin game",
                            CheckIdentifier::Encounter);
                        // Pinned Gen IV encounter-specific Ball evidence is
                        // POSITIVE ONLY. Unknown/evolved/transferred origins or
                        // ball mismatches must not be promoted to hard Invalid.
                        const auto ballAffinity =
                            Gen4SpecialBallEvidence::analyzeSupported(
                                gen4EncounterGameId, species,
                                pk.metLocation(), pk.metLevel(),
                                pk.form(), pk.ball());
                        if (ballAffinity == Gen4SpecialBallEvidence::Affinity::Sport) {
                            add(r, Severity::Info,
                                "Sport Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest source; other provenance remains incomplete",
                                CheckIdentifier::Items);
                        } else if (ballAffinity == Gen4SpecialBallEvidence::Affinity::Safari) {
                            add(r, Severity::Info,
                                "Safari Ball has a compatible HeartGold/SoulSilver Safari Zone encounter source; other provenance remains incomplete",
                                CheckIdentifier::Items);
                        } else if (ballAffinity == Gen4SpecialBallEvidence::Affinity::GreatMarsh) {
                            add(r, Severity::Info,
                                "Safari Ball has a compatible Diamond/Pearl/Platinum Great Marsh encounter source; other provenance remains incomplete",
                                CheckIdentifier::Items);
                        }
                    } else if (directGen4StaticRow != nullptr) {
                        add(r, Severity::Info,
                            "Met data matches an audited Generation IV static/gift encounter for the stored origin game",
                            CheckIdentifier::Encounter);
                    } else if (const auto trade =
                                   Legality::Gen4Trade::matchEvolutionLine(
                                       gen4EncounterGameId, species, pk.pid(), pk.id32(),
                                       pk.gender(), pk.otGender(), pk.abilityNumber(),
                                       pk.language(), std::array<uint8_t, 6>{
                                           pk.ivHP(), pk.ivATK(), pk.ivDEF(),
                                           pk.ivSPE(), pk.ivSPA(), pk.ivSPD()},
                                       pk.metLocation(), pk.metLevel());
                               trade.matched) {
                        std::string detail =
                            "Trainer/PID/IV/ability/language/met data matches an audited Generation IV in-game trade for the stored origin game";
                        if (trade.evolved)
                            detail += " via captured source species " +
                                      std::to_string(trade.sourceSpecies);
                        add(r, Severity::Info, std::move(detail),
                            CheckIdentifier::Encounter);
                    } else if (Legality::Gen4Wild::hasSpecies(gen4EncounterGameId, species) ||
                               Legality::Gen4Static::hasSpecies(gen4EncounterGameId, species) ||
                               Legality::Gen4Trade::hasSpecies(gen4EncounterGameId, species)) {
                        add(r, Severity::Info,
                            "No matching Gen IV wild/static/gift/trade evidence for the stored origin game; external event/PokeWalker evidence is incomplete",
                            CheckIdentifier::Encounter);
                    }
                } else if (storedGen4OriginKind == Gen4Origin::Kind::Gen3Handheld ||
                           storedGen4OriginKind == Gen4Origin::Kind::Gen3GameCube) {
                    add(r, Severity::Info,
                        "PK4 has a Gen III stored origin; native Gen IV encounter tables are not applicable after Pal Park",
                        CheckIdentifier::Encounter);
                } else if (storedGen4OriginKind == Gen4Origin::Kind::Gen4BattleRevolution) {
                    add(r, Severity::Info,
                        "PK4 has a Battle Revolution stored origin; retail D/P/Pt/HG/SS encounter tables are not applicable",
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
