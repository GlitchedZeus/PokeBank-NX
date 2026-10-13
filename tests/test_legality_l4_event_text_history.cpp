#include "Legality/Gen4EventTextHistoryEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
namespace T=Legality::Gen4EventTextHistoryEvidence;
int main() {
    using S=T::Status;
    std::array<std::byte,16> ot{};
    ot[0]=std::byte{0x41};ot[2]=std::byte{0xff};ot[3]=std::byte{0xff};
    std::array<std::byte,22> nickname{};
    nickname[0]=std::byte{0x42};
    nickname[2]=std::byte{0xff};nickname[3]=std::byte{0xff};
    T::Source original{};
    original.originalPCDIndependentlyVerified=true;
    original.originalOTBytesKnown=true;
    original.originalNicknameBytesKnown=true;
    original.nativeOTStillComparable=true;
    original.nativeNicknameStillComparable=true;
    original.sourceOT=ot;
    original.sourceNickname=nickname;
    assert(T::evaluate(true,ot,nickname,original).status==S::CandidateBytesMatch);
    auto foreignOT=ot;foreignOT[8]=std::byte{0xa1};
    auto wrong=T::evaluate(true,foreignOT,nickname,original);
    assert(wrong.status==S::CandidateOTMismatch && wrong.originalTrainerCompared);
    auto foreignNickname=nickname;foreignNickname[8]=std::byte{0xa1};
    assert(T::evaluate(true,ot,foreignNickname,original).status==S::CandidateNicknameMismatch);
    auto unknown=original;unknown.originalPCDIndependentlyVerified=false;
    assert(T::evaluate(true,foreignOT,foreignNickname,unknown).status==S::UnknownOriginal);
    unknown=original;unknown.originalOTBytesKnown=false;
    assert(T::evaluate(true,foreignOT,foreignNickname,unknown).status==S::UnknownOriginal);
    unknown=original;unknown.evolvedFromCardSpecies=true;
    auto evolved=T::evaluate(true,ot,foreignNickname,unknown);
    assert(evolved.status==S::UnknownTransformation && evolved.originalTrainerCompared);
    assert(!evolved.nicknameCompared);
    unknown=original;unknown.nativeOTStillComparable=false;
    assert(T::evaluate(true,foreignOT,nickname,unknown).status==S::UnknownTransformation);
    unknown=original;unknown.palParkOrOtherReencodingPossible=true;
    assert(T::evaluate(true,foreignOT,nickname,unknown).status==S::UnknownTransformation);
    unknown=original;unknown.nativeNicknameStillComparable=false;
    assert(T::evaluate(true,ot,foreignNickname,unknown).status==S::UnknownTransformation);
    assert(T::evaluate(false,foreignOT,nickname,original).status==S::UnavailableNative);
    std::cout<<"L4 V14 raw PCD OT/nickname transformed-source guards: PASS\n";
}
