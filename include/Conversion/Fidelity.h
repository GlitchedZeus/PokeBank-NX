#ifndef CONVERSION_FIDELITY_H
#define CONVERSION_FIDELITY_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include "Utils/Gen3Text.h"

namespace Conversion {
    enum class Loss : uint32_t {
        None                    = 0,
        Gen3EVClamped           = 1u << 0,
        TeraDataDropped         = 1u << 1,
        ZAAlphaDropped          = 1u << 2,
        DivergentGameDataDropped= 1u << 3,
        OriginGameRestamped     = 1u << 4,
        MoveDropped             = 1u << 5,
        RelearnMoveDropped      = 1u << 6,
        HeldItemDropped         = 1u << 7,
        StatTrainingReset       = 1u << 8,
        PLAExclusiveDataDropped = 1u << 9,
        RibbonDataDropped       = 1u << 10,
    };

    enum class Adaptation : uint32_t {
        None                         = 0,
        PidAdjustedForShinyThreshold = 1u << 0,
        TargetDefaultTeraSynthesized = 1u << 1,
        MovePPClamped                = 1u << 2,
        Gen3TransferDateSynthesized  = 1u << 3,
        DefaultNicknameCanonicalized = 1u << 4,
    };

    struct Report {
        uint32_t losses = 0;
        uint32_t adaptations = 0;
        uint8_t sourceOriginVersion = 0;
        uint8_t destinationEntityOriginVersion = 0;

        void addLoss(Loss loss) noexcept { losses |= static_cast<uint32_t>(loss); }
        void addAdaptation(Adaptation change) noexcept { adaptations |= static_cast<uint32_t>(change); }
        bool hasLoss(Loss loss) const noexcept { return (losses & static_cast<uint32_t>(loss)) != 0; }
        bool hasAdaptation(Adaptation change) const noexcept {
            return (adaptations & static_cast<uint32_t>(change)) != 0;
        }
        bool hasLosses() const noexcept { return losses != 0; }
    };

    namespace Fidelity {
        inline uint16_t shinyXor(uint32_t pid, uint32_t id32) noexcept {
            const uint32_t mixed = pid ^ id32;
            return static_cast<uint16_t>(mixed ^ (mixed >> 16));
        }

        inline bool isGen3Shiny(uint32_t pid, uint32_t id32) noexcept {
            return shinyXor(pid, id32) < 8;
        }

        inline bool isModernShiny(uint32_t pid, uint32_t id32) noexcept {
            return shinyXor(pid, id32) < 16;
        }

        // Generation 6 doubled the shiny threshold. A Gen III non-shiny whose XOR is 8..15
        // would become shiny merely by being represented in a modern format. Official transfer
        // behavior avoids that by flipping the PID's top bit while keeping EC = the original PID.
        inline uint32_t adaptGen3PidForModern(uint32_t pid, uint32_t id32, Report* report = nullptr) noexcept {
            if (!isGen3Shiny(pid, id32) && isModernShiny(pid, id32)) {
                if (report) report->addAdaptation(Adaptation::PidAdjustedForShinyThreshold);
                return pid ^ 0x80000000u;
            }
            return pid;
        }

        inline uint8_t modernEvFromGen3(uint8_t value, Report* report = nullptr) noexcept {
            if (value > 252) {
                if (report) report->addLoss(Loss::Gen3EVClamped);
                return 252;
            }
            return value;
        }

        inline bool gen3InternationalLanguageSupported(uint8_t language) noexcept {
            // Current Gen3Text table is the international character table. Japanese requires
            // a separate table and later-generation Korean/Chinese language ids have no Gen III encoding.
            return language == 2 || language == 3 || language == 4 || language == 5 || language == 7;
        }

        inline bool gen3TextRepresentable(const std::u16string& value, std::size_t maxChars) noexcept {
            if (value.size() > maxChars) return false;
            for (char16_t c : value) {
                if (Utils::charToGen3(c) == Utils::GEN3_TERMINATOR) return false;
            }
            return true;
        }

        struct AbilityMapping {
            bool ok = false;
            uint8_t abilityBit = 0;
        };

        inline AbilityMapping mapModernAbilityToGen3(uint16_t abilityId, uint8_t abilityNumber,
                                                      uint16_t gen3Ability1, uint16_t gen3Ability2) noexcept {
            // Hidden ability has no Gen III representation.
            if (abilityNumber == 4) return {};
            if (abilityNumber == 1 && abilityId == gen3Ability1) return {true, 0};
            if (abilityNumber == 2 && abilityId == gen3Ability2) return {true, 1};
            return {};
        }

        inline uint16_t gen3AbilityId(uint8_t abilityBit, uint16_t ability1, uint16_t ability2) noexcept {
            return abilityBit ? ability2 : ability1;
        }

        inline uint8_t gen3AbilityNumberForModern(uint8_t abilityBit,
                                                  uint16_t ability1, uint16_t ability2) noexcept {
            // A duplicate Gen III ability has no meaningful slot distinction in a modern species
            // whose second slot may now mean something different. Normalize it to slot 1.
            if (ability1 == ability2) return 1;
            return abilityBit ? 2 : 1;
        }

        inline uint8_t defaultTeraType(uint8_t primary, uint8_t secondary) noexcept {
            return (primary == 0 && secondary != 255) ? secondary : primary;
        }
    }
}

#endif
