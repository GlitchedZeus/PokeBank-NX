#pragma once
// L1 Stage 7 final informational joining boundary:
//  - Stage 6 custody history is independently witnessed, not inferred
//    from a detached PK1/PK2/PK3/PK4 entity.
//  - Stage 7 native record is validated by actual strict or encrypted
//    production parser before being passed here.
//  - No source saves, PKM buffers or application legality verdict are written.
//  - Neither a compatible nor a contradicted submitted route proves ALL
//    legal original alternative histories; no global Invalid/Legal.
#include "Legality/Gen14NativeReadinessGate.h"
#include "Legality/Gen14CrossOriginReconciliation.h"
#include <cstdint>
namespace Legality::Gen14ProductionReadinessBridge {
namespace N=Gen14NativeReadinessGate;
namespace C=Gen14CrossOriginReconciliation;
enum class Status : uint8_t {
 UnknownNativeRecord, UnknownForeignNativeFormat,
 UnknownOriginalCustody, UnknownSpecimenIdentity,
 UnknownAlternativeHistories, CandidateLocalNativeMismatch,
 CandidateLocalRouteContradiction, ConditionalNativeAndHistoryOnly
};
struct Result {
 Status status=Status::UnknownNativeRecord;
 uint16_t matchedNationalSpecies=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr uint8_t nativeGeneration(N::Format format) noexcept {
 switch(format) {
 case N::Format::PK1:return 1;
 case N::Format::PK2:return 2;
 case N::Format::PK3:return 3;
 case N::Format::PK4:return 4;
 default:return 0;
 }
}
constexpr Result evaluate(N::Format format,const N::Result& native,
                          const C::Route& route,
                          bool samePhysicalSpecimenIndependentlyAttested) noexcept {
 if(native.status!=N::Status::NativeSnapshotOnly &&
    native.status!=N::Status::NativeConsistentWithAttestedHistory)
    return {};
 if(!samePhysicalSpecimenIndependentlyAttested ||
    !route.currentSpecimenIdentityAndSnapshotIndependentlyAttested ||
    !route.rootSpecimen || !route.presentSave)
    return {Status::UnknownSpecimenIdentity};
 if(!nativeGeneration(format)||nativeGeneration(format)!=route.presentGeneration)
    return {Status::UnknownForeignNativeFormat};
 if(native.species!=route.presentSpecies) {
    return {Status::CandidateLocalNativeMismatch};
 }
 const auto custody=C::evaluate(route).status;
 if(custody==C::Status::LocalOriginalSourceContradiction ||
    custody==C::Status::LocalTransitionContradiction ||
    custody==C::Status::LocalPresentMismatch)
    return {Status::CandidateLocalRouteContradiction,native.species};
 if(custody==C::Status::ConditionalSourceAndCustodyOnly ||
    custody==C::Status::ConditionalOriginalStageAndCustodyOnly)
    return {Status::ConditionalNativeAndHistoryOnly,native.species};
 return {Status::UnknownOriginalCustody,native.species};
}
static_assert(nativeGeneration(N::Format::PK1)==1);
static_assert(nativeGeneration(N::Format::PK4)==4);
} // namespace Legality::Gen14ProductionReadinessBridge
