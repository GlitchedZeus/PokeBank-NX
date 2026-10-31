#ifndef CONVERSION_GEN3_PID_SEARCH_H
#define CONVERSION_GEN3_PID_SEARCH_H

#include <cstddef>
#include <cstdint>
#include <optional>

// Gen III / IV PID-derived traits and the search that produces a PID holding a wanted set of
// them. Format facts (no rule here is invented):
//   * nature = PID % 25, gender = (PID & 0xFF) < ratio, ability slot = PID bit 0 (pokeemerald
//     GetNature / GetGenderFromSpeciesAndPersonality / GetMonData MON_DATA_ABILITY_NUM; PKHeX
//     6501f0ab PKM.PIDAbility for Gen III/IV origins, EntityGender.GetFromPID);
//   * shiny when (TID ^ SID ^ PID.high16 ^ PID.low16) < 8 (pokeemerald IsShinyOtIdPersonality;
//     PKHeX ShinyUtil for Gen III-V);
//   * Unown letter (Gen III only) = ((PID bits 24-25) << 6 | (16-17) << 4 | (8-9) << 2 | (0-1))
//     % 28 (pokeemerald GET_UNOWN_LETTER; PKHeX EntityPID.GetUnownForm3);
//   * Wurmple line: (PID >> 16) % 10 < 5 evolves toward Silcoon, otherwise Cascoon (pokeemerald
//     GetEvolutionTargetSpecies EVO_LEVEL_SILCOON / EVO_LEVEL_CASCOON; PKHeX WurmpleUtil).
// A PID satisfying several of these at once is CONSTRUCTED from its parts rather than filtered
// out of a random walk: the low half is drawn inside the wanted gender's byte range with the
// ability / letter bits set, and for a shiny the high half is low ^ TSV ^ k with k < 8. Only
// nature (1 in 25) and the Wurmple value (1 in 2) are left to retries, so a few dozen attempts
// always succeed when the combination exists in the format, and the search fails only when it
// does not (fixed-gender species asked for the other gender, a shiny Unown letter this TSV
// cannot show). Identical IVs/EVs never enter the picture: the PID is independent of them.
namespace Conversion::Gen3PidSearch {
    struct Traits {
        uint8_t nature = 0;       // 0..24
        uint8_t gender = 0;       // 0 male, 1 female, 2 genderless
        uint8_t genderRatio = 0;  // Gen III personal ratio
        uint8_t abilityBit = 0;   // PID bit 0: 0 slot 1, 1 slot 2
        uint32_t tid32 = 0;
        bool shiny = false;
        int unownForm = -1;              // 0..27 when PID-derived Unown letter must survive
        bool constrainAbilityBit = true; // false for Gen III duplicate/single-ability semantics
    };

    // The editors' form: every trait is either pinned or "don't care" (-1).
    struct Wanted {
        int shiny = -1;        // 0 / 1
        int gender = -1;       // 0 male, 1 female, 2 genderless
        int nature = -1;       // 0..24
        int abilityBit = -1;   // PID bit 0
        int unownForm = -1;    // 0..27, Gen III Unown only
        int wurmpleEvo = -1;   // 0 Silcoon line, 1 Cascoon line
        uint8_t genderRatio = 0;
        uint32_t tid32 = 0;    // TID | SID << 16
    };

    inline uint8_t genderForPid(uint32_t pid, uint8_t ratio) noexcept {
        if (ratio == 255) return 2;
        if (ratio == 254) return 1;
        if (ratio == 0) return 0;
        return static_cast<uint8_t>((pid & 0xFF) < ratio ? 1 : 0);
    }

    inline uint16_t trainerShinyValue(uint32_t tid32) noexcept {
        return static_cast<uint16_t>((tid32 & 0xFFFF) ^ (tid32 >> 16));
    }

    inline bool shinyForPid(uint32_t pid, uint32_t tid32) noexcept {
        const uint16_t psv = static_cast<uint16_t>((pid & 0xFFFF) ^ (pid >> 16));
        return static_cast<uint16_t>(psv ^ trainerShinyValue(tid32)) < 8;
    }

