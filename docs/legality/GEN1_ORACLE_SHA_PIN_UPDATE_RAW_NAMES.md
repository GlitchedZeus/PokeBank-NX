# Gen I oracle workflow: coordinator pin update (raw Gen I/II name bytes)

`legality-gen1-exact-pkhex-oracle.yml` pins the Git blobs of the coordinator tree's
`src/Legality/Legality.cpp` and `src/Integration/Gen1/Gen1ReadOnlySave.cpp`. Commit
`5028a0a3` (Lane D, raw name bytes) changed both files, so both pins move:

| File | before | after |
|---|---|---|
| `src/Legality/Legality.cpp` | `f09f804d0becac96aed6a9ced4d663b294f3e2f6` | `dac0fe31a7247cb4dd81162d896d27240ce8e84d` |
| `src/Integration/Gen1/Gen1ReadOnlySave.cpp` | `a2445a20a2ede222a2b14e56f87305ef3263484e` | `7409ad560dc5391165c53e764c8212f6d288ad45` |

## What changed

- `Gen1ReadOnlySave.cpp` (`parsePokemon`) also keeps the stored OT and nickname bytes
  (`PokemonRecord::originalTrainerRaw`, `nicknameRaw`); the decoded strings are unchanged.
- `Legality.cpp` (`analyzeNativeGen1`, `analyzeNativeGen2`) passes those bytes to the GB
  checker as `e.otRaw` / `e.nickRaw`, so the glyph and terminator rules run on the stored
  bytes. "Original trainer name is empty" is raised only when the stored bytes are empty too.
- No other rule, table or message changes in these two files.

## Evidence that the pinned checks still pass with the new blobs

Every step of the workflow after the pin check was run locally on `c33cb2c9` (the same two
blobs as the final head), with the coordinator checkout, the accepted baseline
`d29eada5c087e201e00c749703f1a2de3ddfadeb`, PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`
and PKHeX.Core `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`:

- PKHeX.Core oracle: 72 vectors.
- Accepted baseline and coordinator source-free controls (normal and ASan/UBSan, identical output):
  "72 source-context vectors ... PASS".
- `l5-pk12-fixed` and `l5-pk12-fixed-sanitize`: "corrected guard PASS (200 synthetic contexts)".
- Three-engine differential (`audit_legality_l5_gen1_rby_live_diff.py`): self-test PASS;
  0 DV-field and 0 Stat Experience disagreements, 0 coordinator non-native defect disagreements.
- `l5-gen2-4-guard` and `-san`: PASS.
- `l5-gen1-all151`: "1812 PASS".
- `l5-gen1-real-save-pipeline`: "5436 saved-byte cases x 4 source labels; 6 negative groups PASS".

The workflow's own checker (`tools/legality/verify_gen1_oracle_source_pins.py`) reports both
stale pins on `d113b8a9`; CI runs the full job again on the pushed head.
