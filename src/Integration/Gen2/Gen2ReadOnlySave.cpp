#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2GenderTable.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace PokeVault::Integration::Gen2 {
namespace {

struct Layout {
    RegionLayout region;
    VersionFamily family;
    std::size_t trainer;
    std::size_t money;
    std::size_t currentBoxIndex;
    std::size_t boxNames;
    std::size_t party;
    std::size_t checksumEnd;
    std::size_t checksum;
    std::size_t checksum2;
    std::size_t gender;
    uint8_t boxCount;
    uint8_t boxCapacity;
    uint8_t stringLength;
    uint8_t trainerNameLength;
    std::size_t boxSpacing;
    uint8_t firstBankCount;
};

// PKSM-Core aa22d7... Sav2 and PKHeX 77dcd3a... SAV2Offsets/SAV2.
// PKHeX stores the same additive checksum at two positions and requires both to validate.
constexpr Layout kIntGS{RegionLayout::International, VersionFamily::GoldSilver,
    0x2009,0x23DB,0x2724,0x2727,0x288A,0x2D68,0x2D69,0x7E6D,0,
    14,20,11,7,0x450,7};
constexpr Layout kIntC{RegionLayout::International, VersionFamily::Crystal,
    0x2009,0x23DC,0x2700,0x2703,0x2865,0x2B82,0x2D0D,0x1F0D,0x3E3D,
    14,20,11,7,0x450,7};
constexpr Layout kJpGS{RegionLayout::Japanese, VersionFamily::GoldSilver,
    0x2009,0x23BC,0x2705,0x2708,0x283E,0x2C8B,0x2D0D,0x7F0D,0,
    9,30,6,5,0x54A,6};
constexpr Layout kJpC{RegionLayout::Japanese, VersionFamily::Crystal,
    0x2009,0x23BE,0x26E2,0x26E5,0x281A,0x2AE2,0x2D0D,0x7F0D,0x8000,
    9,30,6,5,0x54A,6};
constexpr std::array<Layout,4> kLayouts{kIntGS,kIntC,kJpGS,kJpC};
constexpr std::size_t kPartyCapacity = 6;
constexpr std::size_t kStoredBody = 32;
constexpr std::size_t kPartyBody = 48;

uint16_t readBE16(std::span<const uint8_t> b, std::size_t o) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(b[o]) << 8) | b[o+1]);
}
uint16_t readLE16(std::span<const uint8_t> b, std::size_t o) noexcept {
    return static_cast<uint16_t>(b[o] | (static_cast<uint16_t>(b[o+1]) << 8));
}
uint32_t readBE24(std::span<const uint8_t> b, std::size_t o) noexcept {
    return (static_cast<uint32_t>(b[o]) << 16) | (static_cast<uint32_t>(b[o+1]) << 8) | b[o+2];
}

bool payloadFor(std::span<const uint8_t> raw, std::size_t& payload, std::size_t& footer) noexcept {
    footer = 0;
    if (raw.size() == kRawSaveSize32K || raw.size() == kRawSaveSize64K) {
        payload = raw.size();
        return true;
    }
    // PKHeX's relaxed Gen1-3 RTC footer handler accepts appended RTC metadata only on the
    // 64 KiB Gen II raw size. The footer is preserved byte-for-byte but excluded from parsing.
    if (raw.size() > kRawSaveSize64K) {
        const std::size_t extra = raw.size() - kRawSaveSize64K;
        if (isKnownRTCFooterSize(extra)) {
            payload = kRawSaveSize64K;
            footer = extra;
            return true;
        }
    }
    return false;
}

bool presentMarker(uint8_t v) noexcept { return v != 0 && v != 0xFF; }

bool validListHeader(std::span<const uint8_t> p, std::size_t o, std::size_t capacity) noexcept {
    if (o > p.size() || p.size() - o < 2 + capacity) return false;
    const uint8_t count = p[o];
    if (count > capacity) return false;
    for (std::size_t i=0;i<capacity;++i) {
        if (presentMarker(p[o+1+i]) != (i < count)) return false;
    }
    return p[o+1+capacity] == 0xFF;
}

