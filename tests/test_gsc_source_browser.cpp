#include "Legacy/GSCSourceBrowser.h"

#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault;

namespace {
struct L {
    std::size_t money, idx, names, party, current, end, sum;
    uint8_t cap, boxes, str, split;
};
constexpr L IGS{0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,20,14,11,7};
constexpr L IC {0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,20,14,11,7};

std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(const L& l){return listLen(l.cap,32,l.str)+2;}
std::size_t boxStart(const L& l,std::size_t b){return b<l.split?0x4000+b*stride(l):0x6000+(b-l.split)*stride(l);}
void emptyList(std::vector<uint8_t>& d,std::size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void text(std::vector<uint8_t>& d,std::size_t ofs,std::size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),static_cast<std::ptrdiff_t>(len),0x50);
    d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,std::size_t ofs,std::size_t cap,std::size_t bodySize,std::size_t str){
    constexpr uint8_t species=25;
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const std::size_t body=ofs+1+(cap+1);
    d[body]=species;d[body+6]=0x12;d[body+7]=0x34;d[body+21]=0x7A;d[body+22]=0xAA;d[body+31]=15;
    const std::size_t ot=body+cap*bodySize;const std::size_t nick=ot+cap*str;
    text(d,ot,str,'O');text(d,nick,str,'M');
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l){
    std::vector<uint8_t>d(0x8000);
    d[0x2009]=0x12;d[0x200A]=0x34;text(d,0x200B,l.str,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;d[l.idx]=0;
    for(std::size_t b=0;b<l.boxes;b++){emptyList(d,boxStart(l,b));text(d,l.names+b*9,9,static_cast<char>('A'+(b%26)));}
    pokemonList(d,boxStart(l,0),l.cap,32,l.str);emptyList(d,l.current);pokemonList(d,l.party,6,48,l.str);
    checksum(d,l);return d;
}

Legacy::GSCSource sourceFor(Integration::Gen2::SourceGame game, const L& layout,
                            std::string path, std::string identity, std::string canonical,
                            std::string fingerprint, int64_t modified) {
    auto bytes=fixture(layout);
    auto parsed=Integration::Gen2::parse(bytes,game);
    assert(parsed);
    Legacy::GSCSource source;
    source.path=std::move(path);
    source.normalizedPath=source.path;
    source.sourceIdentity=std::move(identity);
    source.canonicalPath=std::move(canonical);
    source.fileSize=bytes.size();
    source.modifiedTime=modified;
    source.contentFingerprint=std::move(fingerprint);
    source.gameId=Integration::Gen2::sourceGameId(game);
    source.status=Legacy::GSCSourceStatus::Ready;
    source.save=std::move(parsed.save);
    return source;
}

const Legacy::FRLGSourceCard* card(const std::vector<Legacy::FRLGSourceCard>& cards,std::string_view id){
    const auto it=std::find_if(cards.begin(),cards.end(),[&](const auto& c){return c.gameId==id;});
    return it==cards.end()?nullptr:&*it;
}
}

int main(){
    Legacy::GSCDiscoveryResult discovery;
    discovery.sources.push_back(sourceFor(Integration::Gen2::SourceGame::Gold,IGS,
        "/saves/Pokemon Gold.srm","source-gold","inode:1:1",std::string(64,'a'),100));
    discovery.sources.push_back(sourceFor(Integration::Gen2::SourceGame::Silver,IGS,
        "/saves/Pokemon Silver.srm","source-silver","inode:1:2",std::string(64,'b'),200));
    discovery.sources.push_back(sourceFor(Integration::Gen2::SourceGame::Crystal,IC,
        "/saves/Pokemon Crystal.srm","source-crystal","inode:1:3",std::string(64,'c'),300));
    // Same physical Gold save through a duplicate catalog path must not duplicate the child.
    auto duplicate=sourceFor(Integration::Gen2::SourceGame::Gold,IGS,
        "/alias/Pokemon Gold.srm","source-gold-alias","inode:1:1",std::string(64,'a'),90);
    discovery.sources.push_back(std::move(duplicate));
    Legacy::GSCSource invalid;
    invalid.gameId="gold_gbc";
    invalid.status=Legacy::GSCSourceStatus::InvalidSave;
    discovery.sources.push_back(std::move(invalid));

    const auto cards=Legacy::buildGSCSourceCards(discovery);
    assert(cards.size()==3);
    const auto* gold=card(cards,"gold_gbc");
    const auto* silver=card(cards,"silver_gbc");
    const auto* crystal=card(cards,"crystal_gbc");
    assert(gold&&silver&&crystal);
    assert(gold->title=="Gold"&&silver->title=="Silver"&&crystal->title=="Crystal");
    assert(gold->platformLabel=="GBC"&&gold->sourceLabel=="RETROARCH");
    assert(gold->instances.size()==1&&silver->instances.size()==1&&crystal->instances.size()==1);
    assert(gold->instances[0].kind==Legacy::LegacySaveInstanceKind::BatterySave);
    assert(gold->instances[0].sourceIdentity=="source-gold");
    assert(gold->instances[0].partyCount==1);
    assert(gold->instances[0].mostRecentlyModified);
    assert(Legacy::resolveGSCSaveInstance(discovery,*gold,0)==&discovery.sources[0]);

    auto stale=*gold;
    stale.instances[0].contentFingerprint="stale";
    assert(Legacy::resolveGSCSaveInstance(discovery,stale,0)==nullptr);
    auto wrong=*gold;
    wrong.gameId="silver_gbc";
    assert(Legacy::resolveGSCSaveInstance(discovery,wrong,0)==nullptr);

    Legacy::LegacySourceBindings bindings;
    assert(bindings.assign("source-gold","profile-a"));
    assert(bindings.assign("source-crystal","profile-a"));
    assert(bindings.assign("source-silver","profile-b"));
    const auto profileA=Legacy::buildGSCSourceCardsForProfile(discovery,bindings,"profile-a");
    assert(profileA.size()==2);
    assert(card(profileA,"gold_gbc")&&card(profileA,"crystal_gbc")&&!card(profileA,"silver_gbc"));
    const auto profileB=Legacy::buildGSCSourceCardsForProfile(discovery,bindings,"profile-b");
    assert(profileB.size()==1&&card(profileB,"silver_gbc"));
    const auto nobody=Legacy::buildGSCSourceCardsForProfile(discovery,bindings,"profile-c");
    assert(nobody.empty());

    std::cout<<"Generation II GSC source browser: PASS\n";
}
