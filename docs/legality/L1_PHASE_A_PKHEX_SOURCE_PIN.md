# L1 Phase A — Source-pin registry for PKHeX-native Gen V/VI static/trade templates

Generated native source: `include/Legality/Gen56PkhexSourceCatalog.generated.h`

**Legal/provenance:** PKHeX.Core © PKHeX contributors, GPL-3.0-or-later. This generated extract derives copyrightable table definitions; retain original licensing and attribution. PokeBank NX is AGPLv3. This is NOT the whole PKHeX encounter engine.

- Immutable upstream Git revision: `66ef5983a5d349efa2dd615f2926a1b1ee97306e`
- Supplied owner's archive SHA-256: `b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b`
- The **exact** `EncounterGenerator5.cs` and `EncounterGenerator6.cs` bytes at this revision match the owner source archive's published SHA-256 hashes `cae4866450951cfaa78c1f75e31ef99d7d2dabfa6aac544b9695d5daa7b59bf9` and `ce04375f3b148553b1900d0996ef6a79fbbe31ce0e6d31a0b3a8f0688d49f03e` respectively.
- This does **not** assert the four table files' SHA-256 matches the supplied archive; they are Git-blob pinned to the immutable upstream revision until independently crosschecked against the original ZIP.
- All array names, original row ordinals, game-version masks and static source fields are source-verified. In ORAS the six Cosplay Pikachu templates have dynamically variable encounter locations; those locations are deliberately **not** certified.

| PKHeX table file | Exact Git blob | Extracted rows |
|---|---|---:|
| `Gen5/Encounters5BW.cs` | `9ab0abd941e189261cab2ee302c3a7fd98da5a8b` | 44 |
| `Gen5/Encounters5B2W2.cs` | `9876a9cd700f2fcc337be05741029d1fc88c30f2` | 98 |
| `Gen6/Encounters6XY.cs` | `147d623de132e7210bf8449c40825cd70cb75da0` | 51 |
| `Gen6/Encounters6AO.cs` | `b59214398c756be172107dc09cdff9bf9684ff9c` | 85 |

**Coverage:** 278 static/gift/NPC-trade source templates across 21 arrays. Encounter5/6 **wild encounter area files**, Dream World, Dream Radar, event redemption databases, breeding and exact RNG PID/IV are NOT generated here. They must remain **Unknown / Incomplete**, never invalidated by an empty partial catalog.

**Pipeline:** PKHeX.Core Encounters5*/Encounters6* C# table initializers → named arrays and original index → conservative source-row extract → C++20 constexpr data. Generic `new(...){...}` rows and six original `BaseCosplay with` rows preserved; locations for the cosplay rows are dynamic. Trade positional metadata (OT/TID, language, gender), event payload conditions, flawless IV requirements and source-context RNG must be evaluated separately, not silently assumed.

This lane does not modify the Product UI, native PK5 decoder, accepted Legality::analyze, coordinator PR #298/#301, or original saves.
