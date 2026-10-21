// L1 Stage 7: native PK1-4 snapshot cannot falsify an unattested original
// route. No format, checksum, or matching species can authorize global verdict.
#include "Legality/Gen14ProductionReadinessBridge.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace B=Legality::Gen14ProductionReadinessBridge;
namespace N=Legality::Gen14NativeReadinessGate;
namespace C=Legality::Gen14CrossOriginReconciliation;
C::Route original(unsigned generation){
 C::Route c{};
 c.rootSpecimen=55;c.routeId=70;c.originalSave=101;c.presentSave=101;
 c.capturedSpecies=25;c.presentSpecies=25;
 c.captureGeneration=generation;c.presentGeneration=generation;
 c.acquisition=C::Acquisition::Wild;c.evidence=C::Evidence::SourceTemplateOnly;
 c.originalSaveAndSpecimenIndependentlyAttested=true;
 c.originalAcquisitionSceneIndependentlyAttested=true;
 c.capturedSpeciesAndGenerationIndependentlyAttested=true;
 c.currentSpecimenIdentityAndSnapshotIndependentlyAttested=true;
 return c;
}
int main(){
 uint64_t assertions=0;
 for(unsigned generation=1;generation<=4;++generation){
   const auto format=static_cast<N::Format>(generation);
   auto record=N::Result{N::Status::NativeSnapshotOnly,25};
   auto route=original(generation);
   const auto matched=B::evaluate(format,record,route,true);
   assert(matched.status==B::Status::ConditionalNativeAndHistoryOnly);
   assert(!matched.globalInvalidSupported&&!matched.globallyLegalSupported);
   assertions+=2;
   auto missing=route;missing.originalSaveAndSpecimenIndependentlyAttested=false;
   record.species=26;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::UnknownOriginalCustody);
   ++assertions;
   record.species=25;
   missing=route;missing.originalAcquisitionSceneIndependentlyAttested=false;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::UnknownOriginalCustody);
   ++assertions;
   missing=route;missing.currentSpecimenIdentityAndSnapshotIndependentlyAttested=false;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::UnknownSpecimenIdentity);
   ++assertions;
   assert(B::evaluate(format,record,route,false).status==
          B::Status::UnknownSpecimenIdentity);
   ++assertions;
   record.species=26;
   assert(B::evaluate(format,record,route,true).status==
          B::Status::CandidateLocalNativeMismatch);
   ++assertions;
   record.species=25;
   missing=route;missing.evidence=C::Evidence::OriginalRngStageOnly;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::UnknownOriginalCustody);
   ++assertions;
   missing=route;missing.evidence=C::Evidence::LocalSourceContradiction;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::CandidateLocalRouteContradiction);
   ++assertions;
   missing=route;missing.evidence=C::Evidence::BoundedSearchUnknown;
   assert(B::evaluate(format,record,missing,true).status==
          B::Status::UnknownOriginalCustody);
   ++assertions;
   record.status=N::Status::UnknownNativeChecksum;
   assert(B::evaluate(format,record,route,true).status==
          B::Status::UnknownNativeRecord);
   ++assertions;
   record.status=N::Status::UnknownNativeDecryption;
   assert(B::evaluate(format,record,route,true).status==
          B::Status::UnknownNativeRecord);
   ++assertions;
   record.status=N::Status::NativeSnapshotOnly;
   const auto foreign=static_cast<N::Format>((generation%4)+1);
   assert(B::evaluate(foreign,record,route,true).status==
          B::Status::UnknownForeignNativeFormat);
   ++assertions;
 }
 for(unsigned kind=0;kind<256;++kind){
    for(unsigned evidence=0;evidence<256;++evidence){
       auto c=original(3);
       c.evidence=static_cast<C::Evidence>(evidence);
       auto record=N::Result{N::Status::NativeSnapshotOnly,25};
       const auto out=B::evaluate(static_cast<N::Format>(kind),record,c,true);
       assert(!out.globalInvalidSupported&&!out.globallyLegalSupported);
       if(kind<1||kind>4)
          assert(out.status==B::Status::UnknownForeignNativeFormat);
       else if(kind!=3)
          assert(out.status==B::Status::UnknownForeignNativeFormat);
       else if(evidence>static_cast<unsigned>(C::Evidence::BoundedSearchUnknown))
          assert(out.status==B::Status::UnknownOriginalCustody);
       ++assertions;
    }
 }
 std::cout<<"L1 Stage7 native/lineage 256^2 malformed format and evidence safety: "
          <<assertions<<" PASS (informational only)\n";
}
