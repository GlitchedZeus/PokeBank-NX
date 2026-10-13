# L5 — MAJOR independent QA tranche 7: integration PR #298

**Integration reviewed:** [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), HEAD `c91b53e84ef75669049d159616894ec68cd4b0dd` (draft). **Accepted legality:** `d29eada5c087e201e00c749703f1a2de3ddfadeb` (untouched). Lane heads independently re-fetched: L1 `abafc667`, L2 `cdc128a4`, L3 `0b30c020`, L4 `f61e97cb`, L5 original `2fb0a92f`. This tranche goes only to existing L5 [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294). No edits to coordinator production `Legality.cpp`, shared CI, accepted legality, or umbrella #103.

## Exact-head Actions and Gen III/IV F1/F2

[Integration Actions #38287166355](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38287166355): **completed SUCCESS at exact `c91b53e`**, host clean/full tests and sanitizers PASS; L1/L2/L3/L4/L5 standalone lane jobs all PASS; exact application SHA identity PASS. L5 actual job log records **52/52 synthetic native PK3+encrypted PK4 analyzer EV cases** normal and sanitizer PASS. Independently linked accepted crypto validated **336 encrypted exact-byte roundtrips, 12 bad checksum rejects and 251,904 single-bit tamper rejects** in both normal and ASan/UBSan. The 52-case coverage includes 253–255 individual EVs, 510/511 sum, unhatched egg with nonzero EV, source-free contexts and conflicting source game labels. The source at `src/Legality/Legality.cpp:1631–1653` uses immutable native format for the allowed max and the egg EV rule. **F1/F2 are fixed for these synthetic tested routes**, not complete PKHeX parity nor actual retail-original proof.

## Audit all integrated Invalid emissions

Machine-readable [Q18 all 37 site inventory](Q18_INTEGRATED_INVALID_37_SITE_AUDIT.json) enumerates **every direct `add(r, Severity::Invalid)` occurrence** in exact production analyzer source Git blob `a81cf6b878da50cc85837edf2c8c37ea11357d8a`. Earlier L5 had 36 sites; the new F2 unhatched-egg Invalid makes 37. All **226** `add(r, ...)` call sites use explicit Severity, **no severity passed via a dynamic variable**. The 37 are individually classified; the risks and unproven source conditions are recorded instead of calling 35 suspicious lines "bugs."

**Critical findings:**
1. **L5-F3 P0 — source-free PK1/PK2 ability false Invalid**. `Pokemon1ReadOnly.h:32` and `Pokemon2ReadOnly.h:32` intentionally expose ability `0` (ability did not exist). `Legality.cpp:1666–1677` nonetheless executes ability validation if `!sourceProfile`; `AbilityInfo.cpp` returns slot-index `0` for ability `0`, causing `Invalid("Ability not legal for this species")` for **every nonempty source-free native PK1/PK2**. Known conflicting-generation labels are sanitized to missing at `Legality.cpp:585–592`, triggering the same false-Invalid. Matching exact Gen I/II labels currently suppress this validator, proving the result incorrectly varies with source availability.
2. **L5-F4 P0 — source-free PK1 gender false Invalid**. `Pokemon1ReadOnly.h:36` deliberately returns neutral `gender()==2` because Gen I stores no gender. `Legality.cpp:2627–2647` checks gender if `!sourceProfile`, sees `2` as genderless in Bulbasaur or any other gendered species and emits `Invalid("Gendered species marked genderless")`. With exact `red_gb` source the gender validator is skipped. Missing historical source evidence must not turn a native-absent field into global Invalid.

**Local independent reproduction:** newly committed source-contract [18-case C++ suite](https://github.com/GlitchedZeus/PokeBank-NX/blob/audit/legality-l5-research-qa-20261012/tests/fixtures/legality_l5_gen12_sourcefree_contract.cpp) Git blob `2cdb7770` (SHA256 `d1664950...`) compiled and PASSED under GCC O2, GCC ASan+UBSan O1 and Clang O2 in the current session. This source-model proof does **not** run the whole analyzer; do not mislabel it. **Actual `Legality::analyze` synthetic 12-case native PK1/PK2 baseline-vs-fixed test** has been committed as `tests/fixtures/legality_l5_pk12_sourcefree_native_probe.cpp` with sidecar `tools/audit_legality_l5_pk12_probe.mk`, both normal and sanitized **NOT YET EXECUTED** from full repo.

### Coordinator P0 fix and native acceptance

Modify coordinator-owned `src/Legality/Legality.cpp` to gate **ability validation by verified native generation III+ capability**, not `!sourceProfile`; likewise gate **gender validation by verified native generation II+ capability** (leave modern format behavior intact). Preserve exact source mismatch as a WARNING and source-specific rules as unresolved. Suggested safe targeted gates: `nativeFormatGeneration != 1 && nativeFormatGeneration != 2` for abilities; `nativeFormatGeneration != 1` for gender, combined with existing profile guards. Do not erase current Gen III/IV hard Invalid for genuinely impossible stored ability/gender, and do not alter original saves.

Run from a fully reconciled checkout:
```sh
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline-sanitize
# After coordinator fix:
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed-sanitize
```
The probe differentiates no source, matching exact Gen I/II source and conflicting newer-game source; Bulbasaur (gendered) and Magnemite (genderless); PK1 and PK2, **12 context cases**. It has **not yet been executed through the production linker**: this is the highest priority follow-on.

## Other reviewed hard Invalid gates (NOT promoted to confirmed defects)

- `Legality.cpp:987,1003,1252,1260`: current Gen IV form game, Korean event release and hatch-region restrictions — retail original form transitions/hatching source and release records incomplete.
- `Legality.cpp:1107,1159,1170,1175,1196`: Gen III→IV original evolution lineage, Pal Park field/ball splits and egg markers. These are source-/stored-field-classified Invalid, but **complete competing history exclusion** and original before/after paired saves are not certified.
- `Legality.cpp:1685`: native Gen IV PID-selected ability slot; require exact event/static/Pal Park exception vetting.
- `Legality.cpp:1551,1697`: display-name sentinel based Invalid; check missing name strings against actual valid native species/move domains before hard Invalid. Potential resource coverage risk only, **not confirmed**.
- Other lines are structural format values, valid domain bounds, or later-generation-only gates; each appears in Q18 with exact lines and evidence requirements.

**No new hard Invalid from a failed isolated L1/L2/L3/L4 candidate history was observed**: the examined new L1–L4 encounter, breeding, transfer, event and move branches report INFO with Partial/Incomplete where required; but original source exhaustiveness has not been proven for every existing transfer/egg/exact-event structural Invalid. A failed hypothesis remains local, unknown histories remain Incomplete.

## Parity and release blockers

[Q17 refreshed integrated 336-cell matrix](Q17_INTEGRATION_NATIVE_EVIDENCE_336.json) distinguishes (a) exact production synthetic native PK3/PK4 cases; (b) source-backed PK1/PK2 False Invalid traces; (c) original retail Pokémon histories, of which **zero source-paired authentic cases are certified**; and (d) full PKHeX game×family behavioral parity, **0/336 certified**. The 52 synthetic EV cases **do not imply 52 of 336 cells or 52% parity**. L5's previous **247** Gen IV event template projections still have **zero** authenticated original card receipts. Gen I–IV encounter original multiset exhaustiveness, breeding parent/RNG source, full historical move chronology, release archives with reuse rights, and live retail paired native saves remain blockers.

**Coordinator engineering priorities:** P0 F3/F4 native-format gates and real 12-case PK1/PK2 tests; P1 exact source game form/hatch/transfer proof and original fixtures on Invalid site risks; P1 inspect name sentinel Invalid vs native numeric domain; P2 expand formal game×family original source parity once release/provenance inputs become available. **No L5 production changes or merge decision.**
