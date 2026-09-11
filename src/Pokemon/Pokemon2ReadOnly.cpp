#include "Pokemon/Pokemon2ReadOnly.h"
#include "Pokemon/BaseStatsGen89.h"
#include "Utils/StringHelpers.h"
namespace Pokemon {
Pokemon2ReadOnly::Pokemon2ReadOnly(const PokeVault::Integration::Gen2::PokemonRecord& record,
                                   PokeVault::Integration::Gen2::SourceGame sourceGame,
                                   PokeVault::Integration::Gen2::RegionLayout region)
    : record_(record), sourceGame_(sourceGame), region_(region) {
    dataSize=record_.rawBodySize;
    if(dataSize==0||dataSize>record_.rawBody.size()){dataSize=0;data={};return;}
    buffer=new std::byte[dataSize];
    for(size_t i=0;i<dataSize;++i) buffer[i]=static_cast<std::byte>(record_.rawBody[i]);
    data=std::span<std::byte>(buffer,dataSize);
    derivedGender_=PokeVault::Integration::Gen2::derivedGender(record_.species,record_.dvs[1]);
}
const char* Pokemon2ReadOnly::species() const noexcept { return getSpeciesNameGen89(speciesID()); }
std::u16string Pokemon2ReadOnly::nickname() const { return Utils::utf8ToUtf16(record_.nickname); }
std::u16string Pokemon2ReadOnly::otName() const { return Utils::utf8ToUtf16(record_.originalTrainer); }
} // namespace Pokemon
