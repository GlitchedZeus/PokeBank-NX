# PKSE 1.2.0, 1.2.1 and 1.2.2 compared with PokeBank NX for Gen I–V (source level)

**Scope.** This audit compares the commits in Kiasta's PKSE (AGPL-3.0) from v1.1.3 up to the current `version-1.2.2` development branch with PokeBank NX for Gen I–V only. Every row comes from reading both trees, not from release notes. Our earlier save-safety audit covered PKSE 1.1.3 only (`docs/audits/ROUND6_SAVE_SAFETY_PKSE.md`).

## Pinned sources

| Ref | Commit |
|---|---|
| PKSE v1.1.3 (previous audit) | `1133aedffeeffb5862f35162597e249233903597` |
| PKSE v1.2.0 | `55039848bbeeda114484614a9ed2e1d29804dc47` |
| PKSE v1.2.1 | `15b48b69f632fac45ff42fd5213bed8de4e533b7` |
| PKSE `version-1.2.2` (2026-10-26) | `a79fe61a6d6d596bacae09e6299647aebf5c4340` |
| PokeBank NX audited base | #307 head `d6cc878a` (code of the Round 7 candidate is `e88957f2`) |
| Fixes | branch `wip/pkse-1.2.x-gen15-audit` (forked from `d6cc878a`) |

**The Round 7 candidate is untouched.** Nothing here changes `e88957f2`, its NRO, `docs/qa/CANDIDATE_UNDER_AUDIT.env` or the #307 history. PR #314 (Codex's legality work) is not touched.

## Owner contributions credited, not recreated

- **Left-stick navigation** came upstream from PokeBank NX: PKSE PR #120 (`acc81eb`) is GlitchedZeus's. PokeBank NX has had it in `include/UI/NavigationRepeat.h` (wall-clock repeat) since before that. **Already implemented.**
- **On-screen keyboard.** PokeBank NX has its own `KeyboardDialog`. PKSE's `c4b7efc` behaviour (a held B stops at an empty field; the character limit is enforced) is already in `src/UI/Dialogs/KeyboardDialog.cpp:426-438, 576, 638`. **Already implemented.**
- **Standalone Pokémon-file import/export** is the owner's own work, merged upstream as PKSE PR #128 (`e00dfa0`, `7152194` …). It is not a feature to recreate from PKSE. PokeBank NX has no `PokemonFile` code today (no `.pk3`, `.ek4` or `.pk5` handling under `src/` or `include/`). If the owner ports their PR #128 code, the upstream follow-up fixes listed under F-01 should come with it.

## Classification key

| Label | Meaning |
|---|---|
| ALREADY IMPLEMENTED | We have equivalent behaviour. |
| NEEDS FIX | A real PokeBank NX defect. |
| NOT YET INTEGRATED | A compatibility or feature gap. |
| NOT APPLICABLE | Gen VI+, or a deliberate policy difference. |

Severity is H/M/L. **FIXED (wip)** means the fix is implemented on the WIP branch with a regression test that fails on the old code.

## 1. Gen II detection, Japanese saves and padded saves

| Upstream | Behaviour | PokeBank NX | Class | Sev | Change / test |
|---|---|---|---|---|---|
| `0c993bc` | Try all four GSC layouts (international/Japanese × GS/C) at either file size, so a Japanese GSC in a 32 KiB file opens | `src/Integration/Gen2/Gen2ReadOnlySave.cpp` `kLayouts` loop. Checksum, structural probe and ambiguity refusal are all stricter than upstream. Tests: `test_gsc_gen2_adapter.cpp` (Japanese GS at 0x8000, Japanese Crystal at 0x10000) | ALREADY IMPLEMENTED | – | – |
| `9a9a731` | GSC padded to 128 KiB is unwrapped, trying 32 KiB before 64 KiB | Was refused with `WrongSize` | NOT YET INTEGRATED → **FIXED (wip) `cd5eabfe`** | M | `splitPaddedSave`: only a 0x20000 file with a uniform 0x00/0xFF tail. Recorded as `Metadata::paddingSize`, separate from the RTC footer. The export guard now checks every byte after the payload. Discovery lists 0x20000. Tests: adapter (0x00 and 0xFF fills, padded Japanese Crystal, stray-byte and 96 KiB refusals), staged editor round trip, discovery, and the fuzz invariant |
| `9a9a731` comment | "A Japanese cartridge's SRAM is 32 KiB" | Japanese Crystal keeps its gender byte at 0x8000 (`kJpnC`), so a 32 KiB Japanese Crystal file is refused | NOT APPLICABLE (deliberate) | – | Japanese Crystal uses 64 KiB SRAM, so a 32 KiB file has lost bank 4. Upstream would open it without its gender. **Pinned by a test.** |
| `9a9a731` | Gen II pouch table: Key Items capacity 26 | Writer 25; reader accepts a count up to 26 (`Gen2ReadOnlyInventory.cpp:151`) | NOT APPLICABLE (do not port 26) | L | Upstream's 26-entry writer would overwrite the Balls count byte. Our writer's 25 is right. Tightening the reader to 25 is optional. |

