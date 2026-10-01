# PokeBank NX Legality Engine

Status: **active foundation work**

PokeBank NX already had an informational structural/internal checker. The legality engine extends that work into an exact-game, evidence-driven analyzer for the generations the application currently supports.

## Design rules

- Legality analysis is **read-only**. It never mutates or auto-fixes a Pokémon.
- A clean report means **no problem was found by the checks that ran**. It is not automatically proof that the Pokémon is fully legal.
- Every analysis area tracks coverage. Missing evidence is reported as incomplete coverage rather than silently treated as legal.
- Exact save identity matters. Ruby, Emerald and FireRed share the PK3 entity format but do not share every legal move or encounter.
- Historical mechanics stay historical. Gen I/II DV + Stat Experience rules are not replaced with modern IV/EV assumptions.
- Encounter, move, PID/RNG, event/gift, egg, trade and transfer legality are separate evidence layers.
- Event/gift coverage is tracked separately from retail static encounters: recognizing a Mystery Gift PID pattern does not prove an exact distribution template.
- A recognized Gen IV Mystery Gift anti-shiny PID/IV class raises Event/Gift coverage to **Partial**; it stays incomplete until a specific WC4/PCD/PGT template matches.
- Legality never changes save-write authorization.

## First Gen I-IV milestone

The first legality-engine tranche adds:

- exact source-game profiles for R/B/Y, G/S/C, R/S/E/FR/LG and D/P/Pt/HG/SS;
- generation-correct species and move ceilings;
- exact-game Gen I, II, III and IV move-pool checks using the editor's audited compatibility tables;
- structured check identifiers;
- coverage-aware reports and an explicit Incomplete verdict;
- conservative exact-game encounter evidence where audited data already exists;
- preservation of the existing exact-game Gen III move context;
- Gen III handheld PID/IV correlation for Methods 1, 2, 3 and 4, including Unown's reversed-half variants;
- Gen III truncated-roamer PID/IV correlation for the Ruby/Sapphire and FireRed/LeafGreen roamer bug class;
- standard Pokémon Colosseum/XD XDRNG PID/IV correlation, with shadow-team locks and anti-shiny variants kept separate;
- Pokémon Channel Jirachi XDRNG correlation using PID, IVs, SID, Ruby/Sapphire origin and OT gender, with the fixed distribution template kept separate;
- Gen III regular BA-CD, regular anti-shiny BA-CD_A, force anti-shiny BA-CD_AX, and forced-shiny BA-CD_S event PID/IV correlation, including recovered restricted 16-bit seed evidence;
- Gen IV Cute Charm buffered-PID surface recognition, including Gen IV evolution/gender-ratio edge cases;
- Gen IV Poké Radar Chain Shiny PID/IV/trainer-ID correlation, with radar-slot provenance kept explicitly partial;
- Gen IV Mystery Gift anti-shiny ARNG reroll correlation;
- a pinned 247-row Gen IV WC4/PCD Pokémon-template index for direct invariant-field event candidates, including exact native event met location;
- positive Pokémon Ranger Manaphy PGT egg/trade provenance plus normal Method-1 and anti-shiny ARNG recipient PID/IV evidence;
- native Gen III/IV egg-origin structural checks, including the met-level-0 rule and Gen IV egg-location consistency;
- the pinned 235-entry Gen IV hatch-location map for DP/Pt/HGSS, including the Link Trade egg exception that permits hatching in another Gen IV game;
- separate positive Gen III/IV egg-move evidence tables, including generation-filtered pre-evolution retention, preserving breeding provenance instead of conflating it with TM/tutor/level-up compatibility;
- source-proven Gen IV static constraints for Platinum Shaymin's fateful flag, Lake of Rage Gyarados's forced shiny state, and Spiky-eared Pichu's fixed gender/nature/non-shiny state;
- exact-game Gen IV form-range validation from the pinned DP/Pt/HGSS personal tables, plus same-species form-change provenance for the PKHeX FormChange species instead of losing valid static origins after a legitimate form change;
- category-aware Gen IV static PID evidence matching the pinned EncounterStatic4 contract: ordinary static/gift encounters prefer Method 1 with compatible Cute Charm kept separate, Spiky-eared Pichu uses the PokéWalker PID class, and forced-shiny statics use Chain Shiny;
- pinned Gen IV release restrictions for the unreleased Korean Shaymin static encounter and Korean egg-origin records using Seabreak Path / Flower Paradise hatch locations;
- positive Gen IV no-lead Method J/K wild-slot correlation for Grass and Surf using the preserved encounter slot number;
- Generation III -> IV Pal Park transfer evidence, including impossible Gen III-origin egg rejection, mandatory Pal Park met-location validation, and D/P versus Pt/HGSS split-location field patterns;
- explicit Gen I/II Time Capsule compatibility evidence, separating "could originate in Gen I" from "can currently trade back to Gen I".

