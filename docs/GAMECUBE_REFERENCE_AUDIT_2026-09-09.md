# GameCube Pokémon save/reference audit — 2026-09-09

> Future GameCube research only. This does **not** promote Colosseum/XD into the active milestone. Current work remains whatever `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` say.

This note preserves two unusually valuable independent Colosseum/XD implementations so future Codex sessions do not reverse-engineer the formats from scratch.

## Recommended oracle/reference stack

```text
PKHeX
    primary modern behavior / entity oracle
        ↓
PokéBridge
    modern C / GameCube-homebrew implementation
        ↓
PkmGCTools / LibPkmGC
    older independent C++ implementation / historical oracle
        ↓
Dolphin memory-card code
    independent .gci / memory-card container oracle
        ↓
PokeBank-owned read-only adapter + fixtures
```

Initial PokeBank GameCube support should be read-only. Writeback should remain blocked until per-format serialization, encryption/checksums, staging, reparse/readback and physical/emulator tests are complete.

---

# 1. logdog2325/PokéBridge

Repository:

```text
https://github.com/logdog2325/pokebridge
```

**Classification:** `REFERENCE` (high value; possible algorithm/test-vector source subject to license strategy).

**License:** GPL-3.0.

PokéBridge is actual GameCube homebrew and currently documents support for:

- Gen III GBA `.sav`;
- Pokémon XD `.gci`;
- Pokémon Colosseum `.gci`;
- physical GameCube memory-card scanning/writeback;
- XD GeniusCrypto;
- Colosseum SHA-1-chain XOR encryption;
- CK3/XK3 Pokémon structures;
- checksum recomputation and re-encryption;
- a common normalized edit surface across formats.

Particularly important safety lesson: its README records a prerelease that corrupted a tester's real XD save, after which the project added a **self-verification step that reparses its own re-encrypted output before touching the memory card**.

That pattern should become normal PokeBank write-adapter policy:

```text
serialize
    ↓
checksums
    ↓
encrypt
    ↓
parse OUR output again
    ↓
compare expected semantic contents
    ↓
only then permit destination write
```

This is valuable even if PokeBank never directly reuses PokéBridge source.

---

# 2. TuxSH/PkmGCTools

Repository:

```text
https://github.com/TuxSH/PkmGCTools
```

Reviewed repository head:

```text
2b98e7e86b00b7d6634fa21c8ce2819ac6e5e891
```

The latest visible commits are from 2018, so treat it as a **historical independent implementation**, not the current authority.

## Components / licensing

The root README identifies:

```text
LibPkmGC         backend library       LGPLv3
PkmGCSaveEditor  desktop save editor   GPLv3
```

**Classification:** `HISTORICAL / REFERENCE`, with `LibPkmGC` only a possible direct-integration candidate after explicit LGPL/static-linking/toolchain review.

Do not treat LGPL as automatically drop-in for an NRO. The current library was designed as a desktop C++ library and its build requires Boost.

## Why it is still valuable

The editor/library covers much more than a toy parser. Its documented features include:

- Pokémon Colosseum and XD save editing;
- Pokémon import/export;
- GBA Pokémon support;
- GC ↔ GBA Pokémon conversion intended to mirror Colosseum/XD trade behavior;
- trainer information;
- party;
- PC Pokémon/items;
- bag;
- daycare;
- Strategy Memo;
- selected ribbon text;
- XD Purifier;
- partial mailbox/battle-mode backend support;
- multilingual data handling including Japanese.

The backend contains separate `Colosseum`, `XD`, `GBA` and shared `GC` implementations. Its save layer understands optional `.gci` header data, multiple internal save slots, save counters, corruption/checksum checks and encrypted serialization.

This makes it excellent for **independent regression comparison** against PokéBridge/PKHeX and for finding edge cases that a single implementation may miss.

## Why it should not be our default Switch runtime dependency

Direct native reuse is unattractive without deliberate adaptation:

- CMake requires Boost;
- many library paths use C++ exceptions (`throw std::invalid_argument`, `std::out_of_range`, etc.);
- PokeBank's native build currently keeps `-fno-exceptions`;
- code age means offsets/bugs should be checked against modern implementations;
- LGPL static-link/relinkability obligations require an explicit licensing plan if directly linked/distributed.

Recommended role:

```text
host/reference oracle
historical test vectors
format archaeology
cross-check save-slot/checksum/encryption behavior
```

Do not port the architecture wholesale.

---

# 3. Dolphin memory-card implementation

Dolphin should remain a separate container-level oracle for:

- `.gci` import/export;
- raw GameCube memory-card directory/BAT structures;
- checksums;
- slot/container behavior;
- malformed memory-card fixtures.

PokeBank can initially avoid raw physical-memory-card complexity by supporting extracted `.gci` / emulator files first, then add broader memory-card images only when the GameCube milestone is active.

---

# 4. PokeBank GameCube implementation strategy

When Colosseum/XD becomes active:

```text
1. pin exact PKHeX / PokéBridge / PkmGCTools / Dolphin revisions
2. collect or deterministically generate legal fixtures
3. implement PokeBank-owned READ-ONLY adapter API
4. triangulate parsed trainer/party/PC/Pokémon results across references
5. reject malformed/corrupt slot/container states safely
6. expose source + Party/Boxes/Pokémon Summary read-only
7. physical/emulator acceptance
8. only later design staged writeback
```

For writeback later:

```text
backup original
stage output
recompute integrity fields
encrypt/serialize
reparse staged output
semantic compare
write destination
read back destination
reparse + compare
rollback on failure
```

The PokéBridge corruption story is a useful permanent reminder that "the checksum function returned success" is not enough evidence to write a real user's only save.

---

# 5. Resource priority

```text
PokéBridge     P2 future milestone, but a GameCube jackpot
PkmGCTools     P2 historical independent oracle
Dolphin        P2 container/memory-card oracle
PKHeX          primary host correctness oracle
```

These references should stay parked until GameCube work is explicitly promoted by `docs/NEXT_CODEX_PROMPT.md`.