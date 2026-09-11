#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>
#include <unistd.h>

#include "Legacy/RBYSourceBrowser.h"

namespace fs = std::filesystem;
using namespace PokeVault::Integration::Gen1;
using namespace PokeVault::Legacy;

namespace {
uint8_t diff8(std::span<const uint8_t> s){uint8_t v=0xFF;for(uint8_t b:s)v=static_cast<uint8_t>(v-b);return v;}
std::vector<uint8_t> fixture(){
    std::vector<uint8_t>d(0x8000,0);
    d[0x2598]=0x96;d[0x2599]=0x88;d[0x259A]=0x8B;d[0x259B]=0x8B;d[0x259C]=0x50; // WILL
    d[0x25F3]=0x12;d[0x25F4]=0x34;d[0x25F5]=0x56;d[0x2605]=0x12;d[0x2606]=0x34;
    d[0x29C3]=0x99;d[0x284C]=2;d[0x2F2C]=0;d[0x2F2D]=0xFF;d[0x30C0]=0;d[0x30C1]=0xFF;
    d[0x2ED5]=0xFF;d[0x3523]=diff8(std::span<const uint8_t>(d).subspan(0x2598,0x0F8B));return d;
}
void writeFile(const fs::path&p,const std::vector<uint8_t>&d){fs::create_directories(p.parent_path());std::ofstream o(p,std::ios::binary);assert(o.good());o.write(reinterpret_cast<const char*>(d.data()),d.size());assert(o.good());}
}

int main(){
    const fs::path root=fs::temp_directory_path()/("pokebank-rby-browser-"+std::to_string(static_cast<unsigned long long>(::getpid())));
    fs::remove_all(root);fs::create_directories(root);writeFile(root/"Pokemon Red.srm",fixture());
    const std::array<std::string,1> roots{{root.string()}};auto discovery=discoverRBYSaves(roots);auto cards=buildRBYSourceCards(discovery);
    assert(cards.size()==1);const auto&card=cards[0];assert(card.gameId=="red_gb");assert(card.title=="Red");assert(card.platformLabel=="Game Boy");assert(card.sourceLabel=="RETROARCH");assert(card.artworkKey=="red_gb");assert(card.instances.size()==1);
    const auto&instance=card.instances[0];assert(instance.kind==LegacySaveInstanceKind::BatterySave);assert(instance.trainerName=="WILL");assert(instance.partyCount==0);assert(instance.sourceIdentity.size()==64);
    const RBYSource* resolved=resolveRBYSaveInstance(discovery,card,0);assert(resolved&&resolved->ready()&&resolved->gameId=="red_gb");
    LegacySourceBindings bindings;assert(bindings.assign(instance.sourceIdentity,"profile-A"));
    auto visible=buildRBYSourceCardsForProfile(discovery,bindings,"profile-A");assert(visible.size()==1&&visible[0].instances.size()==1);
    auto hidden=buildRBYSourceCardsForProfile(discovery,bindings,"profile-B");assert(hidden.empty());
    fs::remove_all(root);std::cout<<"RBY Game Sources card/binding tests passed\n";return 0;
}
