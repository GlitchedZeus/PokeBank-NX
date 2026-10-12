#include "Legality/Gen4EventTemplateAudit.h"
#include <cassert>
#include <iostream>
namespace A=Legality::Gen4EventTemplateAudit;
namespace E=Legality::Gen4EventArchiveAttestation;
constexpr auto original=[] {
    E::Source s{};
    s.projectionIndex=0;s.cardId=67;s.species=4;
    s.grade=E::ArchiveGrade::OriginalPCD;
    s.originalPayloadSha256[0]=std::byte{0x4a};
    s.digestIndependentlyChecked=true;
    s.releaseIndependentlyCorroborated=true;
    for(auto& f:s.fields) {
        f.origin=E::FieldOrigin::OriginalPayload;
        f.payloadSha256=s.originalPayloadSha256;
    }
    return s;
}();
static_assert(E::evaluate(original,A::kCount,67,4).status==
              E::Status::ConditionalOriginalWitness);
static_assert(E::evaluate(original,A::kCount,65,4).status==
              E::Status::IdentityMismatch);
int main() {
    for(std::size_t i=0;i<A::kCount;++i) {
        const auto& row=A::kRows[i];
        const auto& t=Legality::Gen4EventTemplate::kGen4EventTemplates[i];
        assert(row.projectionIndex==i && row.cardId==t.cardId);
        assert(row.species==t.species && row.rowsSharingCardId>=1);
        assert(row.archive==E::ArchiveGrade::Unverified);
        for(auto field:row.originalFields)assert(field==E::FieldOrigin::Unavailable);
    }
    assert(A::distinctCardIds()==70 && A::reusedIdRows()==219);
    assert(E::evaluate({},A::kCount,67,4).status==E::Status::UnknownProjection);
    auto s=original;
    s.digestIndependentlyChecked=false;
    assert(E::evaluate(s,A::kCount,67,4).status==E::Status::UnknownArchive);
    s=original;s.releaseIndependentlyCorroborated=false;
    assert(E::evaluate(s,A::kCount,67,4).status==E::Status::UnknownRelease);
    s=original;s.grade=E::ArchiveGrade::ReconstructedFromPGT;
    assert(E::evaluate(s,A::kCount,67,4).status==E::Status::NonOriginalArchive);
    s=original;s.grade=E::ArchiveGrade::Altered;
    assert(E::evaluate(s,A::kCount,67,4).status==E::Status::NonOriginalArchive);
    s=original;s.grade=E::ArchiveGrade::Unreleased;
    assert(E::evaluate(s,A::kCount,67,4).status==E::Status::NonOriginalArchive);
    s=original;s.fields[1].origin=E::FieldOrigin::Unavailable;
    auto missing=E::evaluate(s,A::kCount,67,4);
    assert(missing.status==E::Status::MissingField && missing.missingFields==2);
    s=original;s.fields[2].payloadSha256[3]=std::byte{0x7e};
    auto mixed=E::evaluate(s,A::kCount,67,4);
    assert(mixed.status==E::Status::IncoherentFieldPayload && mixed.incoherentFields==4);
    std::cout<<"L4 247-row projection audit + original-source gates: PASS\n";
}
