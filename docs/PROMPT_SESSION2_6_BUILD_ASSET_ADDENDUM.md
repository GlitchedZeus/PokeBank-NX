# Session 2.6 Addendum — Device Asset + Exact Artifact Gate

This file supplements `docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`.

It does **not** change Session 2.6 priorities. Save-safety audit (#23) and PLA crash hardening (#24) remain the blockers. This addendum only closes the build/package ambiguity discovered after auditing the inherited PKSE sprite pipeline.

## Before the clean native build intended for physical testing

Ensure locally generated RomFS assets are complete:

```bash
make types
make fonts
python tools/gen_hdsprites.py
python tools/check_device_assets.py
```

Required result:

```text
DEVICE ASSET PREFLIGHT: PASS
```

If the preflight fails, repair/generate the assets before handing off a visual-acceptance `.nro`.

Do not claim the first build's missing Pokémon image was definitely caused by missing RomFS assets; that remains a plausible packaging explanation until the next exact build/device test proves it.

## Clean exact-source build

After the application-source commit is finalized and pushed/verified:

```bash
git status --short
make clean
make -j1
```

Build while HEAD is the exact application-source commit.

## Package before later docs-only commits

From that clean exact application-source checkout:

```bash
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

Normal packaging requires:

```text
clean worktree
asset preflight PASS
embedded short SHA found in .nro
artifact byte size recorded
SHA-256 recorded
source-addressed filename
plain-text manifest
ZIP fallback
```

Do not use `--allow-dirty`, `--skip-asset-preflight`, or `--skip-embedded-sha-check` for the normal second-device artifact.

## Record in project docs

Copy the packager's exact manifest values into:

```text
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
docs/BUILD_RECORD.md
PROJECT_STATUS.md
```

Then later docs/status commits may advance branch HEAD. The manifest's application source remains the actual build source.

## Device acceptance additions

Alongside the existing second-device matrix, physically test:

```text
VIEW POKÉMON NORMAL RENDER          PASS/FAIL
VIEW POKÉMON SHINY RENDER           PASS/FAIL/NOT AVAILABLE
VIEW POKÉMON FORM RENDER            PASS/FAIL/NOT AVAILABLE
MISSING ART FALLBACK                PASS/FAIL/NOT TESTED
REPEATED VIEW OPEN/CLOSE            PASS/FAIL
NO LARGE ARTWORK HITCH/CRASH        PASS/FAIL
```

This artwork verification is secondary to the safety/crash gate. A render failure must not justify weakening save-safety behavior.
