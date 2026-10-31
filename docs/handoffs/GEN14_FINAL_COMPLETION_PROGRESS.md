# Gen I–IV Final Completion — Progress Checkpoint

Re-fetch GitHub before acting on any SHA below. Never force-push; never reset backward.

## Authoritative lineage

- Parent: PR #278 head `fix/gen14-narrow-native-actions-20261011` @ `d0a45909db43164fabfd36ed1e738cd07d8d81dd`
  (all 9 exact-head checks green on 2026-10-12; menus + Gen IV Y-carry owner hardware-accepted).
- **Finalization branch: `claude/gen14-final-20261014`** (owner-approved 2026-10-14), created
  from PR #278's head. One draft PR from it targets `fix/gen14-narrow-native-actions-20261011`.
- `claude/pokebankn-gen14-completion-klwcwv` is NOT used: it was created at `main`
  (`5872663a`), an orphan docs-only history unrelated to the application; left untouched.

## Completed and tested in this lane

| Commit | Change | Evidence |
|---|---|---|
| `eb2aa8b3` | Artwork regression root cause: `native-pr-build.yml` did `rm -rf romfs; cp -a recovery-work/romfs romfs` (dropping branch region backdrops) and never ran `fetch_product_art.py` (no trainer portraits / Hisui / Paldea). All NRO workflows now use `tools/prepare_release_romfs.sh` (fill-gaps-only). New `tools/verify_nro_romfs.py` parses the built NRO ASET → RomFS tables and fails on any missing/different required asset. `check_device_assets.py` now requires 10 backdrops + 20 portraits. New combined `gen14-hardware-candidate.yml`. | `tests/test_verify_nro_romfs.py` PASS (synthetic build_romfs-format NRO). Real-NRO verification runs in CI. |
| `2bcc4053`,`87bf4301`,`cb9ef219` | Classic Bag export (Gen I–III) and Gen I Pokémon export moved onto the verified transaction with exact-game strict validators (`Integration/Common/ClassicStrictValidators.h`); same paths/file names. | `test_classic_verified_export` (real checksum-valid Red/Gold, corrupt refusal, fault cleanup) normal + ASan/UBSan; devkitA64 full make. |
| `03782f3a` | Save Instances names the Gen IV folder that hit its scan limit. | devkitA64 full make; product-ui-test. |
| `3ac0ae29` | Diagnostics shows the verified exports folder. | — |
| `2ca38757` | Gen III/IV verified working-copy export (`Integration/Common/VerifiedStagedExport`): temp dir → fsync → read-back → SHA-256 → strict reparse before/after → atomic rename; never overwrites; source never written. Gen IV review shows original→staged slot list; X = Export Copy in Gen III/IV review (Gen III refuses while separate Bag edits pending). Exports: `sdmc:/switch/PokeBank-NX/exports/gen3|gen4`. | Gen IV 3 layouts + Gen III 5 games: export/reparse/reopen/manifest/4 fault points/no-overwrite/source immutability, normal + ASan/UBSan. |
| `6f3b4ddf`+`beb7cf7c` | Review → Discard All now two-step (Y arms confirmation, second Y discards, B keeps, A/X inert) in Gen III/IV. | direct-row touch contract asserts it; local devkitA64 `make native-ui-check` PASS. |
| `92bba2d0` | Product UI Native + Gen IV Candidate gates now trigger for this PR's base. | CI |
| `41a8ceee` | Launch: ROM header identity (GB/GBC title, GBA code @0xAC, NDS code @0x0C) for all 16 games. Contradicting headers are excluded from filesystem/playlist resolution, rejected by Manual Link, and re-checked for remembered links. Unknown/archives never refused. | `test_retroarch_launch_resolution` PASS normal + ASan/UBSan. |
| `f5b876fe` | Gen IV discovery starvation: one shared 256-file budget scanned RetroArch first → large RetroArch folder hid DraStic/melonDS. Per-root budgets, per-directory entry cap, `truncatedRoots`. | `test_gen4_readonly_foundation` PASS normal + ASan/UBSan, new fairness/entry-cap cases. |

## Audited — no change required

- PR #305 Gen IV ball `max(0x83,0x86)`: already present in this lineage
  (`src/Pokemon/Pokemon4ReadOnlyView.cpp`); `Pokemon4Mutable::setBall` writes HG/SS stand-in.
  The regression only existed on the legality integration branch. PR #305's test depends on
  the separate legality engine and is not ported.
- Gen III Negai Boshi string-view hazard: code exists only in legality branches
  (`include/Legality/Gen3NegaiBoshiEventTemplate.h`), not in this application lineage.
- Gen I–III provider discovery already runs each provider (RetroArch / mGBA / Tico) with its
  own budget (`src/Legacy/RetroArchFRLGDiscovery.cpp`).
- Gen IV Create opens the editor (no forced species picker); Met Lv. shown with location;
  Y shiny preview in species picker.

## Tests executed (local, this lane)

- Local devkitA64 available: `sudo dockerd &` then
  `docker run --rm -v "$PWD:/work" -w /work devkitpro/devkita64@sha256:1fc388c3… bash -lc 'make native-ui-check'`.
- Full `make -f Makefile.host host-test` PASS after export work; `host-test host-sanitize` PASS at `f5b876fe`.
- `one-time-*` workflows show zero-job "failures" on every push (inherited invalid/no-op files, not PR checks).

- Baseline `make -f Makefile.host host-test` on `d0a45909` (after submodule init): PASS.

## CI status

See the draft PR checks for the current head.

## NRO candidate (current hardware test candidate)

| Field | Value |
|---|---|
| Application SHA | `9bf9d492bd8942503fbc261a7dcabcfd06d9e8c6` |
| Tree SHA | `d3a9edae0239a229199deb3681fc92bdfea72883` |
| Artifact | `PokeBank-NX-Gen1-4-Candidate-9bf9d492` (run 38544369031, artifact 11884126974, expires 2027-01-12) |
| NRO | `PokeBank-NX-Gen1-4-Candidate-9bf9d492.nro`, 173,640,313 bytes |
| NRO SHA-256 | `f623d23794e721049fb0087f23d089fe7ad1baae716acd68f28092815be5bfa8` |
| Packaged RomFS | PASS: 10 backdrops, 20 portraits, 16 covers, 931 item icons, 18 types, 3260 HOME sprites, 3 fonts |
| CI on that exact head | 9/9 PASS (Host GCC, Host Clang + ASan/UBSan, Native PR Gate, Product UI Native, Gen IV gate x3, Gen I-IV candidate, Touch) |
| Source save writes | DISABLED |
| Hardware acceptance | **PENDING** — `docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md` |

Later docs-only commits do not change this application candidate.

## Next task

1. Owner runs the hardware checklist on the candidate above; fix hardware-reported bugs on this branch.
2. Continue audit: Save Instances / profile assignment, launch adapters, touch parity gaps.
