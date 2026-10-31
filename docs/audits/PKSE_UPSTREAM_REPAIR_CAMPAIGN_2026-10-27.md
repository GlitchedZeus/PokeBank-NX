# PKSE upstream integration and Gen I–V compatibility repair (2026-10-27)

This report covers the repair campaign on the independent QA findings of 2026-10-27 (`PokeBank_NX_Independent_QA_2026-10-27.md`) and the relevant fixes from PKSE 1.2.1 and the 1.2.2 development branch. The source-level gap matrix is in [`PKSE_1_2_X_GEN15_GAP_MATRIX.md`](PKSE_1_2_X_GEN15_GAP_MATRIX.md).

**Status: NOT DEVICE TESTED.** Nothing here has run on a Switch or an emulator. No hardware acceptance is claimed.

## Identity and scope

| Item | Value |
|---|---|
| Starting application SHA | #307 `d6cc878a9652b78a6cdfd8a31cdb5797172d6c9d` (unchanged; remote re-verified before work) |
| Branch | `wip/pkse-1.2.x-gen15-audit` (forked from `d6cc878a`; forward-only, never force-pushed) |
| Round 7 hardware candidate | code `e88957f22a5b6432b56f91ef62e1f1938d391a96`, NRO SHA-256 `9e1b6a31…3b5`. **Untouched**: not rebuilt, re-identified or re-audited |
| PR #314 (Codex legality) | `892b0622…` untouched; evidence handed off below |
| PKSE references | v1.2.1 `15b48b69`, `version-1.2.2` `a79fe61a` (re-verified as current) |
| Native-format references | pret pokefirered `ce6406d8`, pokeemerald `731ad5bf`, pokecrystal `5c877ef5`; PKHeX master `8e3eb3d4` |

No original save is written. There is no injection or writeback. Cross-game true Move stays locked. No Gen VI–IX work was done.

## Q01–Q09 resolution

