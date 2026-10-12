// Gen V-IX foundation: exact game-group format boundaries and PK5/PK6
// *opaque* header evidence. Not a substitute for complete legality checks.
#include "Legality/Gen59FormatDomainEvidence.h"
#include "Legality/Gen56NativeEnvelopeEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#ifdef NDEBUG
#error "Native foundation checks must run with assertions enabled"
#endif

namespace D=Legality::Gen59FormatDomain;
namespace E=Legality::Gen56NativeEnvelope;
namespace G=Enums;
namespace {
void testDomains(uint64_t& checks) {
    const struct {G::GameVersion group;D::Kind kind;uint8_t gen;
                  uint16_t cap;bool mint;} rows[] {
        {G::GameVersion::GG,D::Kind::LetsGo,7,809,false},
        {G::GameVersion::GP,D::Kind::LetsGo,7,809,false},
        {G::GameVersion::GE,D::Kind::LetsGo,7,809,false},
        {G::GameVersion::SWSH,D::Kind::SwordShield,8,898,true},
        {G::GameVersion::SW,D::Kind::SwordShield,8,898,true},
        {G::GameVersion::SH,D::Kind::SwordShield,8,898,true},
        {G::GameVersion::BDSP,D::Kind::BrilliantDiamondShiningPearl,8,493,true},
        {G::GameVersion::BD,D::Kind::BrilliantDiamondShiningPearl,8,493,true},
        {G::GameVersion::SP,D::Kind::BrilliantDiamondShiningPearl,8,493,true},
        {G::GameVersion::PLA,D::Kind::LegendsArceus,8,905,true},
        {G::GameVersion::SV,D::Kind::ScarletViolet,9,1025,true},
        {G::GameVersion::SL,D::Kind::ScarletViolet,9,1025,true},
        {G::GameVersion::VL,D::Kind::ScarletViolet,9,1025,true},
        {G::GameVersion::ZA,D::Kind::LegendsZA,9,0,true},
    };
    for(const auto& row:rows) {
        const auto d=D::classify(row.group);
        assert(d.kind==row.kind && d.generation==row.gen);
        assert(d.maxNationalSpecies==row.cap);
        assert(d.mintedStatNature==row.mint && d.nativeLayoutSupported);
        assert(D::maxSpecies(row.group)==row.cap);
        assert(D::maxMove(row.group)==0);
        for(uint16_t candidate=1;candidate<=1100;++candidate)
            assert(D::impossibleSpecies(row.group,candidate)==
                (row.cap!=0 && candidate>row.cap));
        assert(!D::impossibleMove(row.group,65535));
        checks+=1106;
    }
    for(const auto unsupported : {
        G::GameVersion::Any,G::GameVersion::Invalid,
        G::GameVersion::FRLG,G::GameVersion::DP,G::GameVersion::PT,
        G::GameVersion::HGSS,G::GameVersion::GSC,
        G::GameVersion::RBY,G::GameVersion::Gen7B,
        G::GameVersion::Gen8,G::GameVersion::Gen9
    }) {
        const auto d=D::classify(unsupported);
        assert(d.kind==D::Kind::Unknown && d.generation==0);
        assert(d.maxNationalSpecies==0 && d.maxMove==0);
        assert(!d.nativeLayoutSupported && !d.mintedStatNature);
        ++checks;
    }
}
void testEnvelopes(uint64_t& checks) {
    std::array<std::byte,301> raw{};
    raw[0]=std::byte{0x78};raw[1]=std::byte{0x56};
    raw[2]=std::byte{0x34};raw[3]=std::byte{0x12};
    raw[6]=std::byte{0xAA};raw[7]=std::byte{0xBB};
    const auto immutable=raw;
    unsigned recognized=0;
    for(size_t size=0;size<=raw.size();++size) {
        if(size>raw.size())break; // no indexing beyond physical buffer
        const auto bytes=std::span<const std::byte>(raw.data(),size);
        const auto result=E::inspect(bytes);
        const auto expected=E::formatForLength(size);
        if(expected!=E::Format::Unknown) {
            assert(result.status==E::Status::HeaderOnly);
            assert(result.format==expected);
            assert(result.layout==E::layoutForLength(size));
            assert(result.pidOrEncryptionConstant==0x12345678u);
            assert(result.rawChecksum==0xBBAAu);
            assert(!result.decryptedFieldsAvailable);
            assert(!result.sourceHistoryAuthenticated);
            const auto opposite=expected==E::Format::PK5
                ? E::Format::PK6:E::Format::PK5;
            assert(E::inspect(bytes,opposite).status==E::Status::FormatMismatch);
            assert(E::inspect(bytes,expected).status==E::Status::HeaderOnly);
            ++recognized;
        } else {
            assert(result.status==E::Status::UnrecognizedLength);
            assert(result.format==E::Format::Unknown);
            assert(E::inspect(bytes,E::Format::PK6).status==
                E::Status::UnrecognizedLength);
        }
        ++checks;
    }
    assert(recognized==4);
    // A fake all-zero PK5/PK6 body is *still* only an opaque envelope,
    // never a verified checksum, original PID/IV or acquired Pokemon.
    for(const auto sz : {E::PK5StoredBytes,E::PK5PartyBytes,
                          E::PK6StoredBytes,E::PK6PartyBytes}) {
        std::array<std::byte,260> zeros{};
        const auto r=E::inspect(std::span<const std::byte>(zeros.data(),sz));
        assert(r.status==E::Status::HeaderOnly);
        assert(r.rawChecksum==0 && r.pidOrEncryptionConstant==0);
        assert(!r.decryptedFieldsAvailable && !r.sourceHistoryAuthenticated);
        ++checks;
    }
    assert(raw==immutable); // no source mutation
}
}
int main() {
    uint64_t checks=0;
    testDomains(checks);
    testEnvelopes(checks);
    assert(checks==14u*1106u+11u+302u+4u);
    std::cout<<"Gen V-IX native foundation: "<<checks
             <<" exact format and bounded transport assertions PASS; "
             <<"no native PK5/PK6 decoder or original-history certification\n";
}
