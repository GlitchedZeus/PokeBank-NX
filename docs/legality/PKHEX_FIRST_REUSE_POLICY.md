# PKHeX-first reuse policy — Gen I–IX legality engine

**Status:** coordination policy for standalone draft legality development, **not** permission to merge into Product UI, accepted legality, or `main`. PokeBank NX v1.0 must not be blocked by exhaustive legality parity.

## User-approved architecture and release lanes

- **Main app chat:** game/source discovery, validated save readers, provider identity, Pokémon editor parity, stage/backup/export UX, one hardware-test NRO. It owns the app implementation through Gen IX and subsequent full touch/UI polish.
- **Five legality specialist chats:** detached Gen I–IX rules (encounters/RNG, breeding, moves/transfer, distributions/events, independent QA). Continue improving *well beyond* v1.0. Every stage requires real source changes and host plus sanitizer verification where possible.
- **Coordinator legality branch:** forward-only integration, actual `Legality::analyze` connections, per-game format adapters and safety contracts. Do not merge into the app until explicitly requested near its Gen IX milestone.
- **Codex:** independently audit/fuzz/reference-compare/build the combined engine, with no authority to rewrite original saves or force-merge production branches.
- **After v1.0:** optional, nonblocking, versioned legality database/engine updates. A clean or Incomplete legality report never certifies historical authenticity.

## Reuse PKHeX rather than recreate it

**Default:** if PKHeX.Core already implements a mechanic or data source, prefer that *exact upstream algorithm/data definition* over an invented independent approximation.

PKHeX source snapshot supplied by the project owner: `PKHeX-master.zip`, SHA-256 `b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b`. The archive does **not** contain authenticated Git revision metadata: identify files by full SHA-256 in the companion source map and resolve/pin the corresponding upstream commit before upstream updates. Do not pretend that the mutable `master` branch is immutable.

`PKHeX.Core/PKHeX.Core.csproj` declares `net10.0` and `GPL-3.0-or-later`. PokeBank NX is AGPLv3. GPLv3 §13 permits combination with AGPLv3, with the licenses' notice/corresponding-source obligations retained. This does not settle separate rights to extracted event archives, trademarked graphics, ROM assets, contributed databases or other third-party files. Keep a per-file license/provenance register and preserve author/copyright statements for reused/transliterated portions.

**What can be copied intact:**
- PKHeX's C# source into a **host-side** .NET oracle/test tool or version-pinned upstream reference, retaining GPL notices.
- Source-authenticated immutable legality tables and numerical data into a native generated artifact, retaining metadata and attribution where copyrightable.
- Original PKHeX regression inputs if their rights/provenance permit redistribution (otherwise use local-only test assets).

**What cannot be dropped directly into a Nintendo Switch NRO:** the .NET 10 C# runtime/library. The app uses devkitA64 native C++20 and no bundled .NET runtime. Do **not** promise that copying `.cs` into `src/` produces a running legality engine. Use a minimal hand-adapted C++ shim *only where mandatory*, with a line-to-line upstream method map and differential native results. Avoid maintaining two independently invented legality algorithms.

**Fastest dual-runtime workflow:**
1. Pin a PKHeX version/commit and snapshot hashes; record license and rights.
2. Run *unmodified* PKHeX.Core on host as the behavioral oracle, including exact source-game/specimen, structured encounter and error codes (not just a final green/red status).
3. Extract/generate static per-game resources from the source-pinned PKHeX tables into compact C++ binary/constexpr assets. Do not embed the entire WinForms app, PKHeX GUI or unused resources into the NRO.
4. Translate only unavoidable C# evaluation logic to native C++20 with upstream file/method provenance and tests for every family/version; preserve unknown history as `Incomplete`.
5. Run host oracle-vs-native differential regression, property/fuzz, normal and ASan/UBSan on the *exact final Git HEAD*. Add original-retail specimens only with consent/provenance; synthetic data does not certify retail parity.
6. Merge only reviewed, versioned results onto the coordinator branch; app integration requires separate user approval after Gen IX editors.

