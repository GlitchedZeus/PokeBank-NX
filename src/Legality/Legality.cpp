/**
 * Legality.cpp - PKSE legality checker implementation (Layers 1+2, informational).
 *
 * See Legality.h. Every check reads the base Pokemon interface polymorphically;
 * per-gen behavior branches on the game group / capability flags, never on type
 * (RTTI is disabled). Optional fields that are unwired for a format read 0 and are
 * treated as "not applicable -> skip" so unsupported formats produce no false flags.
 */

#include "Legality/Legality.h"

#include <string>

#include "Pokemon/Pokemon.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/LearnsetTable.h"
#include "Trainer/Trainer.h"     // Trainer::getSpeciesName / getItemName (name-table sentinels)
#include "Names/MoveNames.h"     // Names::getMoveName
#include "Names/ItemNames.h"     // Names::getItemNameG3 (Gen 3 item id space)

namespace Legality {

    namespace {
        void add(Report& r, Severity sev, std::string text) {
            r.issues.push_back(Issue{sev, std::move(text)});
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
    }

    Report analyze(const Pokemon::Pokemon& pk, Enums::GameVersion originGroup) {
        Report r;
        const uint16_t species = pk.speciesID();
        if (species == 0) return r;  // empty slot — nothing to validate

        // LGPE (group GG) is the only pre-Gen8 supported group; the rest have mint/stat nature.
        const bool gen8plus = (originGroup != Enums::GameVersion::GG);

        // Species/form personal data (abilities / gender ratio / per-game presence) for the L2 checks.
        const Pokemon::PersonalInfo& pi = Pokemon::getPersonalInfo(species, pk.form());

        // ---- L1: species id known (name-table sentinel = "Unknown") ----
        if (std::string(Trainer::getSpeciesName(species)) == "Unknown")
            add(r, Severity::Invalid, "Unknown species id " + std::to_string(species));

        // ---- L1: nature range (+ stat/mint nature on Gen8+) ----
        if (pk.nature() > 24)
            add(r, Severity::Invalid, "Nature out of range (" + std::to_string(pk.nature()) + ")");
        if (gen8plus && pk.statNature() > 24)
            add(r, Severity::Invalid, "Stat nature out of range (" + std::to_string(pk.statNature()) + ")");

        // ---- L1: EVs (<=252 each, <=510 total) ----
        const uint8_t ev[6] = { pk.evHP(), pk.evATK(), pk.evDEF(), pk.evSPE(), pk.evSPA(), pk.evSPD() };
        int evTotal = 0;
        for (int i = 0; i < 6; ++i) {
            evTotal += ev[i];
            if (ev[i] > 252)
                add(r, Severity::Invalid, std::string(statName(i)) + " EV over 252 (" + std::to_string(ev[i]) + ")");
        }
        if (evTotal > 510)
            add(r, Severity::Invalid, "EV total over 510 (" + std::to_string(evTotal) + ")");

        // ---- L1: AVs (Let's Go only, <=200 each) ----
        if (pk.hasAwakeningValues()) {
            const uint8_t av[6] = { pk.avHP(), pk.avATK(), pk.avDEF(), pk.avSPE(), pk.avSPA(), pk.avSPD() };
            for (int i = 0; i < 6; ++i)
                if (av[i] > 200)
                    add(r, Severity::Invalid, std::string(statName(i)) + " AV over 200 (" + std::to_string(av[i]) + ")");
        }

        // ---- L1/L2: ability slot valid + ability id legal for the species/form ----
        {
            const uint8_t an = pk.abilityNumber();
            if (an != 1 && an != 2 && an != 4)
                add(r, Severity::Warning, "Unusual ability slot (" + std::to_string(an) + ")");
            // PKHeX encodes "single ability" as ability2 == ability1, so this set collapses naturally.
            const uint16_t ab = pk.ability();
            if (ab != pi.ability1 && ab != pi.ability2 && ab != pi.abilityHidden)
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
        if (Pokemon::getLearnableBits(species, pk.form(), originGroup) != nullptr) {
            for (int i = 0; i < 4; ++i) {
                const uint16_t m = pk.move(i);
                if (m != 0 && !Pokemon::isLearnable(species, pk.form(), originGroup, m))
                    add(r, Severity::Warning, "Move may not be learnable: " + std::string(Names::getMoveName(m)));
            }
        }

        // ---- L1: held item is a known id (skip when none) ----
        // Resolve through the id space the mon's own game uses -- a Gen 3 held item checked against
        // the modern table would either name the wrong item or be reported "unknown" when it is fine.
        if (pk.heldItem() != 0) {
            const char* heldName = (originGroup == Enums::GameVersion::FRLG)
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

        // ---- L2: gender vs the species gender ratio (255 genderless / 254 female-only / 0 male-only) ----
        {
            const uint8_t g = pk.gender();
            if (g > 2)
                add(r, Severity::Invalid, "Gender value out of range (" + std::to_string(g) + ")");
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
}
