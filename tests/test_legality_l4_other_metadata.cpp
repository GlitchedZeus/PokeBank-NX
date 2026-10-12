#include "Legality/Gen4EventOtherMetadataEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace M=Legality::Gen4EventOtherMetadataEvidence;
using S=M::FieldStatus;
void w16(std::vector<std::byte>& data,std::size_t offset,uint16_t value) {
    data[offset]=static_cast<std::byte>(value&255u);
    data[offset+1]=static_cast<std::byte>(value>>8u);
}
void w32(std::vector<std::byte>& data,std::size_t offset,uint32_t value) {
    w16(data,offset,static_cast<uint16_t>(value));
    w16(data,offset+2,static_cast<uint16_t>(value>>16u));
}
int main() {
    constexpr uint32_t pid=0x0f199b50u;
    constexpr M::Observed observed{4,0,66,0,
        static_cast<uint8_t>(pid%25u),pid};
    constexpr M::Source original{
        true,true,0,true,
        true,66,true,
        true,0,true,
        true,static_cast<uint8_t>(pid%25u),true
    };
    static_assert(M::evaluate(true,observed,original).allConditionalMatches());
    assert(!M::evaluate(false,observed,original).nativePresent);
    auto source=original;source.originalGiftIndependentlyVerified=false;
    auto unknown=M::evaluate(true,observed,source);
    assert(unknown.item==S::UnknownOriginal &&
           unknown.ability==S::UnknownOriginal &&
           unknown.form==S::UnknownOriginal &&
           unknown.nature==S::UnknownOriginal);
    source=original;source.originalItem=1;
    assert(M::evaluate(true,observed,source).item==S::CandidateContradicted);
    source.itemHistoryUnchanged=false;
    assert(M::evaluate(true,observed,source).item==S::UnknownTransformation);
    source=original;source.originalAbility=65;
    assert(M::evaluate(true,observed,source).ability==S::CandidateContradicted);
    source.speciesAndAbilityHistoryUnchanged=false;
    assert(M::evaluate(true,observed,source).ability==S::UnknownTransformation);
    source=original;source.originalForm=1;
    assert(M::evaluate(true,observed,source).form==S::CandidateContradicted);
    source.formHistoryUnchanged=false;
    assert(M::evaluate(true,observed,source).form==S::UnknownTransformation);
    source=original;source.originalNature=static_cast<uint8_t>((pid+1)%25u);
    assert(M::evaluate(true,observed,source).nature==S::CandidateContradicted);
    source.originalPidHistoryUnchanged=false;
    assert(M::evaluate(true,observed,source).nature==S::UnknownTransformation);
    source=original;source.originalNature=26;
    assert(M::evaluate(true,observed,source).nature==S::UnknownOriginal);
    source=original;source.originalItemKnown=false;
    assert(M::evaluate(true,observed,source).item==S::UnknownOriginal);

    // Encrypted PK4 uses exact PKHeX native offsets: item 0x0A,
    // ability 0x15, form bits 0x40, with nature derived from PID%25.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4);
    w32(raw,0,pid);w16(raw,0x08,4);w16(raw,0x0a,0);
    raw[0x15]=std::byte{66};
    raw[0x40]=std::byte{1}; // fateful, form zero
    w16(raw,0x06,Encryption::checksum4(raw));
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly pk4(encrypted,Enums::GameVersion::DP);
    assert(pk4.valid() && !pk4.empty());
    const auto immutable=pk4.originalEncryptedBytes();
    const std::vector<std::byte> before(immutable.begin(),immutable.end());
    const auto native=M::evaluateNative(pk4,original);
    assert(native.allConditionalMatches());
    source=original;source.originalItem=2;
    const auto mismatch=M::evaluateNative(pk4,source);
    assert(mismatch.item==S::CandidateContradicted && mismatch.anyContradiction());
    source.originalItemKnown=false;
    assert(M::evaluateNative(pk4,source).item==S::UnknownOriginal);
    auto bad=encrypted;bad[0x30]^=std::byte{0x01};
    Pokemon::Pokemon4ReadOnly damaged(bad,Enums::GameVersion::DP);
    assert(!M::evaluateNative(damaged,original).nativePresent);
    const auto after=pk4.originalEncryptedBytes();
    assert(after.size()==before.size());
    for(std::size_t i=0;i<after.size();++i)assert(after[i]==before[i]);
    std::cout<<"L4 V18 original item/ability/form/PID-nature evidence: PASS\n";
}
