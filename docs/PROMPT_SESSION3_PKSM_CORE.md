# PokeBank NX — Session 3 MAX PKSM-Core Prompt

Last updated: 2026-09-01

Use this only after issue #13 has produced a verified device-test `.nro` and any blocking physical-device regression has been handled.

```text
POKEBANK NX — SESSION 3 — DEVICE FEEDBACK + PKSM-CORE GEN III SPIKE

Use MAX reasoning.

Continue development of:

GlitchedZeus/PokeBank-NX

Development branch:

feature/pokebank-playable

PKSE is upstream only.
NEVER push PokeBank NX changes to upstream.

--------------------------------------------------
START FROM VERIFIED CURRENT STATE
--------------------------------------------------

Fetch and inspect the newest remote state.

Preserve any useful newer local work before switching/resetting anything.

Read:

PROJECT_STATUS.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/UPSTREAM_AUDIT.md
docs/PKSM_CORE_INTEGRATION.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
docs/GAME_SUPPORT_MATRIX.md
docs/BUILD_RECORD.md
GitHub issue #4
GitHub issue #17

Run baseline:

make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1

Do not build deep engine work on a broken baseline.

--------------------------------------------------
PRIORITY ZERO — PHYSICAL SWITCH FEEDBACK
--------------------------------------------------

Before PKSM-Core, inspect any exact hardware-test results I provided for the latest .nro.

If the physical test found a real blocker such as:

- crash
- broken navigation
- Action Sheet regression
- unsafe controller behavior
- unreadable theme/focus
- incorrect game detection
- resource/startup failure

fix the smallest coherent blocking set first.

Then rerun host tests/sanitizers/native build and provide a corrected .nro if another physical pass is needed.

Do not ignore a dangerous or crash-level device regression in order to start architecture work.

If hardware feedback is clean enough, continue immediately to issue #4.

--------------------------------------------------
PRIMARY ENGINEERING TASK
--------------------------------------------------

Begin GitHub issue #4:

Audit and spike PKSM-Core Gen III integration.

Pinned/research details are in:

docs/UPSTREAM_AUDIT.md
docs/PKSM_CORE_INTEGRATION.md

Do NOT rewrite Gen III from scratch before proving whether PKSM-Core can provide the mature implementation.

--------------------------------------------------
FIRST SPIKE ONLY
--------------------------------------------------

Focus narrowly on:

PK3
Sav3
FireRed / LeafGreen GBA
read-only save parsing
box/party extraction
active save slot / sector behavior
PK3 encryption/decryption/checksum behavior
untouched round-trip strategy
conversion hooks
integration/dependency cost

Candidate PKSM-Core areas include:

include/pkx/PKX.hpp
include/pkx/PK3.hpp
source/pkx/PK3.cpp
include/sav/Sav3.hpp
source/sav/Sav3.cpp
include/utils/genToPkx.hpp

Use the exact pinned revision recorded in the project docs unless a deliberate update is justified, tested, and documented.

--------------------------------------------------
INTEGRATION DECISION
--------------------------------------------------

Determine whether the right approach is:

DIRECT REUSE
ADAPTER / WRAPPER
SELECTIVE PORT
REFERENCE ONLY

Base the decision on:

- license compatibility / required notices
- devkitPro compatibility
- dependency footprint
- correctness
- maintenance burden
- testability
- performance
- overlap with existing PKSE code
- ease of comparing against PKHeX

Do not choose based only on code aesthetics.

--------------------------------------------------
LICENSE / REFERENCE RULES
--------------------------------------------------

PKSM-Core is GPLv3-family code with additional attribution/notice requirements described in docs/UPSTREAM_AUDIT.md.

Preserve required notices/attribution for direct reuse.

pkHouse remains REFERENCE ONLY.
Do not copy pkHouse GPLv2 source verbatim into PokeBank NX.

--------------------------------------------------
GEN III SAFETY
--------------------------------------------------

This spike is READ ONLY.

Do NOT enable:

- live Gen III save writing
- installed Switch game save writing
- mutation of personal test saves in place

Use copies, staged files, synthetic/golden fixtures, and known test vectors.

Issue #17 tracks the reproducible fixture corpus. If suitable fixtures are missing, create the smallest legal/reproducible local test-vector strategy needed for the spike and document the gap rather than quietly depending on an unidentified personal save.

--------------------------------------------------
TEST TARGET
--------------------------------------------------

Where fixtures permit, cover:

- valid FireRed save recognized
- valid LeafGreen save recognized
- invalid/truncated save rejected
- active Gen III save slot selected correctly
- sectors assembled correctly
- party Pokémon extracted correctly
- box Pokémon extracted correctly
- PK3 fields parse correctly
- encrypted/decrypted representation behaves correctly
- checksum behavior is understood/tested
- untouched no-edit operation does not corrupt data
- existing PokeBank safety tests remain green

If the PKHeX Oracle is not implemented yet, record the exact golden comparisons needed for issue #5.

--------------------------------------------------
DO NOT EXPAND SCOPE
--------------------------------------------------

Do not immediately jump into:

PK1
PK2
PK4+
full Living Dex
AutoMod generation
Mystery Gifts
modern Switch reverse engineering
live writes
Master Vault implementation

Prove PK3/Sav3 first.

--------------------------------------------------
END-OF-SESSION DELIVERABLE
--------------------------------------------------

Leave one coherent result:

A) PKSM-Core integrated enough to parse Gen III correctly

OR

B) a tested adapter spike proving the integration path

OR

C) a documented blocker showing exactly why direct integration is unsuitable and what selective approach should replace it

Run:

make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1

Update:

PROJECT_STATUS.md
docs/PKSM_CORE_INTEGRATION.md
and issue #4 as appropriate

Commit.
Push to origin.
Verify exact remote SHA.

If native app behavior changed enough to warrant physical testing, provide the actual .nro with:

filename
application source SHA
size
SHA-256

Do not mark DEVICE TESTED without an exact physical report.

Stop at a coherent Gen III checkpoint rather than starting another generation.

Start working immediately.
```