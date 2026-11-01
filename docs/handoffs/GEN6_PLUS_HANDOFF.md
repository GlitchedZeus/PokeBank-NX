# Gen VI–IX application campaign: active handoff (development/gen6-plus)

> Rules: [`docs/ENGINEERING_PROTOCOL.md`](../ENGINEERING_PROTOCOL.md). Source of truth: GitHub.
> This lane is isolated from the Gen I–V repair campaign, #307, #314, #327/#328. No merge into `main` or
> into the Gen I–V repair branch, no force-push, no branch deletion, no source-save write.

Last updated: 2026-11-01 UTC, by Claude (Gen VI–IX session_011W9mYgjDqGPH91UprKtQLu).

## Branching decision (owner, 2026-10-31)

Gen I–V is finished and hardware-accepted first, by the Gen I–V session, on its own repair branch. This lane
keeps developing Gen VI–IX on `development/gen6-plus` using the current Gen I–V application only as its
development foundation. Once Gen I–V is fully accepted, this lane is updated to the FINAL accepted shared UI,
editor, controls, menus, save management and common workflows, reconciled and re-tested, and then offered as
a **separate integration PR for owner approval**. Until then: no PR, no merge. The legality engine (#314,
#327/#328) stays separate.

## Base and branch

- Branch `development/gen6-plus`, started from the Round 10 Gen I–V source `fix/gen4-bag-remove-compaction`
  @ `0842c6f3`; merged with `main` @ `9ce7f737` (PR #330 consolidation) by a normal merge in `b5fe6c0b`
  (docs/workflows/status only; zero src/include/tests/Makefile diff).
- Imported lanes: Gen VI (#309), Gen VII (#311), Gen VIII SWSH (#312) from `claude/gen8-native-20261015` @ `2f065ee2`.

## Architecture (one application, not nine)

| Layer | Files | Role |
| --- | --- | --- |
| Capability registry | `include/Integration/Common/GameCapabilityRegistry.h` | All 38 titles: per-workflow PASS / READ-ONLY / BLOCKED(reason) / NOT APPLICABLE, native mechanics, storage geometry, hardware state and UI-integration evidence. `static_assert`s enforce cumulative inheritance and evidenced hardware claims. |
| Session contract | `include/Integration/Modern/NativeSession.h` | The one backend interface for Gen VI+: views, provider field states, staging, draft bracket, clone/move/remove/create support with reasons, bag, trainer facts, dex, review rows, verified export. |
| Adapters | `Gen6NativeSession`, `Gen7NativeSession`, `Gen8NativeSession` | X/Y/ORAS over the #309 staged session; SM/USUM over #311 (export blocked by the SAV7 signing gate); **Sword/Shield over the new `Gen8StagedSession`** (battle-team protection, canonical SwishCrypto re-encryption verifier, export to `exports/gen8`). |
| Presentation bridge | `include/Pokemon/ModernRecordView.h`, `include/Legacy/ModernReadOnlyTrainer.h` | One presentation Pokémon and one Trainer bridge for the shared panels. |
| Save Instances | `include/Integration/Modern/ModernSources.h` | 3DS: Checkpoint layout, Dekopon, manual (`3ds-single-copy`). SWSH: manual link only (`switch-main-single`); no Switch save-manager layout is assumed. Strict validation, per-profile binding, fingerprint-checked reopen, Home card snapshot. |
| Routing | `SaveSelectScreen.cpp` (`ModernAssignedFile`, `ModernInstalledSnapshot`), `UI.cpp` (`handleModernView`, `handleModernInstalledView`) | ONE Home card per Gen VI–IX game (an installed Switch title merges into it; no "(Save File)" card), Save Instances under that card (X), A opens the assigned save or, with nothing assigned, a read-only snapshot of the installed save; one TrainerViewScreen over the bridge. |
| Shared surface | `src/UI/ModernSharedSurface.inc` + `ModernScreenState.h` | The Gen VI+ adapter of the shared Party/Boxes/Items/Trainer screen and the ONE editor workspace. Only `ExternalLegacy` screens with an adapter-served id reach it (installed-title SWSH stays on the legacy reader). |

## Commits on this lane

| Commit | Content | Evidence |
| --- | --- | --- |
| `ff607c4e` | Lane import (#309/#311/#312) | gen6/gen7/gen8 suites green |
| `82cd400f` | Registry, NativeSession, Gen VI/VII adapters, bridge, Save Instances, routing | registry/adapters/sources/bridge suites (GCC + Clang; ASan/UBSan) |
| `2b25f2d7` | Shared surface wired, registry hardware/UI honesty, review fixes | real-screen `test_ui_modern_surface_host` (98), `..._gen7_host` (27) |
| `b5fe6c0b` | Normal merge of `main` @ `9ce7f737` | docs/workflows only |
| `3697d77d` | Experimental NRO workflow (`.github/workflows/gen6-plus-experimental-nro.yml`) | run 41231980644 FAILED in host gates: LeakSanitizer (CI sets `detect_leaks=1`) found the vendored FlagBrew `rsa.c` leak; fixed in the next commit |
| `5f84ed8c` | Sword/Shield adapter + shared-UI integration; MemeCrypto leak containment | CI run 41235138247 **success** (host gates + devkitA64 NRO + RomFS verification); see below |

## This milestone (Sword/Shield + leak fix)

- `Gen8EditorProvider.h`, `Gen8StagedSession.{h,cpp}`, `Gen8NativeSession.{h,cpp}`: Nickname, OT, OT gender,
  TID16/SID16, held item (SWSH presence ≤ 1607), OT friendship (handler friendship refused), Pokérus, ball
  (1–26), fateful editable; IV/EV/nature/level/species/moves read-only (no pinned SWSH stat table to recompute
  cached stats). Box Clone/Move/Remove; party storage refused; team-referenced slots never moved/removed;
  locked-team members never edited; unreadable team blocks refuse every box change. Dynamax Level,
  Gigantamax, mint nature and team membership shown as native read-only facts. Bag and Pokédex unavailable in
  the adapter with named prerequisites. Export: edited `main` rebuilt from the immutable block list with only
  staged record ranges replaced, then verified (strict reparse, canonical re-encryption, block keys/types/sizes,
  foreign blocks identical, per-byte PK8 whitelist) and published as `edited.bin` + `original_backup.bin`.
- `MiniGmpAllocationScope.h`: the pinned FlagBrew `rsa.c` never clears its `mpz_t` temporaries (≈0.9 KiB per
  SAV7 open, on device too). The submodule is untouched; verify/sign run inside a scope that reclaims them.
- Tests: `test_gen8_native_session` 118, `test_ui_modern_surface_gen8_host` 46 (real screen), `test_modern_sources`
  78 (SWSH link/assign/reopen/variant refusals), `test_game_capability_registry` (SWSH rows); every Gen VI–VIII
  suite passes with `ASAN_OPTIONS=detect_leaks=1`; Clang backend set green.

## Session 3 milestones (Let's Go, BD/SP, MemeCrypto scope audit)

| Commit | Content | Host evidence |
| --- | --- | --- |
| `1509a314` | `MiniGmpAllocationScope` hardened: depth counter (inner scopes no-op), recursive mutex, static hook restore, overflow flag; invariants documented | `test_mini_gmp_scope` 13 (nesting, overflow, 4 threads × 6 real sign/verify) GCC/Clang/ASan+LSan |
| `2a4f4cb3` | Let's Go Pikachu/Eevee adapter (`Integration/Gen7b`) | `test_gen7b_native_session` 68, `test_ui_modern_surface_gen7b_host` 31 |
| `7d785b2e` | Brilliant Diamond/Shining Pearl adapter (`Integration/Gen8b`), shared PK8 edit rules (`Gen8RecordEdits.h`) | `test_gen8b_native_session` 41, `test_ui_modern_surface_gen8b_host` 46, `test_modern_sources` 94 |

- **Let's Go**: strict SAV7b parse (block CRCs, gapless 1000-record storage, party/partner pointer integrity);
  op-log staging; Edit (nickname, OT, OT gender, TID/SID, LGPE balls, fateful); Clone appends at the next
  free index; Release compacts and remaps party/partner pointers; partner/party never released; Move
  blocked (game-assigned positions); Friendship/AVs read-only (they feed the cached CP). Export: header +
  storage blocks + CRCs only, replay-verified.
- **BD/SP**: PKHeX SAV8BS is ONE flat file per slot (the earlier "paired-file" prerequisite was wrong and
  is removed). Revisions 1.0–1.3 by size + Gem8Version word, whole-file MD5, ROM code, battle teams/locks
  from BoxLayout8b. Same PK8 rules as SWSH via `Gen8RecordEdits.h`; box Clone/Move/Remove; export writes
  staged records + MD5 only. Box names shown.
- **Legends: Arceus**: PA8 codec (0x168/0x178, 0x58-byte blocks) over the verified SwishCrypto container;
  Edit (nickname, OT, OT gender, IDs, friendship, Hisui balls, fateful); held items read-only (Pokémon
  cannot hold items in PLA); Effort Levels/Alpha/Noble/mastery read-only; box Clone/Move/Remove; export with
  canonical re-encryption + block verifier to `exports/pla`.
- **Experimental NRO of `7d785b2e` (LGPE + BD/SP)**: CI run 41240043279 green (host gates incl. leak-checked
  sanitizers and Clang, devkitA64 build, ELF symbol check, RomFS verification). Artifact
  `PokeBank-NX-Gen6Plus-Experimental-7d785b2e` (id 13057521770, zip sha256
  `86b3bd8468d418acd91a7f5c501cdcf089c59a8fc561b4e6a18f46d898d3749e`). 
- **Experimental NRO of `b3d40b3a` (LGPE + BD/SP + PLA, current code head)**: CI run 41241375177 green (same
  gates). Artifact `PokeBank-NX-Gen6Plus-Experimental-b3d40b3a` (id 13058177412, zip sha256
  `3791796d43c740e53a547ffe4a0dd9d4e90129cd1de97276126032284ff8e284`); NRO SHA-256 in its `SHA256SUMS`.
  EXPERIMENTAL, NOT DEVICE TESTED.
- **Experimental NRO of `b1cfc9e7` (+ Scarlet/Violet)**: CI run 41253352389 green (same gates). Artifact
  `PokeBank-NX-Gen6Plus-Experimental-b1cfc9e7` (id 13061654379, zip sha256
  `2d5c21d1f8c19ccf3e67ec8e9573cc2c99276a7654cb4cc23bfd34e85c3cd0e9`). 
- **Experimental NRO of `3e41c820` (all 18 Gen VI–IX titles adapter-served; current head)**: CI run 41254557918
  green (host gates incl. leak-checked sanitizers and Clang, devkitA64 build, ELF symbol check for every
  adapter, RomFS verification). Artifact `PokeBank-NX-Gen6Plus-Experimental-3e41c820` (id 13061909874, zip
  sha256 `3f276ef9239248d9780946fe51632853ac91e41d141d0072186c36b04a8c9039`). EXPERIMENTAL, NOT DEVICE TESTED.
- **Scarlet/Violet**: PK9 shares PK8's size, encryption and every offset the shared editor writes, so the
  verified PK8 codec and the shared `Gen8RecordEdits` whitelist are reused; PK9-only reads (language 0xD5,
  Tera 0x94/0x95, scale 0x4A) feed the view. PKHeX's exact SV size rule + SwishCrypto hash, KMyStatus/KParty/
  KBox, TeamIndexes8 + locks (same protection as SWSH), export to `exports/sv`. A Z-A-sized file is refused.
- **Legends: Z-A**: its own module (`Gen9ZaSession`), never SV: Z-A sizes + hash, game 52, Party9a (no count
  byte; leading present slots), 0x198 box stride, DLC revision block (`KSaveRevision`) with the at-rest
  "present" flag set exactly where PKHeX's `WriteSlotBox` sets it; gap bytes and party footers never change.
  PA9 shares PK8 encryption and every editable offset, so the shared rules are reused. Export to `exports/za`.
- PKHeX reference files were fetched from the pinned commit `6501f0ab` for offsets only (not vendored).

## Session 4 milestones (QA D2/D3, 18-title sweep, hardware UX repair)

| Commit | Content | Host evidence |
| --- | --- | --- |
| `5e1d7b7b` | QA D3: bounded, symlink-safe modern discovery (`DiscoveryLimits`, lstat/O_NOFOLLOW + fstat identity, newest-first before reading, truncation notice; explicit assignments never invalidated); 8 MiB read cap (SV DLC saves exceeded 4 MiB). QA D2: independent MiniGmp stress regression (8 threads × 40, 320/320; TSan clean) — the `1509a314` fix is kept, not replaced | `test_modern_discovery_bounds` 61, `test_mini_gmp_stress` |
| `84a4e3a8` | 18-title hardening sweep: 18×18 identity matrix, blocked-op agreement, party release never offered, team/partner protections, export round trip, immutability | `test_modern_title_sweep` 598 (mutation-checked) |
| (this commit) | **Hardware UX repair** (owner finding 2026-11-01): one HOME card per game identity | `test_ui_modern_game_cards_host` 845 (real SaveSelectScreen, scripted console, all 18 titles) |

Hardware UX repair, what changed:
- `loadModernCards` merges an installed Switch title into the one `ModernAssignedFile` card (installed name,
  title id → console cover art and ZR Launch); the separate "(Save File)" / "… File" card is gone. A Switch
  title that is neither installed nor assigned to this profile has no card; each 3DS title has one card.
- A: assigned & valid → shared editor on the assigned save; assigned but missing/invalid/changed → Save
  Instances with the reason first (never the installed save); nothing assigned & installed → the installed
  save is mounted read-only, copied into an app-owned timestamped backup (`backupSaveData`) and that copy is
  opened through the same adapter (`handleModernInstalledView`, source "Installed game (read-only snapshot)");
  nothing assigned on 3DS → Save Instances ("No save is assigned to … yet"). Nothing writes installed savedata.
- X on HOME (new, modern cards only) and in the Games grid → Save Instances under the same card. ZR in the
  Games grid now launches like HOME ZR. 3DS cards report Launch unavailable.
- Root cause found by code reading (not reproduced on hardware): `handleModernView` accepted only READ-ONLY
  registry rows, but every Switch modern id is `NativeSwitch`, so an assigned Switch File card was refused at
  open. It now accepts both.
- HOME title is fitted by measured width (UTF-8 safe, ellipsis) instead of a 27-byte cut.
- Reproduction: with the pre-fix `loadModernCards` the new test fails 212 checks, including two cards per
  installed Switch game named exactly as reported ("Shining Pearl File", "Scarlet File", …).
- Limits: the host cannot mount a console save, so the installed snapshot is proven only up to the
  selection handed to UIManager; the device build links it. 3DS box art has no licence → fallback art remains.

- **Experimental NRO of `7f23ca44` (hardware UX repair; the build to retest with group GUX)**: CI run 41269675592
  green (host gates incl. leak-checked sanitizers and Clang, devkitA64 build, ELF symbol check, RomFS verification).
  Artifact `PokeBank-NX-Gen6Plus-Experimental-7f23ca44` (id 13069750412, zip sha256
  `1f4d7e22e85ee28f681c4198367bd64260de75e9e0687a309488fc1e77d2b0f8`); NRO
  `PokeBank-NX-Gen6Plus-Experimental-7f23ca44.nro` sha256
  `36857430336439417a75e23756d0105fe449458a5fb0e8f2bcd64560ca4e7f7e` (CI SHA256SUMS). EXPERIMENTAL, NOT DEVICE TESTED.

## PKSE 1.2.2 differential audit and port (owner request 2026-11-01)

Upstream `kiasta/PKSE` v1.2.1..v1.2.2 (`fc555b1`, `65dff1d`, `c671aa4`, `474ad9e`, `f4fd79a`) read at source level;
every rule re-checked against pinned PKHeX `6501f0ab` before use.

| # | Finding | Our code | Action |
| --- | --- | --- | --- |
| 1 | Mass cloning on box Move (SWSH/PLA/SV/Z-A): PKSE's whole-box writer reused one "harvested" blank, which after a Move held the moved record's stale bytes | Root cause absent: only staged record ranges are rebuilt, verifier rejects any other byte; Move = clone into a verified blank + remove | `test_modern_storage_integrity` pins it for all 18 titles (mutation-checked) |
| 2 | Untouched slots re-encrypted / gap bytes zeroed | Absent (byte-preserving rebuild) | same test: untouched slots + flat-format bytes preserved |
| 3 | SwishCrypto accepted partial parses / no hash check; BDSP size table | Already fail-closed | truncation / zero / flip refusal for all 18 titles |
| 4 | Battle teams | **Gen VII had no guard** (latent: SAV7 export blocked) | `Gen7BattleTeams.h` (PKHeX BoxLayout7 +0x4C4, locks 0x5C0-6-team, SM demo rule) |
| 5 | Checksums/size | Covered by verifiers + exact-size export checks | storage-integrity test asserts exact size |
| 6 | Hyper Training | Read-only before | **SM/USUM editable** (PK7 0xDE; level 100; never on a 31 IV; a 31 clears the flag; party stats recomputed). LGPE/SWSH/BDSP/PLA/SV/Z-A shown read-only: their records cache battle stats and no PKHeX-pinned per-form base-stat table exists here (`BaseStatsGen89` is species-only, pokemondb-sourced) |
| 7 | Ribbons/marks | none | not started (needs PKHeX-generated ribbon index tables per format) |
| 8 | Alpha | read-only | PLA blocked: Alpha also needs HeightAbsolute/WeightAbsolute (0xAC/0xB0) recomputed from PLA personal height/weight (not pinned). Z-A feasible exactly (PA9 0x23 + Scale 0x4A=255 per PKHeX `EncounterSlot9a`); needs a title-specific Details row (shared Gen V shell is fixed at 18 rows) |
| 9 | Species/stats/moves | blocked on verified tables | unchanged |
| 10 | Favourite folders | — | later |

UI: the modern Details row 3 is now **HyperTrain** (the read-only Experience value moved into the Level row:
"100 • EXP 1059860"), because the shared Gen V shell (`SharedPokemonEditorContract::layoutFor`, shared with Gen I–V)
navigates exactly 18 rows; changing that contract is left to the post-acceptance reconciliation.

## Status per game (host evidence only; NOT DEVICE TESTED)

`docs/gen6-plus/CAPABILITY_MATRIX.md` (generated) is the 12-column table. Summary:

- **X / Y / OR / AS**: full shared workflow incl. Bag and verified Export; Create blocked; party Remove blocked.
- **Sun / Moon / US / UM**: same, **Export blocked** (SAV7 signing gate); Create blocked.
- **Sword / Shield**: linked `main` through the shared surface: editor, Clone/Move/Remove (box), Review,
  Discard, verified Export to `exports/gen8`; Bag/Pokédex read-only via the installed-title reader only;
  Create blocked. Installed-title (console) SWSH unchanged and still read-only.
- **Let's Go Pikachu / Eevee**: linked `main` through the shared surface: editor, Clone, Release, Review,
  verified Export to `exports/lgpe`; Move blocked; AV/CP read-only; Create blocked.
- **Brilliant Diamond / Shining Pearl**: linked SaveData through the shared surface: editor, Clone/Move/
  Remove (box), Review, verified Export to `exports/bdsp`; Create blocked; Bag/Pokédex read-only via the
  installed-title reader only.
- **Legends: Arceus**: linked `main` through the shared surface: editor, box Clone/Move/Remove, Review,
  verified Export to `exports/pla`; Create blocked; held items/Effort Levels read-only.
- **Scarlet / Violet**: linked `main` through the shared surface: editor, box Clone/Move/Remove with team
  protection, Review, verified Export to `exports/sv`; Create blocked; Tera/scale read-only.
- **Legends: Z-A**: linked `main` through its own adapter: editor, box Clone/Move/Remove with team protection,
  Review, verified Export to `exports/za`; Create blocked; Alpha/Plus/Mega read-only.
- Every Gen VI–IX title is now served by a verified shared-contract adapter; none is legacy-reader-only.

No title is accepted on hardware. Gen I–V hardware cells read NOT ACCEPTED (owner rounds were on earlier
candidates; #313 open).

## NRO

- **Experimental CI NRO of `5f84ed8c`**: run 41235138247 (all jobs green: GCC/Clang/ASan/UBSan with
  `detect_leaks=1`, Gen I–V static contracts, devkitA64 build, ELF symbol check incl. Gen VIII, packaged RomFS
  verification). Artifact `PokeBank-NX-Gen6Plus-Experimental-5f84ed8c` (id 13055362433, zip
  sha256 `d8a9a8344158936316d38275faa3de2464ef75d6fbefbe91e9946a1466204315`); the NRO's own SHA-256 is in the
  artifact's `SHA256SUMS`, with `BUILD_IDENTITY` stating `DEVICE_ACCEPTED=false`, `SAV7_EXPORT=DISABLED`,
  `GEN1_5_CANDIDATE=false`. Not downloadable from this container (proxy blocks the artifact store).
- Local devkitA64 link proof of `5f84ed8c`: EXIT 0, all Gen VI–VIII entry points in the ELF; local RomFS lacks
  trainer portraits (proxy) → not distributable; use the CI artifact.
- EXPERIMENTAL, NOT DEVICE TESTED, never a Gen I–V candidate. Owner checklist:
  `docs/hardware/GEN6_PLUS_HARDWARE_CHECKLIST_TEMPLATE.md` (G0, G6, G7, G8).

## Open work (in order)

1. Owner: retest the hardware UX repair with checklist group GUX on the bound experimental NRO (does not
   affect Gen I–V acceptance).
2. PKSE 1.2.2 differential audit and feature port (owner request 2026-11-01): mass-cloning on box move,
   unknown-byte preservation, fail-closed parsing, team protections incl. Gen VII, checksum/size per edit;
   then Hyper Training, ribbons/marks, PLA/Z-A Alpha in the shared editor; favourite folders last.
3. Pinned learnsets/personal tables → unlock Moves/Species (Gen VI/VII) and IV/EV/Nature/Level (SWSH).
4. Licensed artwork only (3DS box art has no redistribution licence; generated fallback cards are used).
5. After Gen I–V acceptance: reconcile onto the final accepted shared UI, re-test, open the separate integration PR.
