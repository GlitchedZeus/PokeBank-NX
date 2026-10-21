#include "Legality/Gen5FullWonderCard.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>

using namespace Legality::Gen5Gift::FullCard;

static void checksum(std::array<uint8_t,kSize>& full) {
    auto value=crc16(std::span<const uint8_t>(full).first(kChecksumOffset));
    full[kChecksumOffset]=static_cast<uint8_t>(value);
    full[kChecksumOffset+1]=static_cast<uint8_t>(value>>8);
}

int main() {
    constexpr std::array<uint8_t,9> standard{'1','2','3','4','5','6','7','8','9'};
    static_assert(crc16(standard)==0x29B1); // public CCITT-FALSE independent check value
    std::array<uint8_t,kSize> full{};
    for(size_t i=0;i<kPgfSize;++i)full[i]=static_cast<uint8_t>(i*17);
    full[kVersionOffset]=0x05; // White and White 2; not Black or Black 2
    full[kVersionOffset+1]=0x80; // unknown high bits must be retained, not invented away
    full[kLanguageOffset]=2; // English metadata claim
    full[0x2CA]=1; // special download animation, included in checksum
    checksum(full);
    const auto before=full;
    const auto good=parse(full);
    assert(good.checksumValid());
    assert(good.error==ParseError::None && good.versionFlags==0x8005);
    assert(good.language==2 && good.storedChecksum==crc16(std::span<const uint8_t>(full).first(kChecksumOffset)));
    assert(good.payload().size()==kPgfSize && good.matchesPayload(std::span<const uint8_t>(full).first(kPgfSize)));
    std::array<uint8_t,kPgfSize> other{};
    std::copy_n(full.begin(),kPgfSize,other.begin());
    assert(good.matchesPayload(other));
    other[2]^=0x80;
    assert(!good.matchesPayload(other));
    assert(good.declaredEligibility(20,2)==Eligibility::DeclaredAllowed);
    assert(good.declaredEligibility(22,2)==Eligibility::DeclaredAllowed);
    assert(good.declaredEligibility(21,2)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(23,2)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(20,1)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(19,2)==Eligibility::Unverified);
    assert(good.declaredEligibility(24,2)==Eligibility::Unverified);
    assert(good.declaredEligibility(20,6)==Eligibility::Unverified);
    assert(full==before);
    assert(parse(std::span<const uint8_t>(full).first(kSize-1)).error==ParseError::WrongSize);
    std::array<uint8_t,kSize+1> oversized{};
    assert(parse(oversized).error==ParseError::WrongSize);
    full[0x2CA]^=1;
    auto corrupt=parse(full);
    assert(corrupt.error==ParseError::BadChecksum);
    assert(corrupt.payload().empty() && !corrupt.matchesPayload(other));
    assert(corrupt.declaredEligibility(20,2)==Eligibility::Unverified);
    full[0x2CA]^=1;
    full[kLanguageOffset]=0; checksum(full);
    assert(parse(full).declaredEligibility(20,2)==Eligibility::Unverified);
    full[kLanguageOffset]=2;
    full[15]^=1; checksum(full);
    const auto changed=parse(full);
    assert(changed.checksumValid() && !changed.matchesPayload(std::span<const uint8_t>(before).first(kPgfSize)));
    std::cout<<"WC5FULL: CRC, exact-size, PGF ownership, complete version/language matrix, corruption and unverified cases PASS\n";
}
