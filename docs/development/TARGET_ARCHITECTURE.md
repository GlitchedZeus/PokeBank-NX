# PokeBank NX — Architecture

Status: TARGET ARCHITECTURE  
Last updated: 2026-09-01

PokeBank NX should remain a native Nintendo Switch application with clear boundaries between UI, game/source discovery, Pokémon-format engines, persistent Vault storage, legality/reference tooling, and the future save-write pipeline.

The purpose of these boundaries is safety and maintainability: a UI button should not know how a Scarlet save is encrypted, and a save adapter should not decide how Vault provenance is stored.

---

# High-level architecture

```text
                 +----------------------+
                 |   Controller / UI    |
                 +----------+-----------+
                            |
                 +----------v-----------+
                 | Application Services |
                 | actions / workflows  |
                 +----+---------+-------+
                      |         |
          +-----------+         +----------------+
          |                                      |
+---------v----------+                 +---------v----------+
| Master Vault/Banks |                 | Game Source Layer  |
| provenance/indexes |                 | detect/read/export |
+---------+----------+                 +---------+----------+
          |                                      |
          |                              +-------v--------+
          |                              | Save Adapters  |
          |                              +-------+--------+
          |                                      |
          +-------------------+------------------+
                              |
                    +---------v----------+
                    | Pokémon Engine API |
                    | parse/convert/etc. |
                    +----+----------+----+
                         |          |
              +----------+          +----------------+
              |                                      |
    +---------v----------+                 +---------v----------+
    | Native C++ engines |                 | Host test/oracles  |
    | PKSE/PKSM adapters |                 | PKHeX + AutoMod    |
    +--------------------+                 +--------------------+
```

Future live writing adds a separate gated layer:

```text
Application workflow
      ↓
Staged Save Service
      ↓
Validation
      ↓
Backup/Journal
      ↓
Explicit per-adapter Write Capability
      ↓
Live title save
```

That path is currently hard disabled.

---

# 1. UI layer

Responsibilities:

- render Home/game cards
- render Party/Boxes/Vault/Banks/Dex
- controller navigation
- action sheets/dialogs
- display summary, legality, provenance, warnings
- gather explicit user choices

Must **not**:

- parse raw save structures
- calculate game checksums
- directly mount/write title saves
- mutate Vault files itself
- decide legality rules
- assume a game is writable because it is detected

UI calls application-service commands and renders results.

---

# 2. Application services / commands

This layer implements user intent.

Candidate commands:

```text
ViewPokemon
AddPokemonToVault
AddVaultEntityToBank
CloneVaultEntity
MakeShinyDerived
PlanTransfer
ExportPokemon
InspectLegality
```

Later:

```text
StagePokemonIntoSave
CommitValidatedSaveWrite
```

Each command validates capabilities/preconditions before calling lower layers.

Example:

```text
A → Add to Master Vault
       ↓
AddPokemonToVault command
       ↓
ask source adapter for exact raw Pokémon
       ↓
Vault transaction
       ↓
result to UI
```

The UI does not write the entity file itself.

---

# 3. Canonical game registry

Current implementation:

```text
include/Games/GameIdentity.h
src/Games/GameIdentity.cpp
```

Responsibilities:

- stable game ID
- display name
- platform
- generation
- exact Switch title ID where applicable
- high-level source-support classification

Stable identity is **release + platform**.

Examples:

```text
firered_gba
firered_switch
```

Do not put parser internals, save offsets, block keys, or Vault state into the game registry. Those belong to adapters/data layers.

---

# 4. Game source layer

A Source represents where data came from.

Examples:

```text
installed Switch title save
RetroArch save
manual imported save
backup save
individual .pk* file
Vault entity
```

Suggested source descriptor:

```text
source ID
canonical game ID
source kind
platform
display label
path/title context
fingerprint
adapter ID
capabilities
```

Source enumeration should be separate from save parsing.

For example:

```text
SwitchSourceDiscovery
  -> sees installed title ID
  -> maps to game identity
  -> constructs source descriptor

SaveAdapterRegistry
  -> chooses matching reader
```

---

# 5. Save adapter interface

Adapters isolate save-family details.

Initial read-only capability shape:

```text
probe
openReadOnly
trainerSummary
partyCount
partyPokemon
boxCount
slotsPerBox
boxName
boxPokemon
exportRawPokemon
saveFingerprint
```

Future staged capabilities:

```text
cloneToWorkingBuffer
setWorkingPokemon
clearWorkingSlot
serializeWorkingSave
validateWorkingSave
changeSummary
```

Future live capabilities are deliberately separate:

```text
writeLive
readBackVerify
```

No adapter advertises those live capabilities in the current alpha.

Possible adapter families:

```text
Gen1SaveAdapter
Gen2SaveAdapter
Gen3SaveAdapter
LGPEAdapter
SwShAdapter
BDSPAdapter
PLAAdapter
SVAdapter
ZAAdapter
SwitchFRLGAdapter
```

Even if multiple adapters reuse one engine, source/platform behavior remains explicit.

---

# 6. Pokémon engine interface

The engine layer handles Pokémon representations independently of where the save was found.

Responsibilities:

```text
parse raw Pokémon
serialize raw Pokémon
normalized fields
checksum/encryption validity
conversion
format-aware edits
format capabilities
```

Potential implementations/adapters:

```text
existing PKSE/PokeBank engine
PKSM-Core adapter
future specialized native implementations
```

