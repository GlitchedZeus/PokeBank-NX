# PokeBank NX — Session Log — 2026-09-01

This is a historical daily log, not the authoritative current-state file. For current verified status always read `PROJECT_STATUS.md` first.

---

## 1. Recovery / baseline state

The interrupted PokeBank NX repository work was recovered and normalized around:

```text
GlitchedZeus/PokeBank-NX
feature/pokebank-playable
```

PKSE remains upstream only.

Verified safety milestone:

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes
```

Recovered application foundation included:

- 23 stable release/platform game identities;
- distinct GBA and Switch FireRed/LeafGreen identities;
- platform-aware native game cards;
- host tests/sanitizers;
- native Switch build;
- hard-disabled live installed-save writes.

The earlier custom Master Vault / RetroArch implementation could not be recovered from available Git history/workspace data and is being rebuilt from specifications.

---

## 2. Reference / architecture documentation pass

Tracked reference projects were formalized:

```text
PKSE
PKSM-Core
PKHeX
Auto Legality Mod / PKHeX-Plugins
pkHouse
pkDex
PKForge
```

Major documentation created/expanded today includes:

- `docs/UPSTREAM_AUDIT.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/PKHEX_ORACLE.md`
- `docs/PKSM_CORE_INTEGRATION.md`
- `docs/PKHOUSE_REFERENCE.md`
- `docs/POKEDEX_SPEC.md`
- `docs/TRANSFER_MODEL.md`
- `docs/SAVE_SAFETY.md`
- `docs/ARCHITECTURE.md`
- `docs/GAME_SUPPORT_MATRIX.md`
- `docs/DEVICE_TEST_CHECKLIST.md`
- `docs/RELEASE_CHECKLIST.md`
- `docs/BUILD_RECORD.md`
- `docs/PROJECT_MAP.md`

GitHub host CI was also added for portable tests/sanitizers/whitespace.

---

## 3. UI reference / controls contract

Visual references supplied by the project owner were translated into a PokeBank NX-specific design contract rather than copied pixel-for-pixel.

Primary visual direction:

```text
Select Game:
large cover-art cards + strong focus

Pokémon Summary:
grouped data + large Pokémon render + tabs

Pokédex:
sprite grid + selected-species preview pane
```

Required themes:

```text
OLED Black
Dark
Light
```

Controller direction intentionally follows a Pokémon HOME-like mental model:

```text
D-pad / Left Stick  navigation
A                   Select / Open / Pokémon Action Sheet
B                   Back / Cancel
X                   Filter / Search / context
Y                   Sort / View / secondary action
L / R               nearby box/tab/Pokémon movement
ZL / ZR              larger jumps / major navigation
+                   More / Options
-                   Help / Controls / Screen Info
Right Stick          optional fast scroll / secondary pane
```

On Pokémon Summary, `+` is reserved/preferred for Compatible Games / Transfer Compatibility.

Contracts:

- `docs/CONTROLS.md`
- `docs/UI_FLOW.md`
- `docs/UI_STYLE_GUIDE.md`

GitHub issue #13 tracks implementation.

---

## 4. Session 1 — Action Sheet — COMPLETED

GitHub issue #2 implemented the controller-first nine-item Pokémon Action Sheet.

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet
```

Behavior:

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

Verified:

- shared Party / Boxes / Storage behavior;
- A opens the menu;
- B/Cancel/navigation are non-mutating;
- View is read-only;
- unfinished actions safely return `Not yet supported`;
- live-write hard lock remains intact;
- host tests PASS;
- ASan/UBSan PASS;
- `git diff --check` PASS;
- native build PASS.

Device artifact:

```text
PokeBank-NX-ActionSheet-82a0779.nro
9,695,669 bytes
SHA-256:
6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
```

Physical status:

```text
NOT DEVICE TESTED
```

Issue #2 was closed as completed during the documentation pass.

---

## 5. Session 2 — UI/theme implementation — INTERRUPTED

The second coding session started issue #13 using HIGH reasoning.

It reported a clean Session 1 baseline and substantial work in a preserved local Session 2 branch/worktree.

Reported local implementation:

- semantic three-theme foundation;
- Select Game semantic raised/focused cards;
- typed/context-aware bottom button hints;
- held D-pad / Left-Stick navigation repeat;
- `+` no longer globally exits;
- contextual `+` Options / Settings / compatibility / More behavior;
- `-` opens read-only Help;
- persisted theme cycling;
- reusable focused-card/modal helpers;
- Action Sheet being routed through the new primitives.

The coding session ended due to usage limits **before** completing the planned host regressions and before final verification/build/commit/push.

Therefore this work remains:

```text
INTERRUPTED LOCAL-ONLY
NOT REMOTE-VERIFIED
NOT HOST TESTED AS A COMPLETE SESSION 2 MILESTONE
NOT NRO BUILDS AS A SESSION 2 MILESTONE
```

A progress warning/comment was added to GitHub issue #13.

---

## 6. Branch history cleanup

During Action Sheet publication, GitHub `main` and `feature/pokebank-playable` temporarily had equivalent application content but different history:

- `main` contained the merged Action Sheet application source;
- feature branch contained the later build/status record.

They were reconciled without force-push or reset at:

```text
9a2151ee70c73bab4451f35a1216c495d60b57ba
merge: reconcile action sheet build record with main
```

Today's documentation refresh then continued on the feature branch before final synchronization.

---

## 7. GitHub issue changes today

### Closed

```text
#2 — Implement controller-first Pokémon action sheet
```

Closed as completed with exact source/build/hash evidence.

### Current

```text
#13 — Implement HOME-style controls and OLED/Dark/Light UI shell
```

Interrupted local progress recorded in issue comments.

```text
#8 — Produce and physically test first recovery-era .nro
```

Action Sheet artifact details recorded; physical test deferred until the combined UI/control build is ready.

### New infrastructure issue

```text
#15 — Automate preservation of device-test .nro artifacts
```

Goal: persist future test builds as GitHub Actions artifacts or prereleases with source SHA, size, SHA-256, and verification metadata.

---

## 8. Current next action

The next HIGH coding block must **recover and finish the interrupted Session 2 work**.

Before any reset/clean/checkout over local work:

```bash
git status
git status --short
git branch -avv
git log --all --oneline --decorate --graph -30
git reflog -30
git stash list
```

Then:

```text
recover local #13 work
    ↓
finish host regressions
    ↓
host tests + ASan/UBSan
    ↓
git diff --check
    ↓
native .nro
    ↓
commit/push/verify
    ↓
record source SHA + binary SHA-256
    ↓
physical Switch test
```

Only after that combined build is stable should the project switch to MAX/deep reasoning for:

```text
#4 — PKSM-Core PK3 / Sav3 Gen III integration spike
```

---

## 9. Safety state at end of day

Unchanged and mandatory:

```text
LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED
```

No UI work, transfer prototype, adapter spike, or convenience shortcut may bypass that lock.