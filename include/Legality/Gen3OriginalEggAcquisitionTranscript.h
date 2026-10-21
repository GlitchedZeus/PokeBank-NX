#pragma once
// L2 Gen III: independently recorded *original* pending and collection
// daycare transcript. Ruby/Sapphire/FRLG store the generated 16-bit pending
// personality, combining it with a new collection RNG high half; Emerald
// retains the complete pending personality. These are conditional evidence
// checks, not inference from a detached PK3 or global legality verdicts.
#include <cstdint>

namespace Legality::Gen3OriginalEggAcquisitionTranscript {
enum class Family : std::uint8_t { RubySapphireFireRedLeafGreen, Emerald };
enum class Status : std::uint8_t {
    NotProvided, UnknownSourceEvidence, IncoherentHistoricalSource,
    PendingPayloadContradicted, CollectionPayloadContradicted,
    ConditionallyMatched
};
struct Transcript {
    bool supplied=false;
    bool pendingPayloadIndependentlyVerified=false;
    bool collectedPayloadIndependentlyVerified=false;
    bool originalPhysicalPairIndependentlyVerified=false;
    bool stageIdentityIndependentlyVerified=false;
    std::uint64_t originalStageIdentity=0;
    std::uint32_t physicalSlot0Snapshot=0,physicalSlot1Snapshot=0;
    // The source's pending field is 16-bit in RS/FRLG and 32-bit in Emerald.
    std::uint32_t archivedPendingPayload=0, archivedCollectedPersonality=0;
};
struct Expected {
    Family source=Family::RubySapphireFireRedLeafGreen;
    std::uint64_t stageIdentity=0;
    std::uint32_t physicalSlot0Snapshot=0,physicalSlot1Snapshot=0;
    std::uint32_t pendingPersonality=0,finalPersonality=0;
};
constexpr Status evaluate(const Transcript& transcript,
                          const Expected& expected) noexcept {
    if(!transcript.supplied) return Status::NotProvided;
    if(!transcript.pendingPayloadIndependentlyVerified ||
       !transcript.collectedPayloadIndependentlyVerified ||
       !transcript.originalPhysicalPairIndependentlyVerified ||
       !transcript.stageIdentityIndependentlyVerified ||
       !transcript.originalStageIdentity ||
       !transcript.physicalSlot0Snapshot ||
       !transcript.physicalSlot1Snapshot ||
       transcript.physicalSlot0Snapshot==transcript.physicalSlot1Snapshot)
        return Status::UnknownSourceEvidence;
    // A transcript about some other original parents or collection event
    // must never provide negative proof for this egg acquisition route.
    if(!expected.stageIdentity || !expected.physicalSlot0Snapshot ||
       !expected.physicalSlot1Snapshot ||
       expected.physicalSlot0Snapshot==expected.physicalSlot1Snapshot ||
       transcript.originalStageIdentity!=expected.stageIdentity ||
       transcript.physicalSlot0Snapshot!=expected.physicalSlot0Snapshot ||
       transcript.physicalSlot1Snapshot!=expected.physicalSlot1Snapshot)
        return Status::IncoherentHistoricalSource;
    const auto archived=transcript.archivedPendingPayload;
    // This is a width check, never an attempt to recover the missing upper
    // PID bits from a modern child or from an unrelated save's RNG state.
    if(expected.source==Family::RubySapphireFireRedLeafGreen &&
       (archived&0xffff0000u))
        return Status::IncoherentHistoricalSource;
    if(archived!=expected.pendingPersonality)
        return Status::PendingPayloadContradicted;
    if(transcript.archivedCollectedPersonality!=expected.finalPersonality)
        return Status::CollectionPayloadContradicted;
    return Status::ConditionallyMatched;
}
static_assert(evaluate({}, {})==Status::NotProvided);
static_assert(evaluate({true,true,true,true,true,1,101,202,1,1},
                       {Family::RubySapphireFireRedLeafGreen,1,101,202,1,1})==
              Status::ConditionallyMatched);
static_assert(evaluate({true,true,true,true,true,1,101,202,0x10001,1},
                       {Family::RubySapphireFireRedLeafGreen,1,101,202,1,1})==
              Status::IncoherentHistoricalSource);
} // namespace Legality::Gen3OriginalEggAcquisitionTranscript