bool layoutFits(std::span<const uint8_t> p, const Layout& l) noexcept {
    // Current PKHeX accepts 32 KiB international GSC and 64 KiB Japanese GSC. Do not allow a
    // structurally plausible Japanese offset map to classify a 32 KiB container by accident.
    if (l.region == RegionLayout::International) {
        if (p.size() != kRawSaveSize32K) return false;
    } else if (p.size() != kRawSaveSize64K) {
        return false;
    }
    if (l.gender && l.gender >= p.size()) return false;
    if (l.checksum + 1 >= p.size() || l.checksum2 + 1 >= p.size() ||
        l.party >= p.size() || l.currentBoxIndex >= p.size()) return false;
    if (!validListHeader(p, l.party, kPartyCapacity)) return false;
    if (p[l.currentBoxIndex] & 0x80) return false;
    if ((p[l.currentBoxIndex] & 0x7F) >= l.boxCount) return false;
    const uint16_t want = calculateChecksum(p, l.region, l.family);
    if (want != readLE16(p, l.checksum) || want != readLE16(p, l.checksum2)) return false;
    // Validate the first authoritative stored box, not the potentially stale current-box copy.
    return validListHeader(p, 0x4000, l.boxCapacity);
}

std::string glyphEN(uint8_t b) {
    if (b >= 0x80 && b <= 0x99) return std::string(1, static_cast<char>('A' + b - 0x80));
    if (b >= 0xA0 && b <= 0xB9) return std::string(1, static_cast<char>('a' + b - 0xA0));
    if (b >= 0xF6) return std::string(1, static_cast<char>('0' + b - 0xF6));
    switch (b) {
        case 0x7F:return " "; case 0x9A:return "("; case 0x9B:return ")"; case 0x9C:return ":"; case 0x9D:return ";";
        case 0x9E:return "["; case 0x9F:return "]"; case 0xBA:return "à"; case 0xBB:return "è"; case 0xBC:return "é";
        case 0xBD:return "ù"; case 0xBE:return "À"; case 0xBF:return "Á"; case 0xC0:return "Ä"; case 0xC1:return "Ö";
        case 0xC2:return "Ü"; case 0xC3:return "ä"; case 0xC4:return "ö"; case 0xC5:return "ü"; case 0xC6:return "È";
        case 0xC7:return "É"; case 0xC8:return "Ì"; case 0xC9:return "Í"; case 0xCA:return "Ñ"; case 0xCB:return "Ò";
        case 0xCC:return "Ó"; case 0xCD:return "Ù"; case 0xCE:return "Ú"; case 0xCF:return "á";
        case 0xD0:return "'d"; case 0xD1:return "'l"; case 0xD2:return "'m"; case 0xD3:return "'r";
        case 0xD4:return "'s"; case 0xD5:return "'t"; case 0xD6:return "'v";
        case 0xDF:return "←"; case 0xE0:return "’"; case 0xE3:return "-"; case 0xE4:return "+";
        case 0xE6:return "?"; case 0xE7:return "!"; case 0xE8:return "."; case 0xE9:return "&"; case 0xEB:return "→";
        case 0xEF:return "♂"; case 0xF0:return "¥"; case 0xF1:return "×"; case 0xF2:return "."; case 0xF3:return "/";
        case 0xF4:return ","; case 0xF5:return "♀"; default:return "?";
    }
}

