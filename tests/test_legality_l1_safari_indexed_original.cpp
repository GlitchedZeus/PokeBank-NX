#include "Legality/Gen3SafariIndexedOriginalHistory.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen3SafariIndexedOriginalHistory;
    namespace S=Legality::Gen3Safari;
    namespace M=Legality::Gen3SafariMethodH;
    struct Vector{
        const char* game;
        std::size_t index;
        uint16_t species;
        uint8_t level,method;
        uint32_t seed,pid;
        std::array<uint8_t,6> ivs;
        M::Path path;
    };
    constexpr std::array<uint8_t,6> hoennIV{14,23,20,23,29,7};
    constexpr std::array<uint8_t,6> kantoIV{19,6,22,5,0,18};
    constexpr std::array<uint8_t,6> rockIV{3,11,21,23,14,17};
    constexpr std::array<Vector,22> vectors{{
      {"ruby_gba",3,43,27,0,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"ruby_gba",13,54,27,1,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"ruby_gba",45,129,10,2,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"ruby_gba",46,129,18,3,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"ruby_gba",38,118,30,4,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"ruby_gba",23,74,15,5,6u,0x794E8AA6u,rockIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",67,43,27,0,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",79,54,27,1,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",112,129,10,2,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",113,129,18,3,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",105,118,30,4,1u,0xAC2141C6u,hoennIV,M::Path::HoennSafariBlock},
      {"sapphire_gba",89,74,15,5,6u,0x794E8AA6u,rockIV,M::Path::HoennSafariBlock},
      {"firered_gba",288,102,23,0,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"firered_gba",279,54,20,1,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"firered_gba",309,129,5,2,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"firered_gba",302,118,8,3,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"firered_gba",304,119,23,4,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"leafgreen_gba",351,102,23,0,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"leafgreen_gba",343,79,20,1,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"leafgreen_gba",372,129,5,2,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"leafgreen_gba",365,118,8,3,13u,0xCBC05712u,kantoIV,M::Path::RegularNature},
      {"leafgreen_gba",367,119,23,4,13u,0xCBC05712u,kantoIV,M::Path::RegularNature}
    }};
    std::size_t positives=0;
    for(const auto& v:vectors){
        const auto& row=S::kGen3SafariEntries[v.index];
        assert(row.species==v.species && row.method==v.method);
        H::Candidate c{};
        c.exactOriginalGame=v.game;c.originalRowIndex=v.index;
        c.originalSpecies=v.species;c.originalMetLocation=row.location;
        c.originalMetLevel=v.level;c.originalForm=row.form;
        c.originalBall=S::kSafariBall;c.originalPid=v.pid;c.originalIVs=v.ivs;
        c.attestedPrePidSeed=v.seed;
        c.originalSaveIndependentlyAttested=true;
        c.originalPhysicalAreaAndRowIndependentlyAttested=true;
        c.originalNoLeadIndependentlyAttested=true;
        c.originalPidIvStageIndependentlyAttested=true;
        c.originalBallIndependentlyAttested=true;
        c.originalHandheldMethodFamilyIndependentlyAttested=true;
        const auto result=H::evaluate(c);
        assert(result.status==H::Status::ConditionalOriginalSafariHistory);
        assert(result.path==v.path);
        assert(result.originalAreaIndex==row.areaIndex);
        ++positives;
        auto foreign=c;
        foreign.exactOriginalGame= v.game[0]=='r'?"sapphire_gba":"ruby_gba";
        if(v.game[0]=='f'||v.game[0]=='l')
            foreign.exactOriginalGame=v.game[0]=='f'?"leafgreen_gba":"firered_gba";
        assert(H::evaluate(foreign).status==H::Status::ForeignRowNotEvidence);
        auto wrong=c;wrong.originalSpecies=0;
        assert(H::evaluate(wrong).status==H::Status::CandidateRowContradicted);
        wrong=c;wrong.originalBall=4;
        assert(H::evaluate(wrong).status==H::Status::CandidateBallContradicted);
        wrong=c;wrong.attestedPrePidSeed^=0x80u;
        assert(H::evaluate(wrong).status==H::Status::CandidatePidIvContradicted);
        auto lost=c;lost.originalSaveIndependentlyAttested=false;
        assert(H::evaluate(lost).status==H::Status::UnknownOriginalSave);
        lost=c;lost.originalPhysicalAreaAndRowIndependentlyAttested=false;
        assert(H::evaluate(lost).status==H::Status::UnknownOriginalAreaOrRow);
        lost=c;lost.originalNoLeadIndependentlyAttested=false;
        assert(H::evaluate(lost).status==H::Status::UnknownLeadStage);
        lost=c;lost.originalBallIndependentlyAttested=false;
        assert(H::evaluate(lost).status==H::Status::UnknownOriginalBall);
        lost=c;lost.originalPidIvStageIndependentlyAttested=false;
        assert(H::evaluate(lost).status==H::Status::UnknownOriginalPidIv);
    }
    assert(positives==22u);
    // Two physically distinct Safari encounter areas with identical
    // Ruby persistent species/met tuple must NOT be deduplicated away.
    H::Candidate repeat{};
    repeat.exactOriginalGame="ruby_gba";repeat.originalRowIndex=3;
    repeat.originalSpecies=43;repeat.originalMetLocation=57;
    repeat.originalMetLevel=27;repeat.originalForm=0;
    repeat.originalBall=5;repeat.originalPid=0xAC2141C6u;
    repeat.originalIVs=hoennIV;repeat.attestedPrePidSeed=1u;
    repeat.originalSaveIndependentlyAttested=true;
    repeat.originalPhysicalAreaAndRowIndependentlyAttested=true;
    repeat.originalNoLeadIndependentlyAttested=true;
    repeat.originalPidIvStageIndependentlyAttested=true;
    repeat.originalBallIndependentlyAttested=true;
    repeat.originalHandheldMethodFamilyIndependentlyAttested=true;
    const auto first=H::evaluate(repeat);
    repeat.originalRowIndex=4;
    const auto second=H::evaluate(repeat);
    assert(first.status==H::Status::ConditionalOriginalSafariHistory);
    assert(second.status==H::Status::ConditionalOriginalSafariHistory);
    assert(first.originalAreaIndex!=second.originalAreaIndex);
    std::cout<<"L1 E06/E08 22 R/S/FR/LG indexed source frames (plus Unknown): PASS\n";
}
