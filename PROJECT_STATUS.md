# PokeBank NX Project Status

Last updated: 2026-09-01

This file is the authoritative verified-state handoff for coding sessions. Do not promote work based only on a chat/session report; GitHub source, test/build evidence, and physical device reports are tracked separately.

## Project identity

- Product: **PokeBank NX**
- Current inherited app/target branding may still contain `PokeVault NX` / PKSE-era names in places
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Development branch: `feature/pokebank-playable`
- Recovery branch: `recovery/interrupted-2026-09-01`

### Important application/source checkpoints

```text
3101fc0
verified recovery checkpoint

fabff21
recovered baseline/status checkpoint

c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes

82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet

9a2151ee70c73bab4451f35a1216c495d60b57ba
branch-history reconciliation checkpoint before the documentation refresh
```

The exact `.nro` produced for the completed Action Sheet milestone was built from **82a0779a...**, not from a later documentation-only commit.

`main` and `feature/pokebank-playable` are intentionally kept synchronized after documentation-only housekeeping. The current coding session must still inspect local work before assuming the remote branch is newer than an interrupted workspace.

---

## Verification vocabulary

Use these states precisely:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

Additional planning/status labels such as `SPECIFIED`, `AUDITED`, `PLANNED`, `INTERRUPTED LOCAL-ONLY`, or `NOT IMPLEMENTED` may be used, but they do not imply verification.

Never say `DEVICE TESTED` unless the user physically ran the exact recorded binary/hash on Nintendo Switch hardware.

---

## Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | Interrupted 23-game identity work recovered/published; old requested commit `1932cf0` not found |
| Stable game identity registry | HOST TESTED | 23 unique release/platform IDs; exact Switch title-ID lookup |
| GBA/Switch FireRed + LeafGreen identity separation | HOST TESTED | `firered_gba`, `leafgreen_gba`, `firered_switch`, `leafgreen_switch` distinct |
| Platform-aware native game cards | NRO BUILDS | Native Switch build uses stable IDs/platform labels |
| Live installed-save write policy | HOST TESTED / NRO BUILDS | backup-only destinations, no generic injection argument, low-level restore rejects live-title writes |
| Controller-first Pokémon Action Sheet | HOST TESTED / NRO BUILDS | issue #2 completed/closed; shared Party/Boxes/Storage action model |
| Action Sheet device artifact | NRO BUILDS | exact binary/hash recorded below and in `docs/BUILD_RECORD.md` |
| Physical Switch validation | NOT DEVICE TESTED | hardware test intentionally deferred until the newer combined UI/control build is available |
| HOME-style controls/theme shell | INTERRUPTED LOCAL-ONLY | issue #13 session reported substantial local implementation, but it was not verified/built/committed/pushed before usage ended |
| OLED Black / Dark / Light design | SPECIFIED | `docs/UI_STYLE_GUIDE.md` + `docs/CONTROLS.md`; reported local code not yet remote-verified |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | `docs/MASTER_VAULT_SPEC.md` |
| PKSM-Core Gen III integration | AUDITED / PLANNED | concrete `PK3` / `Sav3` spike in `docs/PKSM_CORE_INTEGRATION.md` |
| RetroArch Gen I-III adapters | NOT RECOVERED / PLANNED | rebuild after PKSM-Core decision |
| Modern Switch adapter validation | AUDITED / PLANNED | pkHouse/PKHeX comparison plan documented |
| PKHeX Oracle | SPECIFIED | `docs/PKHEX_ORACLE.md` |
| Vault-driven Pokédex | SPECIFIED | `docs/POKEDEX_SPEC.md` |
| Artifact automation | PLANNED | issue #15 tracks persistent `.nro` artifact workflow |
| Final branding/startup/NRO metadata | PLANNED | issue #16 tracks remaining product identity/startup integration |
| Golden save/Pokémon fixture corpus | PLANNED | issue #17 supports PKSM-Core/PKHeX/adapter regression testing |

---

## Completed Action Sheet milestone

