#pragma once
// HGSS SetBreedEggStats final PID transform only. The pending daycare PID
// is supplied by the earlier creation stage; no MT or LCRNG state is used.
#include <cstdint>
#include <string_view>

namespace Legality::Gen4HGSSMasudaPidTransform {
enum class Status : uint8_t {
    UnsupportedGame, UnknownParentLanguages, UnknownTrainerId,
    ReplayedFinalPid, CandidatePidContradicted
};
struct Input {
    std::string_view game{};
    bool parentLanguagesAtCollectionVerified=false;
    uint8_t physicalSlot0Language=0,physicalSlot1Language=0;
    bool trainerIdAtCollectionVerified=false;
    uint32_t trainerId32=0;
    uint32_t pendingPid=0,observedFinalPid=0;
};
struct Result {
    Status status=Status::UnknownParentLanguages;
    uint32_t finalPid=0;
    uint8_t prandomSteps=0;
    bool masudaActive=false;
};
constexpr uint32_t next(uint32_t pid) noexcept {
    return static_cast<uint32_t>(pid*0x6C078965u+1u);
}
constexpr bool shiny(uint32_t pid,uint32_t trainerId32) noexcept {
    return ((pid>>16)^(pid&0xFFFFu)^(trainerId32>>16)^
            (trainerId32&0xFFFFu))<8u;
}
constexpr Result replay(const Input& in) noexcept {
    Result out{};
    if(in.game!="heartgold_nds"&&in.game!="soulsilver_nds") {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.parentLanguagesAtCollectionVerified)return out;
    if(!in.trainerIdAtCollectionVerified) {
        out.status=Status::UnknownTrainerId;return out;
    }
    out.finalPid=in.pendingPid;
    out.masudaActive=in.physicalSlot0Language!=in.physicalSlot1Language;
    if(out.masudaActive&&!shiny(out.finalPid,in.trainerId32)) {
        for(uint8_t i=0;i<4;++i) {
            out.finalPid=next(out.finalPid);++out.prandomSteps;
            if(shiny(out.finalPid,in.trainerId32))break;
        }
    }
    out.status=out.finalPid==in.observedFinalPid?
        Status::ReplayedFinalPid:Status::CandidatePidContradicted;
    return out;
}
} // namespace Legality::Gen4HGSSMasudaPidTransform
