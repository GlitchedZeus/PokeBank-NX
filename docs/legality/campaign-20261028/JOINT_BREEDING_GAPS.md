# Joint breeding continuation — scoped implementation, not completed parity

Reference: official26.10.18, `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`.
The previous [family inventory](../hardening-20261027/RULE_GAPS.md) remains
in force except for the explicitly updated obligations below. No completeness
percentage or completed result-producing-branch denominator is claimed.

| Obligation | Production coverage | Remaining requirement |
|---|---|---|
| B01/B02: Gen II finite ancestry | Existing production solver reused; new maternal fields default to the old paternal behavior. Both-parent finite models independently checked by a separate state interpreter. | Native regional/event limitations remain. Fresh GB replay is required because this header changed. |
| B03: Ruby/Sapphire/Emerald/FireRed/LeafGreen | Their learnsets, tutors, gifts, GameCube donors and evolution/transfer edges feed the new necessary-condition graph. Existing PK3 birth-source/PID validator preserved. | New ancestry adapter is **not wired to native PK3 reports**. General order/event/gender/transfer witness certification remains incomplete. |
| B04: Diamond/Pearl/Platinum/HeartGold/SoulSilver | Their learnsets, machines/tutors, PCDs, Ranch and other encounter donors feed the graph. Original HGSS move insertion/inheritance bodies independently exercised. | New ancestry adapter is **not wired to native PK4 reports**. HGSS mechanism replay does not certify all DP/Pt or Gen V branches. |
| B05: Black/White/Black2/White2 unhatched ordinary eggs | Production retries all existing source assignments, requires one compatible father for the joint inherited set, and attempts a concrete forward birth from actual parent move slots. Male-only baby families include their breedable evolved Ditto parents. Initial-only and Volt Tackle alternatives retain the existing fast path. | Hatched Gen V reports are **not wired**. Parent acquisition/event/form/evolution/transfer certification is deliberately narrower than upper-bound reachability. No witness becomes Incomplete. |
| B05: both-parent level inheritance | One actual compatible father and mother must know the complete shared set for a clean constructive witness. | Original Gen V parent-selection/inheritance body has not been replayed. If exclusive paternal egg/TM sources still have a route, a failed shared-level joint proof remains Incomplete. This includes paternal possession, maternal possession and split-species selection; none is silently assumed to be a certified necessary Gen V rejection. |
| B06: event donors | Distinct joint sets for Gen III gifts/NY/GameCube, Gen IV PCD/Ranch/static/trade, Gen V PGF/PGL/static sources. Exclusive gifts are never per-move unioned. | Event gender/receipt restrictions are relaxed in the upper bound; lower certification refuses unverified gift witnesses. Historical catalog completeness is not authenticated. |
| M02/M04/T04: recursive possession | Finite least fixed point over species/era and all subsets of four queried moves; evolution and forward III→IV→V transfers; no arbitrary ancestry-depth or timeout rejection. | Upper bound pools native games/forms, relaxes intermediate birth order, type-tutor restrictions and HM erasure. A path alone cannot certify legality. Full chronological action histories and native III/IV adapters remain required. |
| Native birth order | New forward builder skips a known move, pushes only a new move when full, and uses actual four-slot parents. 20,000 independent combinations match unchanged HGSS `InheritMoves` and `TryAppendBoxMonMove`. | Gen V phase ordering is pinned-release equivalent, not an independently executed original Gen V body. Light Ball/Gen V duplicate examples in this harness are separate model controls, not additional native-body comparisons. |
| Strict report semantics | No permissive path alone yields clean. Constructive-certification failure marks moves/eggBreeding Partial with a warning. Only exclusive egg/TM paternal absence can add Invalid; existing local ordering failures are preserved. | Necessary-model/source completeness and new stricter disagreements need independent review before release acceptance. No ID or oracle-validity predicate appears in production. |

## Retained difficult cases

All seven original primary egg-combination failures are retained byte-for-byte
and now **Incomplete**, never clean: Chansey113 `[426,516,1,45]`, Smoochum238
`[1,357,122,263]`, and Illumise314 `[33,74,343,0]`, including their four
associated mutations. These are the **actual primary** combinations; earlier
Reflect Type/Wring Out research variants came from another retained dataset.
They must not be substituted for the primary inputs.

The permissive finite model finds no joint route for all seven, but the initial
prototype wrongly promoted that result to definitive Invalid before the native
Gen V shared-level obligation was certified. Those prototype streams remain
retained. The production gate now preserves that unresolved rule/evidence gap.
This removes false clean confidence; it does **not** complete those seven rules.

`GEN5_JOINT_FIXTURES.json` binds33 permanent synthetic records by SHA256.
The fixture includes the original seven, a single-move ordered Chansey pair,
the valid Smoochum source replacement, every newly stricter primary record,
an initial-only egg and the unchanged valid Durant regression.