GitHub issue **#2 — Implement controller-first Pokémon action sheet** is completed and closed.

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet
```

Implemented behavior:

- Pressing A on an occupied Pokémon in Party, Boxes, or Storage opens one shared deliberate action sheet.
- Exact target order:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

- Opening the Action Sheet performs no mutation.
- Moving focus performs no mutation.
- B performs no mutation and closes the sheet.
- Cancel performs no mutation.
- `View Pokémon` uses the existing details path as an explicitly read-only action.
- Vault / Bank / Transfer / Clone / Make Shiny / Provenance actions safely report `Not yet supported` for this milestone.
- Unsupported actions cannot fall through into another action.
- Live installed-save writing remains hard disabled.

### Verification

```text
make -f Makefile.host host-test      PASS
make -f Makefile.host host-sanitize  PASS
git diff --check                     PASS
make -j1                             PASS
```

### Device-test artifact

```text
Filename:
PokeBank-NX-ActionSheet-82a0779.nro

Exact source:
82a0779a5143cca0690d0c7068946d84ebe9f107

Size:
9,695,669 bytes

SHA-256:
6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b

Physical Switch status:
NOT DEVICE TESTED
```

The project is finishing the UI/control milestone before testing so the next hardware pass can cover both the Action Sheet and the newer UI/control shell in one build.

---

## INTERRUPTED SESSION 2 — critical continuity note

GitHub issue **#13 — Implement HOME-style controls and OLED/Dark/Light UI shell** is the current implementation milestone.

The second coding session ran out of usage after reporting substantial changes in a **preserved local Session 2 branch/worktree**.

Reported local work included:

- semantic three-theme foundation;
- OLED Black / Dark / Light palette work;
- Select Game semantic raised/focused cards;
- typed/context-aware bottom controller hints;
- held D-pad / Left-Stick navigation repeat;
- `+` no longer globally exits;
- contextual `+` Options / Settings / compatibility / More behavior being wired;
- read-only `-` Help / Controls behavior;
- persisted theme cycling;
- reusable focus/card/modal helpers;
- Action Sheet integration through the shared UI primitives.

The session ended immediately before/while adding pure host regressions.

### What was NOT completed for Session 2

The Session 2 implementation was **not** confirmed through:

```text
new-code host tests
new-code sanitizer pass
new-code native .nro build
git diff --check final pass
PROJECT_STATUS update
application-source commit
push to origin
verified remote SHA
new .nro artifact
physical hardware test
```

Therefore none of that reported local Session 2 implementation may be claimed as remote `IMPLEMENTED`, `HOST TESTED`, or `NRO BUILDS` yet.

### First action in the next coding session

Use the permanent prompt:

```text
docs/PROMPT_SESSION2_RECOVERY.md
```

**Do not reset/clean/switch over unknown local work first.**

Inspect:

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git log --all --oneline --decorate --graph -40
git reflog -40
git stash list
git worktree list
```

Find the preserved Session 2 branch/worktree/reflog state if it still exists.

Only if the local Session 2 work is genuinely absent should the session reimplement that milestone from `docs/CONTROLS.md`, `docs/UI_STYLE_GUIDE.md`, and issue #13.

Do **not** repeat the old full repository recovery investigation.

---

## Baseline / CI

Portable host verification:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

GitHub workflow:

```text
.github/workflows/host-tests.yml
```

The workflow checks the portable host suites, sanitizers, and whitespace. It does **not** replace a devkitPro native build or physical Switch test.

Native integration:

```bash
make -j1
```

GitHub also contains:

```text
.github/pull_request_template.md
.github/ISSUE_TEMPLATE/device_bug_report.md
```

Use those to keep future PR/device reports tied to exact source and artifact identities.

---

## Working PokeBank NX-specific features

- canonical release/platform game identity catalog;
- 23 current target game identities;
- exact title-ID lookup for native supported Switch identities;
- separate GBA/Switch FireRed and LeafGreen identities;
- platform labels on native game cards;
- app version + abbreviated Git commit display;
- independent host tests/sanitizers;
- GitHub host CI;
- read-only installed-save source posture;
- backup-only current destination posture;
- three-layer live-write hard lock;
- shared safe Pokémon Action Sheet;
- read-only `View Pokémon` action;
- explicit safe not-yet-supported Action Sheet actions.

---

## PKSE foundation present in the tree

The inherited PKSE source still supplies:

- native Switch application/build framework;
- controller UI;
- save selection/backups;
- Party/Boxes browsing;
- Pokémon editing/creation infrastructure;
- trainer/item editing infrastructure;
- legality-related data/UI;
- bank/conversion framework;
- Switch save access;
- generated game/Pokémon tables;
- handlers for the Switch families represented by PKSE 1.1.3.

