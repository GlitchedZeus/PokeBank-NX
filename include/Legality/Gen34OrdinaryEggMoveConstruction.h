#pragma once
// BREED-001: Gen III/IV ORDINARY DAYCARE egg four-slot FIFO construction.
// Sequential stages: base moves, father egg moves, father compatible TM/HM,
// shared parental level-up, then conditional Light Ball Volt Tackle.
// Caller MUST supply already-verified parental compatibility and method
// candidates. This does not prove a real father, breeding pair, event egg,
// missing parent identity, hatch history, or the absence of alternative eggs.
// Parent hypotheses are NEVER grounds for a global Invalid verdict.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen34OrdinaryEggMoveConstruction {
enum class Status : uint8_t {
    ConstructedHypothesis, UnsupportedGame, UnknownLightBallState
};
struct Input {
    std::string_view gameId{};
    uint16_t species=0;
    std::array<uint16_t,4> baseMoves{};
    std::array<uint16_t,4> fatherEggMoves{};
    std::array<uint16_t,4> fatherMachineMoves{};
    std::array<uint16_t,4> sharedLevelMoves{};
    bool lightBallStateKnown=false;
    bool eligibleParentHeldLightBall=false;
};
struct Result {
    Status status=Status::UnsupportedGame;
    std::array<uint16_t,4> moves{};
    uint8_t count=0;
    constexpr bool constructed() const noexcept {
        return status==Status::ConstructedHypothesis;
    }
    constexpr bool matches(const std::array<uint16_t,4>& observed) const noexcept {
        return constructed() && moves==observed;
    }
};
constexpr bool supportedGame(std::string_view game) noexcept {
    return game=="ruby_gba" || game=="sapphire_gba" ||
           game=="emerald_gba" || game=="firered_gba" ||
           game=="leafgreen_gba" || game=="diamond_nds" ||
           game=="pearl_nds" || game=="platinum_nds" ||
           game=="heartgold_nds" || game=="soulsilver_nds";
}
constexpr bool supportsVoltTackle(std::string_view game) noexcept {
    return game=="emerald_gba" ||
           game=="diamond_nds" || game=="pearl_nds" ||
           game=="platinum_nds" || game=="heartgold_nds" ||
           game=="soulsilver_nds";
}
constexpr void append(Result& result,uint16_t move) noexcept {
    if(!move)return;
    for(uint8_t i=0;i<result.count;++i)
        if(result.moves[i]==move)return; // Skip repeated inherited moves.
    if(result.count<result.moves.size()) {
        result.moves[result.count++]=move;
        return;
    }
    // Four-slot ordinary daycare FIFO: drop oldest, append newest.
    for(uint8_t i=1;i<result.moves.size();++i)
        result.moves[i-1]=result.moves[i];
    result.moves.back()=move;
}
constexpr Result construct(const Input& input) noexcept {
    if(!supportedGame(input.gameId) || input.species==0 ||
       input.species>493) return {};
    const bool couldVolt=input.species==172 && supportsVoltTackle(input.gameId);
    if(couldVolt && !input.lightBallStateKnown)
        return {Status::UnknownLightBallState,{},0};
    Result result{};
    result.status=Status::ConstructedHypothesis;
    for(uint16_t move:input.baseMoves)append(result,move);
    for(uint16_t move:input.fatherEggMoves)append(result,move);
    for(uint16_t move:input.fatherMachineMoves)append(result,move);
    for(uint16_t move:input.sharedLevelMoves)append(result,move);
    if(couldVolt && input.eligibleParentHeldLightBall)
        append(result,344); // Volt Tackle, appended after ordinary passes.
    return result;
}
} // namespace Legality::Gen34OrdinaryEggMoveConstruction
