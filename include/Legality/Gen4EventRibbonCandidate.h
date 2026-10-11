#pragma once
// EVENT-001: one externally validated event card's projected persistent
// ribbon requirements. Stored PK4 bits are an immutable FACT, but the
// existing 247-card event index does not project ribbon masks: therefore
// expected bits must be independent, explicitly known source evidence.
// A mismatch disproves only THAT hypothetical event source, not all routes.
#include "Legality/Gen4EventTemplate.h"
#include <array>
#include <cstdint>
namespace Legality::Gen4EventRibbonCandidate {
enum class Status : uint8_t {
    IdentityMismatch, UnavailableRibbonData, UnknownEventRibbonTemplate,
    ConflictingRequirement, CandidateCompatible, CandidateContradicted
};
struct Requirement {
    bool independentlyVerified=false;
    std::array<uint32_t,3> mustHave{};
    std::array<uint32_t,3> mustNotHave{};
};
struct Result {
    Status status=Status::UnknownEventRibbonTemplate;
    std::array<uint32_t,3> missing{};
    std::array<uint32_t,3> unexpected{};
};
constexpr bool cardMatches(uint16_t claimedCard,
                          const Gen4EventTemplate::Candidate& current) noexcept {
    if(!claimedCard)return false;
    for(const auto& t:Gen4EventTemplate::kGen4EventTemplates)
        if(t.cardId==claimedCard &&
           (Gen4EventTemplate::matches(t,current) ||
            Gen4EventTemplate::matchesEvolved(t,current)))
            return true;
    return false;
}
constexpr Result evaluate(
        const Gen4EventTemplate::Candidate& source,
        uint16_t cardId,bool hasNativeRibbonCapability,
        const std::array<uint32_t,3>& observed,
        const Requirement& requirement) noexcept {
    Result result{};
    if(!cardMatches(cardId,source))
        {result.status=Status::IdentityMismatch;return result;}
    if(!hasNativeRibbonCapability)
        {result.status=Status::UnavailableRibbonData;return result;}
    if(!requirement.independentlyVerified)return result;
    for(uint8_t i=0;i<3;++i) {
        if(requirement.mustHave[i]&requirement.mustNotHave[i]) {
            result.status=Status::ConflictingRequirement;return result;
        }
        result.missing[i]=requirement.mustHave[i]&~observed[i];
        result.unexpected[i]=requirement.mustNotHave[i]&observed[i];
    }
    result.status=Status::CandidateCompatible;
    for(uint8_t i=0;i<3;++i)
        if(result.missing[i]||result.unexpected[i])
            result.status=Status::CandidateContradicted;
    return result;
}
} // namespace Legality::Gen4EventRibbonCandidate
