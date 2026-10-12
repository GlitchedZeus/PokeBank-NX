# L2 B07 — original retail Gen III egg PID / Everstone history and joint IV witnesses

**Date:** 2026-10-12. **Lane:** L2 / #287. **Status:** Source-pinned original per-game retail algorithms, NEW exact-GitHub-file standalone C++ tests **PASS** in GCC, Clang and sanitizer runs. **NOT** integrated into the global legality engine or the coordinator-owned host Makefile. Original retail executable differential and original archival paired-parents-and-egg provenance remain unverified.

## Frozen primary references and original algorithms

Research #252 [SOURCE_REGISTRY.json](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/SOURCE_REGISTRY.json) fixed these retail source revisions. Source paths were independently retrieved and re-read on this lane; file Git **blob IDs below** do not replace research #252's original-source SHA-256 registry.

| Retail source game | Pinned original source and Git blob | Original pending / collection PID behavior |
|---|---|---|
| Ruby / Sapphire | [pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f `src/daycare.c`](https://github.com/pret/pokeruby/blob/5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f/src/daycare.c), blob `ec89f89a15b18cf36328ed8d03229a869a9b0f35`; `_TriggerPendingDaycareEgg` lines 363–367; `SetInitialEggData` lines 714–722 | Store `(Random() % 65534)+1` as pending low 16 bits. When collected, one separate `Random()<<16` provides the final high 16 bits. No original Everstone nature-selection path here. |
| Emerald | [pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881 `src/daycare.c`](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/daycare.c), blob `ef4eaabb531c483187dd30f8b227c26d04d28f47`; `GetParentToInheritNature` lines 414–453, `_TriggerPendingDaycareEgg` lines 455–487; `SetInitialEggData` lines 855–863 | Reseed **second independent RNG** using `SeedRng2(gMain.vblankCounter2)` (u16). Female parent or Ditto (Ditto precedence) supplies nature if it originally held Everstone and `Random()<0x7FFF`; otherwise no inheritance. Nature branch uses `(Random2()<<16)|Random()`, requires nonzero PID and nature `%25`, retry max **2,401** candidates, retain last candidate on exhaustion. Without inheritance, `(Random2()<<16)|((Random()%65534)+1)`. Store full PID before collection; collection consumes **no additional PID-high-half RNG draw**. |
| FireRed / LeafGreen | [pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14 `src/daycare.c`](https://github.com/pret/pokefirered/blob/037335f4c725d7c9aecdac87066f2002b4bd7e14/src/daycare.c), blob `efa5a951e7ecca5ba6c22d17ca9dfe86a0284d07`; `_TriggerPendingDaycareEgg` lines 721–752, `SetInitialEggData` lines 1114–1122 | **The apparent Everstone inheritance routine is commented out, NOT executable retail code.** Active pending PID is `(Random()%0xFFFE)+1` and collection adds one `Random()<<16` exactly as original RS. Do not apply Emerald Everstone or Gen IV Masuda effects. |

All three original `src/random.c` routines implement `Random()` by cycling `seed*1103515245+24691` modulo 2^32 and returning the high 16 bits. Ruby RNG source blob `8f82b722f24a4a124fff6eec4f87f708833778e0`, FireRed RNG blob `635a1df5a59fa42a88f30ad884405db2bde95ea5`. Emerald [`src/random.c`](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/random.c), blob `aa2c67750da654bd5d1d4ffbfa61f2fc3390cec6`, has **independent** `gRngValue` and `gRng2Value`, with `SeedRng2(u16)` and `Random2()` cycling the same multiplier on the separate state.

Source boundaries matter: two Ditto cannot be an original eligible ordinary daycare pair; the female/Ditto nature donor is **not automatically father** or physical slot 0. The stage attestation must carry original pair, original gender, original creation-time held item, original Emerald VBlank counter, and **separately known** relevant LCRNG states.

## New L2-owned engineering — local candidate checking ONLY

1. `include/Legality/Gen3EggPIDCreationHistory.h`: independent per-game original PID creation/collection model. Original physically deposited pair order and original parent snapshots are independently verified, not reverse-engineered from a modern PK3. Ruby/Sapphire and FRLG differ from Emerald even if the final egg PID happens to match. Dedicated known-parent stage tokens bind pending and collected egg to the same original event, without equating distinct RNG points in time. Emerald's donor-item check short-circuits the correct primary-stream draw if no original Everstone; its 2,401-stage draw counter is **u16**, not overflowing u8. Missing source evidence becomes Unknown. A mismatched fully specified PID history excludes **only that historical route**, not whole-Pokémon validity.
2. `include/Legality/Gen3JointEggPIDIVHistory.h`: links the new B07 original PID history to the **unchanged accepted** `Gen3EggIVInheritanceStage.h` source-backed six-draw IV model (research #252 RNG-001 and its 24 original-source synthetic fixtures). The **same original egg PID and unhatched IVs**, physical parent snapshot IDs and original daycare event must be independently attested across both stages; never cross-union two different invented parental histories. IV stage's own final seed is a distinct and separately supplied historical RNG boundary. No source save, working copy or PK3 is edited.

**Not addressed by these narrow helpers:** true daycare compatibility/egg groups from the versioned personal data, four-slot move and move-source chain, birth species, original trainer changes, transfer trails, wondercard / gift egg event alternatives, and original historical replay starting from a specific save's witnessed entry. These are not made globally Invalid by these helpers.

## Exact committed-source test validation

All **FOUR** new-file local copies were verified byte-for-byte against their **GitHub blob SHA-1 IDs** before compilation; the already accepted dependency `Gen3EggIVInheritanceStage.h` was likewise verified at blob `23581c067cb831187824beb5fd4beb1ade3b91ae`.

| Exact committed test file | Git blob SHA-1 | Confirmed execution |
|---|---|---|
| `include/Legality/Gen3EggPIDCreationHistory.h` | `69a147b006630fb599702527d85a7b47c83bd783` | Strict GCC and Clang C++20 normal and ASan/UBSan |
| `tests/test_legality_l2_gen3_egg_pid_creation.cpp` | `0ebda9c01b3b5978e2b7c541a3c9f334522e2125` | **20,508** synthetic seeded/fixed tests across five original games + explicit negative and missing provenance assertions |
| `include/Legality/Gen3JointEggPIDIVHistory.h` | `b500f2d9bbeeeb04bd48391a3374ed264a568a7e` | Strict GCC and Clang C++20 normal and ASan/UBSan |
| `tests/test_legality_l2_gen3_joint_egg_pid_iv.cpp` | `d9d577f240e885fcca9710471e1e744150b81e61` | **5,120** synthetic joint original PID/IV histories across five games plus **50** other provenance/malformed controls |

Executed locally with `-std=c++20 -Wall -Wextra -Wpedantic -Werror`, `-O2` for normal, `-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer` for sanitizers, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1`, `UBSAN_OPTIONS=halt_on_error=1`. Four executions per test (GCC normal, Clang normal, GCC sanitized, Clang sanitized), **all pass**. Correctness of stored fixed vector PINs was independently checked with a separate simple mathematical LCRNG recurrence.

These are genuine compiled **exact-source** L2 standalone tests, **not** a full checkout's exact-head GitHub Actions, not an original retail executable differential, and not 25,628 archival Pokémon.

## Remaining integration and independent QA gates

- Coordinator-owned Makefile/workflow must register and compile all **nine** prefixed `tests/test_legality_l2_*` and run exact-head full host, focused RSE, ASan/UBSan. Do not modify coordinator-owned build infrastructure in this lane.
- L5 should independently replay original Emerald's dual RNG and original original-game egg PID creation/collection semantics against matching retail binaries and native paired stage bundles; verify regional differences, VBlank timing, the 0x7FFE / 0x7FFF Everstone gate and 2,401st-attempt fallback. Original full-stage source cannot be inferred from detached PK3.
- L2 branch's ancestry diverges from newer accepted #283 and HGSS sibling #284 advances separately. Coordinator forward-reconciles only while preserving newer accepted commits. Never reset/rebase backward, force push, self-merge, integrate into MAIN/#103 or mutate original saves. Research reference license gate remains unresolved; no copied retail function bodies, PKHeX GPL code or binary assets added.
