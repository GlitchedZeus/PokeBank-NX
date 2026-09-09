# Transfers, HOME semantics, provenance and golden fixtures — 2026-09-09

> Research only. This file does not activate implementation scope. Exact external revisions/licenses must be rechecked before reuse.

This note consolidates the September 9 research on cross-generation transformations, real before/after fixtures, HOME/Bank provenance and test-corpus design.

---

# 1. Treat every generation boundary as its own converter

Do not build one opaque:

```text
convert(anyPokemon, targetGeneration)
```

Instead model the actual sequential transformations:

```text
PK3 -> PK4
PK4 -> PK5
PK5 -> PK6
PK6 -> PK7
PK7 -> PK8 / modern HOME-era model
PK8 -> PK9
```

An 8-step jump should execute known boundaries in sequence so every transformation can be tested, explained and audited.

Each boundary can change a different combination of:

```text
met location
met level
transfer date
moves
relearn moves
held items
PP / PP Ups
friendship
language
nickname / OT encoding
trash bytes
ribbons
PID / encryption constant
ability / ability slot
gender
origin game
markings
EV limits
memories / geodata
handling trainer
HOME tracker/sidecar state
scale/size
format-specific padding
fields that disappear in the destination
```

Primary behavioral oracles:

```text
PKHeX
PKSM-Core
current PKSE where relevant
```

Use multiple independent implementations/test vectors when possible rather than assuming any one old converter is perfect.

---

# 2. PKSM-Core contains useful conversion behavior already

The September sweep confirmed that the already-known/vendored PKSM-Core lineage is not only a save parser. It contains actual conversion logic, including Gen III -> IV and Gen IV -> V behavior.

Fields handled by the Gen III -> IV path include examples such as:

```text
species
nickname/text encoding
experience
gender
nature
language
EVs
IVs
moves
PID
version/origin
ball
Pokerus
OT
met data
ribbons
ability slot
held item mapping
Gen III HM cleanup
checksum
```

Recommended development model remains:

```text
PKSM-Core result ----┐
                     ├-> compare / derive expected behavior
PKHeX result --------┘
                     -> PokeBank-owned native exception-free implementation
```

Do not leak PKSM concrete types through PokeBank UI/business logic. Keep the current host-oracle/native-core boundary.

---

# 3. Poke_Transporter_GB + PCCS is a high-value Gen I/II -> III reference

Repository:

```text
Striaton-Lab-Team/Poke_Transporter_GB
```

The project transfers RBY/GSC Pokémon into RSE/FRLG-era Gen III formats and explicitly uses the Pokémon Community Conversion Standard.

Repository license at the reviewed revision: MIT for project code.

PokeBank must **not** copy its destructive source-save behavior. PokeBank keeps its own Vault/staging/backup/provenance semantics.

## PCCS

Repository:

```text
Striaton-Lab-Team/Pokemon-Community-Conversion-Standard
```

PCCS is valuable because it makes conversion policy explicit rather than pretending there is one uniquely correct unofficial Gen I/II -> III transformation.

Modes described in the research:

```text
FAITHFUL
LEGAL
VIRTUAL
ORIGINAL
```

The policy table covers behavior for fields such as:

```text
Personality Value / PID
Trainer ID
nickname
language
misc flags
OT name
markings
species
item
EXP
PP bonuses
friendship
moves
EVs / Stat Experience
contest stats
Pokerus
met location
met level
game of origin
ball
trainer gender
IVs
ability
ribbons / fateful encounter
Unown letter
size
nature
shininess
```

This suggests a strong future PokeBank UX direction:

```text
Recommended / Legal
Preserve Original Traits
Virtual Console Style
Advanced / Legacy policy
```

Every mode should preview what changes and why. Never silently rewrite the Pokémon.

---

# 4. Real before/after transfer fixtures are the gold standard

The most valuable testing discovery from the sweep is the existence of controlled research material representing both sides of official transfer operations.

## Gen III -> IV: Pal Park

Project Pokémon Pal Park research includes actual Gen III/Gen IV saves and extracted Pokémon used to compare migration behavior, including language/region variation.

That gives near-ideal golden tests:

```text
REAL PK3 BEFORE
    -> official Nintendo Pal Park
REAL PK4 AFTER
```

Research also indicates transfer ordering can affect residual/trash-byte behavior, so fixtures should preserve order/context metadata.

## Gen IV -> V: Poké Transfer

Historical Project Pokémon research investigated the actual transformation. It is less clean than the Pal Park corpus but still valuable evidence/test material.

Caution: some old homemade Poké Transfer implementations predate complete retail behavior and should not be treated as exact just because they exist.

## Gen VI -> VII: Pokémon Bank

Controlled research exists comparing examples like:

```text
original PK6
actual Bank-produced PK7
software-converted PK7
```

including details such as geolocation/metadata behavior.

## GO -> HOME -> Switch-era representations

Modern research archives include multiple representations of the same transferred Pokémon across HOME/game formats, including forms such as:

```text
PK8
PB8
PA8
PK9
... newer HOME-era representations
```

This is especially valuable for validating the HOME sidecar model empirically rather than inventing a flattened universal format.

---

# 5. HOME Tracker and modern provenance rules

Permanent rule:

```text
Never fabricate a HOME Tracker.
```

HOME tracker values are server-assigned historical state. An offline app cannot legitimately recreate official HOME history by generating a plausible number.

PokeBank should separate observed official state from its own local provenance:

```text
struct Provenance {
    optional<HomeTracker> importedObservedTracker;
    vector<PokeBankTransferEvent> localHistory;
    TransferAuthority authority;
};
```

Suggested authority states:

```text
OfficialObserved
OfficialModeled
CompatibilityConversion
Unknown
```

or a richer provenance enum such as:

