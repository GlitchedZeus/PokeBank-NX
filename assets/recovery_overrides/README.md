# Recovery overrides

This directory is the tracked escape hatch for project-authored corrections that cannot be reproduced by the normal generators.

Normal generated output still belongs under `romfs/` and remains gitignored.

If a future session manually fixes a generated asset, **do not leave the only corrected copy in `romfs/`**. Either:

1. fix the tracked generator/mapping/transform so regeneration produces the correction; or
2. if the corrected file is legally safe to redistribute, mirror its final RomFS-relative path under:

```text
assets/recovery_overrides/romfs/
```

Example:

```text
romfs/foo/bar.bin
        ->
assets/recovery_overrides/romfs/foo/bar.bin
```

`tools/recover_workspace.py` copies tracked overrides on top of regenerated RomFS before running the device asset preflight.

Do not use this directory to wholesale mirror third-party Pokémon artwork merely to avoid the generator. Public availability is not treated as blanket redistribution permission. Prefer a deterministic tracked transform/source pin whenever possible.