| # | Initial state at `d6cc878a` | Reproduction | Fix | Files | Tests | Remaining limits |
|---|---|---|---|---|---|---|
| **Q01** Bank unreadable vs missing (P0) | **Confirmed.** Any `readAllBytes` null ran migration and opened an empty Bank. A missing target after an interrupted save was also treated as a first run. | Production `Bank` on host with a fault-injecting `readAllBytes` | `BankLoadPolicy` plus `Bank::LoadOutcome`. Only a genuinely absent file (no folder, no `.previous`, no set-aside copy) is a first run. Every other case keeps the original bytes and blocks `save()` with a reason shown when Storage opens: read failure (left in place), corrupt (unique `.unreadable.N`), truncated, damaged records, newer layout, uninspectable. An interrupted save is restored from a validating `.previous.N`. The harness also caught that a brand-new install (no Bank folder) would have been blocked; fixed. | `include/Trainer/Bank.h`, `include/Trainer/BankLoadPolicy.h`, `src/Trainer/Bank.cpp`, `src/UI/TrainerViewScreenBase.inc` | `test_bank_fault_injection` (11 scenarios on the real `Bank`); `test_bank_load_policy` (real files plus wiring) | Running as root, a permission failure is simulated as an open failure. Recovering from a set-aside `.unreadable.N` is manual: move the file out of `banks/legacy-pkse/`. A recovery UI needs owner approval. |
| **Q02** 32 KiB Japanese Crystal (P1) | **Confirmed.** The optional gender at `0x8000` was required. | Japanese Crystal fixture truncated to `0x8000` | Gender is optional: read only in bounds, otherwise unspecified, never invented. All other checks are unchanged. Japanese Gen II stays read-only. | `src/Integration/Gen2/Gen2ReadOnlySave.cpp` | `test_gsc_gen2_adapter`: 32 KiB accepted with no gender; 64 KiB reads gender; bad checksum refused; Japanese GS stays GS; staged editor refuses Japanese | No real Japanese Crystal 32 KiB sample has been checked |
| **Q03** 128 KiB padded Gen II (P1) | **Confirmed.** Refused as `WrongSize`. | Padded fixtures | Strict envelope: only a `0x20000` file whose tail after a 32 KiB core (tried first), then a 64 KiB core, is all `0x00` or all `0xFF`. Stored as `paddingSize`, separate from RTC. Every byte after the payload is carried verbatim, and the export guard checks it unconditionally. Padding plus a footer is refused. | `Gen2ReadOnlySave.{h,cpp}`, `Gen2StagedEditor.cpp`, `RetroArchGSCDiscovery.cpp` | Both fills; both core sizes; RTC at 32 and 64 KiB; padding plus RTC refused; stray byte refused; 96 KiB refused; staged round trip with padding kept; discovery; fuzz invariant | Which emulators produce this is unverified (no real sample) |
| **Q04** Localized FRLG Switch titles (P1) | **Confirmed.** Only the English IDs were known; the other ten were dropped at discovery. | Table of all 12 IDs | Ten exact descriptors (`firered_switch_jpn/fra/ita/deu/esp`, `leafgreen_switch_*`) with their own backup namespaces. English IDs are unchanged, so existing backups stay in place. `switchReleaseFamily()` is used for behaviour only. Durable true Move stays limited to the validated English pair. | `GameVersion.h`, `GameIdentity.{h,cpp}`, `SaveSelectScreen.cpp` | `test_frlg_switch_releases`: all 12 IDs map to game, language and family; unique backup roots per release and profile; unknown and near-miss IDs refused; `test_game_identity` updated to 42 | Not run against real localized installs. Language display uses PKSE's PKHeX LanguageID table. |
| **Q05** Gen III sector signature (P1) | **Differential, not a defect.** We require the signature; PKSE dropped it. | Signature-free and torn fixtures | **Kept, source-backed.** pret pokefirered `src/save.c` counts a sector only with the signature and a matching checksum, and writes the signature's low byte last as the commit marker. A torn newer slot therefore must not win. Documented at the check. | `Gen3SaveValidation.h` (comment only) | `test_gen3_sector_signature_policy`: torn newer slot falls back to the older slot; empty slot ignored; signature-free in both slots refused; bad checksum, duplicate ID, out-of-range ID and counter mismatch refused; family detection | The real VBA-M sample behind upstream #127 is still needed. Its actual cause is unknown. |
| **Q06** FRLG Meteorite 280 (P1) | **Confirmed.** It could be read but not edited or removed. | Policy and staged fixture holding Meteorite | Added to the FRLG key list only. Sources: pret `items.json` (Key Items), PKHeX `ItemStorage3FRLG.Key`. Unique, with the existing key-item warning. | `ClassicInventoryCatalog.cpp` | FR and LG: catalog and policy; existing item kept; 2 refused; wrong pocket refused; in-place removal; reparse; re-add | – |
| **Q07** Inventory byte fidelity (P0/P1) | **Confirmed.** Gen III rewrote every vacant slot. Both Gen II writers zero-filled each list, even for a same-value edit. | Fidelity fixtures with distinctive vacant and stale bytes. Both new tests fail on the old code at the first no-op. | Native semantics. Gen III (pret `item.c`): in-place quantity; add to first `ITEM_NONE`; in-place removal with no compaction. Gen II (pret `items.asm`, shared `Gen2NativeItemList`): one-byte quantity; append; shift-down removal that keeps stale tail bytes. Pending changes are compared with the opened save. | `Gen3StagedInventoryEditor.cpp`, `Gen2StagedEditor.cpp`, `Gen2UniversalInventory.cpp`, `Gen2NativeItemList.h` | `test_gen3_inventory_byte_fidelity` (RS/E/FR/LG), `test_gen2_inventory_byte_fidelity` (GS/C, both writers, Key Items): same-value no-op; slot-only diff; idempotence; revert; add and remove allowlists; reparse; discard | Gen I already matched native semantics and is unchanged. Gen IV/V writers already edit in place. The effect on the game of the old normalised bytes was never shown; this is a fidelity fix. |
| **Q08** Japanese Gen II editing (P2) | Known limitation, not a bug | Staged editor refuses Japanese | **No change; read-only kept.** Pinned by test. | – | `test_gsc_gen2_adapter` | See the recommendation below |
| **Q09** Evidence gate (P1) | Verification gap | – | Added Bank fault injection, byte-diff allowlists, envelope matrix, a 12-title identity table, a pouch-capacity boundary, and preservation of existing unreleased held items | tests listed above | – | Still needed: a real save corpus (licensed or anonymised) and device or emulator sign-off |

### Earlier fixes on this branch, also from the PKSE comparison

| Commit | Fix |
|---|---|
| `080d7c8b` | First Bank load policy (superseded and extended by `159b6c97`) |
| `2ab98df6`, `460f2f96` | Gen V species change can no longer create a PID/stored-gender contradiction |
| `9c275755` | Gen III Egg keeps タマゴ and Japanese language |
| `e13ced8e` | Gen III Bag and Trainer editors accept an RTC footer and keep it verbatim |
| `cd5eabfe` | Gen II 128 KiB padding |
| `0489dd20` | Gen II OT prompts offer the encoder's 7-character limit |

