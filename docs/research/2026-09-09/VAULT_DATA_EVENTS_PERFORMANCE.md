# Vault storage, data packs, legality, events, fuzzing and performance — 2026-09-09

> Research only. This file does not activate implementation scope. Verify external revisions/licenses/data rights before reuse.

This note consolidates the September 9 research around Master Vault durability, SQLite on Switch, compact runtime data, event storage, legality packs, large-library performance and fuzz testing.

---

# 1. Master Vault should favor immutable/content-addressed ownership

The research strongly supports the existing PokeBank direction: the authoritative Pokémon collection should not be one huge mutable database where every migration rewrites everything in place.

Preferred shape:

```text
vault/
    objects/
        02/02a744...pkv
        13/13fce2...pkv

    packs/
        01.pvp
        02.pvp

    manifests/
        generation-000001.cbor
        generation-000002.cbor
        generation-000003.cbor

    refs/
        CURRENT
        CURRENT.bak

    index/
        search.sqlite   # optional/rebuildable cache
```

Each Pokémon object is logically immutable and content-addressed:

```text
magic
format version
payload length
raw/original Pokémon payload
provenance metadata
SHA-256
```

Once the object hash matches its identity, normal Bank/Dex organization should reference it rather than rewrite it.

Benefits:

```text
crash during metadata migration
    -> old manifest still valid

new objects written but not referenced
    -> harmless unreachable data / later GC

search index corrupts
    -> rebuild it from authoritative objects/manifests
```

This is conceptually aligned with Git/restic/content-addressed storage patterns and with the existing Master Vault philosophy.

---

# 2. Vault migrations should publish new generations, never half-mutate the old one

Bad:

```text
open 30,000 Pokémon
modify each in place
update DB schema halfway
power dies
```

Preferred migration:

```text
read old manifest
 -> validate
 -> write/reuse new immutable objects
 -> write new pack(s) if used
 -> durability barrier / validate
 -> write new manifest generation
 -> verify every referenced object/hash
 -> publish new CURRENT reference
 -> open/validate new generation
 -> keep previous generation
 -> garbage-collect later
```

Crash halfway:

```text
CURRENT -> previous known-good generation
new unreferenced data -> harmless
```

Do not assume `CURRENT` rename is magically power-loss atomic on FAT32/exFAT. A future A/B pointer design with generation number + checksum may be safer:

```text
CURRENT_A
CURRENT_B
```

Select the newest valid pointer/generation.

---

# 3. SQLite works on Switch, but desktop defaults are not automatically appropriate

The research found Switch projects with custom SQLite VFS layers, including `nxmp`, proving native SQLite operation is feasible.

Useful evidence:

- Switch VFS can map SQLite sync to libnx file flush behavior;
- rollback-journal operation can be implemented;
- some Switch VFS examples have incomplete/no-op semantics for things desktop SQLite expects, such as locking/truncate/access behavior.

Therefore:

```text
SQLite CAN work on Switch
!=
any random desktop SQLite configuration is safe for irreplaceable Vault ownership
```

## WAL warning

Some Switch ports explicitly build SQLite with:

```text
SQLITE_OMIT_WAL
```

because WAL/shared-memory/POSIX assumptions do not map cleanly onto the Switch/newlib environment.

Do not default to:

```text
PRAGMA journal_mode=WAL;
```

just because desktop articles recommend it.

If SQLite becomes part of PokeBank, investigate a deliberately tested single-process Switch VFS and modes such as rollback journaling / stronger sync settings.

Current preferred role:

```text
SQLite = rebuildable search/index/cache
not
SQLite = only authoritative copy of every Pokémon
```

---

# 4. PKSM Mystery Gift packs are a strong event-storage architecture reference

PKSM's offline Mystery Gift design avoids thousands of loose card files and giant runtime object structures.

Observed resource shape:

```text
romfs:/mg/sheet4.json.bz2
romfs:/mg/data4.bin.bz2
romfs:/mg/sheet5.json.bz2
romfs:/mg/data5.bin.bz2
...
```

Conceptually:

```text
metadata/index
+
contiguous binary payload pack
```

The loader can slice the exact card type/offset for formats such as:

```text
PGT
PCD
PGF
WC6
WC7
WB7
WC8
...
```

PokeBank direction:

```text
verified source material
EventsGallery / other sources
manual corrections
        -> host event_pack_builder
        -> events.idx
        -> events.dat
```

Runtime:

```text
load compact index
 -> search/filter
 -> read only requested card bytes
```

Benefits:

```text
no giant JSON parser needed in NRO
no SQLite requirement for event payload lookup
no recursive scan of thousands of SD files
small deterministic runtime footprint
```

PKSM is GPLv3; the useful architectural concept is separable from direct source copying.

Event payload rights must be evaluated independently from PKSM's code license.

---

# 5. PKHeX BinLinker / `.pkl` + PoGoEncTool is the leading legality-data architecture

PKHeX uses compact indexed binary resources for encounter/legality data rather than loading enormous editable databases at runtime.

Conceptual indexed blob:

```text
identifier
entry count
offset table
entry 0
entry 1
entry 2
...
```

Project Pokémon's PoGoEncTool demonstrates a closely matching workflow:

```text
human-maintainable JSON research data
        -> deterministic generator/compiler
        -> compact PKHeX legality binary (`.pkl`)
        -> runtime consumption
```

PokeBank should strongly consider its own generalized build-time pack pipeline:

```text
YAML / JSON / CSV / generated research data
        -> poke-data compiler
        -> compact versioned PokeBank Binary Pack
        -> romfs indexed lookup
```

Possible uses:

