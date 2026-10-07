# Vault performance + event redistribution audit — 2026-09-09

> Research only. This note records evidence that narrows two remaining design gaps; it does not activate Vault implementation or event bundling.

## 1. Large-directory evidence on Switch

The deep research did not find an authoritative Pokémon-Vault benchmark at 10k–250k objects, but it did find Switch-specific evidence that unbounded tiny-file directories are a bad default architecture.

Relevant references:

- Atmosphère issue #1987: https://github.com/Atmosphere-NX/Atmosphere/issues/1987
- Atmosphère issue #2291: https://github.com/Atmosphere-NX/Atmosphere/issues/2291
- Atmosphère changelog: https://github.com/Atmosphere-NX/Atmosphere/blob/master/docs/changelog.md
- Hekate releases / SD benchmark philosophy: https://github.com/CTCaer/hekate/releases

The useful engineering conclusion is not a magic file-count limit. It is:

```text
one Pokémon = one loose file
```

should remain a deliberately bad benchmark baseline, not the preferred production design.

The raw payload size of even a 250k-Pokémon Vault is manageable; filesystem metadata cardinality, opens/stats/path lookups and small random reads are the bigger concern.

## 2. Leading architecture to benchmark

Strong candidate:

```text
vault/
  segments/
    seg-000001.pvs
    seg-000002.pvs

  manifests/
    manifest-00000042.pvm
    manifest-00000043.pvm

  indexes/
    primary.pvi
    species.pvi
    origin.pvi
    trainer.pvi

  CURRENT
  CURRENT.bak
```

Each committed segment is immutable and contains many records. Indexes map object identity to segment + offset + length.

Benefits to measure:

```text
far fewer directory entries
fewer opens/stat calls
more sequential I/O
append-only committed segments
snapshot manifests
rebuildable indexes
separate compaction
```

## 3. `vaultbench.nro`

Do not settle final Vault persistence by intuition.

Benchmark deterministic corpuses at:

```text
10,000
50,000
100,000
250,000
```

Compare at minimum:

```text
flat individual files
256-way sharded files
hash-prefix sharding
packed immutable segments
SQLite rollback-journal baseline
packed segments + SQLite/rebuildable index
```

Measure distributions rather than only averages:

```text
p50
p95
p99
worst
throughput
heap peak
file-op count
bytes read / written
```

Workloads:

```text
cold/warm startup
index present vs rebuild
random object lookup
species/shiny/origin/language/OT filters
combined filters
sorting
imports at several batch sizes
bank/tag mutations
manifest checkpoint
missing/corrupt index recovery
orphan segment recovery
corrupt segment detection
first visible page latency
sprite decode/cache separately from storage latency
```

Record environment:

```text
Switch model
HOS / Atmosphere / libnx revision
SD model
FAT32/exFAT
cluster size
free-space percentage
Hekate baseline IOPS
CPU clocks
handheld/docked
```

Hekate's percentile-oriented benchmark philosophy is a good model: occasional multi-second stalls matter even if average performance looks fine.

## 4. EventsGallery redistribution posture

Reference:
https://github.com/projectpokemon/EventsGallery

Technical usefulness is excellent, but the research did not find a clear blanket downstream license/permission grant covering wholesale bundling of the corpus into another application.

Permanent rule:

```text
publicly visible / downloadable / forkable
!=
clear redistribution permission
```

The rights picture may differ across:

```text
raw official Wonder Card payloads
Project Pokémon-authored metadata/organization
contributor descriptions
screenshots/images
research notes
generated/normalized artifacts
```

Do not assume a code/repository license automatically grants rights to every underlying official Pokémon asset.

## 5. Event system must not depend on bundling

Safest architecture regardless of eventual permission result:

```text
PokeBank ships:
  event-pack schema
  event-pack compiler
  event database reader
  optional importer

source/import:
  EventsGallery / other permitted source
      -> deterministic compiler
      -> events.pve / events.idx + data
      -> install into PokeBank
```

If explicit permission is later granted, the same pipeline can produce a bundled pack. If not, PokeBank still works with metadata/import workflows.

## 6. Permission request

Rather than endless license archaeology, ask maintainers explicitly and separately about:

```text
Project Pokémon-authored metadata and organization
raw Wonder Card payloads to the extent they can authorize them
compiled/normalized redistribution
modification/deduplication
free/open-source Switch homebrew distribution
required attribution
future/new contributions
whether screenshots/artwork are excluded
```

Project Pokémon guidelines are useful policy context but do not themselves create a redistribution license for EventsGallery.

## Current conclusion

Two remaining facts still need empirical/policy resolution:

```text
Vault: exact physical Switch performance -> vaultbench.nro
Events: wholesale redistribution permission -> explicit permission / rights review
```

The architecture can proceed without assuming either answer.
