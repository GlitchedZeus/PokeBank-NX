#pragma once
// L4 META-001: immutable native *decrypted view* of strict encrypted PK4.
// No ability to modify an original save or staged creature. Valid/checksum
// quarantined records are required for any known native-field projection.
// Original event PCD fields remain unavailable without separate provenance.
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Legality/Gen4EventTemplate.h"
#include "Legality/Gen4RawTextEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventNativeFieldEvidence {
struct Snapshot {
    bool available=false;
    Gen4EventTemplate::Candidate identity{};
    std::array<uint16_t,4> observedMoves{};
    std::array<uint32_t,3> nativeRibbonWords{};
    std::array<std::byte,22> rawNickname{};
    std::array<std::byte,16> rawOT{};
    Gen4RawTextEvidence::Result text{};
};
inline Snapshot project(const Pokemon::Pokemon4ReadOnly& pokemon) noexcept {
    Snapshot out{};
    if (!pokemon.valid() || pokemon.empty()) return out;
    const auto bytes=pokemon.decryptedBytes();
    // The strict PK4 parser accepts only 0x88 or 0xEC. Check the backing
    // view independently before extracting native original-string bytes.
    if (bytes.size()<0x88) return out;
    const uint16_t nativeLocation=pokemon.metLocationExtended()!=0
        ? pokemon.metLocationExtended() : pokemon.metLocationDP();
    // IMPORTANT: PK4 met-location fields are already in the same ID
    // namespace as Gen4EventTemplateData.inc (including e.g. 3053).
    // Match the accepted Pokemon4ReadOnlyView::metLocation() and
    // Legality.cpp Gen4EventTemplate::matchEvolutionLine contract.
    // Never add 3000: that makes genuine event locations mismatch and
    // could wrap malformed values into unrelated, plausible IDs.
    const uint8_t ball=pokemon.ballDPPt()>pokemon.ballHGSS()
        ? pokemon.ballDPPt() : pokemon.ballHGSS();
    out.identity={
        pokemon.species(),pokemon.tid(),pokemon.sid(),pokemon.pid(),
        pokemon.metLevel(),
        nativeLocation,ball,
        pokemon.form(),pokemon.gender(),pokemon.language(),
        pokemon.originVersion(),pokemon.originalTrainerGender(),
        pokemon.fatefulEncounter()
    };
    out.observedMoves=pokemon.moves();
    out.nativeRibbonWords=pokemon.ribbons();
    for(std::size_t i=0;i<out.rawNickname.size();++i)
        out.rawNickname[i]=bytes[0x48+i];
    for(std::size_t i=0;i<out.rawOT.size();++i)
        out.rawOT[i]=bytes[0x68+i];
    out.text=Gen4RawTextEvidence::classify(true,out.rawNickname,out.rawOT);
    out.available=true;
    return out;
}
} // namespace Legality::Gen4EventNativeFieldEvidence
