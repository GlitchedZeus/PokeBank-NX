# PokeBank NX — Main Development: permanent project instructions

Approved by the repository owner on 2026-10-14. These rules bind every Main
Development session and thread. They do **not** apply to, and must not be imported
into, the separate PokeBank NX Legality Engine project.

## Scope

Main Development owns the PokeBank NX application for Nintendo Switch:
Generation I–IX save support (parsing, encryption/decryption, checksums,
serialization), the one shared Pokémon editor, Trainer, Party, Boxes,
Bag/Items, Pokédex, Save Instances and provider discovery, profile save
assignments, staged editing with verified working-copy exports, the HOME-style
frontend, controller and touch parity, emulator launch/return, native builds and
NRO packaging, hardware bugs, performance and stability, and (when authorized)
the Master Vault and named Banks.

The Legality Engine (encounter, PID/RNG, stats/breeding, moves, events,
historical transfer legality, PKHeX differentials) is a separate project. Main
Development may implement native format checks, safe serialization and editor
capability restrictions only.

## Permanent engineering rules

1. GitHub is authoritative. Fetch the latest branches and PRs before working and
   record the exact HEADs you start from.
2. Never force-push, reset backward, overwrite newer commits or discard another
   developer's work.
3. Never merge into `main` or another integration branch without explicit owner
   approval. (Forward merges of an owned lane's own upstream bases are part of
   that lane's normal work.)
4. Preserve separate ownership between application, legality and independent
   audit work. See `THREADS.md` for branch and file ownership.
5. Every engineering session must be substantial: meaningful production code,
   not just documentation or trivial changes.
6. Add regression tests with every significant implementation.
7. Run the relevant host tests, sanitizers and native builds.
8. Fix real compilation and test failures; never bypass, skip, disable or
   quarantine checks.
9. Maintain one coherent shared Pokémon editor across all generations
   (`ExactFormatEditorProvider` and the shared HOME-style surfaces).
10. Never create separate editors for Gen V, VI, VII, VIII or IX.
11. Extend the existing HOME-style frontend; never build another application
    shell.
12. Never modify original source saves during ordinary editing.
13. All changes are staged and exportable only as validated working copies
    (`Integration/Common/VerifiedStagedExport`).
14. Do not enable live emulator writes, installed-game writes, Inject Save or any
    destructive writeback without the owner's explicit authorization.
15. Never invent Pokémon format rules, encryption schemes, checksums or save
    structures. Cite the authoritative reference (pinned revision) for every
    layout constant.
16. Unknown or unsupported format details stay explicitly unsupported.
17. Preserve controller, touchscreen and UI parity.
18. Preserve existing hardware-accepted behavior.
19. Never call a build hardware accepted until the owner has tested that exact
    NRO.
20. Keep exact commits, SHA-256 hashes, CI evidence and progress checkpoints.

## Additional coordinator rules

- One integrated NRO per hardware test round. Every candidate records code SHA,
  tree SHA, NRO size and SHA-256, CI run IDs, and `NOT DEVICE TESTED` until the
  owner accepts that exact file.
- Report percentages in three separate columns: software implementation,
  automated verification, hardware acceptance. Compilation success is not
  feature completion.
- Save discovery never guesses emulator paths or save identity. A provider path
  is added only after it is verified against the emulator's own source or
  documentation (pinned revision) or confirmed on the owner's SD card.
  Unverified providers stay unsupported; explicit manual file linking is always
  available.
- Gen VIII/IX Switch titles: reading installed saves must be read-only, and the
  only output is an exported working copy. No installed-save write path may be
  added without separate owner approval.
- No commit message, PR text or code comment may contain an AI model identifier.
- Ask the owner before merges, destructive operations, enabling live save writes
  or major scope changes. Do not ask before ordinary investigation or tests.

## Environment

- Linux, GCC 13 and Clang 18 with ASan/UBSan, Python 3, GNU make.
- Host gates: `make -f Makefile.host host-test` and `make -f Makefile.host
  host-sanitize` under GCC and Clang (`CXX=clang++`), Python contract tests,
  `git diff --check`.
- Native: the repository's pinned devkitA64 Docker image (as used by
  `native-pr-build.yml` / `gen14-hardware-candidate.yml`), NRO packaging, and
  `tools/verify_nro_romfs.py`.
- CI: GitHub Actions on every pushed head; the exact-head run is the evidence.
- Hardware: the owner tests physical NROs on an Atmosphère Nintendo Switch.
