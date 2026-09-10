# Official-path transfer corpus audit — 2026-09-09

> Research only. `Official-path captured fixture` means a community-captured before/after specimen from an official Nintendo/Game Freak transfer process. It does **not** mean Nintendo published the file as a regression vector.

This note preserves the strongest new finding from the deep four-gap research: classic transfer boundaries III->IV, IV->V, V->VI and VI->VII are no longer fundamentally under-researched.

## Source grading

| Boundary | Best current source | Research grade | Main caveat |
|---|---|---:|---|
| III -> IV | Project Pokémon Pal Park Research | A | Community/research capture, not Nintendo-published vectors |
| IV -> V | RoC's PC + classic Poké Transfer research | A- | RoC redistribution license unclear |
| V -> VI | RoC's PC + Japanese LanguageID Transporter research | A- | Bulk corpus less laboratory-like than focused edge-case threads |
| VI -> VII | RoC's PC Bank collection | A- / B+ | Pairing excellent; provenance should be checked per collection |
| VII -> VIII | self-capture Bank -> HOME before Feb. 2027 | CAPTURE NOW | official service has a hard deadline |
| VIII -> IX | self-capture through HOME + native HOME dumps | RESEARCH/CAPTURE | server state and per-game HOME views complicate flat pairs |

## III -> IV: Pal Park

Project Pokémon Pal Park research includes real Gen III/Gen IV saves and extracted outputs, real hardware, multiple GBA cartridge revisions and multilingual transfer cases.

Reference:
https://projectpokemon.org/home/forums/topic/41574-gen-4-pal-park-research/

High-value dimensions include:

```text
Japanese / English / Spanish / French / Korean
US Emerald
US FireRed revision 0
US LeafGreen revision 1
US Ruby revision 2
US Sapphire revision 0
transfer order
nickname / OT trash-byte effects
held items
met data
```

Treat as a Grade-A captured-real-hardware fixture source.

## IV -> V: Poké Transfer

Classic Project Pokémon research contains explicit comparisons between:

```text
original Gen IV entity
official Poké Transfer output
third-party converter output
```

including ordinary, hatched, shiny, fateful/event and Gen III-origin cases.

Reference:
https://projectpokemon.org/home/forums/topic/13091-pok%C3%A9-transfer/

### RoC's PC bulk pairing

Repository:
https://github.com/ReignOfComputer/RoCs-PC

Relevant directory relationship:

```text
09 - Gen IV - DPPt Collection/
10 - Gen IV - HGSS Collection/

18 - Gen V - Gen IV Poke Transfer/
    09 - Gen IV - DPPt Collection/
    10 - Gen IV - HGSS Collection/
```

Stable numbering/species names make large before/after pairing practical.

## V -> VI: Poké Transporter

A focused Project Pokémon thread captures a Japanese Gen V edge case where an entity with `LanguageID = 0` was passed through official Poké Transporter and emerged with Japanese language ID set. That kind of obscure real transformation is exactly why official-path fixtures matter.

Reference:
https://projectpokemon.org/home/forums/topic/41618-bug-japanese-pokemon-white-in-game-trade-considered-illegal/

RoC pairing:

```text
14 - Gen V - BW Collection/
15 - Gen V - B2W2 Collection/

23 - Gen VI - Gen V Poke Transporter/
    14 - Gen V - BW Collection/
    15 - Gen V - B2W2 Collection/
```

The output files use PK6-era representations and stable source naming, making automated differential pairing feasible.

## VI -> VII: Pokémon Bank

RoC's PC provides a particularly useful structure:

```text
19 - Gen VI - XY Collection/
20 - Gen VI - ORAS Collection/

28 - Gen VII - Gen VI Poke Bank/
    19 - Gen VI - XY Collection/
    20 - Gen VI - ORAS Collection/
```

Example naming pattern preserved by the research:

```text
001 - Bulbasaur - <stable-id>.pk6
 ->
001 - Bulbasaur - <same-stable-id>.pk7
```

This is well suited to a transfer-pair indexer.

## Pairing validator design

Do not trust filenames alone. For every candidate pair:

```text
1. parse source
2. parse target
3. validate stable identity fields expected to survive
4. record exact field delta
5. compare with PKHeX behavior
6. compare with PKSM-Core where applicable
7. emit a machine-readable rule report
```

Suggested output:

```json
{
  "fixture": "roc_xy_001_bulbasaur",
  "boundary": "6_to_7",
  "source_sha256": "...",
  "target_sha256": "...",
  "source_format": "PK6",
  "target_format": "PK7",
  "deltas": {}
}
```

Host-tool outputs can become:

```text
transfer_pairs_4_5.json
transfer_pairs_5_6.json
transfer_pairs_6_7.json
```

## Fixture manifest rule

Do not dump unknown-rights binaries into the main repo merely because they are technically useful.

Record:

```text
id
boundary
source_kind = community_official_path_capture
capture_method
source URL
redistribution_status
source/destination games + languages
source/target SHA-256
expected ruleset
```

Fixture availability states:

```text
BUNDLED
DOWNLOADABLE
LOCAL_ONLY
SELF_CAPTURED
GENERATED
```

If redistribution is unclear, commit metadata/hash/source URL and require a developer-local import/fetch step rather than bundling the fixture.

## Current conclusion

Later implementation of III->IV, IV->V, V->VI and VI->VII can be driven by differential tests against real official-path captured evidence, PKHeX and PKSM-Core instead of reconstructing every rule from scratch.

The urgent remaining preservation boundary is Bank -> HOME; see `BANK_HOME_PRESERVATION_CAPTURE_PLAN.md`.
