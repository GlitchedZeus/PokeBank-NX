# L5 major tranche 5 — full synthetic native PK3/PK4 EV ingress and executable production regression handoff

**Scope:** L5's existing draft [PR #294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294), original L5 issue #290 / master #285. No production C++, shared CI, shared Makefiles, source/staged save writes, sibling lane branches, Gen V, Master Vault, UI, or umbrella legality PR #103 changed.

**Pinned accepted engine:** `d29eada5c087e201e00c749703f1a2de3ddfadeb`. The current L5 branch is behind the accepted legality head by two commits; the coordinator owns safe forward-only reconciliation and exact-HEAD CI.

## Why this tranche matters

L5 previously established a *source-rule* false-Invalid discrepancy: native Gen III/IV EVs of **253/254/255** in a single stat are possible (aggregate cap 510), but NX's analyzer issues an unguarded Invalid for any value >252. Pinned PKHeX does not hardcap per-stat 252 before native Generation VI. A second reference discrepancy: PKHeX explicitly rejects **nonzero EVs on unhatched eggs**; NX's examined EV section does not directly check this.

Previous Q12 source-contract tests do not traverse encrypted native data or production `Legality::analyze`. This tranche supplies concrete *read-only synthetic native data ingress* and a coordinator-ready test calling the actual analyzer; it does NOT claim either path was run together on GitHub CI.

## Full shuffled and encrypted native fixtures — Q09 / Q16 / Q12

File: `tests/fixtures/legality_l5_native_ev_vectors.json`. **336 checksum-valid synthetic native-format records + 12 intentionally corrupt controls**.

| Native format | Length | Shuffle states | EV values per state | Valid records |
|---|---:|---:|---|---:|
| Gen III PK3 | 80 bytes (0x50) | 24/24 (PID % 24) | 0,1,252,253,254,255 | 144 |
| Gen IV PK4 | 136 bytes (0x88) | 32/32 (PID >> 13)&31 | 0,1,252,253,254,255 | 192 |
| Corrupted encrypted data | respective | six indices per generation | EV 253 base | 12 |

Each exact synthetic record records a canonical source-game-independent PID/shuffle, EV byte and total, egg/non-egg bit, checksum, raw encrypted hex, source-level NX EV-threshold difference, source-level PKHeX egg rule, and **UNDETERMINED full legality**. Gen III uses the raw 48-byte G/A/E/M block arrangement, 16-bit canonical checksum and 32-bit PID XOR OT key. Gen IV uses four 32-byte shuffled substructures, 16-bit LCG-XOR keystream seeded by checksum, and the 0x88 stored record. No private, authentic retail, copyrighted event or ROM byte content is committed.

**Proof completed:** the generator independently asserted encrypt→decrypt byte-for-byte round trips for all 336 valid records, checksum and EV/egg field recovery, and checksum failure for all 12 corrupt controls. After the JSON was committed, a *separate independent JS decoder* decoded **all 348 exact committed hex payloads** and validated expected checksum outcomes. These are source-scope format tests, **not** a run of NX's strict C++ parser or `Legality::analyze`. The separate Python validator `tools/audit_legality_l5_native_ev_vectors.py` is committed and can reproduce in a checkout; it has not been executed in an exact-HEAD GitHub host job.

Counts: **144 PK3**, **192 PK4**, 168 eggs, 168 non-eggs, **168 encoded records with EV 253–255** (source-rule regression targets). At each shuffle index, both the zero/nonzero and egg-state combinations are represented across the balanced sets. Each corrupt fixture is expected to fail native checksum validation.