Constructed Chansey `[1,45,118,516]` has a Clefairy father with Metronome/Bestow
and a Chansey mother with Bestow. Changing only Metronome118 to Mud Bomb426,
its equal-PP replacement, removes the known joint donor, but remains Incomplete
pending the shared-level mechanism evidence. The earlier proposed
Metronome control `[118,516,1,45]` stays in the fixture as **Incomplete**:
released PKHeX accepts it, but our forward witness does not reproduce that order.
Do not invent a fifth parental move or remove/reappend already-known moves.

Durant `[28,226,404,86]` remains clean through an intermediate birth, followed
by TM teaching and another birth. Its actual witness uses a Ledyba father with
Baton Pass, then a Durant father carrying Baton Pass/X-Scissor/Thunder Wave.
The first permissive graph witness instead used a Beedrill gift; the lower
certifier searches ordinary alternatives rather than certifying that gift.

Illumise `[33,227,343,0]` has an explicit Mr. Mime father with Encore/Covet and
an Illumise mother with Covet. B2W2's tutor source supplies Covet and the parents
can trade to Black. Released `CanLearnDirectly5` in `ChainBreedLegality` checks
level/TM sources and omits those tutors. The raw released Invalid / NX clean
result is retained. This is a candidate improvement, **not yet an original Gen V
mechanism-verified superiority claim**.

The first broad replay exposed an actual implementation omission: Tyrogue's
breedable male evolutions were missing from its Ditto route. The generic family
repair preserves eight clean controls. A ninth order/history is Incomplete.
No existing valid expectation was weakened to hide that regression.

The Smoochum pair `[1,357,252,263]` / `[1,357,113,263]` changes Fake Out to
Light Screen (and its associated PP). The former requires one father carrying
two exclusive egg moves, Miracle Eye/Fake Out, and has no known joint donor;
the latter has a concrete Kadabra father. Native reports are Invalid/clean;
both the released full report and standalone chain agree on this pair. This
exercises a paternal-only necessity without pretending the shared-level proof
is finished.

Three exact Happiny rows `[1,204,426,258]` are full-report VALID but standalone
`ChainBreedLegality.IsValid` false in the **same unmodified official assembly**.
NX's known-donor model rejects them. They remain raw false Invalids and unresolved
release blockers, not proven native improvements or excluded reference errors.
`BREEDING_REFERENCE_ANOMALIES.json` preserves all15 initially stricter rows,
exact hashes, both released observations and returned traces. Three Nidoran
rows remain Incomplete pending the maternal mechanism evidence.

Two additional frozen-reference Chansey Endure/Heal Pulse cases and the first
independent Illumise Encore/Wish/Solar Beam case exposed the same overstatement
of shared-level necessity. All three are permanent Incomplete controls, not
comparison exceptions. Existing committed valid expectations remain unchanged.

## Evidence transport and validation scope

The Gen V runner previously ignored CI's `WORK` folder. Its temporary directory
was deleted after the run, so a green8f0301a3 workflow did **not** establish the
claimed retained Gen V raw/secondary artifacts. `run.sh` now honors `WORK` even
on failure and preserves secondary results for the older `GEN5_DIFF_KEEP` path.
The artifact verifier checks every original row, joined bytes, every raw stricter
case's replay, exact specimen hashes and the actual reference assembly hash.
Permanent transport controls reject omitted difficult rows, changed bytes,
missing reports and wrong assemblies. They make no synthetic-legality claim.

Final v3 replays use a copied executable bound in `JOINT_ENGINE_MANIFEST.json`.
Earlier prototype batches used a mutable executable path and are explicitly
uncertified for final-head claims; all their streams remain available. No
prototype count is substituted for the final production replay.

## Evidence boundary and acceptance answers

Original HGSS source is pinned in `HGSS_BREEDING_SOURCE_PINS.json`; the witness
compiles unchanged function bodies with isolated data/allocator/accessor stubs.
It proves the scoped move-slot mechanism, not hardware execution, historical
parent acquisition, distribution receipt or all Gen V mechanics.
`BREEDING_PERSONAL_EQUIVALENCE.json` records the inspected native III/IV versus
B2W2 egg-group/gender resource equality; this justifies that shared table use
for those fields, not arbitrary cross-generation personal-field substitution.

1. Equivalent coverage is limited to the named finite-state constraints,
   preserved local birth validators and executed HGSS mechanism comparisons.
2. Native III/IV and hatched V wiring, complete recursive certificates,
   event/form/transfer constraints and original V maternal evidence remain open.
3. This breeding change establishes **no new verified superiority claim**.
   Earlier native HGSS EV/EXP and encounter evidence remain separately scoped.
4. `JOINT_PRIMARY_METRICS.json` and the independent/replayed metrics report raw
   agreement, false Invalid, false clean, Incomplete and Fishy without exclusions.
5. The unresolved stricter rows, avoidable Partial histories, uncertified source
   branches, historical custody and separately authorized application/device
   integration continue to prevent release acceptance. No app307 change or
   application/NRO build is part of this engine-only work.
