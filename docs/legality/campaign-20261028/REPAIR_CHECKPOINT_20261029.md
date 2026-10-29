# Gen I–V repair checkpoint — October 29, 2026

Scope remains engine PR314. PR307, branch merges and application/device builds are outside this change. Official PKHeX26.10.18 remains the comparison release, pinned to `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`. This is a repair/evidence checkpoint, not an exhaustive parity certificate.

## Production repairs

**HGSS contest-only grass bank.** The retained false-clean `m41147-ball` was accepted through National Park's contest-only `D22R0102` ordinary-grass placeholder. The original entry scripts establish contest state; the native encounter dispatch selects the separate contest table in that state. The ordinary park uses another bank. The pinned map headers show no ordinary-map alias of the contest bank.

The generator now removes only the two complete, source-derived placeholder fingerprints, one per HG/SS version. It retains both ordinary park areas, all eight dedicated contest areas, every other area and every retained slot body. Counts become 1,764 areas and 24,308 slots. There are no specimen IDs in the correction. Eight paired records preserve ordinary Poké/Heal Ball captures and contest Sport Ball captures; the retained HG negative and an SS negative reject contest Heal Ball routes. The SS negative was already rejected before this repair. The original HG negative failed the new regression before the correction. `HGSS_CONTEST_SOURCE_PINS.json`, `HGSS_CONTEST_REPAIR.json` and the catalog protection test bind the evidence.

**Ranch ability uncertainty through the shared report.** CI on `0f1f34cc` retained `h60141-ability`, a Yanma matching a Ranch template. The shared report's general PID-slot rule rejected it before the detailed encounter handling. The native report now owns that rule after encounter selection and preserves the former ordinary exact-source retail guard. Models without immutable PK4 data retain the generic guard. Foreign abilities and ordinary PID-slot negatives still reject.

The original Wii ability writer remains unverified. A Ranch ability differing from the PID-selected slot therefore produces a Warning/Partial coverage, not a clean certificate or an unsupported generic Invalid. Five exact payloads exercise both direct and encrypted/shared paths on all five Gen IV host titles, including repeated hook application. No definite claim that the unusual Ranch specimen is historically legal is made. Evolved/other fixed-ability checks still run; an independent Invalid remains Invalid.

Both source changes pass 32 final GCC/Clang normal/ASan/UBSan suite executions, including 250 native Gen IV cases, 207 Gen V cases, the 41 prior breeding records, ordinary/contest controls, Ranch host-title controls, prior outbreak alternatives, terrain boundaries and release-rule regressions. Counts are not completeness. A fresh isolated table build reproduces the committed candidate bytes. The earlier compiler failure and interrupted restore attempt remain retained; neither produced a measured Pokémon corpus.

## Independent measurements

`REPAIR_HOLDOUT_PLAN.json` declares seeds 2026102894 and 2026102895 before generation and binds the immutable drivers/source dependencies in `REPAIR_ENGINE_MANIFEST.json`. Every emitted III–V row was executed. I/II retain their previous independently declared inputs and unchanged execution branches. Generated Invalid, Fishy, Misc:Error, Incomplete and all differences remain in the denominator.

| Format generation | Raw agreement / records | Raw false Invalid | Raw false clean | Incomplete | Reference Fishy | Reference error |
|---|---:|---:|---:|---:|---:|---:|
|I|11172/12000|111|2|715|9990|0|
|II|11276/12000|82|33|609|7145|0|
|III|40170/40945|0|0|775|4402|0|
|IV|62370/63219|60|0|789|4712|0|
|V|133180/134293|0|38|1075|32263|647|

These are sample agreements with the reference, not real-world accuracy or rule coverage. The existing Gen V generator derives grown copies only from valid base seeds and logs pre-emission exceptions; all emitted rows remain. Transfers are measured in their output-format stream. Historical/overlapping sets stay separate.

On the **same previous independent input**, Gen IV changes from62316/63224 agreement,48 FI,1 FC,859 Incomplete to62317/63224,47 FI,0 FC,860 Incomplete. One false clean is repaired and one unsupported Invalid becomes Incomplete. Gen V on that unchanged sample remains133652/134793,0 FI,47 FC,1094 Incomplete. Different fresh seeds must not be used to claim an accuracy improvement.