const char* glyphJP(uint8_t b) noexcept {
    // Standard Gen II Japanese table subset covering trainer/nickname kana, digits and punctuation.
    static const char* katakana[] = {"ア","イ","ウ","エ","オ","カ","キ","ク","ケ","コ","サ","シ","ス","セ","ソ","タ","チ","ツ","テ","ト","ナ","ニ","ヌ","ネ","ノ","ハ","ヒ","フ","ホ","マ","ミ","ム","メ","モ","ヤ","ユ","ヨ","ラ","ル","レ","ロ","ワ","ヲ","ン","ッ","ャ","ュ","ョ","ィ"};
    static const char* hiragana[] = {"あ","い","う","え","お","か","き","く","け","こ","さ","し","す","せ","そ","た","ち","つ","て","と","な","に","ぬ","ね","の","は","ひ","ふ","ほ","ま","み","む","め","も","や","ゆ","よ","ら","る","れ","ろ","わ","を","ん","っ","ゃ","ゅ","ょ"};
    if (b >= 0x80 && b <= 0xB0) return katakana[b-0x80];
    if (b >= 0xB1 && b <= 0xDF) return hiragana[b-0xB1];
    switch (b) {
        case 0x7F:return " "; case 0xE0:return "'"; case 0xE3:return "-"; case 0xE6:return "?"; case 0xE7:return "!";
        case 0xE8:return "."; case 0xE9:return "&"; case 0xEF:return "♂"; case 0xF5:return "♀";
        case 0xF6:return "0"; case 0xF7:return "1"; case 0xF8:return "2"; case 0xF9:return "3"; case 0xFA:return "4";
        case 0xFB:return "5"; case 0xFC:return "6"; case 0xFD:return "7"; case 0xFE:return "8"; case 0xFF:return "9";
        default:return "?";
    }
}

uint8_t unownForm(uint8_t atk, uint8_t def, uint8_t spe, uint8_t spc) noexcept {
    const uint16_t v = static_cast<uint16_t>(((atk&6)<<5)|((def&6)<<3)|((spe&6)<<1)|((spc&6)>>1));
    return static_cast<uint8_t>(v/10);
}
bool shinyDV(uint8_t atk, uint8_t def, uint8_t spe, uint8_t spc) noexcept {
    return def==10 && spe==10 && spc==10 && (atk==2||atk==3||atk==6||atk==7||atk==10||atk==11||atk==14||atk==15);
}

bool parsePokemon(std::span<const uint8_t> body, std::span<const uint8_t> ot, std::span<const uint8_t> nick,
                  RegionLayout region, bool party, bool egg, PokemonRecord& r) {
    const std::size_t need = party ? kPartyBody : kStoredBody;
    if (body.size() < need) return false;
    const uint8_t species = body[0];
    if (species == 0 || species > 251) return false;
    r.species=species; r.heldItem=body[1];
    std::copy_n(body.begin()+2,4,r.moves.begin());
    r.trainerId=readBE16(body,6); r.experience=readBE24(body,8);
    for (std::size_t i=0;i<5;++i) r.statExperience[i]=readBE16(body,0x0B+i*2);
    const uint16_t dv=readBE16(body,0x15);
    r.dvs[1]=(dv>>12)&0xF; r.dvs[2]=(dv>>8)&0xF; r.dvs[3]=(dv>>4)&0xF; r.dvs[4]=dv&0xF;
    r.dvs[0]=static_cast<uint8_t>(((r.dvs[1]&1)<<3)|((r.dvs[2]&1)<<2)|((r.dvs[3]&1)<<1)|(r.dvs[4]&1));
    for (std::size_t i=0;i<4;++i) { r.pp[i]=body[0x17+i]&0x3F; r.ppUps[i]=(body[0x17+i]>>6)&3; }
    r.friendship=body[0x1B]; r.pokerus=body[0x1C]; r.caughtData=readBE16(body,0x1D); r.level=body[0x1F];
    if (r.level>100) return false;
    r.originalTrainer=decodeGen2String(ot,region); r.nickname=decodeGen2String(nick,region);
    r.partyRecord=party; r.isEgg=egg; r.shiny=shinyDV(r.dvs[1],r.dvs[2],r.dvs[3],r.dvs[4]);
    r.gender=derivedGender(species,r.dvs[1]);
    r.form=species==201?unownForm(r.dvs[1],r.dvs[2],r.dvs[3],r.dvs[4]):0;
    r.rawBodySize=need; std::copy_n(body.begin(),need,r.rawBody.begin());
    if (party) {
        r.status=body[0x20]; r.currentHP=readBE16(body,0x22); r.maxHP=readBE16(body,0x24); r.attack=readBE16(body,0x26);
        r.defense=readBE16(body,0x28); r.speed=readBE16(body,0x2A); r.specialAttack=readBE16(body,0x2C); r.specialDefense=readBE16(body,0x2E);
    }
    return true;
}

