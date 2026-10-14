# Request 02 (to Thread 2): Save Instances providers, native build sources, export root

## 1. Native build: compile the Gen VI sources

`Makefile` (Thread 2) `SOURCES`: append `src/Integration/Gen6`. The files are
`Gen6Save.cpp`, `Gen6StagedSession.cpp` and `Gen6SaveSources.cpp`. They use only
POSIX `stat/opendir/fopen("rb")`, `Utils/CRC16.h`, `Source/SaveInstance.h`, and (under
`__SWITCH__`) `Utils/PokeBankPaths.h` for the locked export root `exports/gen6`.
No Gen VI code has been compiled with devkitA64 yet: Docker was not usable in the
Thread 3 container. The `native-pr-build.yml` run after this change is the evidence.

## 2. Save Instances provider list entries

Discovery stays read-only and exact. For each Ready probe, append the
`Source::SaveInstance` from `Gen6::Sources::probeSave` through the existing
`Source::appendDeduplicated` and `sortNewestFirst`. Fill in `modifiedTime` from `stat`.

```cpp
#include "Integration/Gen6/Gen6SaveSources.h"
namespace G6 = PokeVault::Integration::Gen6::Sources;

// Checkpoint layout (written on Switch by Dekopon's save manager).
for (const auto& f : G6::findCheckpointExports("sdmc:/3ds/Checkpoint/saves/")) { /* probe */ }

// Dekopon: honour user_dir.txt exactly like Dekopon; nullopt => skip, never guess.
std::optional<std::string> pointer = /* read sdmc:/switch/dekopon/user_dir.txt if it exists */;
if (auto dir = G6::dekoponUserDir(pointer))
    for (const auto& f : G6::findEmulatorSaves(G6::ProviderId::DekoponEmulator, *dir)) { /* probe */ }

// Probe: bytes from G6::readCandidate(f.path); context.assignedExactGame comes from the
// profile's explicit assignment (or, for discovery rows, the user confirms the
// content-derived id); context.evidenceGameId = f.evidenceGameId.
```

Manual link: add Gen VI to the existing manual-file flow, with an explicit
X / Y / OR / AS assignment and `ProviderId::ManualLink`.
Azahar and JKSM must **not** be scanned (see `docs/gen6/EMULATOR_SAVE_PROVIDERS.md`).

## 3. Export

Call `Gen6StagedSession::exportWorkingCopy` with `rootDirectory` empty, or set it to
`PokeBank::Paths::exportsRoot() + "/gen6"`. On Switch, any other root is refused.

## 4. Pre-existing base issue found while running the full host suite

On `a17c503` (unchanged base, reproduced with the Gen VI include removed),
`make -f Makefile.host host-test` fails at `Makefile.host.base:337`
(`build-host/pksm-core-c.o: gcc: fatal error: no input files`). The pinned
PKSM-Core `aa22d7a4` contains no `*.c` files, so `PKSM_CORE_C_SOURCES` is empty
and the `-r` link has no inputs. With `make -k` every other target builds. A minimal
fix in Thread 2's file is to make `$(PKSM_CORE_C_OBJECT)` (and its sanitize variant)
conditional on `PKSM_CORE_C_SOURCES` being non-empty, and to drop it from the link
lines when empty. The Gen VI focused targets do not depend on it.