The English B2W2 source-order counterexample from0f1f34cc remains separately measured:2 controlled rows,1 raw agreement,1 raw false Invalid,1 source-explained deliberate disagreement. The original published reconstruction, unchanged-body replay and Pidgey data cross-check support that scoped rejection; an independent ROM rebuild has not been performed. These rows are never subtracted from raw counts. The current automated native-proof report covers only its named level/order mechanisms; other raw stricter cases are not silently certified by it.

All 105 published native-format PKHeX files were re-enumerated and replayed, including Illegal/General directories, eggs and seven CK3/XK3-to-PK3 derivatives. No file or error was excluded. Acquisition custody and source-save identity are not authenticated; host assumptions and every difference remain in `REPAIR_PUBLIC_FIXTURES.json`. No new authenticated acquired fixture is claimed.

## CI evidence and sealed transport

On `0f1f34cc`, ten workflows passed and the Gen III/IV differential failed on the Ranch ability case above. Its complete five emitted streams are retained. Later grown seeds were not reached and are not counted as tested. The older Gen III/IV artifact omitted current-reference DLLs; the new workflow retains those DLLs, original-reference DLLs, intermediate base files and generation diagnostics, and verifies the full expected nine-run set.

The downloaded Gen V artifact 12851442302 matches GitHub's published ZIP digest but has a one-character difference in a seed2 production diagnostic versus its joined view. Pokémon payloads and verdicts agree. This differs from the earlier `1ae` capture's malformed raw hex. Original ZIPs and every conflicting view remain unchanged; their audits are committed. No cause of the discrepancy is asserted, and neither inconsistent loose capture is accepted as verified byte-preservation evidence.

The workflows now package all available original streams into one archive, with per-file hashes, before upload. CI reopens that immutable archive and verifies every raw join, case identity, secondary replay and assembly hash against the complete expected run set. Its verified archive digest is printed in the independent CI log. A downloaded capture must match that digest and pass the same checks. Failed/partial captures can be preserved for forensics but cannot pass the complete-run check. A 134,508-row positive capture control passes; the complete conflicting historical capture correctly fails. Five additional transport controls exercise later loose-file mutation, attested hash mismatch, changed/missing members and path safety. This changes no oracle validity or measurement denominator.

Current-head CI and downloaded sealed-capture verification must be assessed on the forward source commit; this document does not predict a future green result. Complete raw data and prior prototypes are retained outside the repository evidence snapshot.

## Required acceptance answers

1. **Exact or evidence-backed equivalents:** the living `../hardening-20261027/RULE_GAPS.md` distinguishes Equivalent/Evidence from merely Represented. This checkpoint adds source-backed ordinary/contest-bank separation and preserves ordinary native ability enforcement. Earlier reviewed Gen II finite ancestry, unhatched Gen V constructive recipes, native terrain bounds, coupled met setters, original hatch/ball handling and typed gift/recipient checks keep their stated scope. None establishes an entire generation's parity.
2. **Still missing/incomplete:** a complete source-branch/title/region crosswalk; native III/IV and hatched V recursive ancestry; seven original unhatched V shared-level histories; complete historical gift/event ancestry; the general III/IV joint EV-versus-EXP solver; native Ranch generation/ability evidence; regional names and remaining raw differences; original receipts/custody; actual application wiring. See the inventory and continuation queue for individual families.
3. **Better-supported disagreements:** the scoped English B2W2 shared-order rejection, earlier native HGSS one-battle EV/EXP counterexample, all five fishing-outbreak Super Rod alternatives, terrain-byte bounds and reviewed ordinary coupled-met cases have source-backed evidence. The contest-bank repair agrees with current PKHeX. Ranch Incomplete is honest uncertainty handling, not a verified claim of superior historical legality knowledge.
4. **Demonstrated raw accuracy:** the table gives unadjusted generation-specific agreement, both error directions, Incomplete, Fishy and reference errors. Deliberate source-order disagreement is reported separately without removing it. No authenticity or population-accuracy percentage follows from these synthetic samples.
5. **Release blockers:** the concrete missing rules/evidence above, fresh-head CI/sealed-artifact acceptance, independent acquired histories and real frontend integration across all 20 titles. The Gen V staged/source/trainer/slot boundary and handoff tests are available, but PR307's panels are not a finished native legality integration. Frontends must use the full verdict/coverage rather than treating absence of Invalid as clean. No 100% claim is defensible.
