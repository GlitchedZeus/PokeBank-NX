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
// Additional version-specific original gift scene sources:
// pokered scripts/CeladonMansionRoofHouse.asm 12eff10c28799de278ee2cafa3f5ef743c98d166
// pokeyellow counterpart 330efe0c7801790f31569eb82c673face2a25d39
// pokered scripts/OaksLab.asm 19d0ec096c8b765fc5c067dff7381e3421fed07f
// pokeyellow scripts/OaksLab.asm b935d922bd6fd9223ded3a43d4a4fac8ef163c74
// pokeyellow scripts/CeruleanMelaniesHouse.asm 8b8095e012a6f6e38e2313907228a966ecd11c31
// pokeyellow scripts/Route24.asm 8e2931ba5cde850e53a3e7a7e265ff15462fc724
// pokeyellow scripts/VermilionCity_2.asm c6cd9478558dc212e88bb4a6a5339eb4ff154237
// Cinnabar fossil revival scripts: pokered e592814497bb479d85fde8b719eb3f47d201c0be
// pokeyellow daa14412c710598b13c7463d4246759c77cb5e78
// Version-specific / fossil receipt / event flag still independently required.
inline constexpr std::array<PretStaticEntry,20> kPretStaticSources{{
 {7,"Route12",0,143,30,PretStaticKind::OverworldBattle},
 {7,"Route16",0,143,30,PretStaticKind::OverworldBattle},
 {7,"PowerPlant",0,145,50,PretStaticKind::OverworldBattle},
 {7,"CeruleanCaveB1F",0,150,70,PretStaticKind::OverworldBattle},
 {7,"SeafoamIslandsB4F",0,144,50,PretStaticKind::OverworldBattle},
 {7,"VictoryRoad2F",0,146,50,PretStaticKind::OverworldBattle},
 {7,"SilphCo7F",0,131,15,PretStaticKind::Gift},
 {7,"FightingDojo",0,106,30,PretStaticKind::Gift},
 {7,"FightingDojo",1,107,30,PretStaticKind::Gift},
 {7,"CeladonMansionRoofHouse",0,133,25,PretStaticKind::Gift},
 {3,"OaksLab",0,1,5,PretStaticKind::Gift},
 {3,"OaksLab",1,4,5,PretStaticKind::Gift},
 {3,"OaksLab",2,7,5,PretStaticKind::Gift},
 {4,"OaksLab",3,25,5,PretStaticKind::Gift},
 {4,"CeruleanMelaniesHouse",0,1,10,PretStaticKind::Gift},
 {4,"Route24",0,4,10,PretStaticKind::Gift},
 {4,"VermilionCity",0,7,10,PretStaticKind::Gift},
 {7,"CinnabarLabFossilRoom",0,138,30,PretStaticKind::Gift},
 {7,"CinnabarLabFossilRoom",1,140,30,PretStaticKind::Gift},
 {7,"CinnabarLabFossilRoom",2,142,30,PretStaticKind::Gift},
}};