Inherited code is a foundation, not automatic PokeBank NX verification.

---

## Current safety posture

**LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED.**

Current protections include:

1. save-destination UI only exposes safe backup destinations;
2. generic save API cannot request title injection;
3. low-level restore rejects live-title writes before mounting save data;
4. legacy `injectToGame=1` is ignored/rewritten disabled.

Do not weaken this lock for a demo, UI shortcut, transfer prototype, or integration spike.

Future writes must pass the per-adapter gates in `docs/SAVE_SAFETY.md`.

---

## Supported identities / current adapter status

| Platform | Identities | Current PokeBank NX status |
|---|---|---|
| Game Boy | Red, Blue, Yellow | identity only; parser not recovered; PKSM-Core audit first |
| Game Boy Color | Gold, Silver, Crystal | identity only; parser not recovered; PKSM-Core audit first |
| Game Boy Advance | Ruby, Sapphire, Emerald, FireRed, LeafGreen | identity only; first engine spike is PK3/Sav3 |
| Nintendo Switch | FireRed, LeafGreen, LGPE, Sword/Shield, BDSP, PLA, Scarlet/Violet, Legends Z-A | identity mapping builds; inherited handlers present; source-specific validation still required |

See `docs/GAME_SUPPORT_MATRIX.md` for capability-by-capability tracking.

---

## Upstream / reference stack

Detailed audit: `docs/UPSTREAM_AUDIT.md`.

### PKSE

- `https://github.com/kiasta/PKSE`
- Original native Switch foundation.
- **UPSTREAM ONLY.**

### PKSM-Core

- `https://github.com/FlagBrew/PKSM-Core`
- Highest-priority native C++ historical format/save-engine candidate.
- First decision spike: `PK3` + `Sav3`.

### PKHeX

- `https://github.com/kwsch/PKHeX`
- Primary technical correctness/reference implementation.
- Planned host-side PKHeX Oracle.

### Auto Legality Mod / PKHeX-Plugins

- `https://github.com/santacrab2/PKHeX-Plugins`
- Encounter-driven legality/generation reference for development tooling.

### pkHouse

- `https://github.com/Insektaure/pkHouse`
- Modern Switch save-behavior reference.
- **REFERENCE ONLY**; reimplement/cross-check rather than copy/paste.

### pkDex

- `https://github.com/Insektaure/pkDex`
- Pokédex UX/data-organization reference.
- **REFERENCE ONLY**; PokeBank NX owns the Vault-driven Dex architecture.

### PKForge

- `https://github.com/sofianeelhor/PKForge`
- Vault/provenance/atomic-write architecture reference.

### Reuse rule

Before major format/save/legality/conversion/Dex/bank/generation work:

1. inspect tracked references;
2. record relevant project/file/revision;
3. record language/license;
4. classify as `DIRECT REUSE`, `ADAPTER`, `PORT`, or `REFERENCE ONLY`;
5. add tests/golden vectors;
6. preserve attribution/license requirements.

---

## Documentation map

Start with:

```text
docs/PROJECT_MAP.md
```

Five-PM / later-session prompts:

- `docs/PROMPT_SESSION2_RECOVERY.md` — **HIGH**, recover/finish issue #13 and produce the combined `.nro`
- `docs/PROMPT_SESSION3_PKSM_CORE.md` — **MAX**, device feedback first then issue #4 PK3/Sav3 spike

Core files:

- `docs/NEXT_SESSION_PLAN.md` — current execution plan
- `docs/SESSION_RUNBOOK.md` — coding workflow
- `docs/CONTROLS.md` — HOME-style controller contract
- `docs/UI_FLOW.md` — safe navigation/action behavior
- `docs/UI_STYLE_GUIDE.md` — visual/theme contract
- `docs/ARCHITECTURE.md` — module boundaries
- `docs/MASTER_VAULT_SPEC.md` — Vault v1
- `docs/SAVE_SAFETY.md` — live-write gate
- `docs/UPSTREAM_AUDIT.md` — pinned references/reuse classifications
- `docs/PKSM_CORE_INTEGRATION.md` — PK3/Sav3 spike
- `docs/PKHOUSE_REFERENCE.md` — modern Switch research notes
- `docs/PKHEX_ORACLE.md` — correctness tool design
- `docs/POKEDEX_SPEC.md` — Vault-driven Dex design
- `docs/BUILD_RECORD.md` — binary/source bookkeeping
- `docs/DEVICE_TEST_CHECKLIST.md` — physical test procedure
- `docs/RELEASE_CHECKLIST.md` — artifact/release discipline
- `docs/SESSION_LOG_2026-09-01.md` — dated recovery/Action Sheet/interrupted Session 2 history

