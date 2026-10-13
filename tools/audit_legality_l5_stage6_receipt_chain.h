#pragma once
// L5 audit-only, no Pokémon payloads or production legality verdict.
// This validates structure and lineage of *reported* evidence. Source hashes
// and self-asserted consents are NOT independent authentication.
#include <array>
#include <cstdint>

namespace Legality::Audit::L5ReceiptChain {
enum class Artifact : std::uint8_t { Unknown, CatalogProjection, OriginalSave, OriginalWonderCard,
                                        OriginalActionReceipt, ReleaseWindow, PermissionGrant };
enum class State : std::uint8_t { MissingEvidence, StructuralRejected,
    ChainUnanchored, RestrictedResearch, IndependentlyAttestedCandidate };
struct Record {
    std::uint64_t pokemon=0, source=0, chainId=0, sequence=0, identity=0;
    std::uint64_t contentHashHigh=0, contentHashLow=0;
    std::uint64_t previousHashHigh=0, previousHashLow=0;
    std::uint64_t entryHashHigh=0, entryHashLow=0;
    Artifact kind=Artifact::Unknown;
    bool payloadHashIndependentlyVerified=false;
    bool ownerConsentIndependentlyVerified=false;
    bool provenanceSignerIndependentlyVerified=false;
    bool originalDeviceCaptureVerified=false;
    bool originalTimelineVerified=false;
    bool redistributionExplicitlyPermitted=false;
    bool sourceChangedOrRewritten=false;
};
struct Input {
    std::uint64_t targetPokemon=0,targetSource=0,chainId=0;
    std::array<Record,16> records{};
    std::uint8_t count=0;
    std::uint64_t independentlyTrustedHeadHigh=0,independentlyTrustedHeadLow=0;
    bool independentlyTrustedHeadAvailable=false;
    bool entireOriginalTimelineEnumerated=false;
    bool ownershipAndCollectionAuthorityVerified=false;
};
struct Result { State state=State::MissingEvidence;
    std::uint8_t checked=0;
    bool intactReportedChain=false;
    bool originalCandidateAttested=false;
    bool redistributionCleared=false;
    bool wholePokemonLegal=false;
    bool archiveVerified=false;
};
constexpr bool original(Artifact k) noexcept {
    return k==Artifact::OriginalSave || k==Artifact::OriginalWonderCard ||
           k==Artifact::OriginalActionReceipt;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.targetPokemon || !in.targetSource || !in.chainId ||
       !in.count || in.count>in.records.size())return out;
    // The hashes here are 128-bit identifiers of externally recomputed SHA256
    // records. Only a separate byte-level verifier may assert their validity.
    std::uint64_t previousHigh=0,previousLow=0;
    bool hadOriginal=false,permission=false,allIndependentlyVerified=true;
    for(std::uint8_t i=0;i<in.count;++i){
        const auto& r=in.records[i];
        if(r.pokemon!=in.targetPokemon || r.source!=in.targetSource ||
           r.chainId!=in.chainId || r.sequence!=std::uint64_t(i)+1u || !r.identity ||
           !r.contentHashHigh || !r.contentHashLow ||
           !r.entryHashHigh || !r.entryHashLow ||
           r.previousHashHigh!=previousHigh ||
           r.previousHashLow!=previousLow || r.kind==Artifact::Unknown ||
           r.sourceChangedOrRewritten){
            out.state=State::StructuralRejected;return out;
        }
        for(std::uint8_t j=0;j<i;++j)
            if(r.identity==in.records[j].identity){
                out.state=State::StructuralRejected;return out;
            }
        previousHigh=r.entryHashHigh;previousLow=r.entryHashLow;
        ++out.checked;
        // Projection, date or a claimed grant cannot attest original bytes.
        if(original(r.kind)){
            hadOriginal=true;
            allIndependentlyVerified &= r.payloadHashIndependentlyVerified &&
                r.ownerConsentIndependentlyVerified &&
                r.provenanceSignerIndependentlyVerified &&
                r.originalDeviceCaptureVerified && r.originalTimelineVerified;
        }
        if(r.kind==Artifact::PermissionGrant)
            permission |= r.redistributionExplicitlyPermitted &&
                r.provenanceSignerIndependentlyVerified;
    }
    out.intactReportedChain=true;
    if(!in.independentlyTrustedHeadAvailable ||
       previousHigh!=in.independentlyTrustedHeadHigh ||
       previousLow!=in.independentlyTrustedHeadLow){
        out.state=State::ChainUnanchored;return out;
    }
    out.state=State::RestrictedResearch;
    if(!hadOriginal || !allIndependentlyVerified ||
       !in.entireOriginalTimelineEnumerated ||
       !in.ownershipAndCollectionAuthorityVerified)return out;
    out.originalCandidateAttested=true;
    out.state=State::IndependentlyAttestedCandidate;
    out.redistributionCleared=permission;
    out.archiveVerified=hadOriginal;
    // A chain proves neither an exhaustive legality search nor global Legal.
    return out;
}
} // namespace Legality::Audit::L5ReceiptChain
