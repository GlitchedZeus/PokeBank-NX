# L4 Stage 1 — original Gen I/II gift-history witness

Scope: candidate-local static gifts, NPC trades, Stadium gifts and PCNY-style distribution hypotheses for R/B/Y/G/S/C. No original event payloads, ROM bytes, upstream tables, or receipt data are distributed here. This tranche contains **no named event template authentication**.

## Independently inspectable source mechanics

- `pret/pokered` [`engine/events/in_game_trades.asm`](https://github.com/pret/pokered/blob/master/engine/events/in_game_trades.asm): NPC trade copies received nickname, original-trainer data, and sets the trade level from the offered party Pokémon rather than a globally fixed gift level. The individual `data/events/trades.asm` row still needs a version-specific audit before use.
- `pret/pokecrystal` [`engine/events/npc_trade.asm`](https://github.com/pret/pokecrystal/blob/master/engine/events/npc_trade.asm): original OT ID/name and nickname are attached to NPC-trade results; the selected offered party Pokémon supplies the level (`DoNPCTrade` around the `wPartyMon1Level` read). Exact rows are version-specific and not embedded here.
- `pret/pokecrystal` [`engine/link/time_capsule.asm`](https://github.com/pret/pokecrystal/blob/master/engine/link/time_capsule.asm) and [`engine/link/time_capsule_2.asm`](https://github.com/pret/pokecrystal/blob/master/engine/link/time_capsule_2.asm): original-generation trade constraints and species-index projection. Time Capsule constraints apply at the **actual crossing**, not to moves learned subsequently; the test uses generation-one species/move ID boundaries (151/165) only as a candidate-local compatibility check.
- `pret/pokecrystal` [`engine/pokemon/move_mon.asm`](https://github.com/pret/pokecrystal/blob/master/engine/pokemon/move_mon.asm): Pokémon stores game-font original-trainer bytes, original trainer ID and current move slots, which need separate source/history evidence. Native snapshots do not reveal every previous move overwrite or evolution.

`pret` disassemblies are comparison references, **not** event receipts, proof of exact historical release availability, or a license to copy their source/binary material. This work reimplements only small independent constraints; do not bulk-import any upstream data without a rights audit.

## Source-to-record contract

`Gen12GiftHistoryEvidence.h` accepts independently reviewed game-specific source rows and exact native observations. Unattested source identity, impossible native source records, missing receipt for a distribution, unverified evolution endpoints, missing trade input level, uncertain nicknames, unspecified four-move history, and undocumented Time Capsule crossing all remain **Unknown**. Mismatches are candidate-local only. Validated matching is a **conditional witness**, not proof of real distribution or a globally legal Pokémon.

The `LevelRule::OfferedTradeLevel` input is deliberately distinct from `LevelRule::Fixed`; an NPC trade cannot silently be treated as a fixed-level gift. Level comparisons need a verified original offered level. For Gen I↔II crossings, use a verified species/four-move snapshot **at the crossing**. A current Gen II-only move is not evidence that a past Gen I crossing was impossible. A single broken source route must never cause global `Invalid` without independently exhausting all alternatives.

## Local validation / integration boundary

`tests/test_legality_l4_gen12_gift_history.cpp` exercises synthetic source rows only (not genuine distributions), source proof gating, original trainer mismatches, movable nickname and move history, independently witnessed evolution endpoints, dynamic NPC levels, exact-crossing snapshots, and 64,009 species×move boundary pairs. These tests run as a standalone header-only C++20 translation unit with `-Wall -Wextra -Wpedantic -Werror`. The L4 runner includes it. Do not claim that the rest of the L4 suite or the production build was verified solely because this new test runs.

**Coordinator handoff:** Incorporate as **informational candidate evidence only** after source-specific row attestations; no production `Legality.cpp`, acceptance branch or PR #298 changes are made in this lane. Authentic original Stadium/PCNY distributions and individual receipt records remain unresolved.