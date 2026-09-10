# Bank -> HOME public corpus audit — 2026-09-09

> Research/preservation only. This does not change the current FRLG coding milestone.

This note changes the preservation strategy: **do not begin by manufacturing a 20–50 Pokémon custom Bank -> HOME corpus.** Public Project Pokémon material already supplies a meaningful set of before/after official-path captures plus larger source-only candidate sets.

## Strategy change

Old plan:

```text
make 20–50 PK7 fixtures
 -> find a trusted Bank/HOME operator
 -> transfer all of them
 -> collect outputs
```

New plan:

```text
public before/after corpus
        +
public source-only candidate sets
        +
public HOME-native PH/EH corpuses
        ↓
validate/hash/pair/index
        ↓
map missing matrix cells
        ↓
ONLY THEN create custom Bank -> HOME captures for the remaining holes
```

Expected result: custom work may fall to roughly **5–15 deliberately chosen missing cases**, potentially fewer.

---

# 1. Ten explicit PK7 -> Bank -> HOME -> PK9/PA9 pairs

Project Pokémon thread:

https://projectpokemon.org/home/forums/topic/68070-requesting-help-to-transfer-old-pok%C3%A9mons-to-home/

The requester states that the attached old `.pk7` files should be run through real Pokémon Bank -> HOME because they do not have access to a 3DS/Bank. The reply posts returned `.pk9` / `.pa9` files for all ten.

| # | Before | After |
|---:|---|---|
| 1 | `0681 ★ - Excalibur - 96B22D428523.pk7` | `0681 ★ - Excalibur - F19A2D428523.pa9` |
| 2 | `0242 ★ - Blissey - C6EA6937DA48.pk7` | `0242 ★ - Blissey - 9A9E6937DA48.pk9` |
| 3 | `0149 ★ - Dragonite - 5561E549B264.pk7` | `0149 ★ - Dracolosse - 9E97E549B264.pk9` |
| 4 | `0373 ★ - Salamence - 64AE6CC92E17.pk7` | `0373 ★ - Drattak - 306C6CC92E17.pk9` |
| 5 | `0150 ★ - E.T. - 7D9739338B71.pk7` | `0150 ★ - E.T. - 2BEA39338B71.pk9` |
| 6 | `0144 ★ - Colombe - 6EBC6937DA48.pk7` | `0144 ★ - Colombe - 47356937DA48.pk9` |
| 7 | `0384 ★ - Jörmun - B0A53F6A3D8A.pk7` | `0384 ★ - Jörmun - 999B3F6A3D8A.pk9` |
| 8 | `0488 ★ - Selene - EAD35F4818CE.pk7` | `0488 ★ - Selene - B8945F4818CE.pk9` |
| 9 | `0376 ★ - Psyborg - D49CFBA3A7F2.pk7` | `0376 ★ - Psyborg - B47FFBA3A7F2.pk9` |
| 10 | `0248 ★ - Morpheus - CEEE7B3108C6.pk7` | `0248 ★ - Morpheus - 8DDE7B3108C6.pk9` |

## Known contamination / side edit

The transfer helper explicitly disclosed that **Metagross, Tyranitar and Blissey had their contest stats maxed** before the returned files were posted.

Therefore affected returned fixtures must be tagged something like:

```text
REAL_BANK_HOME_TRANSFER
+
KNOWN_POST_OR_SIDE_EDIT
```

Do not treat those specific field deltas as pure Bank/HOME behavior.

The remaining returned files have no comparable manual edit disclosed on the thread, but they still require binary-level validation after local download.

---

# 2. Explicit shiny Jirachi pair

Project Pokémon request/thread discovered in August 2026:

Source:

```text
0385 ★ - ジラーチ - 51A43091FC4D.pk7
```

Returned after real Bank -> HOME route:

```text
0385 ★ - ジラーチ - AE603091FC4D.pk9
```

This is a particularly useful mythical/event-adjacent old-generation specimen.

The forum activity page visibly records the request and the returned filename. Preserve the final canonical thread URL once fetched locally.

---

# 3. Two explicit Gen VII -> tracked Gen IX pairs

Project Pokémon download:

https://projectpokemon.org/home/files/file/5839-glitches-ace-gameboy-mark-shiny-treecko-and-torchic/

The uploader states that the archive includes a **Gen 7 version and a transferred Gen 9 version with a HOME tracker** for the Treecko/Torchic glitch cases.

These are useful because they are unusual VC/ACE-origin edge cases rather than ordinary specimens.

Count: **2 additional before/after specimens**.

---

# 4. Gen IV -> HOME-era exact pair

Project Pokémon download:

https://projectpokemon.org/home/files/file/5455-shiny-sport-ball-shedinja/

The uploader states that the archive contains the **original Gen IV file and a Gen VIII transferred version with a valid HOME tracker**.

This is not a pure PK7 -> modern boundary fixture, but it is excellent for full-route provenance, legacy-origin and unusual-ball behavior.

Count: **1 additional before/after specimen**.

---

# 5. Current explicit-pair count

Public material identified so far:

```text
10 Nobomoth pairs
+1 Jirachi pair
+2 Treecko/Torchic pairs
+1 Shedinja legacy-origin pair
=
14 explicit before/after transferred Pokémon
```

Not all 14 represent the same source boundary, and not all are guaranteed uncontaminated. Treat each fixture independently with evidence/provenance flags.

---

# 6. 48-Pokémon `challengePK7.bin` source corpus

Project Pokémon thread:

https://projectpokemon.org/home/forums/topic/64835-pok%C3%A9mon-home-challenges/

The source corpus was updated to USUM `.pk7` and attached as:

```text
challengePK7.bin
```

