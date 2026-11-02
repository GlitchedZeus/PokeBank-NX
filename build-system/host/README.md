# Host test Makefile structure

The repository root intentionally contains **only** `Makefile` (devkitA64 Switch NRO) and `Makefile.host` (all permanent host tests). No new top-level `Makefile.host.*` files are permitted.

`Makefile.host` includes **10 ordered modules** under `build-system/host/`. These modules consolidate the **40 original host test rule fragments without altering the order in which their rules are evaluated**. Their `BEGIN original` / `END original` delimiters make each former rule set traceable and ensure no regression tests disappear.

| Module | Former fragments |
|---|---|
| `core.mk` | base |
| `classic.mk` | gsc, editor, cleanup3, standalone, shared-editor, hardware-fixes |
| `generations.mk` | gen4, gen5 |
| `sources.mk` | source-instances, product-ui, exports, home-providers |
| `regressions.mk` | gen14, qa, item-quantity-policy, pickers, reroll-mechanics, species-change-moves, round4-r, round4-codex |
| `ui.mk` | host-ui, review-entry, pkse-upstream, multilingual |
| `pokemon-files.mk` | pkse-october, pkse-october-pid, pkfile-gen12, pkfile-gen3, pkfile-gen4, pkfile-gen5 |
| `localization.mk` | pokemon-file-ui, recovery-view, jp-gen12, jp-gen1, jp-gen2 |
| `inventory.mk` | bag-top-up, pokemon-file-menus, machine |
| `deps.mk` | deps (must remain the **last** include) |

Two special-case Makefiles are *not* in the normal 40-rule graph and remain independently executable under `build-system/host/standalone/`:

- `classic_inventory.mk`: `make -f build-system/host/standalone/classic_inventory.mk host-classic-inventory`
- `gen5-create.mk`: `make -f build-system/host/standalone/gen5-create.mk test` or `sanitize`

## Normal build commands

```bash
make -f Makefile.host -j"$(nproc)" host-test
make -f Makefile.host -j"$(nproc)" host-sanitize
make -f Makefile.host host-clean
make -f build-system/host/core.mk build-host/test_inventory_ui_contract
python3 tests/test_host_makefile_layout.py
```

## Guardrails

- **Do not delete or weaken tests.** CI still runs the full GCC/Clang/sanitizer gates; the layout test verifies all 40 fragments and explicit standalone suites are present.
- New host test rules belong in an existing module, or a properly reviewed new module wired into the index. Do not create `Makefile.host.some-feature` in the root.
- Any module split/merge must preserve order, update CI path filters and any direct `make -f` references, and pass exact-head CI before merging into `main`.
- This is build infrastructure only. It does **not** fix the outstanding Gen I–V save/profile bugs or imply Gen VI–IX hardware acceptance.
