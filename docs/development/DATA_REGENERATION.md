# Generated data maintenance

PokeBank NX commits generated source tables to the repository. A normal `make` or CI build **does
not regenerate data** and therefore does not need network access to PKHeX/PokeAPI.

Regenerate only when deliberately adopting upstream data corrections, supporting a new format, or
refreshing an asset set. The generated diff is part of the review evidence; do not mix an unrelated
upstream data refresh into a runtime bug fix.

## One entry point

```bash
python tools/regenerate.py --list
python tools/regenerate.py --tables --repo .
python tools/regenerate.py --assets --repo .
python tools/regenerate.py --tables --ref <PKHeX-commit> --repo .
```

Without `--repo`, the driver processes this checkout plus sibling directories that structurally
look like PokeBank NX checkouts. Use `--repo .` when you want one tree only.

`--list` performs no generator work and no network requests. CI uses it as a smoke test so newly
added `gen_*.py` scripts are automatically visible to the maintenance workflow.

## Current generated-data tools

The driver discovers every `tools/gen_*.py` automatically. At the time this document was written,
PokeBank NX includes generators for:

- species, move, location, learnset, move-presence and move-PP data;
- item pouch and held-item presence data;
- cross-game personal/presence data;
- Generation II move-compatibility data;
- Generation IV personal tables;
- Generation IV character conversion tables;
- Switch Pokédex lookup data already inherited by the project;
- HD Pokémon sprites in `romfs/`.

The exact list is authoritative from:

```bash
python tools/regenerate.py --list --repo .
```

## PKHeX source pinning

PKHeX-derived generators share `tools/pkhex_source.py`. Its default ref is pinned so existing
committed tables are reproducible. To intentionally test or adopt a newer PKHeX revision, use:

```bash
python tools/regenerate.py --tables --ref <commit-or-tag> --repo .
```

or set `PKHEX_LOCAL` to a local PKHeX.Core checkout. Review every generated change before commit.
A newer upstream table is not automatically correct for a PokeBank format adapter simply because
it is newer.

## Assets

Asset generators are detected by their `romfs` output. `--assets` runs only those scripts;
`--force` is forwarded only to asset generators. PokeBank's current HD-sprite script is pinned to
a PokeAPI sprite commit and can therefore be refreshed independently of PKHeX tables.

## Scope rule

Do **not** import a generator merely because PKSE has one. Add a generator when PokeBank actually
consumes that data and the format has entered the supported scope. This keeps future Gen V-IX,
Pokédex, legality-encounter and block-table maintenance from silently becoming current runtime
surface before those features are authorized.