The rest of the app should consume a stable PokeBank-facing model/interface rather than scatter direct `pksm::PK3` or PKSE concrete types throughout UI/business code.

---

# 7. Normalized Pokémon view

For browsing/search/UI, expose normalized optional fields.

Example conceptual model:

```text
species
form
nickname
level
gender
shiny
types
nature?          // absent in Gen I/II
ability?         // generation dependent
held item?       // generation dependent
moves
IV representation
EV/stat-exp representation
OT
TID
SID?
origin/version
met data?
ball?
language?
ribbons/marks
```

Generation-absent data should be optional/unsupported, not fabricated with fake defaults.

The normalized model is not a replacement for immutable raw Vault bytes.

---

# 8. Master Vault service

Defined in `MASTER_VAULT_SPEC.md`.

Responsibilities:

- immutable entity payloads
- stable Vault IDs
- SHA-256
- provenance
- parent/child lineage
- metadata cache
- indexes
- transactions/crash recovery

The Vault accepts exact Pokémon payloads and context; it does not know how to mount a game save.

---

# 9. Bank service

Banks are logical organization over Vault IDs.

Responsibilities:

```text
create/rename/delete bank
box names
slot references
move/copy references between boxes
sort/filter presentation metadata
```

Deleting a Bank must not silently delete Vault payloads.

---

# 10. Provenance service

Provenance is append-oriented historical context.

Event examples:

```text
IMPORT_FROM_SAVE
IMPORT_FILE
GENERATED
CLONE
EDIT
MAKE_SHINY
CONVERT
EXPORT
TRANSFER_PLANNED
TRANSFER_COMPLETED
LEGALITY_CHECK
```

Origin and current location must remain separate.

A UI can render a simplified history while the underlying data remains machine-readable.

---

# 11. Legality service

PokeBank-facing result:

```text
LEGAL
INVALID
UNKNOWN
```

The service can combine:

- native checks
- generated lookup tables
- engine-specific checks
- development-time PKHeX Oracle golden results

The production Switch runtime must not depend on a desktop PKHeX process being available.

`UNKNOWN` is a real result.

---

# 12. PKHeX Oracle boundary

Specified in `PKHEX_ORACLE.md`.

The Oracle is:

```text
host-side
development/test-time
pinned/reproducible
JSON-speaking
```

It is **not**:

```text
Switch runtime dependency
server dependency
HOME protocol client
```

Generated tables/golden vectors may be checked into the project when redistribution/licensing permits.

---

# 13. Pokédex/index service

Specified in `POKEDEX_SPEC.md`.

Collection ownership is derived from the Vault.

```text
Vault entities
     ↓
rebuildable collection index
     ↓
National/Regional/Living/Shiny Dex UI
```

The Pokédex UI must not own independent copies of Pokémon.

Game-save Pokédex registration is an entirely different feature inside future destination-save staging.

---

# 14. Transfer planner

Specified in `TRANSFER_MODEL.md`.

Responsibilities:

```text
source Vault entity
      ↓
target game
      ↓
compatibility
      ↓
conversion plan
      ↓
change/loss warnings
      ↓
derived Vault entity/export/staged destination
```

It should not write a live title by itself.

---

# 15. Save-write service

Specified in `SAVE_SAFETY.md`.

The write service is a deliberate firewall between valid staged data and an installed game.

It requires:

```text
verified backup
validated staged save
adapter capability
explicit user confirmation
write
readback
verification
journal
rollback
```

No generic `inject=true` flag should ever recreate the old unsafe architecture.

---

# 16. Dependency direction

Prefer dependencies flowing inward toward stable interfaces:

```text
UI
 ↓
Application Commands
 ↓
Interfaces/Services
 ↓
Adapters/Engines/Storage
```

Avoid:

```text
Save adapter importing UI classes
Vault importing SaveSelectScreen
PKSM concrete classes leaking everywhere
GameIdentity depending on filesystem paths
Legality engine writing Vault files directly
```

---

# 17. Error model

Lower layers should return structured errors rather than showing UI dialogs directly.

Example classes/categories:

```text
UnsupportedFormat
InvalidSave
CorruptPokemon
MissingSource
ReadFailure
VaultTransactionFailure
InsufficientSpace
ConversionUnsupported
LegalityUnknown
WriteCapabilityDisabled
ValidationFailure
```

UI translates these into human-readable messages.

---

# 18. Logging

Useful logs should include:

```text
app version/commit
adapter ID
game ID
source kind
operation ID
transaction/transfer ID
error category
```

Do not log entire private save files, credentials, console keys, or unnecessary identifying data.

---

# 19. Performance principles

Nintendo Switch constraints matter.

Prefer:

- metadata-first screens
- lazy save opening
- per-box loading
- bounded decrypted Pokémon caches
- rebuildable compact indexes
- streamed hashing/copy where appropriate
- no giant in-memory global database at startup

The startup screen should reflect real initialization stages.

---

# 20. Integration rule

When an upstream/reference project offers mature behavior, integrate it **behind these boundaries** rather than reshaping PokeBank NX around that project's entire architecture.

Examples:

```text
PKSM-Core -> PokemonEngine/SaveAdapter implementation
pkHouse -> reference facts/tests for modern adapters
PKHeX -> host oracle and generated data
PKForge -> Vault/transaction architecture ideas
pkDex -> Pokédex UX ideas
```

This is how PokeBank NX can reuse 30 years of community research without becoming an unmaintainable pile of copied subsystems.
