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


## Gen I continuation — stronger provenance boundary (October 12, 2026)

This continuation advances the source-bound evidence, not the accepted
global legality analyzer. Final tested source state before this handoff:
`6190f2f704f3ade16b4fe2aa8f3f1b73502d6596`.

1. **Original acquisition and record continuity:** The native join
   now requires two separate independent claims:
   `originalAcquisitionContextIndependentlyVerified` and
   `originalIndividualRecordLinkedToNativeIndependentlyVerified`.
   A plausible ROM gift row or modern event listing without the exact
   individual's acquisition and source-to-record history returns Unknown
   before it may compare an OT ID or announce a candidate contradiction.
2. **Pin original English NPC nicknames to original scripts:** All
   twenty original English Red/Blue and Yellow NPC trade rows now carry
   their script nickname. A bounded `encodeEnglishNickname` constructs
   the original 11-byte international game-font nickname (A–Z codes
   0x80..0x99, terminated and padded with 0x50). Only independently
   attested correct-encoding rows can be compared to a raw native name.
   Malformed, unrecognised locale, or a mismatched claimed original
   nickname remain UnknownOriginalNPCTrade—not a positive provenance
   or an invented global Invalid. The original trainer's randomized
   OT ID still requires a separate individual receipt.
3. **Time Capsule return items:** The standalone native catch-rate
   evaluator now requires an independently verified Gen II held-item
   interpretation as well as the crossing snapshot, source receipt and
   returned item witness. The native join checks the byte using the
   existing PKHeX-sourced `Gen1CatchRate::isPossibleTimeCapsuleHeldItem`
   table. Unknown transformations do not become falsely conclusive.
4. **Independent Stadium media attestation:** Original Stadium gifts
   use a dedicated source-media mechanism witness. Verifying a Red/
   Blue/Yellow cartridge script alone is not proof of a separately
   distributed Stadium gift. This boundary is source-gated and has
   focused regression coverage.

### Exact tested source Git object hashes

- `Gen1OriginalNPCTradeMetadata.h`:
  `7793e87fa6e23dcb324bcd1ce170d8f8eef7256c`
- `Gen1OriginalGiftNativePreflight.h`:
  `e33a3ee6d6f9c8474bde625acaf63bb889cf3828`
- `Gen1OriginalGiftNativeEvidence.h`:
  `21ab6529e148a1aa34c7ec1fd8e2211778c0792d`
- `legality_l4_gen1_preflight.cpp`:
  `6f93dbab514d159b7cfb51b1c298f7de4cdef704`
- `legality_l4_gen1_npc_metadata.cpp`:
  `5c92503757cff0b9197dafda44a322fb82564160`
- `legality_l4_gen1_original_native.cpp`:
  `c433370e3b81bd75717b8ccdbd140a654f9e0e2d`

The **two self-contained exact-source fixtures executed 8/8**:
GCC and Clang normal -O2, GCC and Clang -O1 ASan/UBSan, strict
C++20 and -Werror, no finding. Original catch-rate byte and tradeback
test: 65,536 byte comparisons plus 4,096 native move/DV checks.
Original NPC test: 65,536 edition/index tags, 16 reachable English
trades and 4 unused English script rows, original script nicknames,
encoded raw name boundaries. Source mechanism tests explicitly reject
using cartridge-script proof in place of Stadium-media proof.

**Not executed:** `legality_l4_gen1_original_native.cpp` now includes
524,288 independent evidence-gate combinations plus missing acquisition,
unsupported nickname, cross-edition, Stadium and tradeback negatives.
It remains registered in the L4 runner but cannot be claimed green.

Both requested full commands were actually attempted in the isolated
folder and yielded `bash: tests/test_legality_l4_runner.sh:
No such file or directory`. This runtime has no complete checkout;
`git ls-remote` fails DNS. No 28-suite L4, full ASan/UBSan,
native integration or GitHub Actions check may be marked passed based
on those failures. Re-fetch this exact PR HEAD and run from the
actual full repository with both GCC and Clang before coordinator
acceptance; do not merge or change other lanes.

Remaining Gen I provenance: no archival Stadium source payload and
individual receipt authenticated, no source-linked original NPC trade
save demonstrating randomized trainer ID and offered-mon level, no
original Japanese-family trade-table parity, no independently verified
full Time Capsule history, and no globally exhaustive distribution
source catalog. Missing original receipts MUST remain Unknown.


## Major October 13 continuation — original PK1 individual identity, candidate-set safety

