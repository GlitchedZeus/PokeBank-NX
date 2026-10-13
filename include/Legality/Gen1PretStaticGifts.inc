// SPDX-License-Identifier: AGPL-3.0-or-later
// Source-attested specific ORIGINAL Gen I encounter scenes:
// pret/pokered af519899719f0754965776faac0e836a3b906e6d
// pret/pokeyellow e89ead154b9968aa50eed9328ff2b38b6c194382
// scripts/Route12.asm c658ce11573dd35e864bb37614bb00463dd920fd
// scripts/Route16.asm 712ede16c6e00315365e7e32b2f6075346a12878
// data/maps/objects/PowerPlant.asm dcabba03b20d933e45ae76f8358d44fab439900e
// data/maps/objects/CeruleanCaveB1F.asm (pokered) ef29c2e7255059aa9fed988a53692e13aeca8505
// data/maps/objects/SeafoamIslandsB4F.asm b80685fe2cdfbaa73c4fa50ef45c9fce90cb75d6
// data/maps/objects/VictoryRoad2F.asm c4de36839535066f21f6f5c79045f8a40381fc61
// scripts/SilphCo7F.asm (pokered) 0d877ea965fb58c7de10dd768f7375ef01e45c69
// scripts/FightingDojo.asm 4df46ecaab22940955bf2e93e79a90a4b351cc00
// No encounter RNG, event flags, original Trainer or native catch-rate proof.
inline constexpr std::array<PretStaticEntry,9> kPretStaticSources{{
 {7,"Route12",0,143,30,PretStaticKind::OverworldBattle},
 {7,"Route16",0,143,30,PretStaticKind::OverworldBattle},
 {7,"PowerPlant",0,145,50,PretStaticKind::OverworldBattle},
 {7,"CeruleanCaveB1F",0,150,70,PretStaticKind::OverworldBattle},
 {7,"SeafoamIslandsB4F",0,144,50,PretStaticKind::OverworldBattle},
 {7,"VictoryRoad2F",0,146,50,PretStaticKind::OverworldBattle},
 {7,"SilphCo7F",0,131,15,PretStaticKind::Gift},
 {7,"FightingDojo",0,106,30,PretStaticKind::Gift},
 {7,"FightingDojo",1,107,30,PretStaticKind::Gift},
}};