## 2. Emulator save compatibility (VBA-M FireRed and others)

| Upstream | Behaviour | PokeBank NX | Class | Sev | Change / test |
|---|---|---|---|---|---|
| `0c993bc` | Gen III stops requiring the 0x08012025 sector signature. Upstream itself calls this only the "likely" cause of a refused VBA-M FireRed file | `Gen3SaveValidation.h:76-92`: signature, counter and checksum are required per slot, and an invalid slot falls back to the other | NOT APPLICABLE (deliberate) | – | The game's own loader requires both signature and checksum. Relaxing them would let a torn newer slot win over the good older one, which is unsafe for export. The upstream root cause is unproven. |
| `292fd5e` | RTC footers on Gen II/III | `include/Save/RtcFooter.h`, Gen II `splitPayload`, discovery and the Gen III Pokémon editor | ALREADY IMPLEMENTED | – | – |
| `292fd5e` (container) | Footered Gen III save in the **Bag and Trainer** editors | Both required exactly 0x20000 while discovery and the Pokémon editor accepted a footer, so they were unavailable on such saves | NEEDS FIX → **FIXED (wip) `e13ced8e`** | M | Same `splitRtcPayloadSize` rule. The footer is never edited. The Bag + Pokémon merge works over 0x20000 and re-appends the footer. Test: `test_gen3_trainer_edit` `rtcFooterExport` (RS, E, FRLG) |
| `292fd5e` | libretro VBA Next / Beetle GBA 0x22000 (flash + 0x2000 EEPROM) | Refused | NOT YET INTEGRATED | M | Backlog B-03 |
| `292fd5e` | TGB Dual +4, MiSTer +0x200, Meteor +4, no$gba / BizHawk headers, YSMenu trailer | Gen I needs exactly 0x8000; Gen IV exactly 0x80000 | NOT YET INTEGRATED | L | Backlog B-06 |
| `292fd5e` | DeSmuME `.dsv` footer | Gen IV/V discovery reads the last 40 bytes; export keeps the original `.dsv` verbatim | ALREADY IMPLEMENTED | – | Optional: a real 0x7A-footer fixture |
| `d3e3e29` | All 12 Switch FireRed/LeafGreen title ids (JP/EN/FR/IT/DE/ES) | Only the English ids (`include/Enums/GameVersion.h:84-85`, `src/Games/GameIdentity.cpp:35-36`) | NOT YET INTEGRATED | M | Backlog B-02. It touches `GameIdentity` and backup-folder identity, so it needs owner approval. The ids are pinned in upstream `GameVersion.h:163-174` at `a79fe61`. |
| — | 64 KiB "half" Gen III file | Refused, as upstream does | NOT APPLICABLE | – | Wrong-flash-size emulation mirrors one slot over the other |

## 3. Bag capacity, quantity and pouch navigation

Our per-pouch numbers match PKHeX (`6501f0ab`) and PKSM-Core (`aa22d7a`) wherever the two trees differ.

