# Completion campaign: evidence and denominators

Base PR #314 `b174f06a577b16a45d340741f39fc6b64a8e89bd`; app snapshot #307 `678627f5deabaa7120b40f1b9cfb80d6b65d66de` (2026-10-19). The engineering checkout starts from #314; the app branch remains untouched. No main merge, original save writes or auto-repairs are authorized here.

`RULE_INVENTORY.json` indexes every check-result source site under pinned/current PKHeX Legality and a 20-title ×34-area crosswalk. This is a reproducible audit scaffold, **not a certified semantic-rule denominator**. Multi-line helper dispatch and applicability need adjudication. Counts of source sites and broad areas must never be reported as percent of applicable legality rules. Existing broad inventories were not sufficient for the owner's 100% definition. No row is Complete merely because code exists. Native format/party/box, raw names, event context and production path tests are required.

Source pins: PKHeX `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (engine baseline) and `6042efa241a9925ce57e645329ea7f976deabdd3` (fresh current HEAD). File hashes and changed/added/removed paths are in the inventory. A latest-source change is not automatically imported into the engine.

The existing corpus denominators and exceptions remain in GEN1_5_RULE_COVERAGE.md; they are historical measurements, not 100% engine completeness. This session adds measurements separately and never drops Korean/unrecognized event/undecidable records to improve accuracy. Unknown, Fishy and Incomplete stay distinguishable from clean and Invalid.

Development differential seeds declared before implementation: Gen II42813 and Gen I39173. Adversarial holdout seeds: Gen II731921, Gen III892771, Gen IV892779 and Gen V892783. Holdout results, including failures, must be retained; if used to repair a rule, they become development regression cases and a new declared holdout is needed. Synthetic templates are not independent authenticated historical specimens.

Korean original-source lead: Greatpriceman/pokegold-ko `3cca49e5a0e306ea340015a25fc97b6fc07938f9`. Its README explicitly says it cannot currently rebuild the ROM. Treat unlabelled assembly as a research lead, not independently authenticated cartridge behavior; do not claim original-ROM parity from it or import ROM/assets. Read-only save layout/text constants are initially PKHeX-backed and separately classified from trade behavior. Korean/Western/Japanese link semantics need further primary-source tracing.