## Five specialist chat assignments — direct file targets

| Lane | Prefer PKHeX.Core sources | Deliverables |
|---|---|---|
| L1 Encounter/RNG | `Legality/Encounters/Generator/ByGeneration/EncounterGenerator{1..9,9a}.cs`, `Legality/Encounters/Data/Gen*/`, `Legality/RNG/` | Exact original-game candidate generators + evidence alternatives, never a one-candidate global Invalid |
| L2 Egg/breeding | `Legality/Breeding.cs`, `Legality/Encounters/Templates/Gen*/EncounterEgg*.cs`, `Legality/Verifiers/Ability/` | Generation-specific hatch/parents/PID/IV/egg-move constraints and source-receipt uncertainty |
| L3 Moves/history | `Legality/LearnSource/Group/LearnGroup{1..9,9a,HOME}.cs`, `Legality/LearnSource/Sources/`, `Legality/Verifiers/TransferVerifier.cs` | Per-title level-up/machine/tutor/relearner, evolutionary joint four-slot history and Bank/HOME transfers |
| L4 Events/metadata | `Legality/Encounters/Verifiers/MysteryGiftVerifier.cs`, `Legality/Verifiers/Ball/BallVerifier.cs`, `Legality/Encounters/Data/EncounterEvent.cs`, `MysteryGifts/` | Native event/memento/ribbon/Ball and gift restrictions; separate licensed event payloads from rules |
| L5 Native QA/parity | `Legality/LegalityAnalysis.cs`, `Legality/Structures/LegalInfo.cs`, `Legality/Verifiers/`, upstream `Tests/PKHeX.Core.Tests/` | Pinned host oracle harness, honest per-game parity matrix, original-source immutability, sanitizer gates, false-Invalid audit |

**Codex task:** establish an automated source-pin/licensing verifier, host-side PKHeX.Core oracle, test-case exporter, exact-head C++ build/sanitizer matrix, and differential report. Codex should actively refuse any unverifiable "100% parity" or "CI passed" claim.

## Native format reuse priorities

1. **Gen V:** reuse the already-written product `PR #300` `include/Encryption/Encryption5.h`, `Pokemon5ReadOnly.h`, and `Pokemon5ReadOnlyView.h`; compare against PKHeX.Core `PKM/PK5.cs` and `PKM/PK5.cs`-adjacent encryption. Do not build a second divergent native PK5 codec. Product writes remain in staged copies only.
2. **Gen VI:** adapt source-pinned `PKM/PK6.cs`, preserving 232/260-byte distinction and 3DS source family. A bounded PK6 envelope is **not** a decoder.
3. **Gen VII:** `PKM/PK7.cs` for SM/USUM; treat LGPE PB7 separately.
4. **Gen VIII:** distinct PK8, PB8, PA8 rules/records; SWSH, BDSP and PLA cannot share one universal encounter path.
5. **Gen IX:** `PKM/PK9.cs` and `PKM/PA9.cs`, `EncounterGenerator9.cs` / `EncounterGenerator9a.cs`; Scarlet/Violet and Legends Z-A are separate formats/rules, including future patch/DLC coverage.
6. **Across all:** PKHeX HOME history and transfer verifier are the canonical reference; never infer a complete cross-generation physical history from a detached Pokémon.

## Do not

- Create a second giant clean-room "PKHeX equivalent" in parallel.
- Vendor the entire PKHeX repository into the deployed Switch binary.
- Change `main`, the accepted Gen I–IV branch, five source lanes or Product UI without explicit approval.
- Force-push, reset, rebase backwards, mutate original save files, fabricate captures/parents/gifts, or claim unrun tests green.
- Equate licensing of GPL source code with permission to redistribute artwork, proprietary events, ROMs or third-party archives.

**Acceptance language:** implemented / normal-tested / sanitizer-tested / wired into actual analyzer / authenticated against original-retail PKHeX. Report each separately. A source file copied into the tree is not necessarily working application code.