| Upstream | Behaviour | PokeBank NX | Class | Sev | Notes |
|---|---|---|---|---|---|
| `9a9a731` | L/R changes pouch and wraps | Gen I–III `ClassicInventoryUIModel.cpp:43-49`; Gen IV `TrainerViewScreenBase.inc`; Gen V `Gen5BagScreen.h:109-113` | ALREADY IMPLEMENTED | – | Optional Gen IV eight-pocket wrap test |
| `9a9a731` | Real pouch capacity; Add is refused when full | Gen I/II/III/IV/V writers all refuse a full pouch | ALREADY IMPLEMENTED | L | Gen IV/V full-pouch fixtures are missing (backlog T-01) |
| `9a9a731` / `d2ad323` | RSE stack limit 999 upstream | Ours is 99 (`ItemQuantityPolicy.cpp:206`), which equals PKHeX `PlayerBag3RS/3E` | NOT APPLICABLE (upstream is wrong) | – | Do not port |
| `d2ad323` | HMs 1 (Gens 2–4); key items 1 | `ItemQuantityPolicy.cpp:193,230,311,314`; `Gen4BagCatalog.h:64` | ALREADY IMPLEMENTED | – | – |
| `d2ad323` | Gen I key-item list (pret `key_items.asm` plus HM01–05, 36 items) excludes Exp. All (75) | Our `kG1Keys` includes 75, so it is capped at 1. pret and PKHeX allow 99 | NEEDS FIX (policy) | L | Backlog B-08: owner decides (stricter, not unsafe) |
| `d2ad323` | FRLG key pouch adds Meteorite (280), from PKHeX after our pin | Missing from `kG3KeysFRLG` | NOT YET INTEGRATED | L | Backlog B-09: bump the PKHeX pin first |
| `d2ad323` | "Add All Items" bulk operation | – | NOT APPLICABLE (policy) | – | Key items or HMs added early can break story events. A safe subset ("top up existing stacks to the cap", no new slots, no key items or HMs) is backlog B-10 |
| `620c5f6` | `isNew`/`isFavorite` initialised | Already `= false` (`include/Trainer/Inventory.h:10-11`); only Gen 9 reads them | ALREADY IMPLEMENTED / NOT APPLICABLE | – | – |

## 4. Gen III/IV/V Shiny, PID and gender consistency

| Upstream | Behaviour | PokeBank NX | Class | Sev | Change / test |
|---|---|---|---|---|---|
| `620c5f6` | `findPID`: build a PID that keeps shiny, gender, nature and the ability bit | `include/Conversion/Gen3PidSearch.h`; Gen IV `Pokemon4Mutable.cpp`; Gen V `Gen5PidCoordination.h`. Covered by the reroll-mechanics lane (Gen 3/4 "prior defect" 100-trial tests, `gen4Traits`, `gen5Record`) | ALREADY IMPLEMENTED | – | – |
| `620c5f6` | Make Shiny never fails silently | Gen IV returns false and posts a status; Gen III re-checks; Gen V `withShiny5` cannot fail | ALREADY IMPLEMENTED | – | – |
| `620c5f6` | Stat editor no longer rerolls the PID | The shared surfaces never touch the PID. A legacy dialog still calls `regeneratePID` (`TrainerViewScreenBase.inc:3415-3418`); it does nothing for Gen IV/V and only acts on an already inconsistent Gen III record | ALREADY IMPLEMENTED (one leftover call) | L | Backlog B-11: remove the call |
| `620c5f6` invariant ("never a PID that contradicts stored gender") | **Gen V species change** kept the PID while the gender ratio changed. A female Pikachu (PID low byte 0x50) could become an Eevee whose PID reads male. Reproduced on host | `Gen5StagedPokemonRecord.h` (Species), `Gen5SharedScreenState::speciesRefusal`. The fuzzer exempted ratio changes | NEEDS FIX → **FIXED (wip) `2ab98df6` + `460f2f96`** | M | Refused in both paths when the change would *create* the contradiction. A record that already disagrees is left as found, neither refused nor silently repaired. The PID is never rewritten. Fuzzer exemption removed. Test: `test_gen5_species_pid_gender` (incompatible, compatible female and male, shiny, and pre-existing-mismatch cases) |

## 5. Gen III Egg nickname

| Upstream | Behaviour | PokeBank NX | Class | Sev | Change / test |
|---|---|---|---|---|---|
| `d3e3e29` | Egg read as garbled Japanese; `setEgg` set only the flag | `Pokemon3FRLG.h:243-253` sets the egg bits, language Japanese and タマゴ. Test: `test_gen3_staged_pokemon_editor` | ALREADY IMPLEMENTED | – | – |
| `9a9a731` | `setNickname` writes the egg placeholder while the egg bit is set (PKHeX `PK3.Nickname`) | The Egg's Nickname and Language were editable through the staged editor, writing a real name into a language-1 Egg | NEEDS FIX → **FIXED (wip) `9c275755`** | L–M | The entity keeps the placeholder. The staged editor refuses a changed nickname or language on an Egg; restating the unchanged values is allowed. `canStoreNickname` is unchanged because our OT path uses it. Test: FRLG and Emerald staged refusal with the bytes unchanged |
| `d3e3e29` | A transferred Unown keeps its letter (Gen III letter comes from the PID) | `Convert.cpp` `unownForm`; reroll lane `gen3Unown`; conversion golden F06 | ALREADY IMPLEMENTED | – | – |

