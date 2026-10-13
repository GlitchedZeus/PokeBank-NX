// SPDX-License-Identifier: AGPL-3.0-or-later
// Original English R/B and Yellow Celadon Game Corner price, vendor, and
// species+level identity. Data derived from pinned pret disassemblies:
// pret/pokered@af519899719f0754965776faac0e836a3b906e6d
//   data/events/prizes.asm [34cd75d753d02a417258a640c6002ec7980ea853]
//   data/events/prize_mon_levels.asm [eb30ac809ee64baaf36d649a7ebe8be6041aeb0c]
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
//   data/events/prizes.asm [9c8b0229c191f94654664f10e94455976157af52]
//   data/events/prize_mon_levels.asm [53a2a96685becabc6593da3a64cdd7b3debb9002]
// The original engine/events/prize_menu.asm awards the configured species
// at the level looked up in PrizeMonLevelDictionary on successful purchase.
// These are offers, not verified original redemption or save ownership.
inline constexpr std::array<PretGameCornerEntry, 18> kPretGameCornerOffers{{
    // gameMask, vendor (0/1), slot (0..2), species, level, coin cost
    {1,0,0, 63, 9,180}, {1,0,1, 35, 8,500},
    {1,0,2, 30,17,1200}, {1,1,0,147,18,2800},
    {1,1,1,123,25,5500}, {1,1,2,137,26,9999},
    {2,0,0, 63, 6,120}, {2,0,1,35,12,750},
    {2,0,2, 33,17,1200}, {2,1,0,127,20,2500},
    {2,1,1,147,24,4600}, {2,1,2,137,18,6500},
    {4,0,0, 63,15,230}, {4,0,1, 37,18,1000},
    {4,0,2, 40,22,2680}, {4,1,0,123,30,6500},
    {4,1,1,127,30,6500}, {4,1,2,137,26,9999}
}};
static_assert(kPretGameCornerOffers.size()==18);
