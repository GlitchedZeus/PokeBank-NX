# PokeBank NX — Gen V Save Instances hardware smoke

**Purpose:** one exact-head development NRO to verify assigning and reopening real Black,
White, Black 2 and White 2 saves on a Nintendo Switch using the **existing shared
Gen I–IV Pokémon editor shell**. The Gen V adapter adds native PK5 staging
and validated save-source parsing; this is not a separate editor.

**Development-only candidate.** No production acceptance is implied by a
successful devkitA64 compile. Original emulator SAV5 files remain immutable.
No installed-game save writes or Inject Save are authorized.

## Artifact identity before installation

- Prefer the NRO attached to **PokeBank NX Product UI Native** for the same
  exact PR #300 commit, or **PokeBank NX Native PR Gate**'s
  `PokeBank-NX-Gen5-Save-Instances-<run id>` artifact.
- Open `BUILD_IDENTITY.txt`: `APPLICATION_SHA` must equal the PR #300 HEAD
  tested in this session. A binary from an earlier SHA is **not** this candidate.
- Check `SHA256SUMS.txt` against the copied NRO before testing. An uploaded
  build log alone is not a testable NRO.
- Keep the previously known-good NRO and full original external saves backed up
  separately. The application does not modify those source saves.

## Console acceptance matrix

Mark each row Pass / Fail / Not tested and record the visible status string,
profile, provider and exact game ID. Do not paste private trainer IDs.

| Area | Hardware action | Expected result |
| --- | --- | --- |
| Black | On one chosen profile, Games → Black → A → Save Instances | Exact **Black** SAV5 only; recognized provider and original source visible |
| White | Games → White → A → Save Instances | Exact **White** SAV5 only, never borrowed Black |
| Black 2 | Games → Black 2 → A → Save Instances | Exact **Black 2** SAV5; B2W2 format identified |
| White 2 | Games → White 2 → A → Save Instances | Exact **White 2** SAV5; B2W2 format identified |
| Assignment | Select a validated save and press A | Assignment persists for that profile/game; strict re-open enters existing Trainer/Party/Boxes editor |
| Return path | Exit editor and reopen same title | Correct selected game and profile remain focused; no automatic jump to another game |
| Remembered | Reopen the same profile/title after returning to Home | Remembered game card/source identity survives; Save Instances still allow explicit selection |
| Profile isolation | Switch to another profile | No automatic adoption of the first profile's private save |
| Provider | Test RetroArch, DraStic, melonDS, and Manual when actually installed | Real provider name appears; invalid unsupported files fail without crashes |
| Dual copy | Try an NDS file containing two different checksum-valid copies | Primary/Backup are presented as separately validated selections; no automatic newest-copy guess |
| Invalid | Select an unrelated title, wrong raw size, savestate or malformed file | Refused without silently choosing another file |
| No change | Browse party and boxes, then B out | Source file unchanged; no unrequested save/export |
| Staged edit | For a **disposable** save, change a supported PK5 field and Keep | App-owned staged workspace changes; source emulator save still unchanged |
| Review | Open Review after editing | Only verified staged fields are shown; Cancel/Discard does not touch source |
| Controller | D-pad, left stick, A/B, touch, long lists | Same selection and scrolling semantics; no double activation |
| Box touch | Open Boxes; tap a different cell, then tap the same cell | First tap focuses the selected slot; second tap opens the guarded shared Pokémon Actions |
| Box arrows | Tap the existing left/right box arrows; use L/R and ZL/ZR | Arrows and L/R step one box; ZL/ZR jump five, wrapping correctly at Box 1/24 |
| Unsupported touch | Touch a box-name header or Storage/Bank-facing control | No native save writes, generic rename, storage mutation, or crash |
| Home artwork | Game cards and return navigation | Wrong game/artwork must not be substituted; missing Gen V cover artwork should be reported separately |

## Source-integrity proof

Before testing, record a SHA-256 digest of each original .sav/.srm/.dsv on a
computer or known safe hashing application. Recompute afterward. A different
digest means this candidate fails source-integrity acceptance even if the UI
appears to work. Do not experiment with a unique/unbacked-up save.

The app's staged **working-copy export** is separate from original emulator-save
writing. An exported copy is not an Inject operation.

## Bug report essentials

Include the exact NRO `APPLICATION_SHA`, game (one of the four exact titles),
profile label only, provider, source extension, chosen save copy (when shown),
buttons/touches used, first unexpected screen, and whether the source SHA-256
changed. A screenshot or log is helpful; no private Nintendo IDs necessary.

## Pass boundary

This smoke round accepts **profile assignment, safe source selection, verified
native browse/staging, and returning to the correct game**. It does *not*
certify completed Gen V Create UI, party insertion, every Gen V editor field,
native Switch source saving, or the final vault release.
