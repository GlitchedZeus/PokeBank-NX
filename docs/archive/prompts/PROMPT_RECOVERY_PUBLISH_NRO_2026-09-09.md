# PokeBank NX — Recovery Prompt: Finish and Publish Exact FRLG Device Artifact

Use this prompt for the next Codex recovery session.

> This is a recovery/packaging launcher. It does **not** replace `CURRENT_STATUS.md` or `docs/NEXT_CODEX_PROMPT.md`. Those remain authoritative for application source, safety state and active implementation scope.

```text
Continue PokeBank NX on `feature/pokebank-playable`.

Use HIGH reasoning.

This is a RECOVERY + EXACT ARTIFACT + GITHUB PRERELEASE session.
Do not start new features.

Before touching refs, worktrees, build outputs, generated assets, recovery state, or Git history:

1. inspect all worktrees;
2. inspect git status in every relevant worktree;
3. preserve all useful local/uncommitted/recovery/build/generated-asset state;
4. preserve the completed/restored pinned artwork tree if it survived;
5. preserve parked RSE recovery refs including `b5ef83b`, `1a921515`, or equivalents;
6. do not delete/recreate recovery state merely to make the workspace cleaner.

Read, in this order:

- `CURRENT_STATUS.md`
- `docs/CODEX_SESSION.md`
- `docs/NEXT_CODEX_PROMPT.md`
- `docs/PROJECT_RESOURCE_INDEX.md`

Then execute `docs/NEXT_CODEX_PROMPT.md` as the authoritative engineering task.

IMPORTANT CURRENT APPLICATION IDENTITY

Canonical application source is already published and verified as:

`ea0b806bac4acdb5619f22f9841d616ea8a237ff`

Commit message:

`legacy: bind FRLG sources to profiles and expose diagnostics`

Canonical application tree:

`ed5912093886384894c44538d569fe4955fd2e47`

Do not redo the already-completed stale-save/profile-binding source work.
Do not begin RSE.
Do not begin Vault/Banks/Create Pokémon/Trainer Plaza/events/conversion/new UI work.
Do not rerun expensive full sanitizer/PKSM-Core verification unless application source actually changes or a real verification defect is found.

The full pinned asset gate was already reported complete:

- HD Pokémon renders: 3260 / 3260
- base species: 1025 / 1025
- type icons: 18 / 18
- fonts: 3 / 3
- shiny/form coverage: PASS
- FRLG GBA cards: PASS
- asset preflight: PASS

Resume/reuse any trustworthy surviving complete asset/build state instead of deleting or redownloading it.

PRIMARY MISSION

Finish the exact physical-test artifact for `ea0b806b`, preserving the application source identity even though later documentation commits exist on the branch.

Preferred recovery pattern:

- keep `feature/pokebank-playable` and its documentation history intact;
- use an existing trustworthy exact-source build tree if one survived; otherwise create/use an isolated worktree or detached exact-source build context at `ea0b806b` rather than destructively resetting the documentation branch;
- verify native `include/`, `src/`, and Makefile inputs match canonical tree `ed591209...` before trusting the binary;
- reuse the completed pinned RomFS asset tree where valid;
- finish/restart only the exact-source devkitA64 `-fno-exceptions` native build;
- verify embedded source/version identity is `ea0b806b`;
- verify embedded RomFS byte-for-byte against the intended complete asset tree using the established repository method;
- package using the established `tools/package_device_build.py` flow, without diagnostic overrides unless absolutely necessary and explicitly documented;
- produce exact `.nro`, manifest and ZIP fallback if the established flow supports it;
- calculate exact byte sizes and SHA-256 hashes;
- preserve the exact artifacts somewhere durable before temporary runtime/workspace loss can occur.

GITHUB PERSISTENT ARTIFACT GOAL

After the exact artifact exists and all identities/hashes are known, publish it to GitHub as a PRE-RELEASE / device-test artifact if the environment has authenticated GitHub release-upload capability.

There are currently no GitHub Releases in the repository, so create a clearly source-addressed prerelease rather than pretending this is a stable public v1.0 release.

Preferred tag style:

`frlg-device-test-ea0b806b`

Preferred release title:

`PokeBank NX — FRLG Device Test ea0b806b`

The Git tag/release target must identify the APPLICATION SOURCE `ea0b806b`, not a later docs-only branch HEAD.

Attach, if produced:

- exact `.nro`
- exact ZIP fallback
- package manifest
- `SHA256SUMS.txt` or equivalent exact hash record

Release notes must state prominently:

- Application source full SHA: `ea0b806bac4acdb5619f22f9841d616ea8a237ff`
- Canonical tree: `ed5912093886384894c44538d569fe4955fd2e47`
- source commit message
- exact artifact filename / bytes / SHA-256
- asset counts and RomFS verification result
- verification reused/rerun
- live installed-game writes: DISABLED
- RetroArch/legacy writes: DISABLED
- `DEVICE TESTED: NO`
- `DEVICE ACCEPTED: NO`
- test focus: correct current two-Pokémon FRLG save selection + profile isolation

Do NOT mark the prerelease/device artifact as device-tested merely because it built, hashed, uploaded or passed CI.

Do NOT commit the ~150 MB `.nro` or ZIP into normal Git history merely because release upload is unavailable. GitHub Release assets / Actions artifacts are the intended binary-preservation path. If authenticated release upload is unavailable, keep the exact artifact preserved in the available durable artifact location, record all hashes/paths, report the exact upload blocker, and do not destroy/rebuild a good artifact just to retry authentication.

DOCUMENTATION AFTER ARTIFACT

After the exact binary identity is frozen:

- update only the minimal engineering/status/build records needed to identify the artifact and its GitHub prerelease location;
- keep application-source SHA distinct from later documentation commit SHAs;
- do not edit the root `README.md` in this recovery session;
- do not let `PROJECT_MAP.md`, `NEXT_SESSION_PLAN.md`, old prompts, or old build-record sections override `CURRENT_STATUS.md` / `NEXT_CODEX_PROMPT.md`;
- push documentation checkpoints only to `origin/feature/pokebank-playable`;
- never push custom PokeBank NX code upstream.

RSE

Preserve parked RSE work including:

- `b5ef83b`
- `1a921515`
- or equivalent recovered refs

Do not merge, resume, reimplement or push RSE.

FINAL REPORT

Report exactly:

starting remote/documentation SHA
canonical application source SHA = ea0b806bac4acdb5619f22f9841d616ea8a237ff
canonical tree SHA = ed5912093886384894c44538d569fe4955fd2e47
application commit message
whether application source changed during packaging
verification record reused and/or rerun
CI status
HD render count
base species count
asset preflight result
RomFS verification result
NRO filename
NRO exact byte size
NRO SHA-256
ZIP filename / byte size / SHA-256 if produced
manifest filename
SHA256SUMS filename if produced
embedded version/source identity
GitHub prerelease tag
GitHub prerelease URL if successfully published
which artifacts were attached
preserved RSE refs/status
DEVICE TESTED: NO
DEVICE ACCEPTED: NO

Then STOP for physical Switch testing.

PHYSICAL TEST AFTER DELIVERY

The user will test the exact uploaded/downloaded artifact hash and verify:

1. under the user's profile, assign/select the correct FRLG physical save;
2. Source Details show the exact physical path/fingerprint/trainer/party count;
3. the current two-Pokémon save opens;
4. the old level-6 Charmander save is not silently labeled `Main Save` and remains truthfully distinguishable if it is a genuine separate file;
5. restart PokeBank NX and confirm the binding persists;
6. switch to the niece's profile and confirm the user's assigned legacy save is absent;
7. switch back to the user's profile and confirm the assignment remains;
8. Trainer, Items, Party, Boxes 1-14, Pokémon View, Refresh and all read-only/live-write locks still work.

Do not begin RSE until physical acceptance of the corrected FRLG artifact.
```
