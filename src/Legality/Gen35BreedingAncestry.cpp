// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Legality/Gen35BreedingAncestry.h"
#include "Legality/BreedingBirthMoves.h"
#include "Legality/Gen3LegalityDataCxd.h"
#include "Legality/Gen3LegalityData.h"
#include "Legality/Gen5GlobalLinkCatalog.h"
#include "Gen4LegalityInternal.h"
#include <algorithm>
#include <span>
namespace Legality::Breeding {
namespace {
namespace D4=::Legality::Gen4Data;
namespace D5=::Legality::Gen5Data;
namespace CXD=::Legality::Gen3DataCxd;
constexpr uint16_t stride=650;
uint16_t node(uint8_t generation,uint16_t species){return static_cast<uint16_t>((generation-3)*stride+species);}
uint16_t maximum(uint8_t generation){return generation==3?386:generation==4?493:649;}
uint16_t read16(const uint8_t* p){return static_cast<uint16_t>(p[0]|(p[1]<<8));}
const uint8_t* personal5(uint16_t species){return D5::kPersonalB2W2+species*D5::kPersonalB2W2Size;}
std::vector<uint16_t> forms5(const uint8_t* table,size_t size,size_t count,uint16_t species){
    std::vector<uint16_t> out{species};const auto* p=table+species*size;const auto first=read16(p+0x1c);
    if(first)for(uint8_t f=1;f<p[0x20];++f)if(static_cast<size_t>(first)+f-1<count)out.push_back(static_cast<uint16_t>(first+f-1));
    return out;
}
bool breeds(uint16_t s){const auto* p=personal5(s);return s!=132&&p[0x16]!=15&&p[0x17]!=15;}
bool female(uint16_t s){return breeds(s)&&personal5(s)[0x12]!=0&&personal5(s)[0x12]!=255;}
bool male(uint16_t s){return breeds(s)&&personal5(s)[0x12]<254;}
bool compatible(uint16_t a,uint16_t b){const auto* p=personal5(a);const auto* q=personal5(b);return p[0x16]==q[0x16]||p[0x16]==q[0x17]||p[0x17]==q[0x16]||p[0x17]==q[0x17];}
std::vector<uint16_t> nextSpecies(uint8_t era,uint16_t s){
    std::vector<uint16_t> out;
    if(era==3){for(auto i=D4::kEvolution3Start[s];i<D4::kEvolution3Start[s+1];++i)out.push_back(D4::kEvolutions3[i].species);}
    else if(era==4){for(auto i=D4::kEvolutionStart[s];i<D4::kEvolutionStart[s+1];++i)out.push_back(D4::kEvolutions[i].species);}
    else {for(auto i=D5::kEvolutionStart[s];i<D5::kEvolutionStart[s+1];++i)out.push_back(D5::kEvolutions[i].species);}
    return out;
}
uint16_t counterpart(uint16_t s){return s==29?32:s==32?29:s==313?314:s==314?313:s;}
bool contains(const uint16_t* moves,const uint16_t* start,size_t count,uint16_t s,uint16_t m){
    if(s>=count)return false;
    return std::find(moves+start[s],moves+start[s+1],m)!=moves+start[s+1];
}
template<class T> bool level(const T* moves,const uint16_t* start,size_t count,uint16_t s,uint16_t m,int cap=100){
    if(s>=count)return false;
    for(auto i=start[s];i<start[s+1];++i)if(moves[i].move==m&&moves[i].level<=cap)return true;
    return false;
}
bool level3(uint16_t s,uint16_t m,int cap=100){return level(D4::kLevelUpE3,D4::kLevelUpE3Start,387,s,m,cap)||level(D4::kLevelUpRS3,D4::kLevelUpRS3Start,387,s,m,cap)||level(D4::kLevelUpFR3,D4::kLevelUpFR3Start,387,s,m,cap)||level(D4::kLevelUpLG3,D4::kLevelUpLG3Start,387,s,m,cap);}
bool egg(uint8_t era,uint16_t s,uint16_t m){
    if(era==3)return contains(D4::kEggMovesRS3,D4::kEggMovesRS3Start,D4::kEggMovesRS3Count,s,m);
    if(era==4)return contains(D4::kEggMovesDPPt,D4::kEggMovesDPPtStart,D4::kEggMovesDPPtCount,s,m)||contains(D4::kEggMovesHGSS,D4::kEggMovesHGSSStart,D4::kEggMovesHGSSCount,s,m);
    return contains(D5::kEggMoves,D5::kEggMoveStart,D5::kEggMoveCount,s,m);
}
bool level45(uint8_t era,uint16_t s,uint16_t m,int cap=100){
    if(era==4){for(auto t:{Gen4::Table::DP,Gen4::Table::Pt,Gen4::Table::HGSS})for(uint8_t f=0;f<std::max<uint8_t>(1,Gen4::personal(t,s,0).formCount());++f){auto ls=Gen4::learnset(t,s,f);for(auto* q=ls.begin;q!=ls.end;++q)if(q->move==m&&q->level<=cap)return true;}return false;}
    for(auto f:forms5(D5::kPersonalBW,D5::kPersonalBWSize,D5::kPersonalBWCount,s))if(level(D5::kLevelUpBW,D5::kLevelUpBWStart,D5::kLearnsetBWCount,f,m,cap))return true;
    for(auto f:forms5(D5::kPersonalB2W2,D5::kPersonalB2W2Size,D5::kPersonalB2W2Count,s))if(level(D5::kLevelUpB2W2,D5::kLevelUpB2W2Start,D5::kLearnsetB2W2Count,f,m,cap))return true;
    return false;
}
bool direct(uint8_t era,uint16_t s,uint16_t m,int cap=100){
    if(m==0||m>(era==3?354:era==4?467:559))return false;
    if(s==235&&m!=165&&m!=448)return true;
    if(era==3){
        if(level3(s,m,cap)||contains(D4::kOtherMoves3,D4::kOtherMoves3Start,387,s,m))return true;
        // The format-4 donor table deliberately omits Gen III HMs. They remain
        // real native Gen III sources; transfer erasure is a separate edge.
        for(size_t i=0;i<8;++i)if(Gen3Data::kMachineMovesHidden[i]==m&&((Gen3Data::kMachineBits[s]>>(50+i))&1))return true;
        return false;
    }
    if(level45(era,s,m,cap))return true;
    if(era==4){
        // Type tutors are deliberately overapproximated. This model cannot use
        // their absence as evidence against any species/form.
        if(m==307||m==308||m==338||m==434)return true;
        for(auto t:{Gen4::Table::DP,Gen4::Table::Pt,Gen4::Table::HGSS}){
            const auto p=Gen4::personal(t,s,0);
            for(int i=0;i<92;++i)if(D4::kMachineTM[i]==m&&p.tmhm(i))return true;
            for(int i=0;i<8;++i)if((D4::kMachineHMDPPt[i]==m||D4::kMachineHMHGSS[i]==m)&&p.tmhm(92+i))return true;
        }
        for(uint8_t f=0;f<std::max<uint8_t>(1,Gen4::personal(Gen4::Table::HGSS,s,0).formCount());++f){
            auto r=Gen4::formIndex(Gen4::Table::HGSS,s,f);
            for(int i=0;i<52;++i)if(D4::kTutorMoves[i]==m&&((D4::kTutorBitsHGSS[r*7+(i>>3)]>>(i&7))&1))return true;
        }
        return false;
    }
    for(auto f:forms5(D5::kPersonalBW,D5::kPersonalBWSize,D5::kPersonalBWCount,s)){
        const auto* p=D5::kPersonalBW+f*D5::kPersonalBWSize;
        for(size_t i=0;i<D5::kMachineCount;++i)if(D5::kMachineMoves[i]==m&&((p[0x28+(i>>3)]>>(i&7))&1))return true;
        for(size_t i=0;i<D5::kTypeTutorCount;++i)if(D5::kTypeTutorMoves[i]==m&&((p[0x38]>>i)&1))return true;
    }
    for(auto f:forms5(D5::kPersonalB2W2,D5::kPersonalB2W2Size,D5::kPersonalB2W2Count,s)){
        const auto* p=D5::kPersonalB2W2+f*D5::kPersonalB2W2Size;
        for(size_t i=0;i<D5::kMachineCount;++i)if(D5::kMachineMoves[i]==m&&((p[0x28+(i>>3)]>>(i&7))&1))return true;
        for(size_t i=0;i<D5::kTypeTutorCount;++i)if(D5::kTypeTutorMoves[i]==m&&((p[0x38]>>i)&1))return true;
        for(size_t t=0;t<4;++t)for(size_t i=0;i<D5::kTutorCounts[t];++i)if(D5::kTutorMoves[t][i]==m&&((p[0x3c+4*t+(i>>3)]>>(i&7))&1))return true;
    }
    return false;
}
using BirthData=BirthMovesData;
BirthData birthData(uint8_t era,uint16_t species,uint8_t version){
    BirthData out;
    out.voltTackle=species==172&&(era>=4||version==3);
    const auto learn=[&](const auto* moves,const uint16_t* starts,size_t count){
        if(species>=count)return;
        for(auto i=starts[species];i<starts[species+1];++i){out.levels.push_back(moves[i].move);if(moves[i].level<=(era==3?5:1))out.initial.push_back(moves[i].move);}
    };
    if(era==3){
        if(version==3)learn(D4::kLevelUpE3,D4::kLevelUpE3Start,387);
        else if(version==4)learn(D4::kLevelUpFR3,D4::kLevelUpFR3Start,387);
        else if(version==5)learn(D4::kLevelUpLG3,D4::kLevelUpLG3Start,387);
        else learn(D4::kLevelUpRS3,D4::kLevelUpRS3Start,387);
        if(species<D4::kEggMovesRS3Count)out.eggs.assign(D4::kEggMovesRS3+D4::kEggMovesRS3Start[species],D4::kEggMovesRS3+D4::kEggMovesRS3Start[species+1]);
        for(size_t i=0;i<58;++i)if((Gen3Data::kMachineBits[species]>>i)&1)out.machines.push_back(i<50?Gen3Data::kMachineMovesTechnical[i]:Gen3Data::kMachineMovesHidden[i-50]);
    }else if(era==4){
        const auto table=Gen4::learnTable(version);const auto ls=Gen4::learnset(table,species,0);
        for(auto* m=ls.begin;m!=ls.end;++m){out.levels.push_back(m->move);if(m->level<=1)out.initial.push_back(m->move);}
        const bool hgss=version==7||version==8;
        const auto* eggMoves=hgss?D4::kEggMovesHGSS:D4::kEggMovesDPPt;
        const auto* eggStart=hgss?D4::kEggMovesHGSSStart:D4::kEggMovesDPPtStart;
        if(species<460)out.eggs.assign(eggMoves+eggStart[species],eggMoves+eggStart[species+1]);
        const auto pi=Gen4::personal(table,species,0);
        for(int i=0;i<100;++i)if(pi.tmhm(i))out.machines.push_back(i<92?D4::kMachineTM[i]:(hgss?D4::kMachineHMHGSS:D4::kMachineHMDPPt)[i-92]);
    }else{
        const bool bw=version==20||version==21;
        if(bw)learn(D5::kLevelUpBW,D5::kLevelUpBWStart,D5::kLearnsetBWCount);
        else learn(D5::kLevelUpB2W2,D5::kLevelUpB2W2Start,D5::kLearnsetB2W2Count);
        if(species<D5::kEggMoveCount)out.eggs.assign(D5::kEggMoves+D5::kEggMoveStart[species],D5::kEggMoves+D5::kEggMoveStart[species+1]);
        const auto* pi=(bw?D5::kPersonalBW:D5::kPersonalB2W2)+species*(bw?D5::kPersonalBWSize:D5::kPersonalB2W2Size);
        for(size_t i=0;i+1<D5::kMachineCount;++i)if((pi[0x28+(i>>3)]>>(i&7))&1)out.machines.push_back(D5::kMachineMoves[i]);
    }
    if(out.initial.size()>4)out.initial.erase(out.initial.begin(),out.initial.end()-4);
    return out;
}
bool canBeBorn(uint8_t era,uint16_t baby){
    if(baby==183||baby==202||(era>=4&&(baby==113||baby==122||baby==143||baby==185||baby==226||baby==315||baby==358)))return true;
    for(uint16_t s=1;s<=maximum(era);++s){const auto targets=nextSpecies(era,s);if(std::find(targets.begin(),targets.end(),baby)!=targets.end())return false;}
    return true;
}
std::vector<uint16_t> family(uint8_t era,uint16_t baby){
    std::vector<uint16_t> pending{baby},seen;
    while(!pending.empty()){
        auto s=pending.back();pending.pop_back();if(s>maximum(era)||std::find(seen.begin(),seen.end(),s)!=seen.end())continue;
        seen.push_back(s);for(auto to:nextSpecies(era,s))pending.push_back(to);
    }
    return seen;
}

}
bool Gen35Ancestry::provenOrdinaryBirth(uint8_t era,uint16_t baby,uint8_t version,const std::array<uint16_t,4>& moves,uint8_t paternal,uint8_t shared,bool exact,BirthWitness* witness){
    if(era<3||era>5||!baby||baby>maximum(era)||paternal>15||shared>15||(shared&~paternal)||!canBeBorn(era,baby))return false;
    const bool game=era==3?(version>=1&&version<=5):era==4?(version==7||version==8||version==10||version==11||version==12):(version>=20&&version<=23);
    if(!game)return false;
    const auto ordinary=[&](uint16_t parent,uint8_t mask){
        if(personal5(parent)[0x20]>1)return false;
        for(size_t i=0;i<4;++i)if(mask&(1u<<i)){
            const auto m=moves[i];
            if(!direct(era,parent,m)||(era==4&&(m==307||m==308||m==338||m==434)))return false;
        }
        return true;
    };
    std::vector<uint16_t> mothers;
    for(auto m:family(era,baby==32?uint16_t{29}:baby==313?uint16_t{314}:baby))if(female(m))mothers.push_back(m);
    if(mothers.empty()&&!shared)mothers.push_back(132);
    const auto data=birthData(era,baby,version);
    for(auto mother:mothers){
        if(!ordinary(mother,shared))continue;
        for(uint16_t father=1;father<=maximum(era);++father){
            if(mother==132){const auto f=family(era,baby);if(!breeds(father)||std::find(f.begin(),f.end(),father)==f.end())continue;}
            else if(!male(father)||!compatible(mother,father))continue;
            if(!ordinary(father,paternal))continue;
            std::vector<uint16_t> dad,mom;
            for(size_t i=0;i<4;++i){if(paternal&(1u<<i))dad.push_back(moves[i]);if(shared&(1u<<i))mom.push_back(moves[i]);}
            if(dad.empty())continue;
            if(mom.empty()){
                for(uint16_t m=1;m<=(era==3?354:era==4?467:559);++m)if(std::find(dad.begin(),dad.end(),m)==dad.end()&&direct(era,mother,m)&&!(era==4&&(m==307||m==308||m==338||m==434))){mom.push_back(m);break;}
                if(mom.empty())continue;
            }
            std::sort(dad.begin(),dad.end());
            do{
                const bool light=data.voltTackle&&std::find(moves.begin(),moves.end(),344)!=moves.end();
                const auto result=inheritBirthMoves(data,era,dad,mom,light);
                if(era==5&&version<=21&&result!=inheritBirthMoves(data,era,dad,mom,light,false))continue;
                std::array<uint16_t,4> actual{};std::copy(result.begin(),result.end(),actual.begin());
                bool ok=actual==moves;
                if(!exact){ok=true;for(size_t i=0;i<4;++i)if((paternal&(1u<<i))&&std::find(result.begin(),result.end(),moves[i])==result.end())ok=false;}
                if(ok){
                    if(witness){witness->generation=era;witness->version=version;witness->baby=baby;witness->father=father;witness->mother=mother;witness->bornMoves=actual;witness->lightBall=light;std::copy(dad.begin(),dad.end(),witness->fatherMoves.begin());std::copy(mom.begin(),mom.end(),witness->motherMoves.begin());}
                    return true;
                }
            }while(std::next_permutation(dad.begin(),dad.end()));
        }
    }
    return false;
}
Gen35Ancestry::Gen35Ancestry(const std::array<uint16_t,4>& moves):moves_(moves),rules_(3*stride){
    const auto mask=[&](const auto& values){uint8_t out=0;for(size_t i=0;i<4;++i)if(moves[i]&&std::find(std::begin(values),std::end(values),moves[i])!=std::end(values))out|=uint8_t{1}<<i;return out;};
    for(uint8_t era=3;era<=5;++era)for(uint16_t s=1;s<=maximum(era);++s){
        auto& r=rules_[node(era,s)];
        for(size_t i=0;i<4;++i)if(moves[i]){
            const auto m=moves[i];if(direct(era,s,m))r.ordinary|=uint8_t{1}<<i;
            bool inherited=egg(era,s,m)||egg(era,counterpart(s),m)||(era==3?level3(s,m):level45(era,s,m));
            if(inherited)r.inherited|=uint8_t{1}<<i;
        }
        if(s==172)r.gifts.push_back(mask(std::array<uint16_t,1>{344}));
        for(auto target:nextSpecies(era,s))if(target<=maximum(era))r.evolvesTo.push_back(node(era,target));
        if(era<5)r.evolvesTo.push_back(node(era+1,s)); // forward-only transfer; HM erasure relaxed
        for(auto mother:maternalSpecies(era,s)){
            r.mothers.push_back(node(era,mother));
            for(uint16_t father=1;father<=maximum(era);++father)if(male(father)&&compatible(mother,father))r.fathers.push_back(node(era,father));
        }
        // Male-only/genderless families breed with Ditto; they cannot take a
        // different species' paternal moves. Shared-level knowledge is relaxed
        // inside this upper-bound graph and enforced on the queried final egg.
        if(r.mothers.empty())for(auto parent:family(era,s))if(breeds(parent))r.fathers.push_back(node(era,parent));
        std::sort(r.fathers.begin(),r.fathers.end());r.fathers.erase(std::unique(r.fathers.begin(),r.fathers.end()),r.fathers.end());
    }
    const auto seeds=[&](uint8_t era,const auto* entries,size_t count){for(size_t i=0;i<count;++i)if(entries[i].species<=maximum(era))rules_[node(era,entries[i].species)].gifts.push_back(mask(entries[i].moves));};
    seeds(3,D4::kGifts3,D4::kGift3Count);seeds(3,D4::kGiftsNY3,D4::kGiftNY3Count);seeds(3,D4::kStatics3,D4::kStatic3Count);seeds(3,D4::kSlots3,D4::kSlot3Count);seeds(3,D4::kGiftsColo3,D4::kGiftColo3Count);
    seeds(3,CXD::kColoShadows,CXD::kColoShadowCount);seeds(3,CXD::kColoEReader,CXD::kColoEReaderCount);seeds(3,CXD::kXdShadows,CXD::kXdShadowCount);seeds(3,CXD::kColoStarters,CXD::kColoStarterCount);seeds(3,CXD::kColoGifts,CXD::kColoGiftCount);seeds(3,CXD::kXdGifts,CXD::kXdGiftCount);seeds(3,CXD::kXdTrades,CXD::kXdTradeCount);
    seeds(4,D4::kRanch,D4::kRanchCount);seeds(4,D4::kStatics,D4::kStaticCount);seeds(4,D4::kTrades,D4::kTradeCount);
    for(size_t i=0;i<D4::kPcdCount;++i){const auto* p=D4::kPcd+i*D4::kPcdRecordSize+8;const auto s=read16(p+8);if(s&&s<=493)rules_[node(4,s)].gifts.push_back(mask(std::array<uint16_t,4>{read16(p+0x28),read16(p+0x2a),read16(p+0x2c),read16(p+0x2e)}));}
    for(size_t i=0;i<D5::kPgfCount;++i){const auto* p=D5::kPgf+i*D5::kPgfSize;const auto s=read16(p+0x1a);if(s&&s<=649)rules_[node(5,s)].gifts.push_back(mask(std::array<uint16_t,4>{read16(p+0x12),read16(p+0x14),read16(p+0x16),read16(p+0x18)}));}
    for(const auto& c:Gen5GlobalLink::kClaims)if(c.species<=649)rules_[node(5,c.species)].gifts.push_back(mask(std::array<uint16_t,1>{c.move}));
    for(size_t i=0;i<D5::kStaticCount;++i){const auto& t=D5::kStatics[i];if(t.species<=649)rules_[node(5,t.species)].gifts.push_back(mask(std::array<uint16_t,1>{t.move}));}
    chain_=std::make_unique<FiniteChain>(rules_);chain_->solve();
}
std::vector<uint16_t> Gen35Ancestry::maternalSpecies(uint8_t era,uint16_t baby)const{
    std::vector<uint16_t> out;
    for(auto s:family(era,baby==32?uint16_t{29}:baby==313?uint16_t{314}:baby))if(female(s))out.push_back(s);
    return out;
}
uint16_t Gen35Ancestry::findFather(uint8_t era,uint16_t baby,uint8_t wanted,uint8_t shared,bool simple)const{
    if(era<3||era>5||baby==0||baby>maximum(era)||wanted>15||shared>15)return 0;
    for(auto mother:maternalSpecies(era,baby))if(chain_->canKnow(node(era,mother),shared)&&(!simple||simpleWitness(node(era,mother),shared)))
        for(uint16_t father=1;father<=maximum(era);++father)if(male(father)&&compatible(mother,father)&&chain_->canKnow(node(era,father),wanted)&&(!simple||simpleWitness(node(era,father),wanted)))return node(era,father);
    if(shared==0&&maternalSpecies(era,baby).empty())for(auto candidate:rules_[node(era,baby)].fathers)if(chain_->canKnow(candidate,wanted)&&(!simple||simpleWitness(candidate,wanted)))return candidate;
    return 0;
}
bool Gen35Ancestry::simpleWitness(uint16_t target,uint8_t mask,bool allowBirth)const{
    if(!mask)return true;
    const auto canTeach=[&](uint16_t n,uint8_t wanted,int cap){
        const uint8_t era=3+n/stride;const uint16_t species=n%stride;
        // A cross-form union is not a constructive single-individual witness.
        if(personal5(species)[0x20]>1)return false;
        for(size_t bit=0;bit<4;++bit)if(wanted&(1u<<bit)){
            auto move=moves_[bit];
            if(era==4&&(move==307||move==308||move==338||move==434))return false;
            if(!direct(era,species,move,cap))return false;
        }
        return true;
    };
    if(canTeach(target,mask,100))return true;
    // A cyclic permissive witness is not a constructive physical ancestry.
    // These limits only stop positive certification, never prove impossibility.
    const uint32_t key=static_cast<uint32_t>(target)*16+mask;
    if(proofStack_.size()>=16||++proofVisits_>256||std::find(proofStack_.begin(),proofStack_.end(),key)!=proofStack_.end())return false;
    proofStack_.push_back(key);
    struct Pop {std::vector<uint32_t>& stack;~Pop(){stack.pop_back();}} pop{proofStack_};

    int cap=100;
    for(const auto& step:chain_->witness(target,mask)){
        if(step.kind==StepKind::Gift||step.kind==StepKind::PreviousEra)return false;
        if(step.kind==StepKind::Breed){
            if(!allowBirth)return false;
            const auto era=static_cast<uint8_t>(3+step.species/stride);
            const auto baby=static_cast<uint16_t>(step.species%stride);
            // The permissive graph stores its first witness. A known event may
            // reach a state before a wholly ordinary alternative does. Search
            // all ordinary donors before requiring that particular event.
            for(uint16_t father=1;father<=maximum(era);++father)
                if(male(father)&&chain_->canKnow(node(era,father),step.mask)&&simpleWitness(node(era,father),step.mask,false)&&birthWitness(era,baby,father,step.mask,step.mask))return true;
            if(!simpleWitness(step.parentSpecies,step.parentMask,true)||!birthWitness(era,baby,step.parentSpecies%stride,step.parentMask,step.mask))return false;
            cap=100; // the next ancestor is a different physical parent
        }
        if(step.kind==StepKind::Native){if(!canTeach(step.species,static_cast<uint8_t>(step.mask&~step.parentMask),cap))return false;}
        else if(step.kind==StepKind::Evolution){
            if(step.species/stride!=step.parentSpecies/stride){
                for(size_t bit=0;bit<4;++bit)if(step.mask&(1u<<bit)){
                    const auto move=moves_[bit];
                    if(std::find(std::begin(D4::kMachineHMDPPt),std::end(D4::kMachineHMDPPt),move)!=std::end(D4::kMachineHMDPPt)||std::find(std::begin(D4::kMachineHMHGSS),std::end(D4::kMachineHMHGSS),move)!=std::end(D4::kMachineHMHGSS)||std::find(std::begin(Gen3Data::kMachineMovesHidden),std::end(Gen3Data::kMachineMovesHidden),move)!=std::end(Gen3Data::kMachineMovesHidden))return false;
                }
            }else{
                const auto to=step.species%stride;
                if(to==122||to==185||to==424||to==463||to==465||to==469||to==473)return false;
                // Conservatively reserve a level even for stone/trade edges.
                if(--cap<1)return false;
            }
        }
    }
    return true;
}
bool Gen35Ancestry::birthWitness(uint8_t era,uint16_t baby,uint16_t father,uint8_t fatherMask,uint8_t wantedMask,uint8_t version,bool exact,uint8_t requiredShared)const{
    if(!canBeBorn(era,baby))return false;
    std::vector<uint16_t> versions=era==3?std::vector<uint16_t>{1,2,3,4,5}:era==4?std::vector<uint16_t>{10,11,12,7,8}:std::vector<uint16_t>{20,21,22,23};
    if(version)versions={version};
    std::vector<uint16_t> dad;
    for(size_t bit=0;bit<4;++bit)if(fatherMask&(1u<<bit))dad.push_back(moves_[bit]);
    std::sort(dad.begin(),dad.end());
    auto mothers=maternalSpecies(era,baby);
    if(mothers.empty()&&std::find(rules_[node(era,baby)].fathers.begin(),rules_[node(era,baby)].fathers.end(),node(era,father))!=rules_[node(era,baby)].fathers.end())mothers.push_back(132);
    for(auto mother:mothers)if((mother==132||compatible(mother,father))&&personal5(mother)[0x20]<=1){
        for(uint8_t momMask=0;momMask<16;++momMask){
            if(mother==132&&momMask)continue; // Ditto cannot supply shared level-up inheritance
            if(requiredShared!=255&&momMask!=requiredShared)continue;
            if(!chain_->canKnow(node(era,mother),momMask)||!simpleWitness(node(era,mother),momMask,false))continue;
            // With no queried move retained, a concrete mother still needs one
            // native filler outside the father's move list. Never invent it.
            uint16_t filler=0;
            if(!momMask){for(uint16_t m=1;m<=(era==3?354:era==4?467:559);++m)if(std::find(dad.begin(),dad.end(),m)==dad.end()&&direct(era,mother,m)&&!(era==4&&(m==307||m==308||m==338||m==434))){filler=m;break;}if(!filler)continue;}
            std::vector<uint16_t> mom;
            for(size_t bit=0;bit<4;++bit)if(momMask&(1u<<bit))mom.push_back(moves_[bit]);
            if(filler)mom.push_back(filler);
            for(auto game:versions){
                const auto data=birthData(era,baby,static_cast<uint8_t>(game));
                std::sort(dad.begin(),dad.end());
                do{
                    const auto result=inheritBirthMoves(data,era,dad,mom,data.voltTackle&&std::find(moves_.begin(),moves_.end(),344)!=moves_.end());
                    // The newly replayed child-order body is B2W2. Do not
                    // extrapolate an order-dependent clean certificate to BW:
                    // require the prior paternal traversal to agree as well.
                    if(era==5&&game<=21&&result!=inheritBirthMoves(data,era,dad,mom,data.voltTackle&&std::find(moves_.begin(),moves_.end(),344)!=moves_.end(),false))continue;
                    std::array<uint16_t,4> actual{};std::copy(result.begin(),result.end(),actual.begin());
                    bool accepted=actual==moves_;
                    if(!exact){accepted=true;for(size_t bit=0;bit<4;++bit)if((wantedMask&(1u<<bit))&&std::find(result.begin(),result.end(),moves_[bit])==result.end())accepted=false;}
                    if(accepted){
                        BirthWitness witness;witness.generation=era;witness.version=static_cast<uint8_t>(game);witness.baby=baby;witness.father=father;witness.mother=mother;witness.bornMoves=actual;witness.lightBall=data.voltTackle&&std::find(moves_.begin(),moves_.end(),344)!=moves_.end();
                        std::copy(dad.begin(),dad.end(),witness.fatherMoves.begin());std::copy(mom.begin(),mom.end(),witness.motherMoves.begin());certifiedBirths_.push_back(witness);
                        return true;
                    }
                }while(std::next_permutation(dad.begin(),dad.end()));
            }
        }
    }
    return false;
}
bool Gen35Ancestry::provenBirth5(uint16_t baby,uint8_t version,uint8_t wanted,uint8_t shared)const{
    return provenBirth(5,baby,version,wanted,shared,true);
}
bool Gen35Ancestry::provenBirth(uint8_t era,uint16_t baby,uint8_t version,uint8_t wanted,uint8_t shared,bool exact)const{
    certifiedBirths_.clear();proofStack_.clear();proofVisits_=0;
    if(era<3||era>5||!baby||baby>maximum(era)||wanted>15||shared>15||(shared&~wanted))return false;
    const bool game=era==3?(version>=1&&version<=5):era==4?(version==7||version==8||version==10||version==11||version==12):(version>=20&&version<=23);
    if(!game)return false;
    BirthWitness ordinary;
    if(provenOrdinaryBirth(era,baby,version,moves_,wanted,shared,exact,&ordinary)){
        certifiedBirths_.push_back(ordinary);return true;
    }
    if(!possible(era,baby,wanted,shared))return false;
    auto mothers=maternalSpecies(era,baby);
    if(mothers.empty()&&!shared)mothers.push_back(132);
    for(auto mother:mothers)if(chain_->canKnow(node(era,mother),shared)&&simpleWitness(node(era,mother),shared))
        for(uint16_t father=1;father<=maximum(era);++father){
            certifiedBirths_.clear();
            const bool parentAllowed=mother==132?std::find(rules_[node(era,baby)].fathers.begin(),rules_[node(era,baby)].fathers.end(),node(era,father))!=rules_[node(era,baby)].fathers.end():male(father)&&compatible(mother,father);
            if(parentAllowed&&chain_->canKnow(node(era,father),wanted)&&simpleWitness(node(era,father),wanted)&&birthWitness(era,baby,father,wanted,wanted,version,exact,shared))return true;
        }
    certifiedBirths_.clear();
    return false;
}
bool Gen35Ancestry::provenSimple(uint8_t era,uint16_t baby,uint8_t wanted,uint8_t shared)const{
    proofStack_.clear();proofVisits_=0;
    if(era<3||era>5||!baby||baby>maximum(era)||wanted>15||shared>15||(shared&~wanted))return false;
    return wanted==0||findFather(era,baby,wanted,shared,true)!=0;
}
bool Gen35Ancestry::possible(uint8_t era,uint16_t baby,uint8_t wanted,uint8_t shared)const{
    if(era<3||era>5||!baby||baby>maximum(era)||wanted>15||shared>15||(shared&~wanted))return false;
    return wanted==0||findFather(era,baby,wanted,shared)!=0;
}
bool Gen35Ancestry::possibleWithUncertifiedParentRole5(uint16_t baby,uint8_t wanted)const{
    if(!baby||baby>649||wanted>15)return false;
    // The complete Gen V family/event ancestry closure remains uncertified.
    // Even after the B2W2 role replay, keep this upper-bound relaxation when
    // one actual family carrier can possess the whole queried combination.
    // Different individuals' moves are never unioned here.
    for(auto parent:maternalSpecies(5,baby))if(chain_->canKnow(node(5,parent),wanted))return true;
    return false;
}
std::vector<Step> Gen35Ancestry::fatherWitness(uint8_t era,uint16_t baby,uint8_t wanted,uint8_t shared)const{
    const auto father=findFather(era,baby,wanted,shared);return father?chain_->witness(father,wanted):std::vector<Step>{};
}
}
