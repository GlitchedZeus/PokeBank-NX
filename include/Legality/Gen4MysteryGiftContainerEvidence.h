#pragma once
// L4 Stage 3: bounded PGT / PCD *structure*, not proof of distribution.
// Reference: PKHeX.Core/MysteryGifts/{PGT,PCD}.cs (verified 2026-10-12).
// No raw copyrighted event payloads, third-party source code, or card data.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen4MysteryGiftContainerEvidence {
constexpr std::size_t PGTSize = 0x104;
constexpr std::size_t PCDSize = 0x358;
constexpr std::size_t PCDCardId = 0x150;
constexpr std::size_t PCDCompatibility = 0x14c;
constexpr std::size_t PK4WithinPGT = 8;
constexpr std::size_t PK4PartySize = 0xec;

enum class GiftType : uint16_t {
    None=0, Pokemon=1, PokemonEgg=2, Item=3, Rule=4, Goods=5,
    HasSubType=6, ManaphyEgg=7, MemberCard=8, OaksLetter=9,
    AzureFlute=10, PoketchApp=11, SecretKey=12, PokemonMovie=13,
    PokewalkerCourse=14, MemorialPhoto=15
};
enum class Format : uint8_t { Unknown, PGT, PCD };
enum class Delivery : uint8_t { Unknown, DirectPokemon, RangerManaphyEgg,
    ItemOrService, KeyItemUnlock, Other };
enum class Status : uint8_t { MissingOrWrongSize, EmptyGift, UnknownGiftType,
    StructuralPGT, StructuralPCD, PairedPayloadMatches, PairedPayloadMismatch };

struct Projection {
    Status status=Status::MissingOrWrongSize;
    Format format=Format::Unknown;
    GiftType type=GiftType::None;
    Delivery delivery=Delivery::Unknown;
    uint16_t cardId=0; // only known in PCD; never derive from PGT
    bool cardIdPresent=false;
    uint16_t rawCompatibility=0; // uninterpreted! Do not infer region/version.
    uint8_t linkedSlot=0;
    uint32_t itemId=0;
    bool canProjectPokemon=false;
    bool giftPayloadMayBeEncrypted=false;
    bool pgtCanRepresentFullCard=false; // always false
    bool pgtExtractionAllowed=false; // Lock Capsule restriction
};
constexpr uint8_t u8(std::span<const std::byte> p,std::size_t i) noexcept {
    return std::to_integer<uint8_t>(p[i]);
}
constexpr uint16_t u16(std::span<const std::byte> p,std::size_t i) noexcept {
    return static_cast<uint16_t>(u8(p,i)|(static_cast<uint16_t>(u8(p,i+1))<<8));
}
constexpr uint32_t u32(std::span<const std::byte> p,std::size_t i) noexcept {
    return static_cast<uint32_t>(u16(p,i)) | (static_cast<uint32_t>(u16(p,i+2))<<16);
}
constexpr bool known(GiftType t) noexcept {
    return static_cast<uint16_t>(t)<=15;
}
constexpr Delivery delivery(GiftType t) noexcept {
    switch(t) {
    case GiftType::Pokemon:
    case GiftType::PokemonEgg:
    case GiftType::PokemonMovie:return Delivery::DirectPokemon;
    case GiftType::ManaphyEgg:return Delivery::RangerManaphyEgg;
    case GiftType::Item:
    case GiftType::Rule:
    case GiftType::Goods:
    case GiftType::HasSubType:
    case GiftType::PoketchApp:
    case GiftType::PokewalkerCourse:
    case GiftType::MemorialPhoto:return Delivery::ItemOrService;
    case GiftType::MemberCard:
    case GiftType::OaksLetter:
    case GiftType::AzureFlute:
    case GiftType::SecretKey:return Delivery::KeyItemUnlock;
    default:return Delivery::Other;
    }
}
constexpr Projection inspect(std::span<const std::byte> raw) noexcept {
    Projection out{};
    if(raw.size()!=PGTSize && raw.size()!=PCDSize)return out;
    out.format=raw.size()==PCDSize?Format::PCD:Format::PGT;
    out.type=static_cast<GiftType>(u16(raw,0));
    if(out.type==GiftType::None) {out.status=Status::EmptyGift;return out;}
    if(!known(out.type)) {out.status=Status::UnknownGiftType;return out;}
    out.linkedSlot=u8(raw,2);
    out.itemId=u32(raw,4);
    out.delivery=delivery(out.type);
    out.canProjectPokemon=out.delivery==Delivery::DirectPokemon ||
                          out.delivery==Delivery::RangerManaphyEgg;
    // PKHeX PGT VerifyGiftEncryption considers Pokemon and PokemonEgg;
    // do not assume PGT Manaphy or movie bytes have the same cipher state.
    out.giftPayloadMayBeEncrypted=out.type==GiftType::Pokemon ||
                                   out.type==GiftType::PokemonEgg;
    out.pgtExtractionAllowed=!(out.type==GiftType::Item && out.itemId==533);
    if(out.format==Format::PCD) {
        out.cardId=u16(raw,PCDCardId);
        out.cardIdPresent=true;
        out.rawCompatibility=u16(raw,PCDCompatibility);
        out.status=Status::StructuralPCD;
    } else out.status=Status::StructuralPGT;
    return out;
}
// Like original PCD.GiftEquals(PGT), ignore ONLY the standalone PGT slot
// byte at offset 2. A matched raw PGT is not an authenticated release.
constexpr Projection inspectPair(std::span<const std::byte> pcd,
                                  std::span<const std::byte> pgt) noexcept {
    auto out=inspect(pcd);
    const auto inner=inspect(pgt);
    if(out.format!=Format::PCD || out.status!=Status::StructuralPCD ||
       inner.format!=Format::PGT || inner.status!=Status::StructuralPGT) {
        out.status=Status::MissingOrWrongSize;return out;
    }
    for(std::size_t i=0;i<PGTSize;++i) {
        if(i!=2 && pcd[i]!=pgt[i]) {
            out.status=Status::PairedPayloadMismatch;return out;
        }
    }
    out.status=Status::PairedPayloadMatches;
    return out;
}

