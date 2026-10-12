#include "Legality/Gen3GameCubeOriginalFixedHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
    namespace H=Legality::Gen3GameCubeOriginalFixedHistory;
    namespace F=Legality::Gen3GameCubeFixedEncounterEvidence;
    using S=H::Status;using G=H::Game;
    static_assert(F::kTemplateCount==14u);
    static_assert(H::originalGameForRow(0)==G::Colosseum);
    static_assert(H::originalGameForRow(2)==G::Colosseum);
    static_assert(H::originalGameForRow(3)==G::XD);
    static_assert(H::originalGameForRow(10)==G::XD);
    static_assert(H::originalGameForRow(11)==G::Colosseum);
    static_assert(H::originalGameForRow(13)==G::Colosseum);
    constexpr std::array<uint8_t,6> normalIVs{6,1,0,7,17,7};
    constexpr std::array<uint8_t,6> starterIVs{22,28,22,21,28,13};
    constexpr std::array<uint8_t,6> antiIVs{9,31,12,31,25,22};
    constexpr uint16_t starterTid=46057,starterSid=23359;
    std::array<F::Candidate,14> f{};
    // Colo Espeon / Umbreon use the accepted linked first/second
    // starter RNG stage, not arbitrary CXD Method-1 seed matching.
    f[0].species=196;f[0].originGame=15;f[0].language=2;
    f[0].otGender=0;f[0].pokemonGender=0;
    f[0].tid=starterTid;f[0].sid=starterSid;
    f[0].metLevel=25;f[0].metLocation=254;f[0].ball=4;
    f[0].pid=0x0D09CC6Fu;f[0].ivs={1,11,11,3,4,2};
    f[1]=f[0];f[1].species=197;f[1].metLevel=26;
    f[1].pid=0xC7047D49u;f[1].ivs=starterIVs;
    // Colosseum fixed Duking Plusle, zero SID, accepted regular CXD.
    f[2].species=311;f[2].originGame=15;f[2].language=2;
    f[2].otGender=0;f[2].tid=37149;f[2].sid=0;
    f[2].metLevel=13;f[2].metLocation=254;f[2].ball=4;
    f[2].pid=0x0985A297u;f[2].ivs=normalIVs;
    f[2].otName=u"DUKING";
    // XD starter Eevee shares the fixture's PID/IV but not the
    // Colosseum Umbreon/Espeon starter origin/scene.
    f[3].species=133;f[3].originGame=15;f[3].language=2;
    f[3].otGender=0;f[3].tid=starterTid;f[3].sid=starterSid;
    f[3].metLevel=10;f[3].metLocation=0;f[3].ball=4;
    f[3].fateful=true;f[3].pid=0xC7047D49u;f[3].ivs=starterIVs;
    // XD player gifts may use trainer-aware anti-shiny rerolls.
    f[4].species=152;f[4].originGame=15;f[4].language=2;
    f[4].otGender=0;f[4].tid=12345;f[4].sid=35598;
    f[4].metLevel=5;f[4].metLocation=16;f[4].ball=4;
    f[4].fateful=true;f[4].pid=0xB7951831u;f[4].ivs=antiIVs;
    f[5]=f[4];f[5].species=155;f[5].pid=0x0985A297u;f[5].ivs=normalIVs;
    f[6]=f[5];f[6].species=158;
    // XD trades use recipient-determined SID; their separate fixed OT
    // and nickname text must be attested and ordinary CXD is required.
    f[7].species=239;f[7].originGame=15;f[7].language=2;
    f[7].otGender=0;f[7].tid=41400;f[7].sid=1234;
    f[7].metLevel=20;f[7].metLocation=164;f[7].ball=4;
    f[7].fateful=true;f[7].pid=0x0985A297u;f[7].ivs=normalIVs;
    f[7].otName=u"HORDEL";f[7].nickname=u"ZAPRONG";
    f[8]=f[7];f[8].species=307;f[8].tid=37149;
    f[8].metLocation=116;f[8].otName=u"DUKING";f[8].nickname={};
    f[9]=f[8];f[9].species=213;
    f[10]=f[8];f[10].species=246;
    // Colosseum bonus/reward distribution: original disc was Colosseum
    // although stored PK3 origin is Sapphire (2) or Ruby (1).
    f[11].species=25;f[11].originGame=2;f[11].language=1;
    f[11].otGender=0;f[11].tid=31121;f[11].sid=0;
    f[11].metLevel=10;f[11].metLocation=255;f[11].ball=4;
    f[11].pid=0x0985A297u;f[11].ivs=normalIVs;
    f[11].otName=u"コロシアム";
    f[12]=f[11];f[12].species=251;f[12].otGender=1;
    f[12].otName=u"アゲト";
    f[13].species=250;f[13].originGame=1;f[13].language=2;
    f[13].otGender=0;f[13].tid=10048;f[13].sid=0;
    f[13].metLevel=70;f[13].metLocation=255;f[13].ball=4;
    f[13].pid=0x0985A297u;f[13].ivs=normalIVs;
    f[13].otName=u"MATTLE";
    for(std::size_t i=0;i<f.size();++i){
        H::Candidate c{};
        c.originalGame=H::originalGameForRow(i);
        c.originalRowIndex=i;c.originalNative=f[i];
        c.originalGameIndependentlyAttested=true;
        c.originalPhysicalRowIndependentlyAttested=true;
        c.originalGiftOrTradeSceneIndependentlyAttested=true;
        c.originalNativeSnapshotIndependentlyAttested=true;
        c.originalTrainerAndTextIndependentlyAttested=true;
        c.originalRecipientSaveIndependentlyAttested=true;
        const auto result=H::evaluate(c);
        assert(result.status==S::ConditionalOriginalFixedHistory);
        assert(result.fixedKind==F::kEntries[i].kind);
        assert(result.family==F::kEntries[i].policy);
        assert(result.recipientIsGba==(i>=11u));
        auto missing=c;missing.originalGameIndependentlyAttested=false;
        assert(H::evaluate(missing).status==S::UnknownOriginalGame);
        missing=c;missing.originalPhysicalRowIndependentlyAttested=false;
        assert(H::evaluate(missing).status==S::UnknownOriginalRow);
        missing=c;missing.originalGiftOrTradeSceneIndependentlyAttested=false;
        assert(H::evaluate(missing).status==S::UnknownOriginalScene);
        missing=c;missing.originalNativeSnapshotIndependentlyAttested=false;
        assert(H::evaluate(missing).status==S::UnknownOriginalNativeSnapshot);
        missing=c;missing.originalTrainerAndTextIndependentlyAttested=false;
        assert(H::evaluate(missing).status==S::UnknownOriginalTrainerAndText);
        if(i>=11u){
            missing=c;missing.originalRecipientSaveIndependentlyAttested=false;
            assert(H::evaluate(missing).status==S::UnknownRecipientSave);
        }
        auto other=c;
        other.originalGame=c.originalGame==G::XD?G::Colosseum:G::XD;
        assert(H::evaluate(other).status==S::ForeignSourceRowNotEvidence);
        other=c;other.originalNative.species=0;
        assert(H::evaluate(other).status==S::CandidateOriginalIdentityContradicted);
        other=c;other.originalNative.ball=3;
        assert(H::evaluate(other).status==S::CandidateOriginalIdentityContradicted);
        other=c;other.originalNative.ivs[0]=32u;
        assert(H::evaluate(other).status==S::CandidateOriginalPidIvContradicted);
        other=c;other.originalNative.pid^=1u;
        assert(H::evaluate(other).status==S::CandidateOriginalPidIvContradicted);
    }
    // Explicit anti-shiny versus Standard CXD distinction, source-locked
    // to Colo Plusle trainer rather than incorrectly routed through XD.
    H::Candidate anti{};
    anti.originalGame=G::Colosseum;anti.originalRowIndex=2u;
    anti.originalGameIndependentlyAttested=true;
    anti.originalPhysicalRowIndependentlyAttested=true;
    anti.originalGiftOrTradeSceneIndependentlyAttested=true;
    anti.originalNativeSnapshotIndependentlyAttested=true;
    anti.originalTrainerAndTextIndependentlyAttested=true;
    anti.originalNative=f[2];
    anti.originalNative.pid=0x40EF42C4u;
    anti.originalNative.ivs={28,1,11,4,13,27};
    assert(H::evaluate(anti).status==S::ConditionalOriginalFixedHistory);
    // Fixed XD trade cannot be promoted by a candidate CXDAnti shape.
    auto wrongTrade=anti;
    wrongTrade.originalGame=G::XD;wrongTrade.originalRowIndex=7u;
    wrongTrade.originalNative=f[7];
    wrongTrade.originalNative.pid=0x713BBC92u;
    wrongTrade.originalNative.ivs={20,15,1,4,14,15};
    assert(H::evaluate(wrongTrade).status==S::CandidateOriginalPidIvContradicted);
    std::cout<<"L1 E09/E10 14 original GameCube fixed/gift/trade templates + anti-shiny controls: PASS\n";
}