**Engineering commit refs (forward-only, L4 PR #292 only):**
- `0783bf5494f7671b883a92e3d161ed1c9592e40d` — three-object
  original-source/PK1 identity binding and bounded candidate aggregation.
- `39a25b224a703bad53f19da5fa3739634f30feb8` — strict 256-tag
  source-family validation for cartridge vs independently authenticated
  Stadium source media.

### Source evidence and new security boundaries

Original Red/Blue disassembly:
`pret/pokered@af519899719f0754965776faac0e836a3b906e6d`;
original Yellow disassembly:
`pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`.
These sources document retail NPC trade script rows and the randomized
OT ID behavior, **not** proof of a particular individual's acquisition.
The original Gen I/II L4 archive and raw-name join remains reused,
not copied or replaced; L1 continues to own wild/fishing/static slots.

New standalone `Gen1OriginalIndividualEvidence.h` requires independent
identification of **three separate evidence objects**:
(1) original distribution source archive SHA-256 and exact original source
record index; (2) the historical original PK1 record SHA-256 including
its original game-font OT/nickname bytes; and (3) current native PK1
record SHA-256 including current original game-font fields.
A distinct same-individual historical continuity attestation and
complete intervening transformation history are required even if the
two PK1 hashes happen to match. The caller must independently compute
and verify the source hashes; supplied flags are **claims**, not
cryptographic authentication performed by the helper.

For Gen I Stadium gift candidates, original source archive identity
must also exactly agree with the existing independent L4 distribution
archive digest. One unrelated signed archive cannot validate another
candidate with the same species or trainer ID.

The native L4 `Gen1OriginalGiftNativeEvidence.h` now gates
gift/NPC/Stadium comparisons on the individual record evidence before
making any negative claims. Its new `evaluateAll` accepts at most
128 candidate histories, explicitly counts source-dependent Unknown,
excluded and conditional candidates, and never outputs a global
Legal/Invalid verdict—even when *all supplied* alternatives are
excluded. Unknown/malformed status values cannot create a positive or
invalid verdict.

Untrusted serialized origin-family tags are now checked against only
the exact supported values: original cartridge script (1) or original
Stadium media (2). Unknown value 0, modern listing 3, and all other
numeric tags (4 through 255) remain Unknown, not a gift source.

**No Gen I-native ribbons, ability, gender, nature, held-item property,
breeding or modern-generation fields were added.**

### Precise tests executed locally

Three standalone exact-blob-matched C++ tests, each compiled and
executed using GCC and Clang, both normal `-O2` and ASan+UBSan
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror`:

1. `legality_l4_gen1_preflight.cpp` — 65,536 byte-by-byte original
   capture versus Time Capsule tradeback comparisons, 4,096 move/DV
   boundaries, source-family labels 0..255 and PP Up edge values.
2. `legality_l4_gen1_npc_metadata.cpp` — 65,536 serialized English
   R/B/Yellow edition/row labels, source-script fixed nicknames and
   reachable versus unused gift rows.
3. **New** `legality_l4_gen1_individual_chain.cpp` — 4,096 independent
   archive/original/current-record/same-individual evidence masks,
   65,536 pairs of valid/malformed candidate statuses, original/current
   PK1 SHA identity mismatch, archive mismatch, missing original raw
   strings, and more-than-128-candidate overflow protection.

**12/12 actual focused compiler and sanitizer executions PASS** with
no sanitizer findings; Git hash-object for each of those exact source
files matched the committed source blob.

Exact changed production blob pins:
- `Gen1OriginalIndividualEvidence.h`
  `9ea043740c09052723f2e3754d064dbbefda5955`
- `Gen1OriginalGiftNativePreflight.h`
  `b5a42d488dd3c219bc720dc2abdbeba10956e510`
- `Gen1OriginalGiftNativeEvidence.h`
  `bb3e3b9d053b04ed722dfc09cdb5a565e287b235`
- `legality_l4_gen1_individual_chain.cpp`
  `f03cf4a71ccc46361a4e229b8f75f50caa097494`
- `legality_l4_gen1_preflight.cpp`
  `ff22ea01a361b96047197894af3cc0460cf02dbb`
- `legality_l4_gen1_original_native.cpp`
  `71636bfa22eee0777943e4fd3dda3951daf7a6a4`

Exact-source local test log:
`/mnt/data/l4-gen1/major-session-2026-10-13-tests.log`.

### Acceptance is blocked; do not merge

The user-requested full suite commands were **actually attempted** from
the partial local workspace:

```bash
bash tests/test_legality_l4_runner.sh
L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
```

Both returned exit **127**, as the runner is not present; a complete
GitHub checkout cannot be cloned because the shell cannot resolve
`github.com`. This does NOT validate the 28-suite native L4 runner.

The modified `legality_l4_gen1_original_native.cpp` real dependency
integration fixture is registered and expanded to test malformed source
families, individual digest/history mismatches, Stadium archive drift,
and multi-source Unknown. It has **not executed locally against
the actual Gen I reader, L4 original distribution join, or complete
checkout**. Coordinator must run L4 normal and ASan/UBSan with both
GCC and Clang on the exact new HEAD, inspect every compiler result
and rerun independent original-source checks before acceptance.
No user-owned game saves or Pokémon were modified.

**Outstanding historical gaps:** authentic Stadium distribution archive,
actual individual receipt/transfer chain, native original and final
PK1 full-record SHA-256 including game-font OT/nickname bytes, English
vs Japanese NPC script parity, randomized OTID receipt witnesses,
actual original Gen I catch-rate history and verified Time Capsule
record transitions. Synthetic positive fixtures are NOT historical proof.

**Status:** major additional L4 original-source and native identity
engineering delivered; full-checkout acceptance BLOCKED; other lanes
and production legality analyzer remain untouched.


## October 13 major session — read-only raw PK1 and original PokeList provenance

**Engineering heads:** `667274c889b57611803e3f1494ddc6e0f53f5ba9`
(raw-record gate), `103a32f41a44213d6b473c65afa153f16d4ea295`
(original source-slot check), and `bb6d9dd4b214371464a6f6677291fe684a4adc67`
(native-type executable fixture).

The new `Gen1NativeRawRecordWitness.h` is a read-only metadata
consistency gate for the **actual strict Gen I `PokemonRecord`**:
boxed raw records must be exactly **33 bytes**, party records exactly
**44 bytes**. It independently cross-checks the source species marker,
HP, status, type bytes, catch-rate/tradeback byte, four moves,
original Trainer ID, 24-bit experience, five 16-bit stat-experience
values, 4-bit DVs including derived HP DV, move PP/PP Ups, level,
and all five party-only battle stats. Body mismatch/short/overlong data
returns UnknownNative **before** the original gift/trade evidence layer
compares original source-specific fields. A consistency failure is NOT
a global Invalid and cannot modify an original save.

Original game-font OT and nickname bytes are **not stored inside**
`PokemonRecord.rawBody`. The new bounded `compareOriginalListSlot`
therefore checks their independent read-only source list positions
following the exact Gen I parser layout:
`bodyStart = listOffset + capacity + 2`,
`otStart = bodyStart + capacity*bodySize`,
`nickStart = otStart + capacity*nameLength`.
It checks the list count/sentinel, same-slot species marker, complete
body and raw names for **international 11-byte and Japanese 6-byte
name formats**, rejects malformed capacities and out-of-range offsets,
and requires an independently attested same-save and individual link.
The helper is NOT an event-receipt oracle and is not called for wild
encounter enumeration (L1-owned).

**Pinned native source evidence, fetched from GitHub before editing:**
`include/Integration/Gen1/Gen1ReadOnlySave.h`, Git blob
`d52f60cd861b48cab4d577dd702607c72e970a15`;
`src/Integration/Gen1/Gen1ReadOnlySave.cpp`, Git blob
`a2445a20a2ede222a2b14e56f87305ef3263484e`.
The local test header was recreated and its exact Git blob matched
`d52f60cd...`; the real parser .cpp was inspected by live GitHub
connector, but full compiled/linkable source was not downloaded.
The test fixture no longer uses a placeholder record type: it instantiates
the actual source-linked `PokeVault::Integration::Gen1::PokemonRecord`.

**Exact finished source/test Git hashes:**

- `Gen1NativeRawRecordWitness.h`:
  `d1665327c9c9c63e14bfe8edc478d3246c0b008d`.
- `legality_l4_gen1_raw_record_witness.cpp`:
  `a58f82af8fc06c45e09b7ab2580beb01b0720724`.
- `Gen1OriginalGiftNativeEvidence.h`:
  `ef3b258de8ac92d3c5336c0f0de924b22f64dfd8`.
- `legality_l4_gen1_original_native.cpp`:
  `2190dfdca348360a6c900608a2c73c4db4ec7751`.

**Actually executed on these final source blobs:**
four focused test programs with GCC and Clang C++20 normal (`-O2`)
and ASan/UBSan (`-O1 -g -fsanitize=address,undefined`), strict
`-Wall -Wextra -Wpedantic -Werror`: **16/16 PASS, no sanitizer
findings**. Among them the new actual-native-type fixture verifies
both raw-body lengths, individual-bit mutations of decoded source bytes,
65,536 raw name pairs, 8,192 original-list offset probes, malformed
list counts/sentinels and offset overflow; plus the existing 65,536
Time Capsule byte combinations, 65,536 Red/Blue/Yellow trade table tags
and 4,096 original-individual identity proof masks.
Exact local execution log:
`/mnt/data/l4-gen1/major-session-native-verified-final-2026-10-13.log`.
All new sources and tests were compared against committed blob hashes.

**Critical blocked gates:** Both user-requested full runner commands
(`bash tests/test_legality_l4_runner.sh` and
`L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh`) were actually
attempted from the isolated workspace and exited 127 because the
complete checkout and runner script were absent; `git ls-remote`
still fails `Could not resolve host: github.com`. The standalone
native-`PokemonRecord` raw test is NOT the full 28-suite L4 runner,
and `legality_l4_gen1_original_native.cpp` (524,288 proof
combinations) still has not compiled against its full real Gen I/II
distribution dependencies in the local environment. Acceptance remains
**BLOCKED** until the exact-head GitHub Actions/complete checkout
normal+ASan/UBSan matrices, Python tests and native join pass.

Remaining historical gaps: original Stadium distribution and authentic
per-individual receipts, complete acquisition/transfer lineage and
original/current Pokémon source digests, verified native save-slot
identities for genuine users, Japanese trade-source history and the
actual Time Capsule return-item chain. No global Valid/Invalid claims,
wild encounter data ownership transfer, original save writes, coordinator
merge, accepted-branch or MAIN modification occurred.
