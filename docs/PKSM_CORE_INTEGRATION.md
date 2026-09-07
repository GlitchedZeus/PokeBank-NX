# PokeBank NX — PKSM-Core / Gen III Integration

Status: **FRLG HOST ORACLE + NATIVE EXCEPTION-FREE BACKEND IMPLEMENTED**
Last updated: 2026-09-07

Pinned upstream:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
GPLv3
```

PokeBank NX uses PKSM-Core through a narrow PokeBank-owned adapter boundary rather than exposing Core types throughout the app.

## Session 3A — host oracle / adapter

Implementation:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

CI/submodule follow-up:

```text
283073a5215a471ef0ad07619b4856409658cfdc
ci: checkout pinned PKSM-Core submodules
```

Public boundary:

```text
include/Integration/Gen3/PKSMGen3Adapter.h
```

Host implementation:

```text
src/Integration/Gen3/PKSMGen3Adapter.cpp
```

Core components used by the host oracle include `PK3`, `PKX`, `Sav3` and `SavFRLG`. Required pinned nested dependencies are `memecrypto` and `pcg-cpp`.

The build configuration also requires the pinned Core language/personal data paths (`_PKSMCORE_LANG_FOLDER` and `_PKSMCORE_PERSONAL_FOLDER`).

## Safety boundary

PokeBank NX does not rely on `Sav3::isValid()` alone. The adapter performs stricter validation before Core sees a private copy of source bytes:

- exact save/container size;
- all 14 unique Gen III sector IDs;
- sector signatures;
- consistent counters per slot;
- sector checksums;
- wrap-aware newest-slot selection;
- safe fallback to an older valid slot;
- malformed/truncated/entity-checksum rejection.

Core is never allowed to choose a checksum-invalid newer slot merely because its counter is newer.

Source bytes are copied and remain unchanged. No write/resign/repair API is exposed by this milestone.

## Deterministic fixture

The host regression generates a deterministic FRLG-family 128 KiB save rather than storing personal user data.

```text
Size:    131072 bytes
SHA-256: b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

Coverage includes:

- two rotating save slots;
- valid newer-slot selection;
- counter wraparound;
- safe older-slot fallback;
- Party enumeration;
- all 14 PC boxes;
- an 80-byte boxed PK3 crossing a PC-sector boundary;
- truncated input;
- bad signature/checksum;
- invalid/duplicate/missing sector identity;
- mismatched counters;
- unsupported family marker;
- invalid party/entity checksum cases.

Proven PK3 fields include species, PID, TID, SID, EXP, held item (raw/normalized), moves, PP, IVs, EVs, nickname and OT.

The inherited PKSE Gen III crypto path independently agrees with the expected fields and exact encrypted bytes. Untouched 80-byte boxed and 100-byte party records round-trip byte-identically through the PKSM-Core host oracle.

## Session 3B checkpoint A — native Switch backend

Verified source:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

GitHub Actions:

```text
PokeBank NX Host Tests
run #158
PASS
```

Full PKSM-Core is not linked directly into the Switch app. Its cross-generation virtual/conversion graph reaches exception-throwing code (including personal-data and unrelated later-generation paths), while the PokeBank NX native build intentionally uses `-fno-exceptions`.

The chosen architecture is therefore:

```text
PokeBank public Gen III adapter API
        |
        +-- HOST correctness oracle -> pinned PKSM-Core PK3/Sav3/SavFRLG
        |
        +-- SWITCH native backend   -> PKSMGen3NativeAdapter.cpp
```

Native implementation:

```text
src/Integration/Gen3/PKSMGen3NativeAdapter.cpp
```

`src/Integration/Gen3` is now part of the normal native `SOURCES` list.

The native backend selectively implements the already-proven read-only Gen III semantics behind the same adapter API, so callers do not depend on whether the backend is PKSM-Core or the exception-free native implementation.

Integration decision remains **ADAPTER-WRAPPER**, with a **SELECTIVE NATIVE BACKEND** forced by actual devkitA64/`-fno-exceptions` evidence.

## Current verification

At the verified `43f3a9f9...` checkpoint:

```text
existing host suites                     PASS
Gen III PKSM-Core oracle                 PASS
Gen III selective/native mirror tests    PASS
ASan/UBSan                               PASS
native devkitA64 exception-free slice    BUILDS
global C++ exceptions                    DISABLED
live source writes                       DISABLED
```

## Current next step — RetroArch FRLG runtime source

A later interrupted coding session reported uncommitted local work for a bounded RetroArch source catalog. It is not considered implemented until recovered and pushed.

Reported design:

```text
configured RetroArch savefile_directory
.sav / .srm only
max directory depth 2
max 256 candidates by default
full FRLG structural validation before name/path hints
ambiguous valid FRLG remains unclassified
Party / Boxes use the existing Gen III adapter model
```

The next coding task is to recover that local work and wire it into the real application source-discovery lifecycle so the runtime genuinely invokes the provider/catalog.

Expected path:

```text
RetroArch save root
-> bounded catalog
-> validated FRLG source
-> firered_gba / leafgreen_gba when reliable
-> native Gen III adapter
-> Party / Boxes
-> existing PokeBank source/browser lifecycle
```

No live writes, save repair, emulator communication or UI redesign belongs in this step.

## License / provenance

Pinned PKSM-Core remains a GPLv3 submodule and its notices must be preserved. The PokeBank wrapper/native backend must retain required attribution and must not copy source from projects classified as reference-only.

## Related issues

- #4 PKSM-Core Gen III spike — implementation milestone complete.
- #6 RetroArch discovery + legacy read-only adapters — current FRLG runtime work belongs here.
- #17 reproducible fixture corpus — deterministic FRLG fixture is the first active corpus case.
- #5 future PKHeX Oracle — still planned for independent correctness validation at broader scale.
