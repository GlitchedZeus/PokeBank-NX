#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/Experience.h"
#include "Names/SpeciesNames.h"
#include <algorithm>

namespace PokeVault::Integration::Gen1 {
namespace {
#include "Gen1PersonalData.inc"
// Pinned Sav1.cpp / SAV1Offsets: international 12 x 20, PK1 stored length 33.
constexpr size_t Capacity=20, BoxSize=0x462, Current=0x30C0, Checksum=0x3523;
size_t bankOffset(size_t b) { return (b<6 ? 0x4000 : 0x6000)+(b%6)*BoxSize; }
size_t boxOffset(const Metadata& m,size_t b) { return b==m.currentBox ? Current : bankOffset(b); }
size_t bodyOffset(size_t b,size_t s) { return b+22+s*33; }
size_t otOffset(size_t b,size_t s) { return b+22+20*33+s*11; }
size_t nickOffset(size_t b,size_t s) { return b+22+20*44+s*11; }
void be16(std::vector<uint8_t>& b,size_t o,uint16_t v) { b[o]=v>>8; b[o+1]=v&255; }
void be24(std::vector<uint8_t>& b,size_t o,uint32_t v) { b[o]=v>>16; b[o+1]=(v>>8)&255; b[o+2]=v&255; }
void repair(std::vector<uint8_t>& b) {
    const auto s=std::span<const uint8_t>(b);
    b[Checksum]=calculateDiff8(s.subspan(0x2598,0xF8B));
    if (!(b[0x284C]&0x80)) return;
    for (size_t bank : {size_t(0x4000),size_t(0x6000)}) {
        b[bank+0x1A4C]=calculateDiff8(s.subspan(bank,0x1A4C));
        for(size_t i=0;i<6;++i) b[bank+0x1A4D+i]=calculateDiff8(s.subspan(bank+i*BoxSize,BoxSize));
    }
}
void initializeBanks(std::vector<uint8_t>& b,const Metadata& m) {
    if(b[0x284C]&0x80) return;
    for(size_t box=0;box<12;++box) {
        const auto o=bankOffset(box);
        std::fill_n(b.begin()+o,BoxSize,0);
        b[o+1]=0xFF;
    }
    std::copy_n(b.begin()+Current,BoxSize,b.begin()+bankOffset(m.currentBox));
    b[0x284C]|=0x80;
}
uint8_t internalSpecies(uint16_t species) {
    for(unsigned i=1;i<256;++i) if(gen1InternalToNational(i)==species) return i;
    return 0;
}
bool validPokemon(const PokemonRecord& p,std::string& error) {
    if(p.species<1 || p.species>151 || p.level<1 || p.level>100 ||
       p.experience>Pokemon::getExpForLevel(100,kPersonal[p.species][8]) ||
       Pokemon::getLevelFromExp(p.experience,kPersonal[p.species][8])!=p.level) {
        error="Species, level and experience must form a valid Gen I combination"; return false;
    }
    for(size_t i=0;i<4;++i) {
        if(p.moves[i]>165 || p.ppUps[i]>3 ||
           (!p.moves[i] && (p.pp[i] || p.ppUps[i])) ||
           p.pp[i]>StagedPokemonEditor::moveMaxPP(p.moves[i],p.ppUps[i])) {
            error="Move/PP is outside the Gen I range (moves 0..165, PP Ups 0..3)"; return false;
        }
    }
    return true;
}
void copySlot(const std::vector<uint8_t>& from,size_t f,size_t fs,
              std::vector<uint8_t>& to,size_t t,size_t ts) {
    to[t+1+ts]=from[f+1+fs];
    std::copy_n(from.begin()+bodyOffset(f,fs),33,to.begin()+bodyOffset(t,ts));
    std::copy_n(from.begin()+otOffset(f,fs),11,to.begin()+otOffset(t,ts));
    std::copy_n(from.begin()+nickOffset(f,fs),11,to.begin()+nickOffset(t,ts));
}
bool sameRecord(const PokemonRecord& a,const PokemonRecord& b) {
    return a.rawBody==b.rawBody && a.nickname==b.nickname && a.originalTrainer==b.originalTrainer;
}
}

StagedPokemonEditor::StagedPokemonEditor(std::shared_ptr<const ReadOnlySave> s):original_(s),view_(s) {}
std::unique_ptr<StagedPokemonEditor> StagedPokemonEditor::create(const ReadOnlySave& s,std::string& error) {
    error.clear();
    if(s.metadata().region!=RegionLayout::International) {
        error="Gen I boxed editing supports the validated international layout only"; return nullptr;
    }
    auto verified=parse(s.sourceBytes(),s.metadata().sourceGame);
    if(!verified || verified.save->metadata().sourceGameId!=s.metadata().sourceGameId) {
        error="Gen I source failed strict identity/layout validation"; return nullptr;
    }
    return std::unique_ptr<StagedPokemonEditor>(new StagedPokemonEditor(verified.save));
}
uint8_t StagedPokemonEditor::growthRate(uint16_t species) noexcept { return species<=151?kPersonal[species][8]:0; }
uint8_t StagedPokemonEditor::moveBasePP(uint16_t move) noexcept { return move<=165?kBasePP[move]:0; }
uint8_t StagedPokemonEditor::moveMaxPP(uint16_t move,uint8_t ups) noexcept {
    const auto pp=moveBasePP(move);
    return pp+std::min<unsigned>(7,pp/5)*std::min<unsigned>(3,ups);
}
uint8_t StagedPokemonEditor::derivedHPDV(const std::array<uint8_t,4>& d) noexcept {
    return ((d[0]&1)<<3)|((d[1]&1)<<2)|((d[2]&1)<<1)|(d[3]&1);
}
bool StagedPokemonEditor::encodeName(std::string_view text,size_t maximum,
                                     std::array<uint8_t,11>& out,std::string& error) {
    error.clear(); out.fill(0x50);
    // Conservative shared English glyph subset; accents/language-specific glyphs remain read-only.
    if(text.empty() || text.size()>maximum || maximum>10) { error="Name is empty or exceeds Gen I length"; return false; }
    for(size_t i=0;i<text.size();++i) {
        const auto c=text[i];
        if(c>='A' && c<='Z') out[i]=0x80+c-'A';
        else if(c>='a' && c<='z') out[i]=0xA0+c-'a';
        else if(c>='0' && c<='9') out[i]=0xF6+c-'0';
        else if(c==' ') out[i]=0x7F;
        else if(c=='-') out[i]=0xE3;
        else if(c=='.') out[i]=0xF2;
        else if(c=='?') out[i]=0xE6;
        else if(c=='!') out[i]=0xE7;
        else { error="Supported names use A-Z, a-z, 0-9, space, -, ., ? and !"; return false; }
    }
    if(decodeGen1String(out,RegionLayout::International)!=text) { error="Gen I name round trip failed"; return false; }
    return true;
}
std::optional<PokemonRecord> StagedPokemonEditor::boxedPokemon(size_t b,size_t s,std::string& error) const {
    error.clear(); if(b>=12 || s>=Capacity) { error="Box or slot is out of range"; return {}; }
    return view_->boxes()[b].slots[s];
}
std::optional<size_t> StagedPokemonEditor::appendSlot(size_t box, std::string& error) const {
    error.clear();
    if (box >= view_->boxes().size()) { error = "Generation I box is out of range"; return {}; }
    const auto& slots = view_->boxes()[box].slots;
    const size_t count = static_cast<size_t>(std::count_if(slots.begin(), slots.end(), [](const auto& p) { return bool(p); }));
    if (count >= slots.size()) { error = "This Generation I box is full"; return {}; }
    return count;
}
bool StagedPokemonEditor::destination(size_t b,size_t s,std::string& error) const {
    error.clear(); if(b>=12 || s>=Capacity) { error="Box is full or destination is out of range"; return false; }
    const auto& slots=view_->boxes()[b].slots;
    const auto count=std::count_if(slots.begin(),slots.end(),[](const auto& p){return bool(p);});
    if(s!=static_cast<size_t>(count)) { error=slots[s]?"Destination slot is occupied":"Select the first empty slot; Gen I boxes are contiguous"; return false; }
    return true;
}
void StagedPokemonEditor::rebuildChanges() {
    changes_.clear();
    for(size_t b=0;b<12;++b) for(size_t s=0;s<Capacity;++s) {
        const auto& a=original_->boxes()[b].slots[s]; const auto& z=view_->boxes()[b].slots[s];
        const auto prefix=std::to_string(b)+"/"+std::to_string(s)+"/";
        const auto label="Box "+std::to_string(b+1)+" Slot "+std::to_string(s+1)+": ";
        auto add=[&](std::string key,std::string title,std::string before,std::string after) {
            if(before!=after) changes_.push_back({prefix+key,label+title,before,after});
        };
        if(!a || !z) {
            if(bool(a)!=bool(z)) add("presence",z?"Added Pokemon":"Removed Pokemon",
                a?std::string(Names::getSpeciesName(a->species)):"Empty",
                z?std::string(Names::getSpeciesName(z->species)):"Empty");
            continue;
        }
        add("species","Species",Names::getSpeciesName(a->species),Names::getSpeciesName(z->species));
        add("nickname","Nickname",a->nickname,z->nickname);
        add("ot","OT",a->originalTrainer,z->originalTrainer);
        auto number=[&](std::string key,std::string title,unsigned before,unsigned after){add(key,title,std::to_string(before),std::to_string(after));};
        number("level","Level",a->level,z->level); number("exp","Experience",a->experience,z->experience);
        number("tid","Trainer ID",a->trainerId,z->trainerId);
        const char* stats[]{"HP","Attack","Defense","Speed","Special"};
        for(size_t i=0;i<4;++i) {
            auto n=std::to_string(i+1);
            number("move"+n,"Move "+n,a->moves[i],z->moves[i]);
            number("pp"+n,"PP "+n,a->pp[i],z->pp[i]);
            number("ups"+n,"PP Ups "+n,a->ppUps[i],z->ppUps[i]);
            number("dv"+n,std::string(stats[i+1])+" DV",a->dvs[i+1],z->dvs[i+1]);
        }
        for(size_t i=0;i<5;++i) number("stat"+std::to_string(i),std::string(stats[i])+" Stat Exp",a->statExperience[i],z->statExperience[i]);
        // Includes stored type/catch-rate/HP and all raw differences after clone/compaction.
        if(a->rawBody!=z->rawBody && std::none_of(changes_.begin(),changes_.end(),[&](const auto& c){return c.key.starts_with(prefix);}))
            changes_.push_back({prefix+"raw",label+"Stored record","Original bytes","Changed bytes"});
    }
}
bool StagedPokemonEditor::commit(std::vector<uint8_t> candidate,std::string& error) {
    repair(candidate);
    auto check=parse(candidate,metadata().sourceGame);
    if(!check) { error="Staged Gen I structural validation failed: "+check.detail; return false; }
    // Restore all untouched/fully reverted boxes byte-for-byte, including unused tails.
    const auto orig=originalBytes();
    for(size_t b=0;b<12;++b) {
        bool equal=true;
        for(size_t s=0;s<Capacity;++s) {
            const auto& a=original_->boxes()[b].slots[s]; const auto& z=check.save->boxes()[b].slots[s];
            if(bool(a)!=bool(z) || (a && !sameRecord(*a,*z))) {equal=false;break;}
        }
        if(equal && (metadata().boxesInitialized || b==metadata().currentBox)) {
            const auto off=boxOffset(metadata(),b);
            std::copy_n(orig.begin()+off,BoxSize,candidate.begin()+off);
            if(b==metadata().currentBox && metadata().boxesInitialized)
                std::copy_n(orig.begin()+bankOffset(b),BoxSize,candidate.begin()+bankOffset(b));
        }
    }
    repair(candidate); check=parse(candidate,metadata().sourceGame);
    if(!check) {error=check.detail;return false;}
    view_=check.save; rebuildChanges();
    if(changes_.empty()) view_=original_;
    error.clear(); return true;
}
bool StagedPokemonEditor::stageEdit(size_t b,size_t s,const BoxPokemonEdit& e,std::string& error) {
    auto p=boxedPokemon(b,s,error); if(!p) {if(error.empty())error="Box slot is empty";return false;}
    auto next=*p;
    if(e.species) {if(*e.species<1 || *e.species>151){error="Gen I species must be 1..151";return false;} next.species=*e.species;}
    const auto growth=growthRate(next.species);
    if(e.level && (*e.level<1 || *e.level>100)) {error="Level must be 1..100";return false;}
    if(e.experience) next.experience=*e.experience;
    else if(e.level || (e.species && next.species!=p->species))
        next.experience=Pokemon::getExpForLevel(e.level.value_or(next.level),growth);
    if(e.level && e.experience && Pokemon::getLevelFromExp(*e.experience,growth)!=*e.level) {error="Level and EXP disagree";return false;}
    next.level=Pokemon::getLevelFromExp(next.experience,growth);
    if(e.moves) {
        next.moves=*e.moves;
        for(size_t i=0;i<4;++i) if(next.moves[i]!=p->moves[i]) {next.ppUps[i]=0;next.pp[i]=moveBasePP(next.moves[i]);}
    }
    if(e.pp) next.pp=*e.pp;
    if(e.ppUps) next.ppUps=*e.ppUps;
    if(e.dvs) {
        for(auto v:*e.dvs) if(v>15) {error="DVs must be 0..15";return false;}
        next.dvs={derivedHPDV(*e.dvs),(*e.dvs)[0],(*e.dvs)[1],(*e.dvs)[2],(*e.dvs)[3]};
    }
    if(e.statExperience) next.statExperience=*e.statExperience;
    if(e.trainerId) next.trainerId=*e.trainerId;
    if(!validPokemon(next,error)) return false;
    std::array<uint8_t,11> nick{},ot{};
    if(e.nickname && *e.nickname!=p->nickname && !encodeName(*e.nickname,10,nick,error)) return false;
    if(e.otName && *e.otName!=p->originalTrainer && !encodeName(*e.otName,7,ot,error)) return false;
    std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());
    const auto box=boxOffset(metadata(),b),o=bodyOffset(box,s);
    bytes[box+1+s]=bytes[o]=internalSpecies(next.species);
    bytes[o+3]=next.level;
    if(next.species!=p->species) {bytes[o+5]=kPersonal[next.species][5];bytes[o+6]=kPersonal[next.species][6];}
    be24(bytes,o+14,next.experience);be16(bytes,o+12,next.trainerId);
    for(size_t i=0;i<4;++i) {bytes[o+8+i]=next.moves[i];bytes[o+29+i]=next.pp[i]|(next.ppUps[i]<<6);}
    for(size_t i=0;i<5;++i) be16(bytes,o+17+i*2,next.statExperience[i]);
    bytes[o+27]=(next.dvs[1]<<4)|next.dvs[2];bytes[o+28]=(next.dvs[3]<<4)|next.dvs[4];
    if(e.nickname && *e.nickname!=p->nickname) {
        const auto& baseline=original_->boxes()[b].slots[s];
        if(baseline && *e.nickname==baseline->nickname) std::copy_n(originalBytes().begin()+nickOffset(box,s),11,bytes.begin()+nickOffset(box,s));
        else std::copy(nick.begin(),nick.end(),bytes.begin()+nickOffset(box,s));
    }
    if(e.otName && *e.otName!=p->originalTrainer) {
        const auto& baseline=original_->boxes()[b].slots[s];
        if(baseline && *e.otName==baseline->originalTrainer) std::copy_n(originalBytes().begin()+otOffset(box,s),11,bytes.begin()+otOffset(box,s));
        else std::copy(ot.begin(),ot.end(),bytes.begin()+otOffset(box,s));
    }
    if(b==metadata().currentBox && (bytes[0x284C]&0x80)) std::copy_n(bytes.begin()+Current,BoxSize,bytes.begin()+bankOffset(b));
    return commit(std::move(bytes),error);
}
bool StagedPokemonEditor::stageAdd(size_t b,size_t s,const BoxPokemonCreate& c,std::string& error) {
    if(!destination(b,s,error)) return false;
    if(c.species<1 || c.species>151 || c.level<1 || c.level>100) {error="Invalid Gen I species/level";return false;}
    std::array<uint8_t,11> nick{},ot{};
    auto nickname=c.nickname.empty()?std::string(Names::getSpeciesName(c.species)):c.nickname;
    // National names containing symbols use an editable ASCII default.
    if(c.nickname.empty() && c.species==29) nickname="Nidoran F";
    if(c.nickname.empty() && c.species==32) nickname="Nidoran M";
    if(c.nickname.empty() && c.species==83) nickname="Farfetchd";
    const auto owner=c.otName.empty()?original_->trainer().name:c.otName;
    if(!encodeName(nickname,10,nick,error) || !encodeName(owner,7,ot,error)) return false;
    for(auto v:c.dvs) if(v>15) {error="DVs must be 0..15";return false;}
    PokemonRecord p; p.species=c.species;p.level=c.level;p.experience=Pokemon::getExpForLevel(c.level,growthRate(c.species));
    p.moves=c.moves;p.pp=c.pp;p.ppUps=c.ppUps;
    if(!validPokemon(p,error)) return false;
    std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());
    if(b!=metadata().currentBox) initializeBanks(bytes,metadata());
    const auto box=boxOffset(metadata(),b),o=bodyOffset(box,s);
    std::fill_n(bytes.begin()+o,33,0);bytes[box]=s+1;bytes[box+1+s]=bytes[o]=internalSpecies(c.species);bytes[box+2+s]=0xFF;
    bytes[o+3]=c.level;bytes[o+5]=kPersonal[c.species][5];bytes[o+6]=kPersonal[c.species][6];bytes[o+7]=kPersonal[c.species][7];
    // Catch rate is a stored Gen I byte, not a held item; do not claim encounter legality.
    be16(bytes,o+1,static_cast<uint16_t>(((kPersonal[c.species][0]+derivedHPDV(c.dvs))*2*c.level)/100+c.level+10));
    be16(bytes,o+12,c.trainerId.value_or(original_->trainer().trainerId));be24(bytes,o+14,p.experience);
    for(size_t i=0;i<4;++i) {bytes[o+8+i]=c.moves[i];bytes[o+29+i]=c.pp[i]|(c.ppUps[i]<<6);}
    for(size_t i=0;i<5;++i) be16(bytes,o+17+i*2,c.statExperience[i]);
    bytes[o+27]=(c.dvs[0]<<4)|c.dvs[1];bytes[o+28]=(c.dvs[2]<<4)|c.dvs[3];
    std::copy(nick.begin(),nick.end(),bytes.begin()+nickOffset(box,s));std::copy(ot.begin(),ot.end(),bytes.begin()+otOffset(box,s));
    if(b==metadata().currentBox && (bytes[0x284C]&0x80)) std::copy_n(bytes.begin()+Current,BoxSize,bytes.begin()+bankOffset(b));
    return commit(std::move(bytes),error);
}
bool StagedPokemonEditor::stageClone(size_t sb,size_t ss,size_t b,size_t s,std::string& error) {
    auto p=boxedPokemon(sb,ss,error); if(!p) {if(error.empty())error="Clone source is empty";return false;}
    if(!destination(b,s,error) || !validPokemon(*p,error)) return false;
    std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());
    if(b!=metadata().currentBox) initializeBanks(bytes,metadata());
    const auto from=boxOffset(metadata(),sb),to=boxOffset(metadata(),b);
    copySlot(bytes,from,ss,bytes,to,s);bytes[to]=s+1;bytes[to+2+s]=0xFF;
    if(b==metadata().currentBox && (bytes[0x284C]&0x80)) std::copy_n(bytes.begin()+Current,BoxSize,bytes.begin()+bankOffset(b));
    return commit(std::move(bytes),error);
}
bool StagedPokemonEditor::stageRemove(size_t b,size_t s,std::string& error) {
    auto p=boxedPokemon(b,s,error); if(!p) {if(error.empty())error="Box slot is empty";return false;}
    std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());const auto old=bytes;
    const auto box=boxOffset(metadata(),b);const size_t count=bytes[box];
    for(size_t i=s;i+1<count;++i) copySlot(old,box,i+1,bytes,box,i);
    const auto last=count-1;bytes[box]=last;
    std::fill(bytes.begin()+box+1+last,bytes.begin()+box+22,0);bytes[box+1+last]=0xFF;
    std::fill_n(bytes.begin()+bodyOffset(box,last),33,0);std::fill_n(bytes.begin()+otOffset(box,last),11,0x50);std::fill_n(bytes.begin()+nickOffset(box,last),11,0x50);
    if(b==metadata().currentBox && (bytes[0x284C]&0x80)) std::copy_n(bytes.begin()+Current,BoxSize,bytes.begin()+bankOffset(b));
    return commit(std::move(bytes),error);
}
bool StagedPokemonEditor::revertPokemon(size_t b,size_t s,std::string& error) {
    if(b>=12 || s>=Capacity) {error="Box or slot out of range";return false;}
    const auto& p=original_->boxes()[b].slots[s];
    if(!p) return stageRemove(b,s,error);
    // Whole-box restoration avoids ambiguous identities after compaction.
    std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());const auto box=boxOffset(metadata(),b);
    std::copy_n(originalBytes().begin()+box,BoxSize,bytes.begin()+box);
    if(b==metadata().currentBox && (bytes[0x284C]&0x80)) std::copy_n(bytes.begin()+Current,BoxSize,bytes.begin()+bankOffset(b));
    return commit(std::move(bytes),error);
}
void StagedPokemonEditor::discard() noexcept {view_=original_;changes_.clear();}
std::vector<uint8_t> StagedPokemonEditor::finalizedBytes(std::string& error,std::span<const uint8_t> inventory) const {
    error.clear();std::vector<uint8_t> bytes(stagedBytes().begin(),stagedBytes().end());
    if(!inventory.empty()) {
        if(inventory.size()!=kRawSaveSize || !parse(inventory,metadata().sourceGame)) {error="Inventory candidate failed identity/layout validation";return {};}
        for(size_t i=0;i<inventory.size();++i) {
            const bool allowed=(i>=0x25C9 && i<0x25F3)||(i>=0x27E6 && i<0x284C);
            if(allowed) bytes[i]=inventory[i];
            else if(i!=Checksum && inventory[i]!=originalBytes()[i]) {error="Inventory baseline does not match this Pokemon editor";return {};}
        }
    }
    repair(bytes);auto check=parse(bytes,metadata().sourceGame);
    if(!check || check.save->metadata().region!=RegionLayout::International) {error="Export strict reload failed";return {};}
    for(size_t b=0;b<12;++b) for(size_t s=0;s<Capacity;++s) {
        const auto& p=check.save->boxes()[b].slots[s]; const auto& expected=view_->boxes()[b].slots[s];
        if(bool(p)!=bool(expected) || (p && !sameRecord(*p,*expected))) {error="Export Pokemon round trip failed";return {};}
        if(p && !validPokemon(*p,error)) return {};
    }
    return bytes;
}
} // namespace PokeVault::Integration::Gen1
