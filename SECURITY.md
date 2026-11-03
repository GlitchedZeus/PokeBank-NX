# Security and data-safety policy

PokeBank NX reads Pokémon save files on a Nintendo Switch. For this project the most serious bugs are the ones
that could **damage or silently change a save**, so data-safety problems are treated as security issues.

## Supported versions

There is no stable release. Reports are accepted for the current hardware candidate and for `main`
(see [docs/STATUS.md](docs/STATUS.md)). Older candidates are superseded; please retest on the current one if you
can.

## What to report privately

Report these privately, not in a public issue:

- Any way the app could **write to, truncate or replace an original save**: an emulator save, an installed-game
  save, or a file the app only discovered. Backups and exports belong under `sdmc:/switch/PokeBank-NX/` only.
- An **Export Copy** that overwrites an existing file, leaves a partial file after a failure, or produces a save
  that does not match what Review showed.
- A crafted or corrupted save, `.pk*`/`.ek*` file or config file that crashes the app, reads or writes out of
  bounds, or makes it write somewhere it should not.
- Path traversal or symlink tricks in save discovery, backups or exports.
- Anything that leaks personal data (trainer names, account IDs) somewhere it should not go.

## How to report

Use GitHub's private vulnerability reporting:
[Report a vulnerability](https://github.com/GlitchedZeus/PokeBank-NX/security/advisories/new) (Security tab →
"Report a vulnerability"). If that form is unavailable, open a public issue that says only that you have a
data-safety report and ask the maintainer for a private channel; do not include details.

Please include:

- The exact build: NRO file name and SHA-256, or the commit you built.
- Game, emulator / provider and where the save came from.
- Steps to reproduce and what changed (file sizes, hashes or a byte diff if you have them).

**Do not attach real save files publicly.** They contain trainer names and IDs. If a save is needed, say so and a
private way to share it can be arranged, or use a disposable save made for the test.

## What to expect

This is a volunteer project, so there is no guaranteed response time. Reports are acknowledged as soon as the
maintainer sees them. A confirmed issue that can change an original save is treated as the highest priority:
it blocks the next hardware candidate until it is fixed and a regression test is added.

## Out of scope

- Problems in emulators, Atmosphère or other homebrew.
- Edits you deliberately make and export: PokeBank NX is a save editor, and an exported copy is yours to use or
  discard. (Legality checking is a separate, unfinished project.)
- Piracy-related requests. The project provides no ROMs, keys or firmware.
