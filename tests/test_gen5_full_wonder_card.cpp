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
    std::fill(full.begin()+kDateOffset,full.begin()+kDateEnd,0);
    full[kVersionOffset]=0x30; // raw full-card Black/White flags, NOT the PGF catalog's 0x03
    full[kLanguageOffset]=2; // English metadata claim
    full[0x2CA]=1; // special download animation, included in checksum
    checksum(full);
    const auto before=full;
    const auto good=parse(full);
    assert(good.checksumValid());
    assert(good.error==ParseError::None && good.versionFlags==0x30);
    assert(good.language==2 && good.storedChecksum==crc16(std::span<const uint8_t>(full).first(kChecksumOffset)));
    assert(good.payload().size()==kPgfSize && good.matchesPayload(std::span<const uint8_t>(full).first(kPgfSize)));
    std::array<uint8_t,kPgfSize> other{};
    std::copy_n(full.begin(),kPgfSize,other.begin());
    assert(good.matchesPayload(other));
    other[2]^=0x80;
    assert(!good.matchesPayload(other));
    assert(good.declaredEligibility(20,2)==Eligibility::DeclaredAllowed);
    assert(good.declaredEligibility(21,2)==Eligibility::DeclaredAllowed);
    assert(good.declaredEligibility(22,2)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(23,2)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(20,1)==Eligibility::DeclaredDisallowed);
    assert(good.declaredEligibility(19,2)==Eligibility::Unverified);
    assert(good.declaredEligibility(24,2)==Eligibility::Unverified);
    assert(good.declaredEligibility(20,6)==Eligibility::Unverified);
    assert(good.payloadRelation(other)==PayloadRelation::Different);
    std::copy_n(full.begin(),kPgfSize,other.begin());
    assert(good.payloadRelation(other)==PayloadRelation::Exact);
    // Only the documented four receipt-date bytes may differ. A checksum,
    // filename, card ID, or broad partial-PGF match is never enough.
    other[kDateOffset]=29;other[kDateOffset+1]=2;
    other[kDateOffset+2]=0xD0;other[kDateOffset+3]=7; // 2000 leap day
    assert(!good.matchesPayload(other));
    assert(good.payloadRelation(other)==PayloadRelation::ReceiptDateOnly);
    other[kDateOffset]=30;
    assert(good.payloadRelation(other)==PayloadRelation::Different);
    other[kDateOffset]=29;other[kDateOffset+2]=0xD1; // 2001 not leap
    assert(good.payloadRelation(other)==PayloadRelation::Different);
    other[kDateOffset]=28;other[0xB0]^=1;
    assert(good.payloadRelation(other)==PayloadRelation::Different);
    assert(full==before);
    assert(parse(std::span<const uint8_t>(full).first(kSize-1)).error==ParseError::WrongSize);
    std::array<uint8_t,kSize+1> oversized{};
    assert(parse(oversized).error==ParseError::WrongSize);
    full[0x2CA]^=1;
    auto corrupt=parse(full);
    assert(corrupt.error==ParseError::BadChecksum);
    assert(corrupt.payload().empty() && !corrupt.matchesPayload(other));
    assert(corrupt.declaredEligibility(20,2)==Eligibility::Unverified);
    assert(corrupt.payloadRelation(other)==PayloadRelation::Unverified);
    full[0x2CA]^=1;
    full[kLanguageOffset]=0; checksum(full);
    assert(parse(full).declaredEligibility(20,2)==Eligibility::Unverified);
    full[kLanguageOffset]=2;
    full[15]^=1; checksum(full);
    const auto changed=parse(full);
    assert(changed.checksumValid() && !changed.matchesPayload(std::span<const uint8_t>(before).first(kPgfSize)));
    for(const auto [raw,claims]:std::array<std::pair<uint16_t,uint8_t>,4>{{{0x10,2},{0x20,1},{0x30,3},{0xF0,15}}}) {
        full=before;full[kVersionOffset]=static_cast<uint8_t>(raw);checksum(full);
        const auto view=parse(full);
        assert(view.catalogVersionClaims()==claims);
        for(uint8_t version=19;version<=24;++version)for(uint8_t lang=0;lang<=9;++lang) {
            const bool knownLang=lang==1||lang==2||lang==3||lang==4||lang==5||lang==7||lang==8;
            const auto expected=version<20||version>23||!knownLang?Eligibility::Unverified:
                ((claims&(1u<<(version-20)))&&lang==2?Eligibility::DeclaredAllowed:Eligibility::DeclaredDisallowed);
            assert(view.declaredEligibility(version,lang)==expected);
        }
    }
    for(uint16_t raw:{0,3,5,0x40,0x80,0x8005,0x8030,0xFFFF}) {
        full=before;full[kVersionOffset]=raw&255;full[kVersionOffset+1]=raw>>8;checksum(full);
        const auto view=parse(full);
        assert(view.checksumValid()&&view.versionFlags==raw&&view.catalogVersionClaims()==0);
        assert(view.declaredEligibility(20,2)==Eligibility::Unverified);
    }
    for(size_t n=0;n<kSize;++n) assert(parse(std::span<const uint8_t>(before).first(n)).error==ParseError::WrongSize);
    for(size_t byte=0;byte<kSize;++byte) {
        full=before;full[byte]^=1;
        assert(parse(full).error==ParseError::BadChecksum);
    }
    std::cout<<"WC5FULL: CRC, exact-size, PGF ownership, complete version/language matrix, corruption and unverified cases PASS\n";
}