bool parseList(std::span<const uint8_t> p, std::size_t o, std::size_t capacity, std::size_t bodySize,
               std::size_t stringLength, RegionLayout region, bool party,
               std::vector<std::optional<PokemonRecord>>& out) {
    if (!validListHeader(p,o,capacity)) return false;
    const std::size_t listLen=1+(capacity+1)+(bodySize*capacity)+(stringLength*capacity*2);
    if (o>p.size() || p.size()-o<listLen) return false;
    const uint8_t count=p[o]; const std::size_t bodyStart=o+1+(capacity+1);
    const std::size_t otStart=bodyStart+bodySize*capacity; const std::size_t nickStart=otStart+stringLength*capacity;
    out.assign(capacity,std::nullopt);
    for (std::size_t i=0;i<count;++i) {
        const uint8_t marker=p[o+1+i]; const bool egg=marker==0xFD;
        PokemonRecord rec;
        const auto body=p.subspan(bodyStart+i*bodySize,bodySize);
        if (!parsePokemon(body,p.subspan(otStart+i*stringLength,stringLength),p.subspan(nickStart+i*stringLength,stringLength),region,party,egg,rec)) return false;
        if (!egg && marker!=rec.species) return false;
        out[i]=std::move(rec);
    }
    return true;
}

const Layout* selectLayout(std::span<const uint8_t> p, SourceGame hint, SaveError& e, std::string& detail) {
    const VersionFamily wanted = hint==SourceGame::Crystal?VersionFamily::Crystal:VersionFamily::GoldSilver;
    const Layout* selected=nullptr;
    for (const auto& l:kLayouts) {
        if (!layoutFits(p,l)) continue;
        if (l.family!=wanted) continue;
        if (selected) { e=SaveError::AmbiguousLayout; detail="multiple supported Generation II layouts validate"; return nullptr; }
        selected=&l;
    }
    if (selected) return selected;
    for (const auto& l:kLayouts) if (layoutFits(p,l) && l.family!=wanted) {
        e=SaveError::GameHintMismatch; detail="Generation II save structure contradicts the requested GS/Crystal identity"; return nullptr;
    }
    e=SaveError::ChecksumMismatch;
    detail="no supported Generation II layout passed strict structure and both checksum-copy validations";
    return nullptr;
}

} // namespace

