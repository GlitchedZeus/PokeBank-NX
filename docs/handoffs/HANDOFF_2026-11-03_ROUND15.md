# Handoff — November 3, 2026 (Round 15 candidate)

**Status: NOT DEVICE TESTED.** Nothing below is hardware-accepted. Hardware acceptance belongs to the owner's test
of the exact NRO and, for the release, issue [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313).

## The one current Gen I–V candidate

| Item | Value |
| --- | --- |
| NRO | `PokeBank-NX-Gen1-5-Candidate-11ea9790.nro` (Round 15) |
| SHA-256 | `8658ef117cdde0dfcfed473918b583961c1fac6cf33f08c48cae2d1f29772fa4` (177,278,641 bytes) |
| Source | `11ea9790ca31c6b8bb5d02c984e3293df7c21f2b`, tree `eb36b0aa2fe8362725dc151d066297ada263726b` |
| Branch / PR | `fix/round15-gen15-repairs`, draft PR #341 → `fix/round14-gen15-repairs` |
| Build | run 41565071576, artifact 13198671780; packaged RomFS PASS (4267 files) |
| Pre-release | [`nro/11ea9790`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro/11ea9790); exact-file audit run 41565982552 PASS |
| Focused retest | [`../hardware/RETEST_11ea9790.md`](../hardware/RETEST_11ea9790.md), offline [`../hardware/round15/QuickQA-Round15.html`](../hardware/round15/QuickQA-Round15.html) |

Stack (oldest first): #332 `fix/round11` → #333 `fix/round12` (R13 `9c301c48`) → #337 `fix/round13` (R14 `d160aa42`,
frozen) → `fix/round14-gen15-repairs` → #341 `fix/round15-gen15-repairs` (R15 `11ea9790`).

## What Round 15 contains

- PR #338 (closed, integrated unchanged): Gen V import lists only `.pk5`/`.ek5`; a first-boot White 2 offers no
  Add/Import; A and X agree.
- Gen I–V Save Instances chooser + manual SD browser + initialized-save rule (`r15/saveinstances`).
- #313 P0s: profile-scoped source assignment, **Forget** (− then Y; only the app's record; the file is untouched), Gen
  I–III need an assignment, and one exact-game gate for A, +, Items dock, Quick Games, touch and the direct editor
  (`r15/sourceassign`; `tests/test_ui_source_assignment_host.cpp`, 1714 checks).
- EXP Review rows (Species / Experience separate); the Gen I untouched record stays byte-exact.
- B2/W2 Regis (Regirock in-game; Registeel/Regice Incomplete) and HG/SS orb reasons.
- R11 guard H38 kept: a mislabelled import file is refused with a reason in Gen I–V.

## Preserved, NOT in the candidate (next week)

| Branch | Content | State |
| --- | --- | --- |
| `recovery/r15-keyboard-20261103` | Shared multilingual keyboard driven by `TextFieldCapability` (43 files) | Stopped mid-implementation; **unvalidated**. Next: finish live validation per format, route every name prompt through it, Gen V Latin/kana/Hangul. |
| `recovery/r15-master-vault-20261103` | Vault phase 1: plan doc, storage core, flag-off "Copy to Vault" | Stopped; **unvalidated**. Keep isolated from the release lane until its tests pass. |
| `recovery/r15-expreview-20261103`, `recovery/r15-prereq-20261103`, `recovery/r15-saveinstances-20261103`, `r15/sourceassign` | Integrated into #341 | Kept as recovery refs. |
| `r15/retroarch` (in `fix/round14`) | R04 launch diagnostics (`last-launch.txt`, hbmenu warning) | R04 **open — needs Switch evidence** (QuickQA R01). |

## Known open items (reproducible)

1. **R04** RetroArch Gen I–III quit crash: collect `last-launch.txt`, the crash report, and how PokeBank was started
   (see `docs/development/RETROARCH_LAUNCH_RETURN.md`).
2. **ED-21** (d160aa42): a nickname/OT gender symbol was accepted where the owner expected a refusal. Verify by
   generation and language with a raw encode/decode check. The text-safety writers refuse unencodable characters;
   whether ♂/♀ is legal depends on the game's table.
3. **One owner per save file:** a save assigned on Profile A cannot be assigned on Profile B ("Matching saves belong
   to another profile"). Owner decision needed on whether profiles may share one file.
4. **DS discovery roots:** Black/White/Black 2/White 2 automatic discovery is verified for the DraStic backup folder
   only; melonDS and RetroArch DS folders are untested. The SD browser lists every DS save and refuses wrong-game
   files only when picked.
5. **`main` exact-NRO audit is red:** `main` is bound to `3634d1e6`, and since #339 it differs from it in 48 build paths.
   Fix: build an NRO from `main` (or the next integration) and rebind. Never loosen the guard.
6. **Release branch build layout:** the `fix/*` lane still uses the older root `Makefile.host.*` fragments. When it
   merges toward `main`, port its test rules into `build-system/host/*.mk` (`PROJECT_RULES.md` §5).
7. **Round 1/2 NRO archives:** publish from the owner's account before 2027-01-14/16
   (`docs/maintenance/OWNER_CONFIGURATION_CHECKLIST.md` §5).

## Exact next step

The owner runs the 27-check QuickQA on `11ea9790` and exports the JSON. Then intake the results onto #313, fix the
FAILs on a `fix/round16-gen15-repairs` branch from #341, finish the keyboard lane, and keep the Vault isolated until it
is validated.