It was deliberately assembled around Pokémon HOME old-origin challenges and covers origins from:

```text
RBY
GSC
RSE
FRLG
DPPt
HGSS
BW
B2W2
XY
ORAS
SM
USUM
```

A 2026 reply says the user is only missing Turtwig, Chimchar and Piplup from DPPt HOME challenges. That is a strong lead that tracked descendants for much of the 48-Pokémon matrix exist in circulation, but **do not count unlocated descendants as confirmed pairs**.

Status:

```text
HIGH_VALUE_SOURCE_CORPUS
PAIRING_INCOMPLETE
```

---

# 7. Event-heavy source campaign lead

Project Pokémon user `Orbots` posted a Bank/HOME transfer request containing event-heavy old Pokémon such as:

```text
Oblivia Deoxys
Plasma Deoxys
Korean Deoxys
CHT / KOR / Manesh / Alexander / French Mac / JPN / CHS Hoopa
Meloetta
Shaymin
Cherish Ball Totodile
Diamond starter Turtwig
```

The activity/profile evidence shows the request and a later `thank you`, but the returned attachments have not yet been located by the research crawler.

Status:

```text
HIGH_PRIORITY_LEAD
NOT_CONFIRMED_PAIR_SET
```

Do not add these to golden fixtures until both sides are physically obtained and hashed.

---

# 8. HOME-native PH1–PH4 corpus already exists

Project Pokémon maintained HOME/GO corpus:

https://projectpokemon.org/home/files/file/4365-pok%C3%A9mon-go-transfers/

The page documents archives containing:

```text
ph1
ph2
ph3
ph4
pk8
pb8
pa8
pk9
pa9
```

and explicitly warns not to alter immutable values if HOME compatibility is expected.

This corpus is primarily GO -> HOME, so it **does not replace Bank -> HOME before/after fixtures**. It is nevertheless highly valuable for:

```text
PH1 parser
PH2 parser
PH3 parser
PH4 parser
HOME tracker/common block research
HOME game-specific blocks
HOME-native <-> game-representation testing
```

Therefore we no longer need to manufacture HOME-native specimens merely to learn the HOME structures.

---

# 9. Required host-side validator before calling anything a pair

Filename similarity alone is not evidence.

For every candidate pair:

```text
parse source
parse destination
        ↓
compare expected invariants
        ↓
record every changed field
        ↓
classify explained vs unexplained deltas
```

At minimum compare:

```text
species
PID
EC
TID/SID
OT
origin game
language
IVs
ribbons
met data
nickname
gender
form
shiny state
```

Also include format-specific/HOME-era fields as adapters mature.

Suggested manifest record:

```json
{
  "fixture": "nobomoth_excalibur",
  "route": "PK7->BANK->HOME->PA9",
  "evidence": "same_thread_transfer_return",
  "confidence": "A",
  "manual_edits_disclosed": false,
  "source_filename": "0681 ★ - Excalibur - 96B22D428523.pk7",
  "target_filename": "0681 ★ - Excalibur - F19A2D428523.pa9",
  "source_sha256": "...",
  "target_sha256": "...",
  "redistribution_status": "unknown"
}
```

Never promote a fixture to the golden suite until the binary files have been locally downloaded, hashed, parsed, and the pair relationship has been validated.

---

# 10. Evidence/confidence states

Recommended states:

```text
A  same thread/source contains before + requested real transfer + returned after
B  uploader explicitly says before/after transfer pair in one archive
C  source-only or descendant-only lead, pairing incomplete
```

Additional flags:

```text
PURE_TRANSFER_UNKNOWN
KNOWN_POST_OR_SIDE_EDIT
HOME_TRACKER_PRESENT
HOME_NATIVE_PRESENT
REDISTRIBUTION_UNKNOWN
```

Do not let a known side edit invalidate the entire fixture; preserve the pair but exclude contaminated fields from transfer-rule inference.

---

# 11. Revised custom-capture decision

Before asking anyone to run new Bank -> HOME transfers:

```text
1. download public corpus
2. hash all binaries
3. build A/B manifest
4. run field-level diffs
5. map coverage matrix
6. identify actual missing cells
7. request ONLY those custom transfers
```

Potential remaining matrix holes to inspect:

```text
ENG/JPN/KOR/CHS/CHT
strange forms
egg/hatched
old event ribbons
regional metadata
legacy moves
Gen III-origin
Gen IV-origin
Gen V-origin
Gen VI-origin
Gen VII-origin
SWSH destination
BDSP destination
PLA destination
SV destination
ZA destination
```

The biggest still-missing ideal specimen is the exact three-stage chain for one Pokémon:

```text
original.pk7
   ↓ real Bank -> HOME
same Pokémon native PH/EH dump
   ↓ official withdrawal
same Pokémon pk8/pb8/pa8/pk9/pa9
```

Source->destination pairs and separate HOME-native corpuses already exist, but the research has not yet established a public perfect three-stage Bank specimen.

---

# 12. Binary acquisition caveat

The research environment could see Project Pokémon attachment names/links but did not reliably retrieve every binary attachment. Therefore:

```text
VISIBLE_ATTACHMENT != LOCALLY_VERIFIED_FIXTURE
```

Do not claim byte hashes or field deltas until the files have actually been downloaded in a normal/local environment.

---

# Current conclusion

The public internet corpus has replaced a large part of the original custom Bank -> HOME capture campaign.

The next practical preservation step is now:

```text
download public corpuses
 -> hash
 -> pair
 -> validate
 -> field-level diff
 -> coverage matrix
 -> custom capture only for genuine holes
```

Keep the February 2027 shutdown deadline visible, but do not risk a primary modded Switch or rush into a large transfer campaign before the public corpus has been exhausted.