```text
encounters
learnsets
legal move tables
generic lookup data
event metadata
localized names
forms/species metadata
```

The exact file extension/name is an implementation detail; the important decision is **host-side generation + compact deterministic runtime data**.

---

# 6. Generic metadata should come from deterministic upstream data sources

The research found much easier licensing/data options for generic Pokémon metadata than for legality/event truth.

Candidate references:

```text
PokéAPI
Veekun
```

Use generated upstream data for things like:

```text
species names
types
moves
abilities
items
forms
game identifiers
localizations
```

Do not spend developer time manually typing these tables.

Reserve human/research effort for the difficult domains:

```text
encounters
transfer behavior
events
legality
save revisions
```

Always track the upstream source/license and the separate Pokémon IP caveat.

---

# 7. Event redistribution remains unresolved

Technical event research is strong:

```text
EventsGallery
PKHeX
PKSM
Switch-Gift-Data-Manager
wc-beacon
wcparse
older Pokémon Link / Wonder Card tools
```

But the research did **not** establish a clean blanket license that means PokeBank may freely bundle every official Wonder Card/event payload.

Permanent distinction:

```text
publicly downloadable
!=
permission to redistribute inside PokeBank releases
```

Conservative product design until explicit permission/rights are established:

```text
PokeBank ships:
    Wonder Card parsers
    event schema
    metadata/search/index support
    user import

User/import pack supplies:
    exact event binaries where redistribution is unclear
```

PokeBank may independently generate/maintain metadata such as:

```text
event name
game/region/language
distribution window
species/form
OT/TID where sourced
eligibility
source/provenance
redistribution status
```

Never confuse technically valid event data with witnessed official server redemption.

---

# 8. `wcparse` is useful as a secondary Wonder Card oracle

`Kirzi/wcparse` was identified as an independent parser reference for formats including:

```text
WC6
WC7
WC6Full
WC7Full
Gen 7 BOSS
PGF / WC5
PCD
PGT
```

It is browser/JavaScript code, so not a direct Switch-runtime dependency.

Best role:

```text
PKHeX says field X = A
PKSM says field X = B
wcparse gives independent third implementation
```

Useful for resolving strange old Wonder Card interpretation differences.

---

# 9. PKVault contributes a useful staged-session concept, not a storage engine

The researched PKVault backend uses an unsaved session/operation plan and creates backups before applying mutations.

Useful application-level sequence:

```text
Session
 -> TransactionPlan
 -> Preflight
 -> Backup
 -> Apply
 -> Verify
 -> Commit
```

This matches PokeBank's product direction well.

But exception rollback is **not** equivalent to power-loss crash safety. If power disappears halfway through a write/restore, normal language-level rollback code never executes.

So use the application-level staging idea only in combination with PokeBank's lower-level recoverable filesystem/storage transactions.

---

# 10. Fuzzing should become a host-side safety pillar

No mature public Pokémon-specific fuzzing corpus was found that would eliminate the need for PokeBank's own effort.

That is an opportunity.

Run fuzzing on the Linux/native host core, not on the Switch:

```text
PokeBank C++ parsing core
    -> libFuzzer / AFL++ harness
    -> ASan
    -> UBSan
```

High-value fuzz targets:

```text
SaveProbe / format detection
container/wrapper detection
RTC trailer detection
SCBlock parsing
SwishCrypto paths
Gen III sector reconstruction
Gen IV general/storage selection
all historical string decoders
PK1/PK2/PK3/PK4/PK5 entity parsing
Pokémon Box blocks
Stadium parser
HOME containers
Vault manifests
conversion inputs
revision detection
```

Required properties for arbitrary malformed input:

```text
never OOB
never UB
never unbounded allocation
never infinite loop / stack explosion
never mark unknown revision writable
recovery deterministic
unsupported encoding never silently mutates source bytes
unknown wrapper bytes survive no-edit roundtrip
```

Seed fuzzing with the real/golden fixture corpus, then mutate from there.

---

# 11. Large-Vault performance needs a real Switch benchmark, not guesses

The research found useful architectural precedents and community SD benchmarks, but no trustworthy modern benchmark answering:

```text
100,000 Pokémon
actual Switch
FAT32/exFAT
cold startup / search / scrolling / RAM
```

So build a future disposable benchmark NRO.

Suggested corpus sizes:

```text
1,000
10,000
25,000
50,000
100,000
250,000
```

Measure:

```text
cold launch
warm launch
time to first visible box
Vault open time
species search
OT search
shiny filter
generation filter
multi-field query
sort
random Pokémon open
1,000-Pokémon import
index rebuild
manifest migration
garbage collection
sequential/random SD reads
sprite decode/cache hit rate
peak heap
frame time while scrolling
```

The research reinforces avoiding this architecture:

```text
100,000 Pokémon
= 100,000 stat calls
+ 100,000 file opens
+ 100,000 sprite decodes at startup
```

Prefer:

```text
compact sequential index
packed/content-addressed object storage
virtualized UI rows/grid
lazy object reads
LRU sprite cache
prebuilt species/name/game indexes
```

Never create 100,000 live UI objects.

---

# 12. Current infrastructure priorities implied by the research

When promoted into real implementation scope, the highest-leverage reusable infrastructure is:

```text
1. SafeSdFileReplace / SdFileTransaction
2. SaveContainerNormalizer
3. deterministic compact binary data-pack builder
4. manifest-driven fixture subsystem
5. StructuralRevision detector
6. immutable Vault object + manifest generation model
7. host fuzz harnesses
8. real Switch Vault benchmark NRO
```

These are cross-generation investments: implementing them once correctly should make every later adapter cheaper and safer.
