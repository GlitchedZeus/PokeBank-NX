# PokeBank NX — new-chat engineering handoff (2026-10-29)

**Purpose:** Resume work without losing the current Round 10 hardware candidate, QA discoveries, legality findings or the owner's v1.0 release scope. This is a status handoff, not a declaration of release readiness. Links and branch heads were checked on 2026-10-29 unless noted as an external-agent report.

## Project direction (owner's priority)

- **v1.0 target:** All nine Pokémon generations (Gen I–IX) and a real, persistent, safe Pokémon Vault, with save discovery, staged editing/transactions, safe import/export, deposits/withdrawals and recovery. Do **not** confuse presence of a UI or design document with completed native support.
- **v1.1:** Expanded QoL/polish, Japanese editing and other language expansion. Preserve existing multilingual work; Japanese Gen I/II editing remains **locked/read-only** until explicitly accepted, even if writer code exists.
- **Hard safety rules:** Original emulator/install saves immutable; no live writes; staged changes and separately verified Export Copy; refuse unsafe conversions, overwritten occupied slots and unrepresentable information; no silent corruption/loss or invented transfer history. Keep unsupported/uncertain legality at **Incomplete**, not clean.
- **Workflow:** One NRO per useful hardware round; avoid scope creep, endless audits and speculative Gen V changes. Fix actual tested bugs and then move on to Gen VI–IX and Vault.

## Round 10 — actual latest audited Gen I–V Switch candidate

