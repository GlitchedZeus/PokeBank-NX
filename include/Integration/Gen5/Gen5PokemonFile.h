#ifndef POKEBANK_GEN5_POKEMON_FILE_H
#define POKEBANK_GEN5_POKEMON_FILE_H

// Feature A (owner Oct 28): standalone .pk5 / .ek5 files for Black/White and Black 2/White 2.
//
// Import goes ONLY into the app's staged workspace (Gen5StagedPokemonWorkspace::importToEmptyBox):
// Review, Keep, Discard and Export Copy are unchanged and the original save is never written.
// Export is read-only: it returns the PKHeX plaintext PK5 of one Pokemon of an open save (pinned
// PKHeX 6501f0ab PK5.cs / PokeCrypto.cs: decrypted, canonical block order, checksum at 0x06;
// 136 bytes for a box slot, 220 for a party slot as stored), which
// PokeBank::PokemonFile::decode() reads back as the same record. No generation conversion, no
// legality changes, no invented data.

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "PokemonFile/NativePokemonFile.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Stage one decoded standalone file into an empty box slot of `workspace`. Refuses (with the reason
// in `error`) a file of another generation, an out-of-range / occupied / already-staged slot, and a
// record the selected title cannot hold. The workspace is unchanged on refusal.
[[nodiscard]] inline bool importPokemonFile5(Gen5StagedPokemonWorkspace& workspace,
                                             const PokeBank::PokemonFile::Decoded& file,
                                             size_t box,size_t slot,std::string* error=nullptr) {
    const auto fail=[&](const char* why) {
        if(error)*error=why;
        return false;
    };
    if(file.generation!=5 || file.format!=PokeBank::PokemonFile::Format::PK5)
        return fail("Only Generation V files (.pk5 / .ek5) go into a Black/White or Black 2/White 2 "
                    "save; Pokemon are never converted between generations");
    if(file.plain.size()!=(file.party?Crypto::PartySize:Crypto::StoredSize))
        return fail("The Generation V file record has the wrong size");
    return workspace.importToEmptyBox(file.plain,box,slot,error);
}

// The PKHeX plaintext PK5 of one occupied, valid slot of an open save, exactly as stored (box 136
// bytes, party 220 bytes). Non-mutating. nullopt with a reason for an empty / invalid /
// out-of-range slot.
[[nodiscard]] inline std::optional<std::vector<uint8_t>> exportPokemonFile5(
    const Gen5ReadOnlySave& save,Gen5StagedPokemonWorkspace::Slot location,
    std::string* error=nullptr) {
    using Region=Gen5StagedPokemonWorkspace::Region;
    const auto fail=[&](const char* why) -> std::optional<std::vector<uint8_t>> {
        if(error)*error=why;
        return std::nullopt;
    };
    if(!Gen5StagedPokemonWorkspace::canonicalSlot(location) ||
       (location.region==Region::Party && location.slot>=save.partyCount()))
        return fail("Gen V export slot is outside the save's party or boxes");
    const auto pk=location.region==Region::Party ? save.partyPokemon(location.slot)
                                                 : save.boxPokemon(location.box,location.slot);
    if(!pk || !pk->valid())
        return fail("Gen V export slot does not hold a valid PK5 record");
    if(pk->empty())
        return fail("Gen V export slot is empty");
    const size_t expected=location.region==Region::Party?Crypto::PartySize:Crypto::StoredSize;
    const auto decoded=pk->decodedBytes();
    if(decoded.size()!=expected)
        return fail("Gen V export slot holds a record of the wrong size");
    if(error)error->clear();
    return std::vector<uint8_t>(decoded.begin(),decoded.end());
}

// The same export for a slot as the open WORKSPACE holds it (Pokemon Actions -> Export to file): a
// slot the session has not changed is exported from the source save by exportPokemonFile5; a staged
// one (edited, created, cloned, imported) from its staged record under the same rules (valid,
// occupied, exact box / party size). Non-mutating.
[[nodiscard]] inline std::optional<std::vector<uint8_t>> exportStagedPokemonFile5(
    const Gen5StagedPokemonWorkspace& workspace,Gen5StagedPokemonWorkspace::Slot location,
    std::string* error=nullptr) {
    using Region=Gen5StagedPokemonWorkspace::Region;
    if(!Gen5StagedPokemonWorkspace::canonicalSlot(location))
        return exportPokemonFile5(workspace.sourceSave(),location,error);
    const auto& save=workspace.sourceSave();
    const bool party=location.region==Region::Party;
    const auto staged=party?workspace.viewParty(location.slot):workspace.viewBox(location.box,location.slot);
    const auto source=party?(location.slot<save.partyCount()?save.partyPokemon(location.slot):std::nullopt)
                           :save.boxPokemon(location.box,location.slot);
    const bool unchanged=source && staged && std::ranges::equal(source->decodedBytes(),staged->decodedBytes());
    if(unchanged || !staged)return exportPokemonFile5(workspace.sourceSave(),location,error);
    const auto fail=[&](const char* why) -> std::optional<std::vector<uint8_t>> {
        if(error)*error=why;
        return std::nullopt;
    };
    if(!staged->valid())
        return fail("Gen V export slot does not hold a valid PK5 record");
    if(staged->empty())
        return fail("Gen V export slot is empty");
    const size_t expected=location.region==Region::Party?Crypto::PartySize:Crypto::StoredSize;
    const auto decoded=staged->decodedBytes();
    if(decoded.size()!=expected)
        return fail("Gen V export slot holds a record of the wrong size");
    if(error)error->clear();
    return std::vector<uint8_t>(decoded.begin(),decoded.end());
}

} // namespace PokeVault::Integration::Gen5
#endif
