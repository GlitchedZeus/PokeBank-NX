#ifndef POKEBANK_GEN5_IMPORT_RECORD_H
#define POKEBANK_GEN5_IMPORT_RECORD_H

// Feature A (owner Oct 28): may this standalone PK5 record be placed, byte for byte, into a box of
// THIS exact Black/White or Black 2/White 2 save? A pure check on PKHeX's plaintext PK5 layout
// (pinned PKHeX 6501f0ab PK5.cs; decrypted, canonical block order, checksum at 0x06). It never
// edits, legalizes or converts the record: a record that does not fit the title is refused with a
// reason, and nothing is invented to make it fit. Shared by the staging workspace, the independent
// Review verifier and the export-candidate builder so all three apply the same title rules.
//
// The title rules reuse the existing native validators only:
//   * species/form: personal5() of the exact title (BW lacks e.g. Black/White Kyurem and the
//     Therian forms that B2W2 added: PKHeX PersonalTable.BW / B2W2 FormCount);
//   * held item: an item of that title's native item catalogs (Gen5Bag.h BagCatalog5, pinned to
//     PKHeX PlayerBag5BW / PlayerBag5B2W2), so a B2W2-only item is refused in a BW save;
//   * moves, nature, ball, language and met location: the same native ranges the field editor
//     enforces (Gen5MovePP.h, Gen5EncounterLocations.h, StagedPokemon5Record::stage);
//   * Generation IV content in a .pk5 is refused (PKHeX EntityFormat.GetFormat45).

#include "Encryption/Encryption5.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

// Origin game IDs a Gen V record can carry (PKHeX GameVersion: S 1, R 2, E 3, FR 4, LG 5, HG 7,
// SS 8, D 10, P 11, Pt 12, CXD 15, W 20, B 21, W2 22, B2 23).
[[nodiscard]] constexpr bool importOriginVersion5(uint8_t version) noexcept {
    switch(version) {
        case 1: case 2: case 3: case 4: case 5: case 7: case 8: case 10: case 11: case 12:
        case 15: case 20: case 21: case 22: case 23: return true;
        default: return false;
    }
}

// An item that exists in this title: a member of any of its native pouch catalogs.
[[nodiscard]] constexpr bool importHeldItemInTitle5(SaveFamily family,uint16_t item) noexcept {
    if(item==0)return true;
    for(size_t pocket=0;pocket<BagPocket5Count;++pocket)
        if(gen5BagItemInPocket(family,pocket,item))return true;
    return false;
}

// nullopt: the record may be imported into a save of this title. Otherwise the reason it may not.
// `plain` is the PKHeX plaintext PK5, 136 (box) or 220 (party) bytes; only the stored 136 bytes are
// judged, since a box slot never holds the party extension.
[[nodiscard]] inline std::optional<std::string_view> importRecordRefusal5(
    std::span<const uint8_t> plain,bool black2White2) {
    if(!Crypto::recordSizeValid(plain.size()))
        return "Not a Generation V record (a PK5 is 136 or 220 bytes)";
    if(Crypto::read16(plain,6)!=Crypto::checksum(plain))
        return "The PK5 checksum does not match its data";
    if(Crypto::read16(plain,4)!=0)
        return "The PK5 sanity field is set (Battle Revolution or corrupt record)";
    const uint16_t species=Crypto::read16(plain,0x08);
    if(species==0 || species>649)
        return "The record holds no Generation V species";
    // PKHeX EntityFormat.GetFormat45: a Gen IV origin keeps a pre-Gen V version with a Gen IV met
    // location, or uses the Platinum location words at 0x44-0x47.
    if((plain[0x5F]<0x10 && Crypto::read16(plain,0x80)<0x3333) || Crypto::read16(plain,0x46)!=0)
        return "The content is a Generation IV record; it is never converted";
    if(!importOriginVersion5(plain[0x5F]))
        return "The record's origin game is not a game a Generation V Pokemon can come from";
    if(!personal5(species,static_cast<uint32_t>(plain[0x40]>>3),black2White2))
        return black2White2?"This species form does not exist in Black 2/White 2"
                           :"This species form does not exist in Black/White";
    const auto family=black2White2?SaveFamily::Black2White2:SaveFamily::BlackWhite;
    if(!importHeldItemInTitle5(family,Crypto::read16(plain,0x0A)))
        return black2White2?"The held item does not exist in Black 2/White 2"
                           :"The held item does not exist in Black/White";
    for(size_t i=0;i<4;++i)
        if(!validNativeMove5(Crypto::read16(plain,0x28+i*2)))
            return "A move is outside the Generation V move list";
    if(plain[0x41]>24)
        return "The nature is outside the native 0-24 range";
    if(plain[0x83]>25)
        return "The Poke Ball is outside the Generation V ball list";
    switch(plain[0x17]) {
        case 1: case 2: case 3: case 4: case 5: case 7: case 8: break;
        default: return "The language is not a Generation V language";
    }
    if(!isNativeMetLocation5(Crypto::read16(plain,0x80)))
        return "The met location is not a Generation V location";
    return std::nullopt;
}

} // namespace PokeVault::Integration::Gen5
#endif
