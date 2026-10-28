// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Legality/FiniteBreedingChain.h"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
namespace Legality::Breeding {
// A generous necessary-condition model over the known Gen III-V data. It
// ignores intermediate birth order, form restrictions, event gender locks and
// HM deletion at transfer. A route is NOT sufficient to certify an egg. Every
// such relaxation adds routes; finite exhaustion is distinct from a depth cap.
class Gen35Ancestry {
public:
    struct BirthWitness {
        uint8_t generation=0,version=0;
        uint16_t baby=0,father=0,mother=0;
        std::array<uint16_t,4> fatherMoves{},motherMoves{},bornMoves{};
        bool lightBall=false;
    };
    explicit Gen35Ancestry(const std::array<uint16_t,4>& moves);
    bool provenBirth5(uint16_t baby,uint8_t version,uint8_t fatherMoves,uint8_t sharedMoves) const;
    bool provenSimple(uint8_t generation,uint16_t baby,uint8_t fatherMoves,uint8_t sharedMoves) const;
    bool possible(uint8_t generation,uint16_t baby,uint8_t fatherMoves,uint8_t sharedMoves) const;
    std::vector<Step> fatherWitness(uint8_t generation,uint16_t baby,uint8_t fatherMoves,uint8_t sharedMoves) const;
    const std::vector<BirthWitness>& certifiedBirths() const {return certifiedBirths_;}
private:
    std::array<uint16_t,4> moves_;
    std::vector<Species> rules_;
    std::unique_ptr<FiniteChain> chain_;
    mutable std::vector<BirthWitness> certifiedBirths_;
    std::vector<uint16_t> maternalSpecies(uint8_t generation,uint16_t baby) const;
    uint16_t findFather(uint8_t generation,uint16_t baby,uint8_t fatherMoves,uint8_t sharedMoves,bool simple=false) const;
    bool simpleWitness(uint16_t node,uint8_t mask,bool allowBirth=true) const;
    bool birthWitness(uint8_t era,uint16_t baby,uint16_t father,uint8_t fatherMask,uint8_t wantedMask,uint8_t version=0,bool exact=false,uint8_t requiredShared=255) const;
};
}