    inline uint8_t unownForm(uint32_t pid) noexcept {
        const uint32_t value = ((pid & 0x03000000u) >> 18) | ((pid & 0x00030000u) >> 12)
                             | ((pid & 0x00000300u) >> 6)  | (pid & 0x00000003u);
        return static_cast<uint8_t>(value % 28);
    }

    inline uint8_t wurmpleEvo(uint32_t pid) noexcept {
        return static_cast<uint8_t>(((pid >> 16) % 10u) < 5u ? 0 : 1);
    }

    inline uint32_t withUnownFormValue(uint32_t pid, uint32_t value) noexcept {
        constexpr uint32_t formBits = 0x03030303u;
        return (pid & ~formBits)
             | ((value & 0xC0u) << 18) | ((value & 0x30u) << 12)
             | ((value & 0x0Cu) << 6)  | (value & 0x03u);
    }

    inline uint32_t stampUnownForm(uint32_t pid, int form) noexcept {
        if (form < 0 || form >= 28) return pid;
        uint32_t patterns[10] = {};
        std::size_t count = 0;
        for (uint32_t value = static_cast<uint32_t>(form); value <= 0xFFu; value += 28)
            patterns[count++] = value;
        // Choose the pattern from bits outside the form mask, so stamping is idempotent:
        // stampUnownForm(stampUnownForm(pid, f), f) == stampUnownForm(pid, f).
        const uint32_t stable = pid & ~0x03030303u;
        return withUnownFormValue(pid, patterns[(stable >> 2) % count]);
    }

    inline bool satisfies(uint32_t pid, const Wanted& w) noexcept {
        if (w.shiny >= 0 && shinyForPid(pid, w.tid32) != (w.shiny != 0)) return false;
        if (w.gender >= 0 && genderForPid(pid, w.genderRatio) != w.gender) return false;
        if (w.nature >= 0 && static_cast<int>(pid % 25u) != w.nature) return false;
        if (w.abilityBit >= 0 && static_cast<int>(pid & 1u) != w.abilityBit) return false;
        if (w.unownForm >= 0 && unownForm(pid) != w.unownForm) return false;
        if (w.wurmpleEvo >= 0 && wurmpleEvo(pid) != w.wurmpleEvo) return false;
        return true;
    }

