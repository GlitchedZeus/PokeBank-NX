# Gen VI–IX application campaign: active handoff (development/gen6-plus)

> Rules: [`docs/ENGINEERING_PROTOCOL.md`](../ENGINEERING_PROTOCOL.md). Source of truth: GitHub.
> This lane is isolated from #307, #314, #327/#328 and the unaccepted `main` merge. No merge into `main`,
> no force-push, no branch deletion, no source-save write.

Last updated: 2026-10-31 UTC, by Claude (coordinator session_011W9mYgjDqGPH91UprKtQLu).

## Base and branch

- Branch `development/gen6-plus`, started from the Round 10 Gen I–V source
  `fix/gen4-bag-remove-compaction` @ `0842c6f3` (application code `3634d1e6`, owner checklist
  `docs/hardware/RETEST_3634d1e6.md`). Nothing from that candidate was changed; everything here is additive.
- Imported as new files: the Gen VI lane (#309), Gen VII lane (#311) and Gen VIII SWSH lane (#312) from
  `claude/gen8-native-20261015` @ `2f065ee2`.

## Architecture (one application, not nine)

| Layer | Files | Role |
| --- | --- | --- |
| Capability registry | `include/Integration/Common/GameCapabilityRegistry.h` | All 38 titles: per-workflow PASS / READ-ONLY / BLOCKED(reason) / NOT APPLICABLE, native mechanics per title, storage geometry. `static_assert` enforces cumulative inheritance. |
| Session contract | `include/Integration/Modern/NativeSession.h` | The one backend interface for Gen VI+: views, provider field states, staging, draft bracket, clone/move/remove/create support with reasons, bag, trainer facts, dex, review rows, verified export. |
| Adapters | `include/Integration/Gen6/Gen6NativeSession.h`, `include/Integration/Gen7/Gen7NativeSession.h` | Gen VI (X/Y/ORAS) over the #309 staged session; Gen VII (SM/USUM) over the #311 staged session (export blocked by the SAV7 signing gate). |
| Presentation bridge | `include/Pokemon/ModernRecordView.h`, `include/Legacy/ModernReadOnlyTrainer.h` | One presentation Pokémon and one Trainer bridge for the existing shared Party/Boxes/Items/Trainer panels. |
| Save Instances | `include/Integration/Modern/ModernSources.h` | Discovery (Checkpoint layout, Dekopon), strict validation, per-profile binding (`3ds-single-copy`), fingerprint-checked reopen, Home card snapshot. |
| Routing | `src/UI/SaveSelectScreen.cpp` (`ModernAssignedFile`), `src/UI/UI.cpp` (`handleModernView`), `src/Legacy/BackupReadOnlyTrainer.cpp` | Shared Home cards, the Gen V Save Instances overlays serve 3DS titles, the one TrainerViewScreen opens over the bridge, emulator backups open read-only. |
| Shared surface | `src/UI/ModernSharedSurface.inc` (in progress) | The Gen VI+ adapter of the shared Trainer/Party/Boxes/Items screen and the ONE editor workspace; written once against NativeSession. |

Adding a generation = one adapter + one registry row + identity rows + fixture-backed tests. Never a new editor.

## Commits on this lane

| Commit | Content | Host evidence |
| --- | --- | --- |
| `ff607c4e` | Import of the #309/#311/#312 lane files (new files + 3 include lines + host-UI MemeCrypto include) | gen6/gen7/gen8 suites green against Round 10 headers; full GCC host suite green |
| `82cd400f` | Registry, NativeSession, Gen VI/VII adapters, bridge, Save Instances, routing, native Makefile | `test_game_capability_registry` 1,585, `test_gen6_native_session` 101, `test_gen7_native_session` 177, `test_modern_sources` 58, `test_modern_trainer_bridge` 25 (GCC + Clang; ASan/UBSan on the gen6-plus set); Gen I–V static contracts unchanged |

## Status per game (host evidence only; NOT DEVICE TESTED)

See `docs/gen6-plus/CAPABILITY_MATRIX.md` (generated from the registry) for the full 12-column table.
Summary of what is true today:

- **X / Y / Omega Ruby / Alpha Sapphire**: Dashboard card, Save Instances (Dekopon, Checkpoint copy, manual),
  Party, 31×30 Boxes, shared editor with provider field states (nickname, OT, IDs, held item, friendship,
  IV/EV, nature, level, ball, Pokérus, OT gender, fateful editable; species/form/gender/shiny/ability/moves/
  language/met read-only until a verified writer and a pinned XY/ORAS learnset exist), Clone, box Move, box
  Remove, Bag (Items/Medicine/Berries editable; Key Items/TMs read-only), Trainer facts, Pokédex counts,
  Review, Discard, verified Export Copy to `exports/gen6`. **Create blocked** (EC/PID/personal/met/handler/
  memory/geolocation defaults not verifiably derivable). Party Remove blocked (party compaction unverified).
- **Sun / Moon / Ultra Sun / Ultra Moon**: same workflows through the same adapter contract; **Export blocked**
  (MemeCrypto re-signing has no retail acceptance evidence; see `docs/gen7/README.md`); Create blocked.
- **Let's Go, Sword/Shield, BD/SP, Legends: Arceus, Scarlet/Violet, Legends: Z-A**: registered with honest
  BLOCKED editor/export status and the named adapter prerequisite; the installed-title readers remain as
  before. Z-A is PA9/SAV9ZA, not SV.

## Owner hardware checklist

`docs/hardware/GEN6_PLUS_HARDWARE_CHECKLIST_TEMPLATE.md` (G0 safety, G6-01…G6-21, G7-01…G7-03). No NRO is
bound yet; the native devkitA64 build of this lane has not run in this session (no toolchain here) and must
be confirmed by CI before any binding.

## Open work (in order)

1. Land `src/UI/ModernSharedSurface.inc` + state machine + host-UI harness test; wire into
   `TrainerViewScreenCompositeOverlay.cpp` (three additive lines).
2. Artwork: 3DS covers and Gen VI/VII trainer portraits (pinned libretro-thumbnails / Showdown sources).
3. Native CI build on this lane; bind one NRO; owner runs the G6/G7 checklist.
4. Pinned XY/ORAS/SM/USUM learnsets and species/form rules → unlock Moves/Species editing in the Gen VI/VII
   provider tables (never before the data is pinned).
5. Gen VIII/IX adapters per the registry prerequisites (SWSH over #312 first).