---

## Current task

### Priority 1 — HIGH: finish issue #13 safely

Use:

```text
docs/PROMPT_SESSION2_RECOVERY.md
```

Recover the interrupted local Session 2 UI/control/theme implementation if it survives, then take it through:

```text
host regressions
    ↓
sanitizers
    ↓
git diff --check
    ↓
native .nro build
    ↓
PROJECT_STATUS update
    ↓
application-source commit
    ↓
push + remote SHA verification
    ↓
record/preserve new .nro
```

Do not start PKSM-Core before this coherent Session 2 milestone is saved.

### Priority 2 — physical Switch test, issue #8

Test the exact newer combined Action Sheet + UI/control `.nro`.

Record exact source SHA and binary SHA-256 before promoting any behavior to `DEVICE TESTED`.

### Priority 3 — MAX/deep engineering, issue #4

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

If hardware feedback does not reveal a blocking UI crash/regression, begin the PKSM-Core Gen III spike:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
box/party extraction
round-trip strategy
adapter/dependency decision
```

Issue #17 tracks the reusable golden fixture corpus supporting this and later parser/adapter work.

### Later product priorities

1. issue #3 — Master Vault v1 + Banks;
2. issue #9 — professional Summary + provenance;
3. issue #6 — RetroArch discovery + read-only Gen I-III adapters;
4. issue #11 — modern Switch adapter audit;
5. issue #5 — PKHeX Oracle;
6. issue #7 — Vault-driven Pokédex/Living Dex;
7. issue #10 — conversion/transfer without live writes;
8. generated collections/events;
9. live writes only after explicit per-adapter safety/device gates.

### Supporting issues

- issue #15 — automate/persist device-test `.nro` artifacts;
- issue #16 — final PokeBank NX branding/startup/NRO metadata;
- issue #17 — reproducible Pokémon/save golden test corpus.

Supporting issues should improve product reliability without derailing the current `#13 → #8 → #4` execution path.

---

## Known blockers / cautions

- Session 2 UI/theme code may exist only in the previous local coding workspace; inspect before reset/clean.
- No exact PokeBank NX recovery-era or Action Sheet build has yet been physically validated on Switch hardware.
- Previous custom Master Vault/RetroArch implementations were not recoverable and must be rebuilt from current specs/tests.
- pkHouse/pkDex remain reference-only under current policy.
- Temporary runtimes can lose `.nro` artifacts; record source SHA/hash and preserve the binary before ending build sessions.
- Golden fixtures must not become a route for committing personal saves, ROMs, console keys, credentials, or other inappropriate assets.

---

## Recovery record

The old requested historical checkpoint `1932cf0` was not present in available refs, reflogs, stashes, unreachable project history, archives, or GitHub during recovery.

Recovered work was published and preserved. Do not restart forensic recovery unless the verified GitHub state is genuinely unavailable.

Recovery artifacts created during the recovery runtime included:

```text
/mnt/data/PokeBank-NX-RECOVERY.bundle
/mnt/data/PokeBank-NX-WORKTREE-RECOVERY.tar.gz
```

Those paths were runtime artifacts, not permanent GitHub files.

---

## Exact build commands

Host:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

Native recovery environment used:

```bash
export DEVKITPRO="$PWD/build-deps/devkitpro-root/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
export PORTLIBS="$DEVKITPRO/portlibs/switch"
export PATH="$PWD/build-deps/pkgconf-install/bin:$DEVKITPRO/devkitA64/bin:$DEVKITPRO/tools/bin:$DEVKITPRO/portlibs/switch/bin:$PATH"
export LD_LIBRARY_PATH="$PWD/build-deps/pkgconf-install/lib"
make -j1
```

`build-deps/` content was local/ignored and is not committed.