```text
Native
OfficialBankObserved
OfficialHomeObserved
OfficialGoHomeObserved
CompatibilityConverted
Unknown
```

Example:

```text
HOME tracker:
    imported unchanged if genuinely present

PokeBank history:
    FireRed GBA
      -> compatibility conversion
    Platinum DS
      -> compatibility conversion
    Scarlet Switch

Authority:
    PokeBank NX compatibility conversion
```

Do not label that chain as an official Bank/HOME journey unless it was actually observed/imported as such.

Handling Trainer data is also separate from OT history and should not be flattened into one trainer identity.

---

# 6. Preserve modern HOME/game-specific representations rather than flattening them

Research supports modeling HOME-era Pokémon more like:

```text
canonical semantic identity/state
+
original PKM bytes
+
HOME snapshots/sidecars when observed
+
game-specific representation(s)
+
provenance
```

not:

```text
one universal PKM + tracker
```

This protects game-specific fields and future transfer semantics that do not map cleanly into one structure.

---

# 7. Multilingual Pokémon-object corpus

Project Pokémon's maintained NPC in-game Trade/Gift collection was identified as a particularly strong entity/conversion fixture corpus.

Coverage found in the sweep spans Gen I through Gen IX and includes languages such as:

```text
English
French
Spanish
German
Italian
Japanese
Chinese
Korean
```

Use cases:

```text
character conversion
OT names
nicknames
language flags
gender
species/forms
Gen IV/V text conversion
Chinese/Japanese/Korean handling
modern PKM structures
cross-gen conversion
legality preservation
```

This corpus is useful even without whole-save files because thousands of individual Pokémon objects can drive differential conversion tests.

Research noted maintainer sharing/attribution language for the collection, but exact redistribution terms/provenance should still be recorded per imported fixture.

---

# 8. Public save corpuses and fixture sources

Useful sources identified during the sweep include:

```text
Project Pokémon public Saves section
Project Pokémon Base Pokémon Save Files
Pal Park research fixtures
RoCs-PC
NX_Saves
PKMDS fixtures
PKHeX generated blank/test saves
Pokémon HOME / Save File Backups-style collections
user's own physical controlled saves
```

Examples found in public corpuses include:

```text
Sword pre-DLC
Sword post-DLC 1
Shield DLC
Legends Arceus
Scarlet Italian
Scarlet Japanese
Scarlet Korean
Scarlet Spanish
LGPE language variants
FRLG specialized saves
GameCube saves
```

An aggregate fixture project encountered in the research described roughly:

```text
~60 real save files
20 games
18,513 Pokémon
207 PKx fixtures
```

Treat those counts as research-source claims to verify before relying on them; the important part is the source-list leverage.

---

# 9. Fixture repository/manifest design

Do not dump arbitrary `.sav` files into the main Git repository.

Preferred structure:

```text
tests/
  fixtures/
    public/
    generated/
    local-required/
    mutations/
    golden/

  manifests/
    corpus.json
```

or a future separate `pokebank-fixtures` repository if size/rights justify it.

Example manifest entry:

```text
{
  "id": "sv-jpn-example-001",
  "game": "pokemon_scarlet",
  "revision": "...",
  "language": "ja",
  "container": "sav",
  "sha256": "...",
  "source": "...",
  "redistributable": false,
  "expected": {
    "trainer": "...",
    "party_count": 6,
    "parse": "valid"
  }
}
```

For an official transfer golden fixture, also record:

```text
operation
source/destination games
hardware/environment
before format + SHA-256
after format + SHA-256
expected changes
must-preserve fields
researcher/source
redistribution status
notes such as transfer order
```

If redistribution rights are unclear:

```text
commit source URL
commit expected SHA-256
commit metadata/expected results
DO NOT commit the copyrighted fixture itself
```

A developer fetch/import script can populate locally permitted corpuses.

---

# 10. Differential test strategy

For entity fixtures:

```text
load
 -> identify format
 -> parse
 -> serialize
 -> parse again
 -> compare semantic identity
```

For conversions:

```text
source PKM
 -> PokeBank converter
 -> compare against PKHeX oracle
 -> compare against PKSM/Core or real official after-fixture where possible
 -> field-by-field diff
```

Boundary-specific suites:

```text
tests/conversion/3_to_4/
tests/conversion/4_to_5/
tests/conversion/5_to_6/
tests/conversion/6_to_7/
tests/conversion/7_to_8/
tests/conversion/8_to_9/
```

Fixture dimensions should include:

```text
normal/shiny
male/female/genderless
egg
event
forms
ribbons
odd balls
legacy/invalid moves
languages
OT edge cases
text/trash-byte cases
size/scale cases
HOME sidecars where applicable
```

---

# 11. Golden test levels

Recommended escalation:

```text
Level 1: parser golden test
Level 2: untouched byte-preservation roundtrip
Level 3: one-field edit / expected-region diff
Level 4: version/revision compatibility
Level 5: real transfer before/after golden test
Level 6: corruption/recovery fixtures
Level 7: fuzzing
Level 8: physical Switch write/commit torture tests
```

No live-write adapter should skip directly to Level 8 without the earlier levels.

---

# 12. Clean-room/license posture

PKHeX is an exceptional behavioral oracle but GPLv3. PKSM is also GPL-family code. PokeBank should deliberately track whether a given external source is:

```text
DIRECT-CANDIDATE
PINNED-HOST-TOOL
REFERENCE
HISTORICAL
```

If direct licensing strategy is not explicitly compatible, use these projects as:

```text
behavioral reference
format research
fixture validator
host oracle
```

and independently implement PokeBank-owned native behavior from the observed/specification facts.

The same source/code license does not automatically establish redistribution rights for attached save files, event payloads or ROM-derived data.