**Source/format pins**, all from accepted branch at the SHA above:
- `src/Encryption/Encryption3FRLG.cpp` Git blob `eec27b73a15d6652371ed2e066690ef0fa9fb7ef` — 24 Gen III block shuffles, PID ^ OT xor, canonical checksum.
- `src/Encryption/Encryption4.cpp` Git blob `406935761713db95ae738398b65ba0a37d4cf97b` — 32 Gen IV shuffle indices, checksum-seeded 16-bit LCG xor.
- `include/Encryption/Encryption.h` Git blob `9eab685b76511d5ea08e0fffdff63529a5824fe6`.
- `include/Pokemon/Pokemon3FRLG.h` Git blob `f93b121fb59b08495668161e932b7aa2bbb82588`: EV offsets 0x38–0x3D, egg bit 30 at 0x48, checksum 0x1C.
- `src/Pokemon/Pokemon4ReadOnly.cpp` Git blob `bc82e07fa737bf5a3fa15533bd77083b0300436a`: EV offsets 0x18–0x1D, egg bit 30 at 0x38, checksum 0x06.

## Exact analyzer differential — coordinator-ready native 36-case probe (UNEXECUTED)

`tests/fixtures/legality_l5_ev_native_analyzer_probe.cpp` constructs *new synthetic* PK3 using the accepted `Pokemon3FRLG` implementation and **encrypted PK4** through `Encryption::encryptArray4`, validates native checksum and EV field reads, and calls `Legality::analyze` directly with both exact-source and no-source contexts. It checks **6 EV values × 2 generations × 2 contexts = 24** cap cases plus **3 egg EV values ×2 generations×2 contexts = 12** egg-status cases. **36 total**.

The two modes test the same production behavior before and after a fix:
- `--baseline`: requires current NX's *known* cap rejection at EV 253/254/255 and absence of an egg-specific EV Invalid for eggs EV=1/100; this is the expected failing *reference behavior*, not an endorsement.
- `--after-fix`: must observe **no spurious >252 EV hard-cap Invalid** on native PK3/PK4, and must see an **egg-specific nonzero-EV Invalid** for EV 1/100, without needing historical parent inference. Note that unrelated incomplete/invalid findings are not globally suppressed.

An **opt-in sidecar** `tools/audit_legality_l5_ev_probe.mk` reuses the accepted `GEN34_BALL_ANALYZER_SOURCES` dependency graph, excludes its own `main()`, compiles this new standalone analyzer probe without modifying `Makefile.host`, and offers baseline and fixed, normal and ASan/UBSan targets.

From a **coordinator-approved full checkout with toolchains installed**, run:

```bash
python3 tools/audit_legality_l5_native_ev_vectors.py
make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-baseline
make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-baseline-sanitize
# After coordinator patches and accepts the production EV behavior:
make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-fixed
make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-fixed-sanitize
```

**Disclosure:** these four C++ actual-analyzer build targets were **not executed** by L5; no full checkout was available locally and GitHub Actions remains queued. The outcome is an executable acceptance recipe and source-only scaffolding, not a certified completed production fix.

## What is still blocked

1. **Coordinator MAIN fixes F1 and F2.** Preserve original saved EV bytes, use verified native format for per-stat cap (255 in Gen III/IV, 510 aggregate), and add explicit unhatched egg EV check without asserting original breeding history.
2. **Full native actual-analyzer run.** Execute 36-case baseline reproducer on accepted commit, then fixed mode after production fix, plus strict 348-fixture native decode path and ordinary/ASan/UBSan compilation. Do not make the new fixture "pass CI" by silently relaxing assertions or skipping SHA identity.
3. **Authenticated historical Pokémon fixtures.** The synthetic vectors are not retail-original PK3/PK4 and cannot certify a Pokémon's method, original parents, event archive, capture-time RNG or absence of other legal histories.
4. **Exact-head CI and branch reconciliation.** Preserve newer accepted commits and ask coordinator to forward-only reconcile 2-behind branch; no L5 force pushes, rebases backward, MAIN merge or edits outside lane.
5. **Full Gen I–IV PKHeX parity.** Q17's 336 game/family research scoreboard still has zero certified original method×game behavioral parity cells. Original source encounters, entire move acquisition chronology and event distribution/reuse license remain independent unresolved gates.

**Safety rule:** A failed hypothetical source route may be candidate-only Contradiction; absent original stage or an unverified candidate set is **Incomplete/Unknown**, never fabricated global Invalid.