    // Deterministic: the same seed and wants give the same PID, so a staged edit replays to the
    // same bytes. Returns nullopt only when no PID in the format holds the wanted traits.
    inline std::optional<uint32_t> construct(uint32_t seed, const Wanted& w,
                                             std::size_t maxAttempts = 4096) noexcept {
        if (w.gender > 2 || w.nature > 24 || w.abilityBit > 1 || w.unownForm > 27 ||
            w.wurmpleEvo > 1 || w.shiny > 1)
            return std::nullopt;
        const uint8_t ratio = w.genderRatio;
        const bool variableRatio = ratio != 0 && ratio != 254 && ratio != 255;
        if (w.gender >= 0) {
            if (!variableRatio && genderForPid(0, ratio) != w.gender) return std::nullopt;
            if (variableRatio && w.gender == 2) return std::nullopt;
        }
        const uint16_t tsv = trainerShinyValue(w.tid32);

        // Low-byte bits that are pinned before the gender byte is drawn: the ability bit and, for
        // Unown, the letter's two low bits (every pattern for one letter shares bit 0, because 28
        // is even, so an ability bit that disagrees with the letter is simply impossible).
        uint32_t patterns[10] = {};
        std::size_t patternCount = 0;
        if (w.unownForm >= 0) {
            for (uint32_t v = static_cast<uint32_t>(w.unownForm); v <= 0xFFu; v += 28) {
                if (w.abilityBit >= 0 && static_cast<int>(v & 1u) != w.abilityBit) continue;
                // Shiny: high = low ^ TSV ^ k only touches bits 16-18 through k, so PID bits
                // 24-25 are forced to (pattern bits 2-3) ^ (TSV bits 8-9); keep the patterns
                // whose bits 6-7 agree. None agreeing means this TSV has no shiny of this letter.
                if (w.shiny == 1 && ((v >> 6) & 3u) != (((v >> 2) & 3u) ^ ((tsv >> 8) & 3u))) continue;
                patterns[patternCount++] = v;
            }
            if (patternCount == 0) return std::nullopt;
        }

        uint32_t walk = seed;
        const auto step = [&walk]() noexcept {
            walk = walk * 0x41C64E6Du + 0x00006073u;
            return walk;
        };
        for (std::size_t attempt = 0; attempt < maxAttempts; ++attempt) {
            const uint32_t r1 = step();
            const uint32_t r2 = step();
            const uint32_t pattern = patternCount ? patterns[(r2 >> 4) % patternCount] : 0u;
            uint32_t mask = 0, value = 0;
            if (w.abilityBit >= 0) { mask = 1u; value = static_cast<uint32_t>(w.abilityBit); }
            if (patternCount) { mask = 3u; value = pattern & 3u; }
            const uint32_t stride = mask + 1u;

            // Gender byte: a uniform draw from the wanted range among bytes carrying `value`.
            uint32_t lowByte;
            const uint32_t draw = (r1 >> 8) & 0xFFFFu;
            if (w.gender >= 0 && variableRatio) {
                uint32_t first, count;
                if (w.gender == 1) {             // female: [0, ratio)
                    first = value;
                    count = (ratio - 1u - value) / stride + 1u;
                } else {                         // male: [ratio, 256)
                    first = (ratio & ~mask) | value;
                    if (first < ratio) first += stride;
                    count = (255u - first) / stride + 1u;
                }
                lowByte = first + stride * (draw % count);
            } else {
                lowByte = (draw & 0xFFu & ~mask) | value;
            }
            uint32_t low = (lowByte & 0xFFu) | (((r1 >> 24) & 0xFFu) << 8);
            if (patternCount) low = (low & ~0x300u) | ((pattern & 0x0Cu) << 6);

            uint32_t high;
            if (w.shiny == 1) {
                uint32_t k = (r2 >> 29) & 7u;
                if (patternCount)
                    k = (k & 4u) | ((pattern & 3u) ^ (tsv & 3u) ^ ((pattern >> 4) & 3u));
                high = (low ^ tsv ^ k) & 0xFFFFu;
            } else {
                high = (r2 >> 8) & 0xFFFFu;
                if (patternCount)
                    high = (high & ~0x0303u) | ((pattern >> 4) & 3u) | (((pattern >> 6) & 3u) << 8);
                // Not shiny wanted but the draw landed shiny: bit 3 of the high half is outside
                // every pinned field and lifts the XOR from < 8 into 8..15.
                if (w.shiny == 0 && static_cast<uint16_t>((low ^ high ^ tsv) & 0xFFFFu) < 8) high ^= 0x0008u;
            }
            const uint32_t pid = (high << 16) | low;
            if (satisfies(pid, w)) return pid;
        }
        return std::nullopt;
    }

    // Conversion entry point: a start PID that already holds every trait is kept; otherwise a
    // PID is constructed from it. `maxAttempts` bounds the nature / Wurmple retries.
    inline std::optional<uint32_t> find(uint32_t startPid,
                                        const Traits& wanted,
                                        std::size_t maxAttempts = 1000000) noexcept {
        Wanted w;
        w.shiny = wanted.shiny ? 1 : 0;
        w.gender = wanted.gender;
        w.nature = wanted.nature;
        w.abilityBit = wanted.constrainAbilityBit ? static_cast<int>(wanted.abilityBit) : -1;
        w.unownForm = wanted.unownForm;
        w.genderRatio = wanted.genderRatio;
        w.tid32 = wanted.tid32;
        if (maxAttempts == 0) return std::nullopt;
        if (satisfies(startPid, w)) return startPid;
        return construct(startPid, w, maxAttempts > 4096 ? 4096 : maxAttempts);
    }
}

#endif
