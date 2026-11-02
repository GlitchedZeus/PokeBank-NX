# Independent native contradictions survive a capped RNG search

Follow-up to the 3576a74e production milestone and 44016106 evidence snapshot.
This is a narrow Gen III aggregation repair; it does not settle the capped
origin or audit every finding dependency.

The retained fresh-input pair b8634 / m8635-pp is an XD-origin Dragonite viewed
in a Gen III save. Both hit the actual native RNG guard. The base record remains
Incomplete. The mutation stores Hyper Beam63 with three PP Ups and24 current PP;
the unmodified official release reports MovePPTooHigh. Previously the engine
changed that existing native Invalid to a Warning solely because the unrelated
origin search was capped.

Original Emerald source at pret/pokeemerald
`5eff78649e7170a877b961ef0b3da13b81a16038` independently supplies the PP table,
packed masks and CalculatePPWithBonus function: Hyper Beam has base5, so three
PP Ups give8, never24. The unchanged function is compiled against every native
move and packed PP-Up byte, with both compilers and ASan/UBSan/LeakSanitizer.
The same source pins verify native255/510 EV limits and their award-writer guards.
This is original-source evidence, not an authenticated acquired specimen or ROM
execution claim.

Findings now carry an explicit independent-of-RNG property, conservatively false
by default. Only the already established PP maximum and total-EV510 contradictions
are marked independent. A capped origin still downgrades unresolved dependent
findings; it cannot negate those proven local bounds. The warning no longer
claims every Invalid finding is undecided. Gen III public API documentation now
states the source-backed contradiction/uncertainty distinction.

The original pair is preserved byte-for-byte in gen3_rng_cap_priority.txt. Across
all five GBA host titles, the base remains Incomplete even without warning text,
the actual PP mutation is Invalid while PID/RNG findings remain warnings, and an
EV511 derivative remains Invalid with Partial RNG coverage. The shared primitive
EV limit already made that derivative Invalid before this repair; it is a
precedence control, not a newly discovered false clean. Native findings now also
retain the independent EV contradiction.

All nine affected suites pass GCC/Clang normal and ASan/UBSan/LeakSanitizer again
(36 executions). No source or runtime fixture changes during validation. Every
row of all six original/prior-independent/fresh reference streams was replayed
unchanged. Exactly one verdict changes: m8635-pp, Incomplete→Invalid. It is a
reference-invalid PP contradiction, not a stricter disagreement. The prior
independent III sample is now16000/16378 agreement, zero raw false Invalid/false
clean,378 Incomplete. All original and fresh sample metrics, all IV/V verdicts,
all reference errors and all difficult records are unchanged. Full tables and
that literal before/after row are in RNG_CAP_REPAIR_METRICS.json.

Remaining work includes classifying other native findings by proof dependency,
complete general parent/EV/event/RNG histories, original regional evidence and
real acquisition custody. No broader family is upgraded to complete. Earlier
milestone measurements and initial harness setup failures remain preserved in
session evidence; no oracle or reference data changed. No application branch,
original save or other legality branch was modified or merged.
