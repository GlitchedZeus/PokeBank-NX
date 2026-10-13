#pragma once
// L4: native Gen I PK1 reader consistency. No replay of L1 encounters and no
// legality verdict. Pinned to Gen1ReadOnlySave.cpp PK1 33/44-byte decode.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen1NativeRawRecordWitness {
enum class Status : uint8_t { UnknownBody, DecodedMismatch, Consistent };
enum Field : uint32_t {
    Species=1u<<0, HP=1u<<1, StatusByte=1u<<2, Types=1u<<3,
    CatchRate=1u<<4, Moves=1u<<5, TrainerId=1u<<6,
    Experience=1u<<7, StatExperience=1u<<8, DVs=1u<<9,
    PPAndUps=1u<<10, Level=1u<<11, PartyStats=1u<<12
};
struct Result {
    Status status=Status::UnknownBody;
    uint32_t contradicted=0;
    bool originalSourceAuthenticated=false, globallyInvalid=false;
};
constexpr uint16_t be16(const uint8_t* p) noexcept {
    return static_cast<uint16_t>((uint16_t(p[0])<<8)|p[1]);
}
constexpr uint32_t be24(const uint8_t* p) noexcept {
    return (uint32_t(p[0])<<16)|(uint32_t(p[1])<<8)|p[2];
}
// The native reader keeps raw body bytes but NOT original OT/nickname raw
// bytes inside PokemonRecord. Those reside in a separate list region.
template <class NativePK1> constexpr Result evaluate(const NativePK1& r) noexcept {
    Result out{};
    const std::size_t expected=r.partyRecord?44:33;
    if(r.rawBodySize!=expected || r.rawBody.size()<expected)return out;
    const auto* b=r.rawBody.data();
    if(b[0]!=r.rawSpecies)out.contradicted|=Species;
    if(be16(b+1)!=r.currentHP)out.contradicted|=HP;
    if(b[4]!=r.status)out.contradicted|=StatusByte;
    if(b[5]!=r.type1 || b[6]!=r.type2)out.contradicted|=Types;
    if(b[7]!=r.catchRate)out.contradicted|=CatchRate;
    for(size_t i=0;i<4;++i)if(b[8+i]!=r.moves[i])out.contradicted|=Moves;
    if(be16(b+0x0c)!=r.trainerId)out.contradicted|=TrainerId;
    if(be24(b+0x0e)!=r.experience)out.contradicted|=Experience;
    for(size_t i=0;i<5;++i)
        if(be16(b+0x11+i*2)!=r.statExperience[i])out.contradicted|=StatExperience;
    const auto packed=be16(b+0x1b);
    const uint8_t atk=(packed>>12)&15,def=(packed>>8)&15;
    const uint8_t spe=(packed>>4)&15,spc=packed&15;
    const uint8_t hp=static_cast<uint8_t>(((atk&1)<<3)|((def&1)<<2)|((spe&1)<<1)|(spc&1));
    if(r.dvs[0]!=hp || r.dvs[1]!=atk || r.dvs[2]!=def ||
       r.dvs[3]!=spe || r.dvs[4]!=spc)out.contradicted|=DVs;
    for(size_t i=0;i<4;++i) {
        if(r.pp[i]!=(b[0x1d+i]&0x3f) || r.ppUps[i]!=(b[0x1d+i]>>6))
            out.contradicted|=PPAndUps;
    }
    if((r.partyRecord?b[0x21]:b[3])!=r.level)out.contradicted|=Level;
    if(r.partyRecord) {
        if(be16(b+0x22)!=r.maxHP || be16(b+0x24)!=r.attack ||
           be16(b+0x26)!=r.defense || be16(b+0x28)!=r.speed ||
           be16(b+0x2a)!=r.special)out.contradicted|=PartyStats;
    }
    out.status=out.contradicted?Status::DecodedMismatch:Status::Consistent;
    return out;
}
enum class NameStatus : uint8_t { UnknownSourceBytes, Mismatch, BytesMatch };
// PK1 has no saved cartridge version. This compares only independently
// identified source-list name bytes, never Unicode presentation strings.
constexpr NameStatus compareRawListNames(std::span<const uint8_t> ot,
    std::span<const uint8_t> nickname,
    std::span<const uint8_t> attestedOT,
    std::span<const uint8_t> attestedNickname,
    bool sameListSlotIndependentlyVerified) noexcept {
    if(!sameListSlotIndependentlyVerified)return NameStatus::UnknownSourceBytes;
    const bool intl=ot.size()==11,jp=ot.size()==6;
    if((!intl&&!jp) || nickname.size()!=ot.size() ||
       attestedOT.size()!=ot.size() || attestedNickname.size()!=ot.size())
        return NameStatus::UnknownSourceBytes;
    for(size_t i=0;i<ot.size();++i)
        if(ot[i]!=attestedOT[i] || nickname[i]!=attestedNickname[i])
            return NameStatus::Mismatch;
    return NameStatus::BytesMatch;
}
static_assert(!Result{}.globallyInvalid);
} // namespace Legality::Gen1NativeRawRecordWitness
