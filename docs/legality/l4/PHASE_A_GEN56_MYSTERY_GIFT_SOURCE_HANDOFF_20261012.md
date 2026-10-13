# L4 Phase A — Gen V/VI event and Mystery Gift source-evidence handoff

Date: 2026-10-12. Repository: GlitchedZeus/PokeBank-NX.
PR #292, branch `feature/legality-l4-events-meta-20261012`.
**New Gen V–IX L4 program, Phase A**: separate from the completed
Gen I–IV seven-stage program and not integrated into production or
coordinator PR #298/#301. MAIN, accepted legality, Product PR #300 and
all other lanes were kept unchanged.

## Actual source-backed implementation

- New `include/Legality/Gen56MysteryGiftSourceEvidence.h`: read-only
  bounded inspection of **exact 0xCC-byte PGF**, **0x108-byte WC6** and
  **0x310-byte WC6Full** records. The full WC6 payload begins at
  0x208; wrapper-only version/language restrictions at 0x000/0x1FF
  are not fabricated on a standalone WC6.
- The original gift species must be nonzero and the delivery discriminator
  must be supported (PGF entity card type 1; WC6 entity type 0).
  Unknown, item and malformed cases never yield Pokémon candidates.
  Neither the serialized *GiftUsed* flag nor a card-local date proves
  receipt; full wrappers and standalone gifts are distinct archive forms.
- Source metadata projection covers card ID, native gift original trainer
  numbers, origin, language, Ball, egg flag, and **generation-local raw
  ribbon flags**. These are *not* a parsed or cryptographically validated
  PK5/PK6 specimen.
- Before comparing a current native observation the caller must
  independently verify the **exact serialized file/WC6Full wrapper is
  bound to the original SHA-256**, original format and card ID, original
  release, individual recipient save/receipt and one-to-one source-to-
  current identity, plus whether the compared fields survived historical
  transformations. Missing any proof remains Unknown, not Invalid.
- PKHeX's hash-keyed `mgrestrict5.pkl` / `mgrestrict6.pkl` are **not
  embedded**. Only independently pinned language/console-region
  restriction bits may yield source-local mismatch; otherwise this is
  UnknownRestrictions. Language indexes use PKHeX's `1 << Language`;
  source-supplied region bits are **explicitly normalized** to index
  `1 << ConsoleRegion` before passing to this adapter.
- Breeding egg Ball helper follows PKHeX BallVerifier's *bounded*
  generation distinction: Gen V bred eggs default Poké Ball; Gen VI
  cannot breed-inherit Master/Cherish, but other Gen VI balls require
  **BallContext6 species/form/ability** (not reimplemented). **Never
  call this helper for PGF/WC6 event-gift eggs**.
- No original/retail gift archive bytes, PKHeX code/resources, card
  binaries or ROM assets are copied into PokeBank. No source-save writes
  or original receipt authentication is asserted. Each result is
  candidate-local and cannot declare global Valid or Invalid.

## PKHeX source provenance and rights

Consulted the **actual upstream C# sources** and pinned the immutable
GitHub commit `kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e`:

| Source file (PKHeX.Core/) | Git blob SHA-1 |
|---|---|
| `MysteryGifts/PGF.cs` | `540a8f78c9a25053a294b4c92ae605ff925d4e30` |
| `MysteryGifts/WC6.cs` | `254c8567843a8e1492936f632d0c4adf26104ade` |
| `MysteryGifts/WC6Full.cs` | `37b34fde6bd9f4e4d5d863aa2f8755ab07726272` |
| `Legality/Encounters/Verifiers/MysteryGiftVerifier.cs` | `79f7b134eab5ba8a17929b762b231f0d4ffe1d55` |
| `Legality/Verifiers/Ball/BallVerifier.cs` | `07ad723e2d2801f2ad29cd5cfe59c4c7e86a4118` |

The **owner-supplied PKHeX-master.zip is a DIFFERENT source pin**
(SHA-256 `b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b`);
its exact bytes were not mounted in this execution. These live upstream
files were checked directly, but **no byte-for-byte equivalence to the
owner's ZIP has been established**. Any future differential oracle or
upstream update must compare to the archive map and run original .NET
PKHeX.Core host methods; dotnet was not installed locally for this
run. Upstream PKHeX.Core is GPL-3.0-or-later; the independent native
metadata adapter includes no wholesale GPL code or retail database.
Source-only reuse and binary redistribution rights require their
separate L5 Q08 review; do not treat a code license as an event
distribution archive license.

## Verified tests — independently executed on exact committed blobs

`tests/fixtures/legality_l4_gen56_gift_source.cpp` is appended to the
L4-only runner without changing the 28 registered Gen I–IV tests.
PR-only four-way workflow path filters include its C++ fixture and
`Gen56MysteryGiftSourceEvidence.h`.

Final exact tested Git blob hashes:
- Header: `d017ec4930c2182447fbfe19f7452c3d5614c082`
- Fixture: `4a0ad982caa488ddaca010404eae53badbe792e0`

**4/4 focused programs PASS**: GCC and Clang C++20, each normal
`-O2` and ASan/UBSan `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer`, strict warnings including `-Werror`.
Checks: 16,384 independently attested source/receipt gating combinations;
65,536 language mask variants; 65,536 claimed original card IDs;
256 card-type labels; 256 Ball IDs; malformed/truncated WC6Full and
PGF; exact WC6Full wrapper offsets; zero species; unspecified Ball;
conditional source mismatch and unknown historical transfers.
No sanitizer finding and no retail-source fixture. Tested bytes'
`git hash-object` results match those committed above.

Representative commands:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 \
  tests/fixtures/legality_l4_gen56_gift_source.cpp -o /tmp/l4-g56
/tmp/l4-g56
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/fixtures/legality_l4_gen56_gift_source.cpp -o /tmp/l4-g56-san
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4-g56-san
# Repeat with clang++.
```

## Actual blockers and next major Phase A tranche

- Complete GitHub checkout still unavailable:
  `git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD`
  reports `Could not resolve host: github.com`. Thus the full L4
  28-suite registry, native encrypted PK5/PK6 links and host CI were
  **not executed locally**; exact final-head GitHub workflow must be
  inspected, not presumed PASS.
- **No verified PKHeX host oracle:** .NET is absent; direct upstream
  source reading is not differential verification and not the exact
  owner ZIP.
- No native PK5 parsing was duplicated: Production PR #300 owns it.
  Native observed `Native` fields here must be populated through a
  separately reviewed read-only PR #300 bridge, not a guessed binary
  layout. Gen VI PK6 native evidence must reuse coordinator PR #301's
  native foundation, not an additional universal decoder.
- Additional needed work: source-verified `mgrestrict5/6.pkl`
  key/bit mapping with permission review, Gen VI `BallContext6`
  species/form/ability host differential oracle, receiver-save proof,
  event-specific OT and ribbon history, real source-pin parity to ZIP,
  and full app-side native linkage. No authentic retail event, region
  restriction, entitlement or original historical receipt can be
  inferred from structural matches.

**L4 Gen V/VI Phase A implementation: delivered and focused-tested.
Complete-checkout and coordinator acceptance: BLOCKED.**
