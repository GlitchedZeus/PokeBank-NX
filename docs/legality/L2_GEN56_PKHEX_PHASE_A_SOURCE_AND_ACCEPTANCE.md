# L2 Phase A — Gen V / VI PKHeX-first original egg permissions and source-parent histories

**Branch / PR:** `feature/legality-l2-breeding-20261012` / #295. Draft. No coordinator merge, production analyzer dispatch or save writes. New phase AFTER the seven-stage Gen I–IV program; does not replace its pending acceptance.

## Pinned source / license

Native output includes a narrow adaptation of **PKHeX.Core GPL-3.0-or-later** logic and an EXACT copy of the Gen V Hidden Ability 82-byte exclusion bitmap (not an independent replacement algorithm). Preserve PKHeX attribution, license/corresponding-source obligations and PokeBank NX AGPL obligations.

Upstream **Git SHA** inspected separately: `kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e`.

| Original PKHeX source | Original Git blob | Implemented native member |
|---|---|---|
| `Legality/Verifiers/Ability/AbilityBreedLegality.cs` | `98c2f5d3695d30da8f5b8663faf149d10adf7afd` | `BanHidden5`, `isHiddenPossible5`, `isHiddenPossible6` |
| `Legality/Encounters/Templates/Gen5/EncounterEgg5.cs` | `704e6ea78a027b652f7348e9063d0f216baa0d75` | original Gen V egg Ball.Poke, ability permission |
| `Legality/Encounters/Templates/Gen6/EncounterEgg6.cs` | `8f6660e78f8e7f7340a01abf656e4ff92d34ebbe` | Gen VI inherited ball permission, ability/form |
| `Legality/Breeding.cs` | `a4889d7704b3a7b4aac64318c6bf085378e3420e` | hatch restrictions, split baby species, species determined by gender |

The owner-supplied `PKHeX-master.zip` is separately recorded in `docs/legality/research/pkhex/PKHEX_USER_ARCHIVE_SOURCE_MAP.json` on the Gen V–IX foundation lane, with SHA-256 `b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b` and **unverified Git revision**. That ZIP's original `Breeding.cs` SHA-256 is `605de22e8eac51091601c2c7424141ad1157093537e096631e3fa7302a700e61`. **Do not assert** it is byte-identical to the separately pinned upstream revision. The ZIP was not mountable into this session. Upstream differential acceptance compares against the explicit pinned revision above; comparison to the supplied ZIP remains a separate required source-reconciliation gate.

## Committed implementation

- `Gen56PKHeXEggTemplate.h`: eight source titles BW/B2W2/XY/ORAS, original Hidden Ability bitmask/form exclusions, Gen V Poké Ball versus Gen VI inherited-ball context, Gen I–VI no-hatch species, split-baby and gender-determined egg species. Only *candidate-local* incompatibility; a detached current Pokémon is never declared globally Invalid.
- `Gen56OriginalEggHistories.h`: up to eight independently identified original egg histories with source game, exact egg/event/pair identities, physical deposited parents, source-held items, donor role/sex, and separately attested Hidden Ability transmission method. No parental identity or original RNG inferred from a detached PK5/PK6. A separately proven matching route wins over bad siblings; duplicates and unenumerated alternatives never prove exhaustive global illegality.
- Two exact-source native regression fixtures and one 9,590-row native CSV exporter.
- A host-only `net10.0` **actual PKHeX.Core** ProjectReference oracle (not deployed in NRO) and branch-only GCC/Clang, normal+ASan/UBSan and C# differential GitHub workflow.

## Acceptance

Local source-exact GCC/Clang normal and ASan/UBSan compile-and-run verified both fixtures and CSV exporter on all four combinations. The native CSV has 9,590 rows and identical SHA-256 `bb6b436888a2af886ea5307165d8ac40639119a3d4141f63343e470b4bf25a80` in all four modes. The real PKHeX.Core C# oracle **was not run locally** (no dotnet or full Git checkout). GitHub `L2 Gen V VI PKHeX First Breeding Phase A` must PASS on the exact head and compare every row before claiming upstream oracle parity.

Still NOT covered: source-authenticated original hidden transmission probability and Ditto-specific mechanics, complete Gen V/VI daycare RNG/PID/IV, egg-move inheritance chains, all form/ball exceptions and unmodified PK5/PK6 decoder integration, source ZIP revision reconciliation, whole-app host/device/link gates and coordinator wiring. These remain Incomplete/Unknown; they are not a 100% legality engine or a Gen VI save reader.