Current encounter coverage is **partial** for R/B/Y, Gold/Silver/Crystal (now including 9,856 pinned wild-slot rows plus static/gift/trade evidence), the five Gen III games, and all five Gen IV games. Gen I now checks exact-game PK1 catch rates, R/B/Y pre-evolution catch-rate provenance, and Time Capsule held-item-byte compatibility, including ambiguous bytes that can represent either history, without pretending PK1 stores met location/level. Gen IV now has pinned wild-slot, static/gift, and fixed in-game trade evidence for D/P/Pt/HG/SS, including the source-proven fixed static constraints currently present in the pinned reference. External event and PokeWalker templates are still incomplete, so an encounter non-match is not treated as illegal. Gen I now includes released international R/B/Y static/gift/Game Corner and in-game trade templates; Japanese Blue is deliberately separate, and Stadium/external event templates remain incomplete, so unmatched evidence stays unknown rather than illegal.

## Planned layers

1. **Format / structure** — ids, checksums, field ranges and format invariants.
2. **Internal consistency** — level/EXP, names, gender, ability, forms and stats. Exact DP/Pt/HGSS native form counts are now enforced before Gen IV encounter evidence is accepted.
3. **Exact-game move legality** — compatibility tables are wired now; event/tutor chronology and full tradeback provenance remain to be completed.
4. **Encounter provenance** — Gen I released R/B/Y static/trade templates, Gen II Gold/Silver/Crystal static/gift and fixed in-game trade evidence, and Gen IV wild, static/gift, and fixed in-game trade evidence are imported; Gen IV static evidence also preserves fixed gender/nature/shiny/fateful constraints where the pinned template explicitly supplies them, and rejects the unreleased Korean Shaymin static case; Stadium/external events, Japanese Blue, PokéWalker course-slot data is now pinned as positive species/level/gender/move evidence; course unlock/language history and evolved-species reconstruction, and remaining per-method restrictions are still incomplete.
5. **PID/RNG correlation** — Gen III handheld Methods 1/2/3/4 plus the truncated R/S/FRLG roamer class, category-aware Gen IV static/gift PID evidence, normal Gen IV Method-1 PID/IV, deterministic PokéWalker PID plus pinned 27-course/162-slot encounter evidence, Cute Charm buffered-PID surfaces, Chain Shiny PID/IV/trainer-ID correlation, Gen IV Mystery Gift anti-shiny ARNG rerolls, and positive no-lead Method J/K Grass/Surf and Old/Good/Super Rod slot+level+activation correlation, plus HG/SS Headbutt, HG/SS Rock Smash no-lead activation, and D/P/Pt Honey Tree level correlation, are recognized. Bug Contest/Safari activation, Mt. Coronet Feebas tiles, Rock Smash Illuminate-lead paths, special-lead branches, PokéWalker IV/course provenance, radar-slot proof for Chain Shiny, exact Gen IV event-template linkage, plus Gen III BA-CD different-OT/template restrictions, CXD shadow-team locks/anti-shiny variants, and exact Channel/event template linkage remain incomplete.
6. **Egg / breeding legality** — native Gen III/IV egg-state structure, exact-generation/game egg moves and generation-filtered pre-evolution retention are checked now. Hatched PK4 egg-origin records are validated against the pinned 235-entry DP/Pt/HGSS hatch-location table; Link Trade eggs may hatch in any valid Gen IV location, while the Korean Seabreak Path / Flower Paradise release restriction is enforced first. The egg-move table is positive evidence only; exact parent-pair feasibility, Masuda PID rerolls and event-egg templates remain incomplete.
7. **Event / gift legality** — a generated 247-template Gen IV WC4/PCD Pokémon index now provides positive direct-template evidence for invariant fields including exact native event met location, and Ranger Manaphy PGT egg/trade provenance is recognized separately. OT text/trash bytes, ribbons, dates, evolved-event reconstruction, traded-recipient identity after hatch, and remaining distribution restrictions are still incomplete.
8. **Transfer legality** — Gen III -> IV Pal Park marker/state validation, D/P versus Pt/HGSS split-location patterns, and Gen I/II Time Capsule compatibility/origin evidence are active. Split-ball/external-event provenance, exact historical Time Capsule direction where data is ambiguous, and later transfer chains remain incomplete.
9. **Bulk/provenance checks** — duplicate identities, clone lineage and Vault history once the Vault backend exists.

PKHeX is used as a reference implementation for architecture and cross-checking. PokeBank NX does not claim PKHeX parity until equivalent evidence and regression coverage exist.
