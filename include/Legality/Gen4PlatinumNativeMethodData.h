#pragma once
// L3 Stage 4: original Platinum native level-up, TM/HM and tutor methods.
// Source: pret/pokeplatinum@fe1ea418a7364c93daf0d07ae55e195452d8803c, res/pokemon/<species>/data.json,
// original move IDs: pret/pokeruby@63a8cbf for 1..354 and
// veekun/pokedex canonical historical Gen IV numeric ID mapping.
// Twelve exact National species, Deoxys normal form only. D/P/HGSS Unknown.
// Source compatibility never proves an actual teaching history.
#include <array>
#include <cstdint>
namespace Legality::Gen4PlatinumNativeMethodData {
struct Row {uint16_t species,offerStart,offerCount,tutorStart,tutorCount; uint64_t tmLow,tmHigh;};
inline constexpr std::array<Row,12> kRows{{
    {1,0,14,0,6,0x2101e0884350720ULL,0x29002662420ULL},
    {2,14,16,6,6,0x2101e0884350720ULL,0x29002662420ULL},
    {3,30,18,12,7,0x2101e0886354730ULL,0xa9002662428ULL},
    {4,48,10,19,7,0x12821ee4cc510623ULL,0x2900246a401ULL},
    {5,58,11,26,7,0x12821ee4cc510623ULL,0x2900246a401ULL},
    {6,69,18,33,11,0x168e5ee4ce714633ULL,0x3b00246a409ULL},
    {7,87,14,44,9,0x2c01e40cc533265ULL,0x6c002462200ULL},
    {8,101,16,53,9,0x2c01e40cc533265ULL,0x6c002462200ULL},
    {9,117,18,62,11,0x2c81e40ce537275ULL,0xec00646a288ULL},
    {92,135,14,73,11,0xd211bf08b4970e20ULL,0xa567002ULL},
    {133,149,11,84,5,0x2001e00ac530620ULL,0x2462000ULL},
    {386,160,14,89,11,0xb998fc3f5bbde2dULL,0x2900e7e99acULL}
}};
inline constexpr std::array<uint16_t,174> kOffers{
    545, 1581, 3657, 4630, 6733, 6735, 7716, 9803, 10982, 12874, 13862, 16260, 17131, 19346, 545, 557, 
    585, 1581, 3657, 4630, 6733, 6735, 7716, 10315, 12006, 14410, 15910, 18820, 20203, 22604, 545, 557, 
    585, 534, 1581, 3657, 4630, 6733, 6735, 7716, 10315, 12006, 14410, 15910, 16464, 20356, 23275, 27212, 
    522, 557, 3636, 5228, 8274, 9912, 13224, 14499, 17461, 19027, 522, 557, 564, 3636, 5228, 8786, 
    10936, 14760, 16547, 20021, 22099, 849, 933, 915, 522, 557, 564, 620, 3636, 5228, 8786, 10936, 
    14760, 16547, 18449, 21557, 25171, 30465, 34186, 545, 2087, 3729, 5230, 6711, 8236, 9957, 11446, 13152, 
    14737, 16002, 17742, 19184, 20536, 545, 551, 657, 2087, 3729, 5230, 6711, 8236, 10469, 12470, 14688, 
    16785, 18562, 20814, 22768, 24632, 942, 545, 551, 657, 622, 2087, 3729, 5230, 6711, 8236, 10469, 
    12470, 14688, 16785, 20098, 23886, 27376, 30776, 607, 634, 2740, 4308, 6318, 7781, 9837, 11653, 13683, 
    15095, 17034, 18831, 20674, 22187, 551, 545, 782, 4124, 7725, 11362, 14892, 18658, 22052, 25987, 29560, 
    555, 547, 4709, 8804, 13082, 17124, 21086, 25377, 29559, 33708, 37698, 41577, 45922, 49727
};
inline constexpr std::array<uint16_t,100> kTutors{
    189, 210, 173, 235, 402, 282, 189, 210, 173, 235, 402, 282, 189, 210, 173, 200, 
    235, 402, 282, 189, 210, 9, 7, 173, 257, 129, 189, 210, 9, 7, 173, 257, 
    129, 189, 210, 9, 7, 466, 173, 314, 200, 239, 257, 129, 291, 189, 196, 205, 
    8, 401, 173, 428, 334, 291, 189, 196, 205, 8, 401, 173, 428, 334, 291, 189, 
    196, 205, 8, 401, 173, 200, 324, 428, 334, 196, 9, 7, 8, 466, 173, 180, 
    271, 282, 389, 253, 189, 173, 270, 387, 129, 189, 196, 9, 7, 8, 173, 324, 
    428, 271, 282, 129
};
constexpr const Row* row(uint16_t species) noexcept {
    for(const auto& r:kRows)if(r.species==species)return &r;
    return nullptr;
}
constexpr uint16_t offerCount(uint16_t species) noexcept {
    const auto* r=row(species);return r?r->offerCount:0;
}
constexpr uint16_t packedOffer(uint16_t species,uint16_t index) noexcept {
    const auto* r=row(species);
    return r && index<r->offerCount?kOffers[r->offerStart+index]:0;
}
constexpr bool machineCompatible(uint16_t species,bool hidden,uint8_t number) noexcept {
    const auto* r=row(species);
    if(!r || number==0 || number>(hidden?8:92))return false;
    const uint8_t bit=hidden?static_cast<uint8_t>(99+number):number-1;
    return bit<64 ? ((r->tmLow>>(bit))&1u)!=0:
                    ((r->tmHigh>>(bit-64))&1u)!=0;
}
constexpr bool tutorCompatible(uint16_t species,uint16_t move) noexcept {
    const auto* r=row(species);if(!r)return false;
    for(uint16_t i=0;i<r->tutorCount;++i)
        if(kTutors[r->tutorStart+i]==move)return true;
    return false;
}
static_assert(offerCount(1)==14 && offerCount(6)==18);
static_assert((packedOffer(1,0)>>9)==1 && (packedOffer(1,0)&511u)==33);
static_assert(machineCompatible(1,false,6));
static_assert(!machineCompatible(1,true,2));
static_assert(row(386)!=nullptr && row(252)==nullptr);
} // namespace Legality::Gen4PlatinumNativeMethodData
