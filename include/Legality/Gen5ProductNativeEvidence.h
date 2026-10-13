#pragma once
// Read-only Generation V evidence using the EXACT product PK5 reader.
// Product Encryption5/Pokemon5ReadOnly blobs are shared, not reinvented.
// PKHeX Core oracle: PKM/PK5.cs (snapshot SHA pinned separately).
// A valid checksum does NOT attest to encounter, move, gift or provenance.
#include "Pokemon/Pokemon5ReadOnly.h"
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen5ProductNativeEvidence {
enum class Status : uint8_t {
    UnknownLength, InvalidSanity, InvalidChecksum, InvalidSpecies,
    EmptySlot, InvalidNature, InvalidEVTotal, Parsed
};
struct Evidence {
    Status status=Status::UnknownLength;
    uint16_t species=0;
    uint8_t nature=0;
    uint16_t evTotal=0;
    uint32_t pid=0;
    bool partyRecord=false;
    bool checksumVerified=false;
    bool sourceHistoryAuthenticated=false;
};
// Product read-only parser owns a copy; no source pointer survives.
inline Evidence inspect(std::span<const uint8_t> encrypted) {
    namespace Gen5=PokeVault::Integration::Gen5;
    Evidence result{};
    if (!Gen5::Crypto::recordSizeValid(encrypted.size()))return result;
    const Gen5::Pokemon5ReadOnly pokemon(encrypted);
    result.partyRecord=pokemon.partyRecord();
    if (!pokemon.sanityValid()) {
        result.status=Status::InvalidSanity;
        return result;
    }
    if (!pokemon.checksumValid()) {
        result.status=Status::InvalidChecksum;
        return result;
    }
    result.checksumVerified=true;
    // Native semantic getters quarantine species>649; inspect raw decrypted
    // species only AFTER size, sanity, checksum validation.
    const uint16_t species=Gen5::Crypto::read16(pokemon.decodedBytes(),8);
    if (species>649) {
        result.status=Status::InvalidSpecies;
        return result;
    }
    result.species=species;
    if (species==0) {
        result.status=Status::EmptySlot;
        return result;
    }
    result.pid=pokemon.pid();
    result.nature=pokemon.nature();
    if (result.nature>24) {
        result.status=Status::InvalidNature;
        return result;
    }
    for(const auto ev:pokemon.evs())result.evTotal+=ev;
    if(result.evTotal>510) {
        result.status=Status::InvalidEVTotal;
        return result;
    }
    result.status=Status::Parsed;
    return result;
}
} // namespace Legality::Gen5ProductNativeEvidence
