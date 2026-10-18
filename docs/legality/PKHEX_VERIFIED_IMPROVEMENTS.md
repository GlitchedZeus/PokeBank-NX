# Verified improvements over PKHeX

Rules where PokeBank NX rejects (or flags) a record PKHeX accepts, each backed by original-game
source, a hex witness, a reproduction and a test. PKHeX is kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722;
decomps are pret/pokeplatinum, pret/pokeheartgold and pret/pokediamond as checked out under
`/home/user/ext`. Each generation keeps its own heading; entries are appended, never renumbered.

## Generation IV

### IV-1. Pt/HG/SS write both met-location fields; a disagreeing pair was never written by a game

- **Rule.** A native (origin 7, 8, 10, 11, 12) non-egg PK4 whose Pt/HG/SS met-location field (0x46)
  is set and whose egg-location fields (0x44, 0x7E) are 0 must hold in its D/P met-location field
  (0x80) what Pt/HG/SS write for 0x46: the same value when D/P know the place (1-111, 2000-2010,
  3000-3076), Faraway Place (3002) otherwise. `src/Legality/Gen4Legality.cpp` `Analyzer::verifyMisc`
  (:1352-1371). Full write-path map: `PKHEX_DISAGREEMENT_REVIEW.md` item 14.
- **Primary source.** pokeplatinum `src/pokemon.c:1732-1740` (one `case` for
  `MON_DATA_MET_LOCATION` and `MON_DATA_MET_LOCATION_PTHGSS`: `sub_0201708C(value)` or value 0 puts
  the value into both fields, else 3002 into `MetLocation_DP` and the value into
  `MetLocation_PtHGSS`), `src/unk_02017038.c:38-45` (the D/P-known ranges); pokeheartgold
  `src/pokemon.c:1285-1294` with `LocationIsDiamondPearlCompatible`. Every met-location write in
  both decomps goes through this setter (`src/unk_02092494.c:440-590` strategies and their callers;
  `src/trainer_memo.c:703-840` and its callers), and D/P write only the D/P field.
- **Hex witness** (Gen III/IV differential, seed 1 base corpus, record `m6211-metlocext`, Pearl, a
  wild slot with 1 in the Pt/HG/SS field and 23 in the D/P field; PKHeX VALID, PokeBank NX Invalid
  "D/P met location field holds 23 where Pt/HG/SS write 1 for met location 1"):
  `6FDF3AB0000081FC3600000070912A2E093D0000460D0002000000000000000000000000000000002700370032005D001E19141900000000DA52D0170000000002000000000001003A013D0143012E013F012D013501FFFF000000000000000B00000000000000003E0149015701580149015601FFFF00000000001A0A1200001700000419070000`
- **Reproduction.** `tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 4` lists the record
  under "DELIBERATELY STRICTER ... met-location-fields" (35 records in that run, all `metlocext`
  mutations; the predicate `met_location_fields` in `summarize.py` confirms the disagreement from the
  bytes). Unit test: `tests/test_legality_gen4.cpp` fixture `met-location-dp-field`; title matrix
  Gen IV rows (`tests/test_legality_title_matrix.cpp`). PKHeX's 29 `.pk4` test fixtures all satisfy
  the rule (fixture driver of this campaign).
- **PKHeX's verdict.** VALID: `G4PKM.cs:229-237` reads the Pt/HG/SS field whenever it is set and
  never compares the two outside Pal Park (`MiscVerifierG4.cs:35-45`).
- **Ours.** Invalid; Warning for a My Pokémon Ranch gift (the Ranch writes on the Wii, no decomp).