## Phase 3: PKSE 1.2.1 fixes rechecked on live routes

| # | Item | Result |
|---|---|---|
| 1 | Gen IV/V Make Shiny is consistent or refused clearly | **ALREADY IMPLEMENTED AND VERIFIED**. Gen IV `rerollPid` posts a status on failure; Gen V `withShiny5` cannot fail. Reroll-mechanics lane (100-trial prior-defect tests), all host tests green. |
| 2 | PID-linked gender, nature, ability and shiny stay consistent | **CONFIRMED DEFECT AND FIXED** for Gen V species change (`2ab98df6`, `460f2f96`). Other paths already verified by the reroll lane and `fuzz_gen5_edits`; the fuzzer's ratio-change exemption was removed. |
| 3 | IV/EV edits never clear Shiny | **ALREADY IMPLEMENTED AND VERIFIED**. The shared rerolls never touch the PID. The legacy dialog's `regeneratePID` returns early when the PID already satisfies every trait and always pins Shiny (`Pokemon3FRLG::rerollPID`). |
| 4 | Discarded or cancelled edit leaves no false unsaved indicator | **CONFIRMED DEFECT AND FIXED** in the Gen III Bag: change tracking now compares with the opened save (`d381ec31`; 3→4→3 test). Gen I/II already did. |
| 5 | Gen III Egg name | **CONFIRMED DEFECT AND FIXED** (`9c275755`). |
| 6 | Native pouch limits, Gen I–V | **ALREADY IMPLEMENTED AND VERIFIED** (matches PKHeX; upstream's RSE 999 and GSC Key 26 deliberately not ported). Coverage added: Gen V capacity boundary (`b80137df`). **Coverage still incomplete:** Gen IV full-pocket fixture. |
| 7 | Existing unsupported held items preserved | **ALREADY IMPLEMENTED; COVERAGE ADDED** (`b80137df`): Safari Ball and Berry Juice survive an Edit → Keep and a reparse; choosing them anew is refused. |
| 8 | Source, backup and export round trips | **ALREADY IMPLEMENTED AND VERIFIED** (`VerifiedStagedExport`, source guards, backup snapshot tests). This campaign added original-unchanged assertions to each new test. |
| 9 | Standalone `.pk1`–`.pk5` and `.ekN` | **NOT APPLICABLE / BLOCKED BY MISSING FEATURE**. PokeBank NX has no Pokémon-file code. The owner's PKSE PR #128 is not integrated. If ported, bring PKSE `fa06a1b`, `aa4b72e`, `18c2555` and `620c5f6` (`.ekN`) with it. |

## PKSE adopted and deliberately rejected

| Adopted behaviour (re-implemented, not copied) | Upstream |
|---|---|
| Unreadable or interrupted bank is never a first run | `620c5f6` |
| Gen II padded saves, smallest core first | `9a9a731` |
| Japanese Crystal gender optional; all GSC layouts at either size | `0c993bc` |
| Twelve FRLG Switch title IDs | `d3e3e29` (table at `a79fe61a`) |
| FRLG Meteorite key item | `d2ad323` |
| Gen III Egg placeholder name | `9a9a731`, `d3e3e29` |
| Never a PID that contradicts stored gender | `620c5f6` (enforced as a refusal) |
| RTC-footered emulator saves | `292fd5e` |

| Rejected | Why |
|---|---|
| Drop the Gen III sector signature (`0c993bc`) | The games require it; it would load torn slots |
| RSE stack limit 999; GSC Key Items capacity 26 (`9a9a731` / `d2ad323`) | Upstream is wrong against PKHeX and the byte layout |
| Direct source write and restore to game | Writeback stays disabled |
| Auto-Legalize, Trade Evolve, Gen 1/2 Transporter conversion, legality L3 | No automatic legalisation; cross-generation conversion locked; legality is #314's |
| Add All Items (`d2ad323`) | Unsafe bulk operation; see the proposal below |
| Alpha toggle, Hyper Training, Gen VI–IX fixes | Out of Gen I–V scope; future-generation backlog |

## Gen I–V input and output fidelity (source-backed)

| Gen | Input envelopes accepted | Staged edits change only | Output |
|---|---|---|---|
| I | Exact 0x8000 (+RTC through discovery) | Inventory: entries and terminator, matching the game. Trainer: name and money. | Verified export; strict reparse; original immutable |
| II | 0x8000 / 0x10000; known RTC footer; uniform 128 KiB padding. Japanese read-only, including 32 KiB Crystal | Native list ops (one quantity byte; append; shift-remove with stale tail kept); checksums and mirror at finalize | Footer and padding verbatim (guarded) |
| III | 0x20000 (+RTC) in every editor; both slots validated as the game does | Item slots in place (game semantics); touched sectors' checksums; inactive slot never written; footer never written | Footer re-appended verbatim after the sector-wise merge |
| IV | 0x80000; DeSmuME footer read | Native in-place pouch edits (unchanged here) | `edited.sav` plus verbatim `original_backup` |
| V | SAV5 copies; DeSmuME footer | In-place bag (`Gen5StagedBag`); PID edits coordinated | Working-copy export, Gen V backup listing |

## Recommendation on Japanese Gen II editing (Q08)

Keep Japanese Gen II **read-only** for this milestone. Editing needs a separately validated Japanese serializer:

- Japanese layout offsets;
- 5-character kana names;
- 9 boxes of 30;
- the Japanese Crystal bank-4 gender, which is absent from 32 KiB files;
- checksum and mirror placement;

all proven by native-byte round trips on real Japanese saves. Removing the region check would apply international offsets to Japanese saves. Read-only Japanese support, now including 32 KiB Crystal, is safe today.

## Feature proposal (deferred, needs owner approval): Add All Items

Study of PKSE `d2ad323` (per-item caps from PKHeX `GetMaxCount`; Key Items off by default; HMs 1 in Gens 2–4). A PokeBank NX version would be:

- a staged, reviewable "Top up" per pocket: existing stacks raised to `quantityRule().maximum`; no new slots unless the owner asks for "add missing";
- Key Items excluded always, or with a per-item story warning if the owner insists;
- uses the native slot writers above, so unrelated bytes are kept;
- refuses cleanly when a pocket is full;
- one Review entry with a full rollback via Discard;
- tests: slot count and order unchanged, every stack ≤ cap, byte allowlist, full-pocket refusal.

## Favourite and remembered folders (PKSE `f4fd79a`): not implemented

PokeBank NX has no free file browser: saves come from bounded discovery and launch bindings. Favourites need that browser first, which is a separate UI feature for owner approval. If built, it should:

- persist through the existing atomic settings writer;
- canonicalise paths;
- refuse an empty path (PKSE `c671aa4`);
- keep separate contexts for saves and imports;
- use our visual language.

Latent items to bring along: `Settings.cpp` 128-byte line buffer (B-14) and `directoryExists("")` (B-15).

## Owner feedback for the older 26186464 NRO (kept separate)

Two owner reports exist for that build:
- 49 PASS / 9 FAIL / 1 NOT TESTED, recorded in `docs/hardware/ROUND6_RESULTS_26186464.md`;
- 54 PASS / 4 FAIL / 1 NOT TESTED, an uploaded report that is **not in this session or repository**. Add it unchanged as its own file; never merge the two.

Round 7 (`e88957f2`) is awaiting owner testing. The following are code-status findings only (read-only review), not device results:

| # | Item | Code status at this branch | Expected behaviour |
|---|---|---|---|
| 1 | Gen IV/V Bag Help clipping | Still present: fixed 700 px popup with no wrap (`ScreenChrome.h:800-813`); long rows (`InventoryHelp.h:30-41`) estimated past the 640 px usable width. The layout test checks overlap, not containment. | Wrap or shorten; add a containment check |
| 2 | Gen I–III Bag Help sizing | Still present: full 900×610 panel with its own geometry (`ClassicInventoryOverlay.cpp:952-1058`) | Content-sized, using the Gen IV/V modal |
| 3 | Gen V event Met Location descriptions | Likely addressed (`SpecialMetLocationNotes.h:64-130`; tests) | – |
| 4 | Gen V Create auto-selects Met Location 1 | Still present (`Gen5SharedScreenState.h:812`; validator rejects 0; tests pin 1) | Unset until chosen; Keep requires a choice |
| 5 | Gen V Trainer: Pokédex editable, Gender read-only | Still present (`Gen5SharedReadOnlySurface.inc:1289-1292`); no Gen V trainer-gender writer | Pokédex read-only; Gender editable only with a validated native writer |
| 6 | Gen V Gender uses a separate confirmation | Still present (`Gen5SharedReadOnlySurface.inc:318-335, 568-576`) | Inline Male/Female selector with the PID warning shown inline |
| 7 | Gen I Play Time editing | Still present: no Gen I `stagePlayTime` (`Gen1StagedTrainerEditor.h:55-56`) | Add a Gen I writer (the save stores play time), with tests |
| 8 | Gen II calculated Special | Likely addressed; the Special row's Calculated cell is intentionally blank, with split values below (`Gen2UnifiedPokemonWorkspace.inc:919-940`) | Needs a hardware look; perhaps a "see below" pointer |
| 9 | L/R as space/backspace in search | Still present: search keys ignore L/R (`SharedSearchInput.h:202-210`); L/R page lists when search is closed | L = backspace, R = space while search is open only |
| 10 | Accumulating Gen V recovery copies | Still present: no recovery-management view; recovery-only histories refuse delete; `.previous`/`.failed` hidden | Owner-confirmed recovery view; never casual deletion of last copies |

## Validation (local, at branch head; CI not run because no PR is open)

All on head `b80137df` plus the hub-contract fix:

| Gate | Result |
|---|---|
| GCC `host-test` | rc 0 (217 PASS lines) |
| GCC ASan/UBSan `host-sanitize` | rc 0 (217 PASS lines, no sanitizer reports) |
| Clang 18 `host-test` | rc 0 (208 PASS lines; host-UI harness is GCC-only by design) |
| `Makefile.host.classic_inventory` | rc 0 (7 suites) |
| QA fuzz smoke, contracts and matrix | pass (no crash, no invariant failure) |
| QA tools (hardware-accepted gate, 301 guards; NRO audit; retest tooling) | rc 0. The first run caught a real Q04 regression: the release-date sort-key literal pinned by `test_game_hub_contract.py` (C01–C07, R2-LAUNCH, AUD-09). Fixed by resolving the family first and keeping the exact pinned lines; the guard was not changed. |
| Python contracts (`tests/test_*.py`) | 37/37 pass; cross-gen `--candidate`: 0 failing |
| Native devkitA64 NRO build | **NOT RUN**: no devkitA64 in this container, and dispatching a CI build would mint an unauthorised candidate identity |

## Next hardware round checklist (for a future, owner-authorised candidate only)

All items NOT DEVICE TESTED. Before testing, back up the SD card; every test uses copies.

1. **Bank, missing:** fresh install with no `banks/` folder. Storage opens; save works.
2. **Bank, unreadable:** copy a junk file over `banks/legacy-pkse/bank.dat`. Storage shows the "kept as bank.dat.unreadable.N" message; Save is refused; the junk is preserved byte-identical. Removing it restores normal use.
3. **Bank, interrupted save:** not testable on device without pulling power; host-verified only.
4. **Gen II padded save:** a 128 KiB padded Gold `.srm` is discovered and opens. Edit money and Export Copy. The export is 128 KiB with identical padding and opens in an emulator; the original is unchanged (SHA-256).
5. **Gen II Japanese Crystal 32 KiB:** opens read-only with no trainer gender shown; Edit is refused.
6. **Localized FRLG on Switch:** each installed non-English release shows as its own card and creates its own backup folder. Opening it never touches `save:/`.
7. **Gen III Bag fidelity:** set an item to the same quantity, then to another and back. Review shows no pending change. Export and byte-compare: only that slot and its checksum differ.
8. **Gen II Bag fidelity:** same check; only the list region differs.
9. **FRLG Meteorite:** on a save holding Meteorite, remove it with the key-item warning, then re-add it.
10. **Gen III Egg:** an Egg's Nickname and Language edits are refused with the Egg message.
11. **Gen V species change:** female Pikachu with a low PID byte ≥ 31 → Eevee is refused with the "other gender" message.
12. **Gen III emulator save with RTC footer** (mGBA/VBA-M): Bag and Trainer edits export with the footer intact.

## Handoff

- **Branch:** `wip/pkse-1.2.x-gen15-audit`.
- **Merging into #307:** forward-merge only, through the ownership protocol, after owner approval.
- **Expected conflicts with #314:** `Pokemon3FRLG.cpp`, `Gen3SharedPokemonSurface.inc` and the Gen II reader both appear in #314's history.
- **Legality evidence for #314** (no engine edit):
  - a Gen V record's stored gender must equal `genderFromPid5(pid, ratio)`;
  - a Gen III Egg's nickname is タマゴ and its language Japanese.
- **Open evidence requests:**
  - a real VBA-M FireRed save (upstream #127);
  - real Japanese Crystal 32 KiB and 128 KiB padded GSC samples;
  - the owner's 54/4/1 report for `26186464`.
