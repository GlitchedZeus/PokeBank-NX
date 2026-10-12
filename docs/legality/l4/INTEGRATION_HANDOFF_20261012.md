# L4 exact-head integration handoff — 2026-10-12

**Scope:** Gen I–IV legality L4 Events and Metadata; parent #285 / lane #289 / [draft PR #292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292). This is a **validation and coordinator-integration handoff**, not a declaration of accepted production parity. No further L4 feature expansion before coordinator reconciliation.

## Preserved and verified live heads before handoff

- L4 PR head prior to this documentation-only checkpoint: `9daca67c1df7f91461129373c09d04199388733f` (19 forward-only lane commits, 35 changed lane files). A later documentation commit may advance this SHA; **re-fetch PR #292 immediately before applying**.
- Accepted legality branch: `feature/legality-engine-gen1-4-20260929` at `d29eada5c087e201e00c749703f1a2de3ddfadeb` during this session. Branch comparison: **L4 19 ahead / 2 behind** accepted, independent changes preserved. Never reset, rebase backward, force-push, or overwrite later commits.
- CI `PokeBank NX Host Tests`: previous-head [run #3965](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38216397418) completed **success** for clean build, host tests, focused RSE bridge and ASan/UBSan. At the beginning of this handoff, L4 prior-head [run #3982](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38219175894) was **in progress** (host tests and RSE passed, sanitizer running). **Refresh status**, and do not transfer this result to a newer commit without verifying its exact SHA.

## Delivered source families and conservative behavior

| Catalog | L4 source | Scope / proof rule |
|---|---|---|
| V01–V06 | `Gen14EventDistributionSourceEvidence.h` | Source-attested original Gen I Stadium, Gen II PCNY/Stadium, Gen III Mystry Mew, WISHMKR/Channel, Berry Fix/Wondercard, Colo/XD gift vs ordinary Shadow; missing original gift/individual receipt or redistribution permission never silently becomes attested |
| V07/V10 | `Gen4EventTemplateAudit.h`, `Gen4EventArchiveAttestation.h`, `GEN4_WC4_TEMPLATE_AUDIT.csv` | All 247 PKHeX projection entries and card-ID collisions, no real original PCD receipt authenticated; archive/release/field digest/source grade independently gated |
| V08/V11 | `Gen4EventNativeFieldEvidence.h`, `Gen4EventNativeAttestedEvidence.h`, `Gen4UnifiedEventCandidateEvidence.h` | One original-gift candidate + multiple historical fields, not independent global legality verdicts |
| V09 | `Gen4EventRangerGiftHistory.h` | Ranger Manaphy conditional egg PID/IV, original recipient and hatch history; source-free native candidate Unknown |
| V12 | `Gen34EventTrainerIdentityEvidence.h` | PK3/PK4 original TID/SID, OT gender, exact origin game, language; original independently sourced and field-history preserved |
| V13 | `Gen3EventRawTextEvidence.h` | Actual decrypted/checksum-prevalidated PK3 10-byte nickname + 7-byte OT + original game-font/egg/trade re-encoding cautions |
| V14 | `Gen4EventTextHistoryEvidence.h` | Original PCD OT/nickname bytes, history-aware rewrite/Pal Park/evolution eligibility |
| V15 | `Gen4ContestRibbonNativeEvidence.h` | Native Gen IV five contest ribbon rank-prefix families and independently verified eligible Sinnoh contest stages |
| V16 | `Gen4EventBallHistoryEvidence.h` | Retained original event ball, egg and GBA→Pal Park candidate history; D/P/Pt/HGSS split fields; never infer unseen captures |
| V17 | `Gen4EventDateHistoryEvidence.h` | Native PK4 egg/met day-month-year + original distribution window/console clock source attestation |
| V18 | `Gen4EventOtherMetadataEvidence.h` | Held-item, ability, form and PID-derived nature history; item swaps, evolution, form changes only comparable with independently proven history |

**Production policy:** Source-free original history, absent trustworthy archive and missing original receipt = **Incomplete / Unknown**, NEVER new hard **Invalid**. Even an attested mismatch is local to an individually identified hypothetical event route; the overall specimen may have other legal histories. Do not emit production Severity::Invalid until every authentic original encounter, trade, egg, gift, RNG and transfer alternative is exhausted by accepted analyzers. No original source save is modified.

## Exact-source validation and evidence

1. The **tracked** header `include/Legality/Gen14EventDistributionSourceEvidence.h` and **tracked** test `tests/test_legality_l4_gen1_3_event_sources.cpp` were compiled and executed locally after **matching both files' Git blob SHA-1** against the exact source tree:
   - header `f6b044f8b8487156e9b427d0e1c06081a95184fe`
   - test `cdf6797c0af58d1c25bd6799d128e484e1615c1f`
   - GCC C++20 `-O2` **PASS**; GCC ASan/UBSan `-O1 -g` **PASS**; Clang C++20 `-O2` **PASS**; Clang ASan/UBSan `-O1 -g` **PASS**. All `-Wall -Wextra -Wpedantic -Werror`; each exhaustive run checked **38,400** source combinations with 18 *synthetic* conditional witnesses and no unexpected original/Shadow promotion. This is **one** of the fourteen C++ test translation units.
2. GitHub tree/API static preflight on previous head: all **14/14** `tests/test_legality_l4_*.cpp` runner entries exist; **6/6** original native PK4 linker source files exist; all direct internal `#include` paths resolve. No missing file detected. Static presence is **not** compilation or execution.
3. Live L4 CSV vs generated C++ data: **247/247** exact structural projection matches, 70 distinct card IDs, 42 reused-ID groups, 219 rows that share a card ID; **0** original event receipts authenticated.
4. Independent L5 Q07 join at L5 PR #294 head `cbffad5704ac07a2037b709db70a9c0d99628ef1`: **247/247** index/species/TID/SID/card ID plus native game/language matches L4, no discrepancies. Generated C++ template exact Git blob `6dd1fea4a9ba3669f957fbd4e70d076cb0f85529` agrees with L5 pin. **This is projection parity, not original authenticity.**
5. Two lane-owned Python verifier modules and two nine-case synthetic fixture suites were executed locally in an earlier validation session (18 fixture cases PASS). The scripts' *full Git checkout* run, including live L5 JSON input, still needs recording at the integrated exact SHA. Other earlier standalone source-equivalent C++ checks are **not** a substitute for all 14 checked-out tests.
6. No independent, authorized original Pokémon event PCD/PGT archive hash/release receipt with original moves, ribbons or raw OT/nickname text has been supplied. Tests' synthetic source booleans are **never retail proof**.

## Shared build/CI gap: owned by the coordinator

- GitHub `.github/workflows/host-tests.yml` correctly checks out a `feature/legality-` PR's **exact head SHA**, runs `make -f Makefile.host host-test`, focused RSE, then `host-sanitize`. Its shared targets do **NOT** invoke `tests/test_legality_l4_runner.sh` or register any of the 14 lane test programs.
- Exact previous-head `src/Legality/Legality.cpp` was checked: **none** of the fourteen new L4 helper header namespaces/names is referenced. Existing production analyzer continues using earlier `Gen4EventTemplate::matchEvolutionLine` path. **No L4 runtime integration is being claimed.**
- The coordinator alone chooses how to register these 14 programs and their ASan/UBSan variants in shared `Makefile.host` and the host workflow, when to reconcile isolated L1–L5 candidate modules and when/how to wire them into `Legality.cpp`.
- Before any production wire-up, resolve source-attestation input origins, missing archives, exact original stage transitions and duplicate candidate/exhaustiveness semantics; field-specific Unknown must not promote global Valid or Invalid. Verify genuine native encrypted PK4 positive/negative/Unknown fixtures, checksum, zero/empty, bad size, tamper, GBA origin 1–5/15, 3053 met-location already in native namespace, and no-save-write invariants.

## Coordinator execution commands after fetching this exact HEAD

```bash
# Checkout an immutable exact application-head commit; do not reset any
# advancing shared branch; use a detached validation checkout.
git fetch origin feature/legality-l4-events-meta-20261012
L4_SHA="$(git rev-parse FETCH_HEAD)"
git worktree add --detach /tmp/pokebank-l4-verify "$L4_SHA"
cd /tmp/pokebank-l4-verify
L4_EXPECTED_HEAD="$L4_SHA" CXX=g++ bash tests/test_legality_l4_runner.sh
L4_EXPECTED_HEAD="$L4_SHA" CXX=g++ L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
L4_EXPECTED_HEAD="$L4_SHA" CXX=clang++ bash tests/test_legality_l4_runner.sh
L4_EXPECTED_HEAD="$L4_SHA" CXX=clang++ L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
# Independently fetch L5 Q07 JSON at an immutable L5 SHA and supply it:
python3 tools/legality_l4_l5_q07_join.py --l5-q07 /path/to/Q07_WC4_EVENT_PROVENANCE_JOIN.json
# After coordinator registers L4 tests, run all existing accepted gates:
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

The coordinator must record **exit status, exact SHA, compiler, sanitizer and test counts**. If any test fails, keep PR #292 draft and open a bounded L4 correction; do not merge around it. If the shared baseline advances, re-evaluate new dependencies and carry **both** histories forward; no force push/rebase backward.

## Unresolved / blockers that must stay open

- **RUN-BLOCKED:** full exact-head source checkout not available in this assistant execution container (GitHub DNS fails). The complete 14-test runner **has not been executed** at HEAD, including all seven native PK4-linked programs. This is the primary compiler/behavior merge blocker.
- **REGISTRATION-BLOCKED:** no shared test target/CI registration for 14 standalone L4 programs.
- **WIRING-BLOCKED:** no production `Legality.cpp` use of L4 source history helpers, severity/candidate exhaustion policy not integration-reviewed.
- **EVIDENCE-BLOCKED:** every one of 247 original Gen IV archive grades unverified, no per-original SHA+release+fields/license; L5 Q07/Q09 original paired test sources missing. V01–V06 original retail event/receipt/region histories also incomplete, including PCNY/Stadium/GBA event card/GC transfer distinctions. Arbitrary clock history, traded OT, renamed/evolved strings, original PID and target species evolution history fundamentally unavailable from a detached specimen.
- **NO HARD INVALID:** any missing genuine source evidence returns candidate-specific Unknown / Incomplete until source supplied.

**Decision:** Keep draft PR #292 as a substantial reviewable *conditional source-evidence package*; **NOT green/merge-ready** until exact full 14 C++ normal/sanitizer runs and production registration/wiring conditions are independently resolved. Freeze new L4 feature development pending coordinator integration.
