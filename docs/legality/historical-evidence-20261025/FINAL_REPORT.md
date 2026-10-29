# Legality evidence continuation — October 25, 2026

Starting legality head: `0b01ad6c614c1652a6c9591195ebff22cb761d8f` on existing draft [PR #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314). Implementation checkpoint: `7160ed05bcbcc1c239b35af8c4b4710b4bdb3e83`. Published legality head: `45448775e3dba93a9410c2d7cab1e7a62c0f63d7`; tree `81e59a86832c30fdc9c90c5476aded5e8d2a2a9e`. All 49 checks across 24 workflows PASS on this exact published head. PR #307/application and main remain untouched; no merge or hardware deployment.

The supplied research led to a real full-card reader correction and released-template provenance in the Gen V report. Its field claims were checked against the actual data before implementation. All 81 full-card files and 202 YOSHIRA specimen files came from one pinned public community archive. These files establish content integrity and format evidence; their historical authenticity was not independently established.

Implemented:

1. Fixed real WC5FULL raw game-mask interpretation. Archive 0x30/0xF0 differs from low-nibble PGF 0x03/0x0F. Single-game 0x10 Black / 0x20 White have archive witnesses; unobserved/unknown masks remain Unverified.
2. Preserved exact PGF comparison and added a separately named, four-byte receipt-date-only relation. A full distributor card has no receipt date. All 63 released archived cards pair to63 catalog payloads under that relation, with all non-date bytes checked; malformed substituted dates refuse pairing. All 18 unreleased/debug cards remain excluded.
3. Pinned archive commit/tree/Git-blob/SHA256 for 81 originals; CRC16-CCITT-FALSE matched81/81, XMODEM0/81. Generator refuses truncated, extended, zeroed and CRC-recomputed altered files, leaving original evidence unchanged.
4. Wired released-template provenance into the actual Gen5 gift candidate/report path. Receiving game/language claims stay paired per original card. This adds information, does not authenticate receipt, does not remove the existing unsupported-receipt warning, and does not change verdicts.
5. Added native archive replay, calendar/ownership/unknown-mask/mutation tests, four retained synthetic/oracle English/Italian Karrablast/Meloetta PK5 production-path fixtures, and normal/sanitizer CI evidence artifacts. All pre-existing receiving checks remain intact.

Findings kept explicit:

- Research/field-map language labels are reversed: native 4 Italian / 5 German, supported by actual files and the primary enum. An unreleased ITA-labelled file actually has language7 and duplicates a Spanish file; filenames never establish byte semantics.
- WORLD11 Scrafty(KOR), PGF index 14, has archived paired claim 0x83 versus catalog annotation 0x23. The conflict remains unresolved and cannot establish receipt or disqualify another valid event alternative.
- The 202 published YOSHIRA specimens all carry OT YOSHIRA, DVs 10/1/12/5 and HP DV 5. Existing engine predicates already model this pattern. The original full distributor-save binary/custody was not authenticated here, and no blanket regional Mew predicate was invented.

The retained Gen5 production replay evaluates every one of 135,193 unchanged inputs against the new engine. Its machine-readable output is byte-identical to the prior engine:58,505 NoProblems, 72,410 Invalid, 4,278 Incomplete. Input SHA256 `1b3d20cfaeefb80916885c576425deb5f48f9fca4b6049ac7e36be4c72a24689`; old/new native SHA256 `1641e2721e4419b2cd568cebf832c0d3e2e8274ba0140ae07f8f279b3efe2543`. This is retained regression evidence, not a fresh holdout or historical specimen count. Frozen parity percentages are unchanged; no full parity claim is made.

Validation:

| Command | Result |
| --- | --- |
| `bash outputs/legality-evidence-20261025/run-focused.sh gcc` | PASS: 26 configurations; real 81-file normal+san replay |
| `bash outputs/legality-evidence-20261025/run-focused.sh clang` | PASS: 26 configurations; real 81-file normal+san replay |
| `python3 tools/legality_completion/full_card_evidence.py --archive <verified archive> --pgf <pinned709-card resource> --check --reader <compiled archive test>` | PASS, 81 originals / 63 released / 18 unmatched; exact source regeneration |
| `python3 tests/test_gen5_full_card_generator.py --archive <verified archive> --pgf <pinned resource>` | PASS, 4 source-forgery/refusal controls; immutable original bytes |
| `/tmp/legality-oct25-diff/gen5_diff < <retained135193-record corpus>` then `cmp <prior native output> <new output>` | PASS, byte-identical |
| Python syntax compilation, all 46 workflow YAML trigger/jobs parsing, `git diff --check` | PASS |
| Read-only four-title Gen V workspace bridge against actual current app checkout `11b44e09562ddbfc3e9e6dc08b138f022b8c3d3a` | PASS; app unchanged |
| [Focused CI run 40245637156](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40245637156) on the published head | PASS: app boundary, GCC and Clang; successful logs have no sanitizer diagnostics |

Both successful CI artifacts were downloaded and checked independently against the original source manifest. Each contains two normal/sanitizer reports, all 81 exact original full-card files and the pinned manifest. Both reports record 63 released rows, 18 excluded debug/unreleased files and zero authenticated receipts. Artifact identities and SHA256 digests are retained in `CI_ARTIFACT_VERIFICATION.json`:

- [GCC evidence](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40245637156/artifacts/12619250018).
- [Clang evidence](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40245637156/artifacts/12618149493).

The artifact links expire under GitHub retention. Downloaded copies and their original hashes are retained with this report.

All eight source leads, primary links, verified versus unverified status and future evidence requirements are in SOURCE_ASSESSMENT.md. Remaining gaps include original matching saves for all five Enigma cases, authenticated PCNY fixed-parent correlations and Korean link histories, original Japanese Mobile receipts, GameCube native transfer/RNG provenance, Ranch/GTS traces and missing original cards, unsupported Gen5 receipt histories, general Gen III–V joint ancestry and unresolved Gen IV/V newer-reference disagreements. Missing evidence remains Unknown/Incomplete. No historical completeness or deployed UI/device percentage is measured.

Changed production/build files and tests (enumerated against the starting head):

- `.github/workflows/legality-completion.yml`
- `Makefile.host.gen5-legality`
- `include/Legality/Gen5FullCardCatalog.h`
- `include/Legality/Gen5FullWonderCard.h`
- `src/Legality/Gen5Legality.cpp`
- `tests/fixtures/gen15_session2/full_card_engine.txt`
- `tests/test_gen5_full_card_evidence.cpp`
- `tests/test_gen5_full_card_generator.py`
- `tests/test_gen5_full_wonder_card.cpp`
- `tests/test_gen5_gift_receivability.cpp`
- `tools/legality_completion/full_card_evidence.py`

Full changed-file list: CHANGED_FILES.txt.

First publication CI exposed a newer-GCC warning in the new test's copied structured binding. The loop now uses a const reference; every assertion and warning-as-error flag remains intact. The affected test was rebuilt and passed in all four compiler/mode configurations and in final-head focused CI. No production code changed after the 52-configuration pass or 135,193-record replay. `CI_FOCUSED_GCC_RAW.log` preserves the original failure; `CI_FOCUSED_SUCCESS.log` preserves the corrected run.

Final publication verification: all 49 checks across 24 workflows passed on the exact published head. `FINAL_CHECKS.json` and `FINAL_CI.json` retain every check/run URL and identity; `FINAL_VALIDATION.json` combines them with local validation and verified artifact digests. The draft state, integration base and published code head were checked again. A subsequent WIP-only documentation checkpoint records this result; the PR remains at its passing code head.