- **Application source:** `3634d1e6f3f216160db162fd953d82ede45f7af0`, tree `9b242ddf13981d45c4fd535e65c8e1da037835d4`.
- **Working branch:** `fix/gen4-bag-remove-compaction`, HEAD verified `0842c6f32a4fa5bf1497666283aaddacfbd9012a` (later commits include checklist and audit binding; NRO was built from `3634d1e6`). NOT merged into main.
- **NRO:** `PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro` — 177,159,857 bytes; SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`.
- **Download:** https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40775834570/artifacts/12854482976
- **Exact-file audit:** https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40780152474 — `NRO AUDIT: PASS` per main Claude report.
- **Official Round 10 checklist:** https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/gen4-bag-remove-compaction/docs/hardware/RETEST_3634d1e6.md (H01–H17); status **NOT DEVICE TESTED** unless user later supplies results.
- **Round 10 changes vs Round 9:** Gen IV Bag Remove now compacts occupied stacks without hiding following items; Berry/TM-HM Add sorts ascending ID; malformed pockets refuse unsafe edits. Host GCC tests, Clang, ASan/UBSan, 39/39 Python files, QA contracts, device-flag syntax and 15/15 Japanese-kit real-save unedited round trips reported passed. Gen V Bag behavior unchanged.
- **Round 9 fallback remains:** `d6a20a2a` artifact, but has the older Gen IV Bag Remove defect. Do not accidentally test/label it as Round 10.

## Round 11 test artifact — NEW, not a new NRO

- The user requested the same interactive **HTML** workflow as previous rounds. A new **self-contained offline interactive HTML** checklist was generated and smoke-tested for 17 quick / 48 full cases, PASS/FAIL/NOT TESTED buttons, per-test notes, screenshots/filenames, search, filters, progress stats, Markdown report export and JSON backup. Local browser autosave is best-effort; advise periodic JSON backups. The tested HTML is separately linked in the final handoff.
- Two editable Markdown worksheets exist: Round 11 quick H01–H17 and expanded H01–H48. **They are drafts**, not a released/CI-bound Round 11 build; enter actual NRO filename and calculated SHA-256. If testing the audited 3634d1e6 NRO, label it “Round 10 NRO / Round 11 worksheet”.
- The 48-test worksheet includes earlier Round 10 smoke coverage, per-generation editors/UI, navigation, Bag, standalone Pokémon import/export, Bank/Recovery inspection, and a **known gap** check for native Gen I/II/IV/V Vault access. Do not ask owner to create corrupt personal saves or force power loss on physical SD.
- When owner uploads completed HTML-generated `.md` results (and screenshots), triage failures by severity and precise game/provider/repro, distinguish bugs from missing v1.0 features, and prepare a repair batch for main Claude. Do not ask them to redo completed tests.

## Independent Claude API QA agent — discoveries and archival status

**Campaign 1 (Gen IV Remove):** QA local commits `dabba3cb` + `b09a0169`, patch and bundle preserved in isolated GitHub archive. Fix was independently integrated/reconciled into Round 10 by main Claude. Do not apply it again.

**Campaign 2 (Gen IV sorting / Handler / recovery):**
- Gen IV Berry/TM Add sorting QA local `39431be2`, `7b164d70`, `62b76716`; separately integrated in Round 10.
- **Upstream PKSE #147 Handling Trainer on cross-game withdrawal:** QA local `1c4a5ea1`; no destination trainer was supplied to conversion. Local patch introduced `applyReceivingHandler` and tests, but destination save language, friendship/memories, UI/hardware routes and exact PKHeX/HOME parity are unresolved. **Archived, unmerged, not release-approved.** This matters when enabling modern cross-game Vault withdrawals; avoid partially valid HT records.
- **Durable recovery version ordering:** QA local `b531e3d1`; confirmed current code previously picked *lowest unused* `.previous.N`, while missing-target recovery picks *highest N*, so cleanup of older backups could cause stale restore/data loss. Fix and test independently archived. **Not integrated** in Round 10; archive warns of the 1,000,000 numbering fallback risk. Must be reviewed and safely integrated before treating recovery cleanup as release-ready. Avoid destructive Recovery cleanup in hardware acceptance before the fix.
- Independent API fuzz soak passed reported ASan/UBSan existing targets: `fuzz_gen3_sav` 1,868 runs; `fuzz_gen5_sav` 267; `fuzz_gen5_bag` 9,270; `fuzz_gen5_edits` 1,776. `fuzz_pk_records` timed out; not claimed. Native Gen V Bag Remove/Sort still unverified; do not imitate Gen IV without game evidence.

**Campaign 3 (import/export and Bank custody):** QA agent reported **no new production defect**, **five new property tests**, no production code modifications, local `qa3/import-bank-safety` based on `0842c6f3` (report mistakenly left its commit as `$H`; require actual `git rev-parse HEAD` before accepting provenance). Host suite and new ASan/UBSan reportedly passed, Clang/hardware not run. Hostile seeded input counts: Gen I 2,400, Gen III 3,300, Gen IV 4,400, Gen V 3,000 = 13,100; Bank custody tested 40 seeded deposit/withdraw/swap/save/reload/discard sequences, plus full Bank and bad record refusal. Gen II `.pk2` not freshly tested; real UI, power-loss, files collision not tested. **Campaign 3 patch/bundle were not uploaded to this chat or GitHub; user must download/upload if integration desired.** Do not claim five property tests are in Round 10/NRO.

**Persistent limitation verified directly in `src/Trainer/Bank.cpp`:** existing Bank format tags support `FRLG`, `GG` (Let's Go), `SWSH`, `BDSP`, `PLA`, `SV`, `ZA`. No native Bank tag for Gen I/II/IV/V; those classic generations have standalone `.pk*` file paths rather than a complete native Vault deposit/withdrawal path. `docs/MASTER_VAULT_SPEC.md` is a design spec, **not implemented proof**. This is the key v1.0 architectural gap. Existing Bank is a useful foundation, but do not claim all Gen I–IX supported.

**Archive branch (verified):** `archive/qa-campaigns-2026-10-28`, currently `62ac5338261d11d4be1f5c8382191b979881cc04` before this handoff's updates. Old ZIP contains Campaign 1/2 QA patch/bundles and issue #147; separate `durable-generation-order/` folder contains two recovery patches and review notes. Archive branch is independent; do not merge its archive content or duplicate already integrated Gen IV fixes.
- Root: https://github.com/GlitchedZeus/PokeBank-NX/tree/archive/qa-campaigns-2026-10-28/docs/qa-archives/2026-10-28

## Independent legality (Codex) — NOT included in Round 10 NRO

- **PR #314:** https://github.com/GlitchedZeus/PokeBank-NX/pull/314 ; verified open/draft at `eabe41f31903d2e724e25d04d18b36c26a0e340a`. Main app PR #307 also open/draft at `d6cc878a9652b78a6cdfd8a31cdb5797172d6c9d` (older track); neither was merged by this handoff.
- Codex reports 24 workflows / 49 jobs passing, but **not release acceptance, full PKHeX parity or app UI wiring**. Outstanding: Gen III–V recursive breeding/ancestry, joint Gen III/IV EV/EXP solver, unresolved Gen V historical/native name applicability, Gen IV Korean GTS, events/Ranch provenance, false-Invalid / false-clean cases, source-branch coverage and actual 20-title integration.
- Fresh/reused measured PKHeX 26.10.18 comparison, **synthetic agreement only** (not population legality accuracy):
  - Gen I: 11,172/12,000; 111 raw false Invalid, 2 raw false clean, 715 Incomplete.
  - Gen II: 11,276/12,000; 82 false Invalid, 33 false clean, 609 Incomplete.
  - Gen III: 40,170/40,945; 0 false Invalid, 0 false clean, 775 Incomplete.
  - Gen IV: 62,370/63,219; 60 false Invalid, 0 false clean, 789 Incomplete.
  - Gen V: 133,180/134,293; 0 false Invalid, 38 false clean, 1,075 Incomplete, 647 reference errors.
- Native HGSS contest grass corrected; Ranch ability ambiguity Incomplete rather than invented certainty; Gen V B2W2 name-pattern disagreements incomplete pending primary/native evidence; no unsupported verdict promotion. Verdict semantics: **Invalid wins; any partial rule family stays Incomplete; only NoProblemsFound means clean.** Keep PR #314 separate until owner explicitly authorizes integration, complete wiring/device validation and evidentiary parity.
- Rule inventory: https://github.com/GlitchedZeus/PokeBank-NX/blob/eabe41f31903d2e724e25d04d18b36c26a0e340a/docs/legality/hardening-20261027/RULE_GAPS.md
- Application handoff: https://github.com/GlitchedZeus/PokeBank-NX/blob/eabe41f31903d2e724e25d04d18b36c26a0e340a/docs/legality/hardening-20261027/APP_HANDOFF.md

## Next-chat immediate workflow

1. Give user the **Round 11 interactive HTML download** (and GitHub archive link), explain 17 essential vs 48 full, PASS/FAIL/NOT TESTED, notes, Markdown export + JSON backup. No fake candidate SHA, no claim legality is in the app.
2. User tests `3634d1e6` on their Switch or a later independently audited build. After completing tests, user uploads generated Markdown report and any screenshots; triage by actual failures first.
3. If Claude API Campaign 3 artifacts arrive, verify checksums/provenance and archive them on isolated branch. Integrate regression tests independently and only when they fit the source base. Do not block hardware testing on those tests alone.
4. In subsequent engineering work, prioritize critical recovery generation-order repair, modern Handler language/context for safe cross-game routes, and especially **real Gen I–IX Vault persistence/format adapters and custody**. Do not claim full Vault based on current Bank seven-group support. Keep v1.1 QoL/language expansion deferred.
5. Update GitHub README/branches/PRs only with user's explicit request or once the feature/test state is actually confirmed. The goal is a tested v1.0, not more planning scripts.

## How to report

Be concise, technical and candid about proven vs proposed work. Refer to exact branches, SHAs and build identities; never invent tests, merge status or ETA. Main Claude builds NRO; independent Codex handles legality; API QA agent audits locally, and its unpushed artifacts need deliberate transfer. Never infer private save content or upload private saves as public GitHub attachments.