enum class ArchiveGrade : uint8_t { Unknown, OriginalPCD, OriginalPGT,
    ConvertedOriginal, Reconstructed, Altered, Unreleased };
enum class ArchiveStatus : uint8_t { UnknownSource, UntrustedSource,
    UnknownDigest, UnknownRelease, UnknownReceipt,
    ConditionalOriginalSource };
using Digest=std::array<std::byte,32>;
struct Attestation {
    ArchiveGrade grade=ArchiveGrade::Unknown;
    Digest sourceDigest{};
    bool exactSourceIndependentlyIdentified=false;
    bool digestIndependentlyVerified=false;
    bool originalFieldsIndependentlyExtracted=false;
    bool releasedIndependentlyCorroborated=false;
    bool recipientHistoryIndependentlyKnown=false;
};
constexpr bool nonzero(const Digest& d) noexcept {
    for(auto b:d)if(b!=std::byte{0})return true;
    return false;
}
constexpr ArchiveStatus attest(const Projection& p,
                               const Attestation& s) noexcept {
    if(!(p.status==Status::StructuralPGT ||
         p.status==Status::StructuralPCD ||
         p.status==Status::PairedPayloadMatches) ||
       !s.exactSourceIndependentlyIdentified) return ArchiveStatus::UnknownSource;
    if(s.grade==ArchiveGrade::Unknown)return ArchiveStatus::UnknownSource;
    if(s.grade==ArchiveGrade::Reconstructed || s.grade==ArchiveGrade::Altered ||
       s.grade==ArchiveGrade::Unreleased)return ArchiveStatus::UntrustedSource;
    if((p.format==Format::PGT && s.grade==ArchiveGrade::OriginalPCD) ||
       (p.format==Format::PCD && s.grade==ArchiveGrade::OriginalPGT))
        return ArchiveStatus::UnknownSource;
    if(!s.digestIndependentlyVerified || !nonzero(s.sourceDigest) ||
       !s.originalFieldsIndependentlyExtracted)return ArchiveStatus::UnknownDigest;
    if(!s.releasedIndependentlyCorroborated)return ArchiveStatus::UnknownRelease;
    if(!s.recipientHistoryIndependentlyKnown)return ArchiveStatus::UnknownReceipt;
    return ArchiveStatus::ConditionalOriginalSource;
}
static_assert(PCDSize==856 && PGTSize==260);
static_assert(PK4WithinPGT+PK4PartySize<=PGTSize);
static_assert(delivery(GiftType::ManaphyEgg)==Delivery::RangerManaphyEgg);
static_assert(delivery(GiftType::MemberCard)==Delivery::KeyItemUnlock);
} // namespace Legality::Gen4MysteryGiftContainerEvidence
