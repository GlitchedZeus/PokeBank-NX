/**
 * Gen3Cxd.h - Colosseum / XD (GameCube) origin judged from the fields a transfer keeps.
 *
 * A PK3 whose origin game is 15 (GameVersion.CXD) came from Pokemon Colosseum or
 * Pokemon XD through the link cable. The Gen III analyzer (Gen3Legality.cpp) judges
 * such a record in a Gen III save in full. Later formats keep most of its fields: a
 * Pal Park PK4 (Gen4Legality.cpp) and a Poke Transfer PK5 (Gen5Legality.cpp) still
 * hold the species, PID, IVs, TID/SID, OT name and gender, language, ball, fateful
 * flag and the National and Earth Ribbons, but their met level and met location are
 * the transfer's, and the moves may have changed since. judge() runs the same PKHeX
 * GameCube rules (EncounterGenerator3GC, the Colosseum/XD templates, shadow team
 * locks, the Poke Spot, starter and trainer-ID RNG, CXDVerifier, the fateful, ball,
 * ribbon and ability rules) on exactly the fields the caller marks as known, so the
 * Gen IV and Gen V analyzers can stop reporting "the Colosseum/XD encounter is not
 * verified". It is a pure function: it reads the input, allocates its own state and
 * writes nothing back.
 *
 * Contract:
 * - Fields the caller does not know are declared unknown with the *Known flags (and
 *   hasOriginalMet). Unknown fields are never judged, so they can neither prove nor
 *   disprove anything: a missing field can only make the result less decisive.
 * - hasOriginalMet == false applies PKHeX's rules for a record whose Format is not 3:
 *   the met location is not compared (EncounterShadow3Colo.cs:178-182, "transfer
 *   location verified later") and the template level must be within the evolution
 *   chain (IsMatchLevel: evo.LevelMax >= Level, :173-176); the origin chain starts at
 *   level 2 (EncounterOrigin.GetLevelOriginMin3, EncounterOrigin.cs:73-82) and a Poke
 *   Spot level must be at most metLevel (MethodPokeSpot.cs:163). metLevel then holds
 *   the transfer level, as PKHeX's pk.MetLevel does.
 * - Verdict::Invalid means a verified PKHeX rule rejects every GameCube history for
 *   the known fields (reasons in `invalid`). Verdict::Matched means one template, its
 *   RNG correlation and every rule run on the known fields hold (`encounter` names it;
 *   `warnings` holds PKHeX Fishy findings the caller should keep as Warnings).
 *   Verdict::Undecided means a search guard was reached (`capped`) or a filter pattern
 *   could not be evaluated: nothing is proven either way, and `invalid` then lists
 *   findings that are undecided, not proven.
 * - Moves are judged only when movesKnown (a PK3 still in a Gen III save, where the
 *   moves are Gen III moves): the CXD learn source is Emerald's (LearnSource3E). A
 *   caller judging a later format verifies moves itself from `encounterSpecies`,
 *   `encounterLevel` and `encounterMoves`.
 * - Eggs: no Colosseum/XD template is an egg, so isEgg == true matches nothing (Invalid).
 * - Species above 386 or 0 are outside Gen III: Undecided with a warning.
 *
 * Rules and data follow PKHeX.Core at the commit pinned in Gen3LegalityDataCxd.h; see
 * docs/legality/GEN3_PKHEX_RULE_INVENTORY.md (GameCube section) for file:line cites.
 */
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Legality::Gen3Cxd {

/// The PKHeX GameCube template kind that matched.
enum class Template : uint8_t {
    None,
    XdTrade,            ///< EncounterTrade3XD (Duking's Meditite/Shuckle/Larvitar, Hordel's Elekid)
    ColoShadow,         ///< EncounterShadow3Colo (snagged in Colosseum)
    ColoEReaderShadow,  ///< EncounterShadow3Colo, Japanese e-Reader Card e Room (met location 128)
    ColoStarter,        ///< EncounterStarter3Colo (Espeon, Umbreon)
    ColoGift,           ///< EncounterGift3Colo with Version CXD (Duking's Plusle)
    XdShadow,           ///< EncounterShadow3XD (snagged in XD, including Miror B. rebattles)
    XdGift,             ///< EncounterStatic3XD (Eevee, Chikorita, Cyndaquil, Totodile)
    XdPokeSpot,         ///< EncounterSlot3XD (Poke Spot wild slot)
};

/// The transferred fields. Everything a flag does not cover is read as given.
struct Input {
    uint16_t species = 0;       ///< current National Dex species (1-386)
    uint8_t currentLevel = 1;   ///< current level (from EXP)
    /// true: metLevel/metLocation are the GameCube ones (a PK3 in a Gen III save).
    /// false: they were overwritten by a transfer (Pal Park, Poke Transfer); metLevel is
    /// the transfer level, metLocation is ignored.
    bool hasOriginalMet = true;
    uint8_t metLevel = 0;
    uint8_t metLocation = 0;
    uint8_t ball = 0;           ///< Gen III ball id (4 = Poke Ball)
    bool fateful = false;       ///< PK3 obedience/fateful flag (0x4F bit 7), kept by every transfer
    bool isEgg = false;
    uint32_t pid = 0;
    std::array<uint8_t, 6> ivs{};  ///< HP Atk Def Spe SpA SpD
    uint16_t tid = 0, sid = 0;
    uint8_t otGender = 0;       ///< 0 male, 1 female
    uint8_t language = 0;       ///< Gen III language id (1 JPN, 2 ENG, 3 FRE, 4 ITA, 5 GER, 7 SPA)

    bool namesKnown = false;    ///< ot/nickname/isNicknamed hold the Gen III strings (decoded)
    std::u16string ot;
    std::u16string nickname;
    bool isNicknamed = false;

    bool movesKnown = false;    ///< moves are the Gen III moves (record still in a Gen III save)
    std::array<uint16_t, 4> moves{};

    bool ribbonsKnown = false;  ///< ribbonNational / ribbonEarth are known
    bool ribbonNational = false;
    bool ribbonEarth = false;

    bool abilityKnown = false;  ///< abilityBit is known (1: the species' second ability)
    uint8_t abilityBit = 0;
};

enum class Verdict : uint8_t {
    Invalid,    ///< a verified rule rejects every Colosseum/XD history for the known fields
    Matched,    ///< one template and every rule on the known fields hold
    Undecided,  ///< a search guard or an unevaluable pattern: nothing proven either way
};

struct Result {
    Verdict verdict = Verdict::Undecided;
    Template matched = Template::None;
    std::string encounter;              ///< e.g. "XD shadow Pokemon Zapdos, level 50, met location 162"
    uint16_t encounterSpecies = 0;      ///< template species (what was snagged / received)
    uint8_t encounterLevel = 0;         ///< template level (Poke Spot: the slot's minimum)
    uint8_t encounterLevelMax = 0;      ///< Poke Spot: the slot's maximum; otherwise encounterLevel
    uint8_t encounterLocation = 0;      ///< template met location (Poke Spot: the area)
    std::array<uint16_t, 4> encounterMoves{};  ///< fixed template moveset; all 0 when level-up moves
    /// Transfer only (hasOriginalMet false): the movesets of later templates of the same kind,
    /// species and level that differ from the matched one only in a met location the transfer
    /// lost (e.g. Colosseum Shadow Suicune with Surf or Hydro Pump). PKHeX defers to the next
    /// encounter when the moves do not fit, so the caller tries these in order.
    std::vector<std::array<uint16_t, 4>> alternativeMoves;
    bool shadow = false;                ///< a shadow Pokemon: purified, so it carries the National Ribbon
    bool xd = false;                    ///< an XD template: the fateful (obedience) flag is set
    /// IFixedNickname (Hordel's Elekid "ZAPRONG"): the judge checked the nickname against the
    /// trade, so the player-nickname rules (the nicknamed flag included) do not apply.
    bool fixedNickname = false;
    /// Every GameCube template compatible with the known species, level and location,
    /// before the RNG, lock, ball and fateful rules (PKHeX enumerator order).
    std::vector<std::string> candidates;
    std::vector<std::string> invalid;   ///< PKHeX Invalid findings (undecided when verdict is Undecided)
    std::vector<std::string> warnings;  ///< PKHeX Fishy findings and Unknown notes
    bool capped = false;                ///< an RNG search guard was reached
    uint8_t pidType = 0;                ///< PKHeX PIDType index (see pidTypeName in Gen3Legality.cpp)
};

/// Judges a Colosseum/XD origin from the known fields. Pure; never throws on any input.
Result judge(const Input& in);

} // namespace Legality::Gen3Cxd
