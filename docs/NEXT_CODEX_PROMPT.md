# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

## Current verified application source

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

The strict FireRed/LeafGreen GBA route is complete through bounded RetroArch discovery, the UIManager-owned catalog, normal Game Sources cards, and read-only Party / Boxes. GitHub Actions run #184 passes all twelve host suites.

Do not redo the PKSM-Core oracle, exception-free native backend, FRLG scanner, source cards, read-only view bridge, accepted UI, Left Stick work or PLA hardening.

## Current hard gate — waiting for physical Switch result

The exact FRLG browser device artifact is READY but **NOT DEVICE TESTED**.

Exact identity:

```text
Application source: f6a3052daeffe7cd30d7acceba81a5dfda7615ee
Embedded version:   f6a3052d
NRO size:           155174825 bytes
NRO SHA-256:        809c94c842a3c385d23907c61e5ecfa201b24f08e8ac935e87a4add60770282d
ZIP size:           148435779 bytes
ZIP SHA-256:        66b1e16eb5443fd6ce682f2485aacf0cb50a2b00cca0846b8d170cc89727b701
```

Asset verification already passed:

```text
HD Pokémon renders:         3260
Base species coverage:      1025 / 1025
Embedded RomFS comparison:  3281 / 3281 files byte-identical
Native Switch build:        PASS
Asset preflight:            PASS
```

Do **not** rebuild/repackage merely because a new session starts. Do **not** begin RSE unless the user reports the physical result for this exact artifact.

Required physical flow:

```text
Game Sources
  -> FireRed GBA / LeafGreen GBA (RETROARCH)
  -> select source
  -> trainer/source view
  -> Party
  -> Boxes 1-14
  -> View Pokémon
```

Also verify that GBA and Switch FRLG identities are visibly distinct, View works, and Edit/Clone/Transfer/Move/Save/writeback remain blocked.

## If the user reports PASS

First record the exact artifact as **DEVICE TESTED** in `CURRENT_STATUS.md` and the relevant issue/status documentation.

Then begin the next coding milestone: strict read-only Ruby/Sapphire/Emerald production support.

Before touching RSE source, inspect and preserve local state. A previous interrupted session reported useful local RSE work at:

```text
1a921515
```

That commit was intentionally parked and not pushed. If it exists locally, recover/audit/reconcile it before reimplementing equivalent work from scratch. Never reset/clean/restore over it before preservation.

Target identities:

```text
ruby_gba
sapphire_gba
emerald_gba
```

Target path:

```text
bounded RetroArch discovery
        -> strict RSE save validation
        -> exact release + GBA identity
        -> existing Game Sources screen
        -> strict read-only Party / Boxes
```

Reuse the PokeBank-owned Gen III API where sound. Do not leak PKSM-Core types into UI code or route selected saves through older permissive Trainer parsing. Invalid/ambiguous sources remain unavailable rather than guessed. Preserve source path, game ID, platform, Pokémon locations and original bytes.

## If the user reports a blocker/failure

Do not start RSE.

Recover/preserve any local changes first, reproduce the exact blocker, fix only the minimum FRLG/browser/device issue, rerun host tests/sanitizers/diff/native build, rebuild with the full pinned asset set, and produce a new exact NRO with filename/size/SHA-256 for another physical test.

Do not call the failed artifact DEVICE TESTED/PASS.

## Read-only safety

Legacy files and installed titles remain immutable sources. Do not implement editing, clone-to-save, delete, move, injection, repair/resign, writeback, conversion, Master Vault/Banks or installed-title writes in this milestone. View may operate on a PokeBank-owned read model. Disabled actions must return safely and never fall through.

## Verification for RSE when physical FRLG acceptance is complete

Preserve all twelve existing host suites and add focused coverage for RSE structure, identity, malformed/truncated input, Party/Box mapping, source immutability and blocked mutation operations. Keep FRLG regression coverage green.

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Commit/push coherent application work early only to:

```text
origin / feature/pokebank-playable
```

Never push custom code upstream to `kiasta/PKSE`.

## Stop conditions

Before a physical FRLG result: **do no new coding work**.

After FRLG PASS: stop after one coherent, pushed read-only RSE production-source checkpoint. Do not continue into Gen I/II, GameCube, DS/3DS, Master Vault/Banks, conversion, events, physical-link hardware, Right Stick, final UI polish or live writes.

RSE end report should include:

```text
starting SHA
recovered parked RSE work/ref if any
implementation SHA(s)
exact identities
validation and slot behavior
browser path
read-only controls
Party/Boxes behavior
immutability proof
host tests
sanitizers
diff check
native build
CI
NRO size impact
remaining blocker
exact next coding task
```
