// SPDX-License-Identifier: AGPL-3.0-or-later
// NPC TRADE OFFERS, not a full proof of completed original trade.
// English R/B data/events/trades.asm pret/pokered @ af519899719f0754965776faac0e836a3b906e6d
// blob 966d7de02124fb4975720446a28d84f5db4ab3a6
// Yellow data/events/trades.asm pret/pokeyellow @ e89ead154b9968aa50eed9328ff2b38b6c194382
// blob 8a75f425150dd002f3a8c50985bad13b2199e80b
// Original table index, requested and offered National species, and source
// reachability preserved. Four unused ROM rows must NOT certify encounters.
inline constexpr std::array<PretTradeEntry, 20> kPretTradeOffers{{
 {3,0,33,30,true,"TERRY"},
 {3,1,63,122,true,"MARCEL"},
 {3,2,12,15,false,"CHIKUCHIKU"},
 {3,3,77,86,true,"SAILOR"},
 {3,4,21,83,true,"DUX"},
 {3,5,80,108,true,"MARC"},
 {3,6,61,124,true,"LOLA"},
 {3,7,26,101,true,"DORIS"},
 {3,8,48,114,true,"CRINKLES"},
 {3,9,32,29,true,"SPOT"},
 {4,0,108,51,true,"GURIO"},
 {4,1,35,122,true,"MILES"},
 {4,2,12,15,false,"STINGER"},
 {4,3,115,89,true,"STICKY"},
 {4,4,151,151,false,"BART"},
 {4,5,114,47,true,"SPIKE"},
 {4,6,18,18,false,"MARTY"},
 {4,7,55,112,true,"BUFFY"},
 {4,8,58,87,true,"CEZANNE"},
 {4,9,104,67,true,"RICKY"},
}};
