# Trace brief: deduplicate, trace and classify (PokeBank NX historical QA reconciliation)

Inputs
- /tmp/claude-0/-home-user-PokeBank-NX/e3d26a85-4f48-5ea5-b234-ef4545c1ae50/scratchpad/qa_recon/all_records.json :
  2754 extracted records (schema: SCHEMA.md in the same folder) with a global id `rid`.
  Rounds: PRE (Sept–mid Oct device tests), R1 7fea1797, R2 e10f307f, R3 62a2f8c3, R4 c60d4da0 (owner
  retests Oct 23), OCT24 (c60d4da0), R5 1ca2faec (Oct 25), OCT25, R6 26186464 (Oct 26), OCT26, R7 e88957f2 /
  R8 6ec5ccb9 / R9 d6a20a2a (checklists only: NO owner results exist), R10 3634d1e6 = tested in R11
  (worksheet, owner results), R12 066e1db9 (current candidate, NOT DEVICE TESTED), REQ (requirements),
  GH (GitHub). Note: the test file numbers Oct 24/25/26 as its own "Round 5/6/7"; ignore that numbering.
- Current source: /tmp/claude-0/-home-user-PokeBank-NX/e3d26a85-4f48-5ea5-b234-ef4545c1ae50/scratchpad/wr11
  at commit 6c7c8e99 (application tree identical to candidate 066e1db9). Full git history is available
  (git log -S / -G / --follow, git log --oneline <path>). READ-ONLY: do not edit, build, commit or push.
  Do not run make; grep and read sources/tests instead.

Your job (only for the AREAS in your assignment)
1. Take every record in your areas (all kinds, including PASS records: they show acceptance).
2. Deduplicate into UNIQUE ISSUES by underlying problem/feature (one issue may have many occurrences
   across rounds/docs). Never drop an occurrence: every non-PASS record in your areas (result FAIL /
   NOT TESTED / PARTIAL, or kind owner-request / owner-correction / deferred / owner-decision /
   requirement / engineering-finding) must be listed under exactly one issue's occurrences
   (or, for a non-issue such as a duplicate summary row, under `noise` with a reason).
   Records you think belong to another area: put them in `misfiled` with the area they belong to.
3. Give each issue a stable ID: PBX-<CODE>-<NNN> using your CODE(s) (listed in your assignment),
   numbered in order of first appearance in history (earliest round first).
4. Trace each issue in the CURRENT source code (file:line evidence), git history (repair commit sha +
   subject), automated tests (test file + what it asserts), and later hardware rounds (PASS/FAIL records
   for the same behaviour in later rounds).
5. Classify with EXACTLY one status:
   - "Hardware Accepted": the most recent owner hardware result for this behaviour is PASS (cite round,
     candidate, item). Add `rechecked_needed: true` if the code for it changed after that candidate
     (e.g. in the Round 11 repairs) so it needs re-verification in R12.
   - "Fixed in Code — Awaiting Hardware": a later commit changes the code so the reported problem should
     be gone, you verified the change in current source, and there is NO later owner PASS.
   - "Open Defect": the problem still exists in current source, or no fix exists.
   - "Missing Feature": a requested/required capability that does not exist in the shipped app (a design
     doc, unused backend class, model enum or test-only code does NOT count as implemented; it must be
     reachable from the UI in current source).
   - "Intentionally Unsupported": an explicit, recorded owner/product decision to not support it (cite).
   - "Needs Investigation": evidence is insufficient or contradictory (say exactly what is unknown).
   NEVER infer a PASS. A passing test, commit message or PR description is not hardware acceptance.
6. Flag `safety_critical: true` for anything touching original-save immutability, overwrite/loss of a
   Pokémon or item, checksums/serialization, export integrity, backups/recovery deletion.
7. Flag `v1_required: true` if any REQ record or owner statement makes it a v1.0 requirement.
8. Also list ACCEPTED FEATURES in your areas: behaviours the owner PASSed on hardware (per generation),
   with the latest PASS (round/candidate/item) and whether its code changed since (needs R12 recheck).

Output: ONE JSON object written to the path in your assignment:
{
 "areas": [...],
 "issues": [ {
   "id": "PBX-XXX-001", "title": "...", "gens": ["I".."V"|"all"|"app"], "area": "...",
   "category": "defect|missing-feature|request|safety|requirement|decision",
   "safety_critical": bool, "v1_required": bool,
   "first_seen": "round item_id (candidate)",
   "occurrences": [ {"rid":"...","round":"...","candidate":"...","item_id":"...","result":"...","src":"..."} ],
   "owner_words": [ {"rid":"...","text":"verbatim"} ],          // most relevant 1-3, verbatim
   "status": "<one of the six>", "rechecked_needed": bool,
   "status_basis": "evidence: file:line / commit / test / later round result",
   "repair_commits": ["sha subject"], "automated_tests": ["tests/... : what it checks"],
   "last_owner_result": "round/candidate/item: RESULT" or "none",
   "hardware_acceptance_candidate": "candidate+round+item where accepted, or the candidate that must verify it (e.g. '066e1db9 R12 H07'), or 'none (not on any checklist)'",
   "r12_checklist_item": "Hxx" or null,
   "next_action": "concrete next step"
 } ],
 "accepted_features": [ {"gen":"..","feature":"..","latest_pass":"round cand item","code_changed_since":bool,"evidence":".."} ],
 "noise": [ {"rid":"..","reason":".."} ],
 "misfiled": [ {"rid":"..","belongs_to_area":".."} ]
}
Validate with python3 -c "import json;d=json.load(open(PATH));print(len(d['issues']))".
Reply briefly: issue count by status, safety-critical count, v1-required count, notable findings
(especially anything that was accepted then regressed, or a requirement that silently disappeared).
