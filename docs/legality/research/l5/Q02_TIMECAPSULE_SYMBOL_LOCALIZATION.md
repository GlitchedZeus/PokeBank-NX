# Q02 follow-up: Crystal Time Capsule source-symbol localization

The frozen #252 source registry includes `C-TIMECAPSULE` with an **empty `symbol` field**. L5 does **not** edit that pinned historical registry. This is a supplementary source-grounded symbol record, not a licensing permission or new legality rule.

- **Original game:** Pokémon Crystal (Gen II retail decompilation).
- **Pinned source:** [`pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6` `engine/link/time_capsule.asm`](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/link/time_capsule.asm#L3-L67).
- **Recorded whole-file SHA-256:** `bb5fa2bdba1260aa7e17cd40887cf246a7c05400495ad6490ef1001237b50e1f`; revalidated at the pinned source revision in `Q02_REMOTE_HASH_BATCH_03.json`.
- **Git blob SHA:** `fe31d7f3fcddc7d39c4b8f90df8e1abc89f41edb`.
- **Verified symbols and line anchors:** `ValidateOTTrademon` (line 3), `CheckAnyOtherAliveMonsForTrade` (line 67), `PlaceTradePartnerNamesAndParty` (line 103).

## Exact source evidence and limitations
The first routine checks link-trade candidate species and level, and when `wLinkMode == LINK_TIMECAPSULE` it obtains species type data with a special-case path for Magnemite and Magneton (lines 29–57). Its `.abnormal` branch sets carry (lines 63–65). The second routine requires another living Pokémon or a living partner (lines 67–102). Both are **bounded checks**, not an exhaustive Gen I ↔ Gen II trade/history authorization, move species ban, event authenticity or original historical proof.

This resolves the **symbol location gap** for the one blank registry symbol without modifying or pretending the original frozen entry had a symbol. The rest of Q02 remains a pin-integrity audit (79/79 SHA-256 and 78 originally configured symbols, one separately localized). No hard Invalid conclusion follows from this source snippet alone.

**Research status:** source symbol localized, historical provenance remains Unknown when detached original trade history cannot be reconstructed.
