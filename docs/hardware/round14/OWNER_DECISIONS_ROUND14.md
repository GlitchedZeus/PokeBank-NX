# Round 14 owner decisions (November 2, 2026)

**Status: NOT DEVICE TESTED.** The Round 14 candidate `d160aa42` (PR #337) is frozen for the owner's hardware test.
The owner's comprehensive 2,806-check interactive checklist (all Gen I–V games) is the authoritative acceptance
record; the short defect retest `RETEST_d160aa42.md` does not replace it. Every failed hardware requirement stays
open until the owner tests a new NRO.

| # | Decision (owner) | Where it is implemented | State |
| --- | --- | --- | --- |
| 1 | Species change: keep the level; keep EXP if it is inside the new species' range for that level, otherwise that level's minimum; preserve Met Level, Met Location and history; Review shows every actual change, including species and EXP. Gen I reconciles an inconsistent box-level byte only in the staged edited record, never silently in an untouched original. | Rule in `d160aa42`; Review rows and the Gen I safeguard are being verified on `fix/round14-gen15-repairs` | In progress |
| 2 | Black 2 / White 2 Regis: both versions allow Regirock first; Black 2 then unlocks the Iron Key (Registeel), White 2 the Iceberg Key (Regice); the opposite key comes through Unova Link. Verify the save's key/access flags where supported, otherwise mark access Incomplete. | Next candidate | In progress |
| 3 | HeartGold Blue Orb → Kyogre, SoulSilver Red Orb → Groudon, at the Embedded Tower, after defeating Red and receiving the Kanto starter from Oak (Mr. Pokémon gives the orb). Incomplete unless the conditions can be verified. | Next candidate | In progress |
| 4 | Gen I–III get the same Save Instances experience as Gen IV/V (discovery, manual assignment, safe file browsing) in the shared provider system. A missing save opens the save chooser, never an empty editable workspace. | Next candidate | In progress |
| 5 | A file with no initialized game save or valid trainer data is not an editable save. A valid initialized save is not rejected because one optional trainer field is empty. Validation follows each game's native initialization rules. Creating a new save is a separate, explicitly supported operation (not implemented). | Next candidate | In progress |
| 6 | Draft PR for `fix/round13-gen15-repairs` stacked on #333 (#337). Keep `d160aa42` frozen; do not merge unaccepted code into main. | Done (PR #337) | Done |

**Outstanding, not fixed in Round 14:** shared multilingual keyboard (Round 13 R14), RetroArch Gen I–III quit crash
(R04), Master Vault and Move to Vault (v1.0). Each has its own lane; none is reported fixed until hardware-tested.
