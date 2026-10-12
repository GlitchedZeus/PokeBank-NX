#pragma once
// HGSS GenerateEggPID only: creation-time Everstone gate and supplied
// consecutive MT outputs. Collection-time international rerolls are NOT
// part of this stage. Inputs cannot be recovered from a detached PK4.
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
namespace Legality::Gen4HGSSPendingEggPid {
enum class Status : uint8_t {
    UnsupportedGame, UnknownParentItems, UnknownCreationStage,
    IncoherentCreationStage, UnknownLCRNGState, UnknownParentNature,
    UnknownMTTranscript, IncompleteMTTranscript, MalformedMTTranscript,
    TranscriptCountContradicted, ReplayedPendingPid
};
struct Input {
    std::string_view game{};
    // Physical daycare slots at PID CREATION, not later collection.
    bool parentItemsAndOrderVerified=false;
    std::array<uint16_t,2> heldItems{};
    std::array<bool,2> parentPidKnown{};
    std::array<uint32_t,2> parentPid{};
    bool creationStageVerified=false;
    uint64_t parentStageToken=0,mtStageToken=0;
    bool lcEntryStateKnown=false;
    uint32_t lcEntryState=0;
    bool consecutiveMTOutputsVerified=false,exactConsumedMTCountVerified=false;
    std::span<const uint32_t> mtOutputs{};
};
struct Result {
    Status status=Status::UnknownParentItems;
    uint32_t pendingPid=0,lcFinalState=0;
    uint16_t mtConsumed=0;
    uint8_t lcConsumed=0;
    int8_t everstoneSlot=-1;
    bool retailRetryLimitReached=false;
};
constexpr uint16_t draw(uint32_t& state,Result& out) noexcept {
    state=state*0x41C64E6Du+0x6073u;++out.lcConsumed;
    return static_cast<uint16_t>(state>>16);
}
constexpr Result replay(const Input& in) noexcept {
    Result out{};
    if(in.game!="heartgold_nds"&&in.game!="soulsilver_nds") {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.parentItemsAndOrderVerified)return out;
    if(!in.creationStageVerified||!in.parentStageToken||!in.mtStageToken) {
        out.status=Status::UnknownCreationStage;return out;
    }
    if(in.parentStageToken!=in.mtStageToken) {
        out.status=Status::IncoherentCreationStage;return out;
    }
    const bool stone0=in.heldItems[0]==229,stone1=in.heldItems[1]==229;
    out.lcFinalState=in.lcEntryState;
    if(stone0||stone1) {
        if(!in.lcEntryStateKnown){out.status=Status::UnknownLCRNGState;return out;}
        const auto donor=static_cast<int8_t>((stone0&&stone1)?
            (draw(out.lcFinalState,out)%2?0:1):(stone0?0:1));
        // Exactly 0x7FFF is a failed proc, not an inherited nature.
        if(draw(out.lcFinalState,out)<0x7FFFu)out.everstoneSlot=donor;
    }
    if(out.everstoneSlot>=0&&!in.parentPidKnown[out.everstoneSlot]) {
        out.status=Status::UnknownParentNature;return out;
    }
    if(!in.consecutiveMTOutputsVerified||!in.exactConsumedMTCountVerified||in.mtOutputs.empty()) {
        out.status=Status::UnknownMTTranscript;return out;
    }
    if(in.mtOutputs.size()>2401) {
        out.status=Status::MalformedMTTranscript;return out;
    }
    for(uint32_t pid:in.mtOutputs) {
        ++out.mtConsumed;
        const bool accepted=out.everstoneSlot<0 ||
            (pid!=0&&pid%25u==in.parentPid[out.everstoneSlot]%25u);
        if(accepted||out.mtConsumed==2401) {
            out.pendingPid=pid;
            out.retailRetryLimitReached=!accepted;
            out.status=out.mtConsumed==in.mtOutputs.size()?
                Status::ReplayedPendingPid:Status::TranscriptCountContradicted;
            return out;
        }
    }
    out.status=Status::IncompleteMTTranscript;
    return out;
}
} // namespace Legality::Gen4HGSSPendingEggPid