## 6. Left stick, held repeat and keyboard limits

| Upstream | Behaviour | PokeBank NX | Class | Sev | Notes |
|---|---|---|---|---|---|
| `acc81eb` (owner's PR #120) | Left-stick navigation with repeat | `include/UI/NavigationRepeat.h:73-139`; `tests/test_navigation_repeat.cpp` | ALREADY IMPLEMENTED | – | Ours repeats on wall-clock time |
| `f1c95e8`, `df0ddd8`, `30e7264` | Held L/R repeats only where it navigates | No extra repeat mask; L/R are edge-only | NOT YET INTEGRATED | L | Backlog B-12. Detail-page L/R actions must never repeat |
| `c4b7efc` | Character counts match the game's limits | Gen II OT prompts offered 10 while the staged encoder allowed 7 (`kInternationalTrainerNameMax`), so 8–10 characters were typed and then refused | NEEDS FIX → **FIXED (wip) `0489dd20`** | L | Test: `tests/test_gen2_ot_prompt_limit.py` |
| — | Japanese 5-character limits for Gen I/II prompts | Gen III enforces Japanese limits at commit; Gen I/II prompts do not drop to 5 | NOT YET INTEGRATED | L | Backlog B-13 (Japanese Gen I/II editing is read-only or limited today) |

## 7. Favourite folders and remembered browser folders

| Upstream | Behaviour | PokeBank NX | Class | Sev | Notes |
|---|---|---|---|---|---|
| `f4fd79a`, `c671aa4` | File-browser favourites, last folder, `settings.cfg` keys | We have no save or import file browser: sources come from bounded discovery plus the launch-binding picker | NOT APPLICABLE today | – | Only relevant if a browser is added |
| `f4fd79a` | `loadSettings` line buffer 128 → 1024 | `src/Utils/Settings.cpp:28` `char line[128]` | NEEDS FIX (latent) | L | Backlog B-14: every current key is short; a future path key would split |
| `c671aa4` | `directoryExists("")` must be false | `PokeBankPaths.h:119-122` has no empty guard; no current caller passes "" | NOT APPLICABLE today (latent) | L | Backlog B-15 |

## 8. Safety for future persistent Banks

| Upstream | Behaviour | PokeBank NX | Class | Sev | Change / test |
|---|---|---|---|---|---|
| `620c5f6` | An unreadable `bank.dat` is never a first run | `Bank::load` treated any `readAllBytes` failure as "no bank yet", ran the legacy migration and opened empty. The data stayed on disk, but the user saw an empty Bank | NEEDS FIX → **FIXED (wip) `080d7c8b`** | **H** | `BankLoadPolicy::decide`: only an absent file with no preserved generation is a first run. An existing file that can't be read is quarantined (`.unreadable.N`, or write-blocked). Uninspectable paths are write-blocked with no migration |
| `620c5f6` | Interrupted save recovers | `Bank::save` already uses `DurableFile::replace` (unique temp, fsync, `.previous.N`), which is stronger than upstream's `.tmp` + rename. **But `load()` never called `recoverMissingTarget`**: a power cut between the two renames left no `bank.dat`, which then opened as a first run | NEEDS FIX → **FIXED (wip) `080d7c8b`** | **H** | A missing target with a `.previous.N` that passes `Bank::validateStorageImage` is restored; if none validates, the Bank is write-blocked. No generation is ever deleted. Test: `test_bank_load_policy` (real files, GCC and ASan) |
| `620c5f6` | `readAllBytes` uses nothrow `new[]` with matching `delete[]` | `src/Utils/ReadAllBytes.cpp:17`; Bank uses `delete[]` | ALREADY IMPLEMENTED | – | – |

## 9. Standalone Pokémon-file import/export (owner's PR #128) and its follow-ups

| Upstream | Behaviour | PokeBank NX | Class | Sev | Notes |
|---|---|---|---|---|---|
| PR #128 (`e00dfa0`, `7152194` …) | Native `.pkN` import/export (owner-authored) | Not present in PokeBank NX | NOT YET INTEGRATED (owner's own code; **do not recreate from PKSE**) | M | **F-01**: if ported, include `fa06a1b` (PKHeX plaintext convention and safe normalisation), `aa4b72e` (Gen I box record with no party-level cache), `18c2555` (PokemonFile rework) and `620c5f6` (`.ek3`/`.ek4`/`.ek5` read encrypted first, then plain; export plain only). Tests: round-trip every sample as `.pkN` and `.ekN`, plus a Gen I box record with a 0 level byte. Imports must land in a staged copy, never in an original save |

## Deliberately not adopted

| Upstream | Why not |
|---|---|
| Direct source writes / restore to game (`FileUtilities` restore, `injectToGame`) | Writeback stays disabled at compile time; originals are never written |
| `d3e3e29` "Auto-Legalize Unsupported Transfers", Trade Evolve, Gen 1/2 → Poké Transporter conversion | No automatic legalisation; cross-generation True Move stays locked |
| `7125b79` / `d3e3e29` legality Layer 3 | Legality belongs to Codex's PR #314. New evidence goes through a handoff (below), never an engine edit here |
| `d2ad323` Add All Items (full version) | An unsafe bulk operation |
| PKSM bank import, the 1.2.2 Alpha toggle, Hyper Training, Gen VI–IX fixes (Let's Go, Legends: Arceus, BDSP, SV, Z-A, Gen 6/7 BEEF) | Gen VI+ or out of scope |

## Legality handoff for #314 (evidence only; no engine edit)

- **PID gender in Gen V species identity.** A Gen V record's gender must equal `genderFromPid5(pid, ratio)` for variable ratios; the editor now enforces this. If #314 has a gender/PID consistency check, it should flag records whose stored gender disagrees with the PID under the current species' ratio (PKHeX does).
- **Gen III Eggs.** An Egg's nickname is タマゴ and its language is Japanese until it hatches (PKHeX `PK3.Nickname`). A legality check should flag a Gen III Egg with any other name or language.

## AGPL and attribution

- No PKSE source file was copied. Each fix re-implements the behaviour on PokeBank NX's own code paths (`DurableFile`, `Gen5PidCoordination`, the staged editors) and cites the upstream commit SHA in its comment and commit message.
- The Gen III egg-name rule follows PKSE `9a9a731` and PKHeX `PK3.Nickname`.
- PokeBank NX stays under the same AGPL-3.0 licence (`LICENSE`).

## Backlog, by priority

| # | Item | Sev | Notes |
|---|---|---|---|
| B-02 | Non-English Switch FireRed/LeafGreen title ids | M | Owner approval: `GameIdentity` and backup-folder identity change |
| B-03 | libretro GBA 0x22000 container | M | Accept only with a valid 0x20000 slot; keep the 0x2000 EEPROM tail verbatim |
| F-01 | Port the owner's PR #128 with its follow-up fixes | M | Owner decision; staged import only |
| T-01 | Gen IV/V full-pouch refusal fixtures | L | Tests only |
| B-06 | Gen I and other emulator footers (TGB, MiSTer, Meteor) | L | Fixtures per container |
| B-08 | Gen I Exp. All cap 1 vs pret/PKHeX 99 | L | Owner decision |
| B-09 | FRLG Meteorite key item | L | Bump the PKHeX pin first |
| B-10 | "Top up existing stacks" (safe subset of Add All Items) | L | No new slots, key items or HMs |
| B-11 | Remove the leftover `regeneratePID` call in the legacy stat dialog | L | Gen III inconsistent-record case |
| B-12 | Context-scoped held L/R repeat | L | Never on action bindings |
| B-13 | Japanese 5-character limits in Gen I/II prompts | L | – |
| B-14 | `settings.cfg` long-line read | L | – |
| B-15 | `directoryExists("")` guard | L | – |
| UI-01 | Lock the Gen III Egg Nickname/Language rows (the backend already refuses) | L | – |

## Status

- Every fix is **NOT DEVICE TESTED**. No new hardware acceptance is claimed.
- The Round 7 NRO (`e88957f2`) is unchanged. These fixes belong to a future round, once the owner approves merging `wip/pkse-1.2.x-gen15-audit`.
- Merging into #307 must be coordinated. #314's history touches `Pokemon3FRLG.cpp`, `Gen3SharedPokemonSurface.inc` and the Gen II reader, so expect textual conflicts there.
