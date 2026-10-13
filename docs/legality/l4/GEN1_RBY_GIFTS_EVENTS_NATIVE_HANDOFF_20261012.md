# L4 — Gen I Red/Blue/Yellow event, gift and native metadata handoff

Date: 2026-10-12. Repository: `GlitchedZeus/PokeBank-NX`.
PR #292 only, `feature/legality-l4-events-meta-20261012`.
This is **L4 source and read-only candidate-evidence** work, not a new
L1 wild-encounter table or coordinator production-analyzer change.

## Substantial engineering

- New `Gen1OriginalNPCTradeMetadata.h` separates actual English
  Red/Blue NPC exchange-script species/row reachability from Yellow's
  DIFFERENT script rows. Preserves all 20 documented ROM rows (16
  reachable, 4 unused), including unused Yellow MEW for MEW. No
  gift-archive binaries, game-font names or ROM scripts were bundled.
- New `Gen1OriginalGiftNativePreflight.h` constrains a validated native
  PK1 to 1..151 species, 1..100 level, Gen I 0..165 move IDs, 0..15
  DVs, 0..3 PP Ups. It distinguishes independently documented original
  catch-rate bytes from independently witnessed Time Capsule tradeback
  held-item bytes; a single catch-rate byte is NEVER original-source
  or cross-generation receipt proof. This neither duplicates L2 stat
  mechanics nor invents Gen I abilities, ribbons, genders or nature.
- New `Gen1OriginalGiftNativeEvidence.h` is a read-only template over
  the already validated Gen I `PokemonRecord`, reusing L4-owned
  `Gen12OriginalDistributionJoinEvidence` and
  `Gen12GiftHistoryEvidence`. It handles version-specific gifts,
  original Stadium media hypotheses, original NPC offered/received
  species and offered-trade-level history, script reachability,
  per-individual generated NPC trainer IDs, independent original
  raw-name encoding/snapshot, original/converted event source digests,
  cross-edition transfers, and Time Capsule snapshots.
- The original English ROM NPC OTID is generated at the trade (the
  original script calls `Random` for that record); it is not a
  fixed `TradeMons` lookup. It is compared only after independent
  proof of that individual's original receipt. Raw modern decoded
  `PokemonRecord.originalTrainer`/`nickname` strings NEVER substitute
  for source-linked game-font bytes.
- Modern event-listing claims, unsupported/unused original rows,
  unlinked raw names, missing save/cartridge version or receipt,
  unpinned distribution archive, and unwitnessed catch-rate conversion
  return Unknown. Contradictions of independently pinned hypothetical
  candidates are NOT global Invalid or original acquisition proof.
  Original source saves are read-only.

## Independently re-fetched original evidence

Pinned GitHub source revisions and immutable Git blobs:

- `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`,
  `data/events/trades.asm`, blob
  `966d7de02124fb4975720446a28d84f5db4ab3a6`;
  `engine/events/in_game_trades.asm`, blob
  `3107550fcd2d059a1786e8e1a68709b2c20b3377`.
- `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`,
  `data/events/trades.asm`, blob
  `8a75f425150dd002f3a8c50985bad13b2199e80b`;
  `engine/events/in_game_trades.asm`, blob
  `8cde05c078dc90e6b5d032493e67c4b59fa26d1a`.
- Exact existing L4 join:
  `Gen12OriginalDistributionJoinEvidence.h`,
  blob `adb2eb6cc6c42831754c2255ce70b4a1a3146b8c`.
  Native catch-rate classification:
  `Gen1CatchRateEvidence.h`,
  blob `0986f52ed15cbfcc7bfe6a0a13c6d2c7eef9fd48`.
  Strict native `Gen1ReadOnlySave.h`,
  blob `d52f60cd861b48cab4d577dd702607c72e970a15`.

These pinned public disassemblies are documentary original
**mechanic references**, NOT independently authenticated gift media
or an individual player's receipt. English metadata is not silently
promoted to Japanese-source rows or arbitrary ROM variants.

## Precisely executed local tests (no extrapolation)

Two genuinely standalone **exact committed-source** suites:

- `legality_l4_gen1_preflight.cpp` tests **65,536** original-byte
  vs tradeback-item combinations, **4,096** raw move/DV state checks,
  full PP Up boundary and 32 proof masks.
- `legality_l4_gen1_npc_metadata.cpp` tests **65,536**
  raw edition/row tags and all documented unused/usable English rows.

Both were built and executed with GCC and Clang C++20, each normal
`-O2` and ASan+UBSan `-O1 -g`, with
`-Wall -Wextra -Wpedantic -Werror`.
**8/8 focused executions PASS; no sanitizer finding.**
Their local `git hash-object` SHA-1s match exact committed Git blobs:

- `Gen1OriginalGiftNativePreflight.h`:
  `b06050094f4c57fc0b2356c396f2e04c02c0ba52`.
- `Gen1OriginalNPCTradeMetadata.h`:
  `da8f0e7cc2c703da7db47121c8dbba8840f6356b`.
- Preflight fixture `6aae7084f7b279b398e718369de525cdfe1ff007`.
- NPC fixture `3896844c94376766cd5d3f0c07cb94c1e766217e`.

The third **native integration regression was authored, committed and
registered but NOT executed locally**. It uses the actual read-only
Gen I `PokemonRecord` and existing L4 archive/history headers,
tests synthetic positive R/B gift, Yellow NPC trade, Stadium gift,
unused Yellow Mew trade, malformed native fields, source/charset
degradation, independently attested catch rate and 131,072 proof masks.
Committed blobs:

- `Gen1OriginalGiftNativeEvidence.h`:
  `b43e80b16bf3a24dbfd8ce06758ae0892ad6007d`.
- `legality_l4_gen1_original_native.cpp`:
  `de1cb028b61154ffd6e19c7908a5e5a0fb2a6d31`.

Both fixture types are appended to the existing L4 runner and the PR
workflow paths now cover new Gen I source headers. The existing
28 registered L4 C++ source programs were **not renamed or reduced**.

## Exact remaining acceptance blockers / gaps

The request's full-run commands from an actual checkout:

```bash
L4_EXPECTED_HEAD="$(git rev-parse HEAD)" bash tests/test_legality_l4_runner.sh
L4_EXPECTED_HEAD="$(git rev-parse HEAD)" L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
# Repeat both using CXX=clang++.
```

were NOT executable in this environment: `git ls-remote
https://github.com/GlitchedZeus/PokeBank-NX.git HEAD` failed
`Could not resolve host: github.com`, and there is no full
checkout under `/mnt/data`. Therefore no claim that the 28-suite
full tree, native integrated fixture, four-way GitHub CI, or host
integration tests passed. Treat acceptance as **BLOCKED**.

Historical gaps (Unknown, never fabricated): original individual
Stadium gift media and archive per-record SHA, distribution release
and exact ownership; the player's saved original NPC trade receipt
including randomized OT ID and level of offered species; game-font
raw-name and original save version/region provenance; complete
evolution, move edits and Time Capsule dates/held-item history;
Japanese vs English original ROM row parity and imported distribution
license/rights. A modern species/name match is not a distribution
receipt. L1 owns wild/fishing/static encounter slot enumeration and
L3 owns joint move-history legality; L4 must not duplicate those.
No global `Legality::analyze` severity changes, emulator writes,
coordinator merge, accepted branch or MAIN edits occurred.

**Engineering implementation and standalone tested source: DELIVERED.
Full original-repository native acceptance: BLOCKED.**
