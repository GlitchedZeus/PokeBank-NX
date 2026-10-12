#pragma once
// L1 Stage 4: separate original GameCube game/scene from a transferred PK3.
// PK3 canonical offset 0x46 origin (bits 7..10) is 15 for BOTH Colo/XD;
// see PKHeX PK3, Pokémon GBA original data, and pinned L1 fixed-history rules.
// A native byte field can corroborate an independently witnessed scene,
// never decide whether Colosseum or XD generated a creature by itself.
#include <array>
#include <cstdint>
namespace Legality::Gen3GameCubeOriginalNativeStage {
enum class Game : uint8_t { Unknown, Colosseum, XD };
enum class Route : uint8_t { Unknown, ShadowPurified, FixedGiftOrTrade, BonusDiscReward };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalScene, UnknownOriginalNative,
    UnknownRecipientHistory, UnknownPurificationHistory,
    CandidateNativeOriginContradicted, CandidateNativeIdentityContradicted,
    ConditionalGameCubeOriginHistory, UnknownMalformedNativeIvs
};
struct NativeFields {
    uint32_t personality=0,originalTrainerId=0;
    uint16_t species=0,originGame=0;
    std::array<uint8_t,6> ivs{};
    bool checksumAndDecryptionVerified=false;
};
struct Candidate {
    Game originalGame=Game::Unknown;
    Route originalRoute=Route::Unknown;
    NativeFields native{};
    uint32_t originalPersonality=0,originalTrainerId=0;
    uint16_t originalSpecies=0;
    bool originalGameIndependentlyAttested=false;
    bool originalSceneIndependentlyAttested=false;
    bool originalCapturedPidSpeciesOtIndependentlyAttested=false;
    bool originalPurificationIndependentlyAttested=false;
    bool originalRecipientSaveIndependentlyAttested=false;
};
struct Result { Status status=Status::UnknownOriginalGame; bool recipientOriginIsGba=false; };
// Enum bytes can originate from decoded archive metadata. A value outside
// the documented source domain is NOT a witnessed scene or retail game.
constexpr bool recognizedGame(Game game) noexcept {
    return game==Game::Colosseum || game==Game::XD;
}
constexpr bool recognizedRoute(Route route) noexcept {
    return route==Route::ShadowPurified ||
           route==Route::FixedGiftOrTrade ||
           route==Route::BonusDiscReward;
}
constexpr Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested || !recognizedGame(c.originalGame))return {};
    if(!c.originalSceneIndependentlyAttested || !recognizedRoute(c.originalRoute))
        return {Status::UnknownOriginalScene};
    // Colosseum is the originating bonus-disc / Mt Battle reward source;
    // XD never becomes the original disc just because native origin is GBA.
    if(c.originalRoute==Route::BonusDiscReward &&
       c.originalGame!=Game::Colosseum)
        return {Status::UnknownOriginalScene};
    if(!c.native.checksumAndDecryptionVerified ||
       !c.originalCapturedPidSpeciesOtIndependentlyAttested ||
       c.originalSpecies==0 || c.originalSpecies>386)
        return {Status::UnknownOriginalNative};
    // Five-bit native IVs are a strict PK3 domain. A damaged encrypted
    // record is missing reliable evidence, not a negative historical route.
    for(uint8_t iv:c.native.ivs)
        if(iv>31)return {Status::UnknownMalformedNativeIvs};
    if(c.originalRoute==Route::ShadowPurified &&
       !c.originalPurificationIndependentlyAttested)
        return {Status::UnknownPurificationHistory};
    const bool gba=c.originalRoute==Route::BonusDiscReward;
    if(gba && !c.originalRecipientSaveIndependentlyAttested)
        return {Status::UnknownRecipientHistory,true};
    if((!gba && c.native.originGame!=15) ||
       (gba && !(c.native.originGame>=1 && c.native.originGame<=5)))
        return {Status::CandidateNativeOriginContradicted,gba};
    if(c.native.personality!=c.originalPersonality ||
       c.native.originalTrainerId!=c.originalTrainerId ||
       c.native.species!=c.originalSpecies)
        return {Status::CandidateNativeIdentityContradicted,gba};
    // No XD vs Colosseum certification: native origin 15 is shared.
    // Shadow source list, trainer, purification and scene remain separately
    // witnessed; native bytes cannot recover them retroactively.
    return {Status::ConditionalGameCubeOriginHistory,gba};
}
} // namespace Legality::Gen3GameCubeOriginalNativeStage