ParseResult parse(std::span<const uint8_t> raw, SourceGame hint) {
    ParseResult result; std::size_t payloadSize=0,footer=0;
    if (!payloadFor(raw,payloadSize,footer)) { result.error=SaveError::WrongSize; result.detail="unsupported Generation II battery-save size or RTC wrapper"; return result; }
    const auto payload=raw.first(payloadSize); SaveError error=SaveError::None; std::string detail;
    const Layout* l=selectLayout(payload,hint,error,detail); if (!l) { result.error=error; result.detail=std::move(detail); return result; }
    auto save=std::shared_ptr<ReadOnlySave>(new ReadOnlySave()); save->sourceBytes_.assign(raw.begin(),raw.end());
    save->metadata_.sourceGame=hint; save->metadata_.family=l->family; save->metadata_.region=l->region; save->metadata_.sourceGameId=sourceGameId(hint);
    save->metadata_.payloadSize=payloadSize; save->metadata_.rtcFooterSize=footer; save->metadata_.boxCount=l->boxCount; save->metadata_.boxCapacity=l->boxCapacity;
    save->metadata_.currentBox=payload[l->currentBoxIndex]&0x7F;
    save->trainer_.trainerId=readBE16(payload,l->trainer); save->trainer_.name=decodeGen2String(payload.subspan(l->trainer+2,l->trainerNameLength),l->region);
    save->trainer_.money=readBE24(payload,l->money); if (save->trainer_.money>999999) { result.error=SaveError::InvalidTrainerData; result.detail="Generation II money exceeds the game's stored range"; return result; }
    if (l->family==VersionFamily::Crystal) { const uint8_t g=payload[l->gender]; if (g>1) { result.error=SaveError::InvalidTrainerData; result.detail="Crystal trainer gender is out of range"; return result; } save->trainer_.gender=g; }
    std::vector<std::optional<PokemonRecord>> partySlots;
    if (!parseList(payload,l->party,kPartyCapacity,kPartyBody,l->stringLength,l->region,true,partySlots)) { result.error=SaveError::InvalidParty; result.detail="invalid Generation II party list"; return result; }
    for (auto& x:partySlots) if (x) {
        x->sourceGame=hint; x->region=l->region;
        save->party_.push_back(std::move(*x));
    }
    save->boxes_.resize(l->boxCount);
    for (std::size_t box=0;box<l->boxCount;++box) {
        const std::size_t bankBase=box<l->firstBankCount?0x4000:0x6000;
        const std::size_t bankIndex=box<l->firstBankCount?box:box-l->firstBankCount;
        const std::size_t offset=bankBase+bankIndex*l->boxSpacing;
        if (!parseList(payload,offset,l->boxCapacity,kStoredBody,l->stringLength,l->region,false,save->boxes_[box].slots)) { result.error=SaveError::InvalidBox; result.detail="invalid authoritative Generation II stored box"; return result; }
        for (auto& x:save->boxes_[box].slots) if (x) { x->sourceGame=hint; x->region=l->region; }
        save->boxes_[box].name=decodeGen2String(payload.subspan(l->boxNames+box*9,9),l->region);
        if (save->boxes_[box].name.empty()) save->boxes_[box].name="Box "+std::to_string(box+1);
    }
    result.save=std::move(save); return result;
}

const char* sourceGameId(SourceGame g) noexcept { switch(g){case SourceGame::Gold:return "gold_gbc";case SourceGame::Silver:return "silver_gbc";case SourceGame::Crystal:return "crystal_gbc";} return ""; }
const char* regionName(RegionLayout r) noexcept { return r==RegionLayout::Japanese?"Japanese":"International"; }
const char* familyName(VersionFamily f) noexcept { return f==VersionFamily::Crystal?"Crystal":"Gold/Silver"; }
const char* errorName(SaveError e) noexcept {
    switch(e){case SaveError::None:return "none";case SaveError::WrongSize:return "wrong-size";case SaveError::UnsupportedRegion:return "unsupported-region";case SaveError::ChecksumMismatch:return "checksum";case SaveError::AmbiguousLayout:return "ambiguous-layout";case SaveError::InvalidStructure:return "structure";case SaveError::InvalidTrainerData:return "trainer";case SaveError::InvalidParty:return "party";case SaveError::InvalidBox:return "box";case SaveError::GameHintMismatch:return "game-hint";} return "unknown";
}
uint16_t calculateChecksum(std::span<const uint8_t> p, RegionLayout r, VersionFamily f) noexcept {
    const Layout* l=nullptr; for(const auto& x:kLayouts) if(x.region==r&&x.family==f){l=&x;break;} if(!l||p.size()<=l->checksumEnd) return 0;
    uint16_t sum=0; for(std::size_t i=0x2009;i<=l->checksumEnd;++i) sum=static_cast<uint16_t>(sum+p[i]); return sum;
}
std::string decodeGen2String(std::span<const uint8_t> b, RegionLayout r) {
    std::string out; for(uint8_t v:b){ if(v==0||v==0x50) break; if(r==RegionLayout::International) out+=glyphEN(v); else out+=glyphJP(v); } return out;
}
bool isKnownRTCFooterSize(std::size_t n) noexcept { return n==7 || (n>=0x0C && n<=0x30 && (n%2)==0); }

} // namespace PokeVault::Integration::Gen2
