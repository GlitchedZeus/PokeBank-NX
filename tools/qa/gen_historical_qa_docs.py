#!/usr/bin/env python3
"""Render the four historical-QA documents from the traced reconciliation data.

Inputs (this folder): all_records.json, trace_G1..G7 (*.json), trace_G8_checklist_lineage.json.
Output: <repo>/docs/qa/{MASTER_HARDWARE_ACCEPTANCE_LEDGER,HISTORICAL_ROUND_1_12_AUDIT,
V1_RELEASE_FEATURE_GAPS,PERMANENT_REGRESSION_MATRIX}.md and counts.json here.
"""
import collections
import glob
import json
import os
import sys

HERE = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "qa", "reconciliation")
REPO = sys.argv[1]
OUT = os.path.join(REPO, "docs", "qa")

ROUND_ORDER = ["PRE", "M1", "R1", "R2", "R3", "R4", "OCT24", "R5", "OCT25", "R6", "OCT26", "R7", "R8", "R9",
               "R10", "R11", "GH", "R12", "REQ"]
CAND = {"R1": "7fea1797", "R2": "e10f307f", "R3": "62a2f8c3", "R4": "c60d4da0", "OCT24": "c60d4da0",
        "R5": "1ca2faec", "R6": "26186464", "R7": "e88957f2", "R8": "6ec5ccb9", "R9": "d6a20a2a",
        "R10": "3634d1e6", "R11": "3634d1e6", "R12": "066e1db9"}
STATUSES = ["Hardware Accepted", "Fixed in Code — Awaiting Hardware", "Open Defect", "Missing Feature",
            "Intentionally Unsupported", "Needs Investigation"]
AREA_NAMES = collections.OrderedDict([
    ("vault", "Vault"), ("transfer", "Transfer"), ("bank-storage", "Legacy Storage / Bank"),
    ("actions-menu", "Actions menus"), ("clone", "Clone"), ("move-carry", "Move / carry"), ("release", "Release"),
    ("backups-recovery", "Backups & recovery"), ("export-copy", "Export Copy"),
    ("review-keep-discard", "Review / Keep / Discard"), ("open-save", "Open save"),
    ("save-discovery", "Save discovery"), ("import-export-file", "Pokémon file import/export"),
    ("create", "Create"), ("species-change", "Species change"), ("moves", "Moves"),
    ("encounters-met", "Encounters / Met Location"), ("legality", "Legality"),
    ("pokemon-edit", "Pokémon editor"), ("stats", "Stats"), ("pokemon-view", "Pokémon view"),
    ("visuals-sprites", "Visuals / sprites"), ("language-names", "Language & names"),
    ("bag-items", "Bag items"), ("pc-items", "PC items"), ("bulk-topup", "Top Up / Add Missing"),
    ("trainer", "Trainer"), ("help-footer-controls", "Help / footers / controls"), ("touch", "Touch"),
    ("launch-quit", "Launch / quit"), ("install-home", "Install / Product Home"), ("settings", "Settings"),
    ("search", "Picker search"), ("other", "Other / process / scope")])

# ---- the one documented override (owner message 2026-11-01, mandatory feature 8) ----
OVERRIDES = {
    "PBX-SPC-008": {
        "status": "Open Defect", "v1_required": True,
        "note": ("Reclassified at assembly from Needs Investigation: the owner's 2026-11-01 reconciliation brief "
                 "lists \"Species-change level and history preservation\" as a mandatory feature. Gen I–III Edit "
                 "still resets an existing Pokémon's Level/EXP to the encounter minimum on a species change "
                 "(evidence below), so the requirement is not met. The exact rule (keep Level always? keep Level, "
                 "recompute EXP?) still needs the owner's wording."),
    },
}


def load():
    recs = {r["rid"]: r for r in json.load(open(os.path.join(HERE, "all_records.json")))}
    issues, accepted, extras = [], [], {}
    for f in sorted([f for f in glob.glob(os.path.join(HERE, "trace_G[1-7]*.json")) if "G8" not in f]):
        d = json.load(open(f))
        for i in d["issues"]:
            o = OVERRIDES.get(i["id"])
            if o:
                i["status_basis"] = o["note"] + " Original basis: " + i.get("status_basis", "")
                i["status"] = o["status"]
                i["v1_required"] = o["v1_required"]
            issues.append(i)
        accepted += d.get("accepted_features", [])
        for k in ("item_ops_matrix", "untested_safety_functions"):
            if k in d:
                extras[k] = d[k]
    lineage = json.load(open(os.path.join(HERE, "trace_G8_checklist_lineage.json")))
    return recs, issues, accepted, extras, lineage


def rkey(r):
    rd = (r or "").split()[0] if r else ""
    return ROUND_ORDER.index(rd) if rd in ROUND_ORDER else len(ROUND_ORDER)


def cell(s, limit=None):
    s = "" if s is None else str(s)
    s = s.replace("\r", " ").replace("\n", " ").replace("|", "\\|").strip()
    if limit and len(s) > limit:
        s = s[:limit - 1].rstrip() + "…"
    return s


def gens(i):
    g = i.get("gens") or []
    return ", ".join(g) if isinstance(g, list) else str(g)


def original_result(i):
    occ = sorted(i.get("occurrences", []), key=lambda o: rkey(o.get("round")))
    for o in occ:
        if o.get("result") in ("FAIL", "NOT TESTED", "PARTIAL", "NO FIXTURE"):
            return f'{o.get("result")} ({o.get("round")} {o.get("item_id") or ""})'.strip()
    if occ:
        o = occ[0]
        kind = o.get("result") or "n/a"
        return f'{kind} ({o.get("round")} {o.get("item_id") or ""})'.strip()
    return "n/a"


def listed(xs, n):
    xs = [x for x in (xs or []) if x]
    if not xs:
        return "—"
    head = "; ".join(cell(x) for x in xs[:n])
    return head + (f" (+{len(xs) - n} more)" if len(xs) > n else "")


def counts(issues, extras):
    c = collections.Counter(i["status"] for i in issues)
    defects = [i for i in issues if i.get("category") in ("defect", "safety")]
    missing_v1 = [i for i in issues if i["status"] == "Missing Feature" and i.get("v1_required")]
    safety_unaccepted = [i for i in issues if i.get("safety_critical") and i["status"] != "Hardware Accepted"]
    recheck = [i for i in issues if i["status"] == "Hardware Accepted" and i.get("rechecked_needed")]
    return {
        "unique_issues": len(issues),
        "historical_unique_defects": len(defects),
        "by_status": {s: c.get(s, 0) for s in STATUSES},
        "defects_by_status": {s: sum(1 for i in defects if i["status"] == s) for s in STATUSES},
        "hardware_accepted": c.get("Hardware Accepted", 0),
        "hardware_accepted_needing_r12_recheck": len(recheck),
        "fixed_awaiting_hardware": c.get("Fixed in Code — Awaiting Hardware", 0),
        "still_open_defects": c.get("Open Defect", 0),
        "needs_investigation": c.get("Needs Investigation", 0),
        "missing_features_total": c.get("Missing Feature", 0),
        "missing_v1_features": len(missing_v1),
        "safety_critical_issues": sum(1 for i in issues if i.get("safety_critical")),
        "safety_critical_without_hw_acceptance": len(safety_unaccepted),
        "untested_safety_functions": len(extras.get("untested_safety_functions", [])),
        "v1_required": sum(1 for i in issues if i.get("v1_required")),
    }


HEADER_NOTE = ("**Status rules.** A status is never inferred from a passing test, a commit message or a PR "
               "description. *Hardware Accepted* means the most recent owner hardware result for that behaviour "
               "is PASS; *(recheck)* marks accepted behaviour whose code changed after the accepted candidate and "
               "must be re-verified on the current candidate. Current source: `6c7c8e99` (application tree "
               "identical to candidate `066e1db9`, NOT DEVICE TESTED).")


def write_ledger(issues, C):
    by_area = collections.defaultdict(list)
    for i in issues:
        by_area[i.get("area", "other")].append(i)
    out = ["# Master hardware acceptance ledger (Gen I–V, Rounds 1–12)", "",
           "Generated from the historical reconciliation of every Round 1–12 hardware record "
           "(see [`HISTORICAL_ROUND_1_12_AUDIT.md`](HISTORICAL_ROUND_1_12_AUDIT.md) for method, sources and the "
           "verbatim owner evidence of each ID). One row per unique issue; every historical occurrence is kept in "
           "the audit's evidence record. IDs are stable: never renumber, never delete a row; a resolved row "
           "changes status.", "", HEADER_NOTE, "",
           "## Totals", "",
           "| Status | Issues |", "|---|---|"]
    for s in STATUSES:
        extra = f" (of which {C['hardware_accepted_needing_r12_recheck']} need an R12 recheck)" if s == "Hardware Accepted" else ""
        out.append(f"| {s} | {C['by_status'][s]}{extra} |")
    out += [f"| **Total unique issues** | **{C['unique_issues']}** |", ""]
    cols = ("| ID | Title | Gen | First seen (round · test ID) | Original result | Current status | Repair commit(s) | "
            "Automated test(s) | Hardware acceptance candidate | Next required action |")
    sep = "|---|---|---|---|---|---|---|---|---|---|"
    for area, name in AREA_NAMES.items():
        rows = sorted(by_area.get(area, []), key=lambda i: i["id"])
        if not rows:
            continue
        out += [f"## {name} (`{area}`, {len(rows)})", "", cols, sep]
        for i in rows:
            st = i["status"] + (" *(recheck)*" if i["status"] == "Hardware Accepted" and i.get("rechecked_needed") else "")
            flags = (" ⚠️" if i.get("safety_critical") else "") + (" 🎯v1" if i.get("v1_required") else "")
            out.append("| " + " | ".join([
                f"`{i['id']}`{flags}", cell(i.get("title"), 160), cell(gens(i)), cell(i.get("first_seen"), 80),
                cell(original_result(i)), cell(st), listed(i.get("repair_commits"), 2),
                listed(i.get("automated_tests"), 2), cell(i.get("hardware_acceptance_candidate"), 90),
                cell(i.get("next_action"), 200)]) + " |")
        out.append("")
    leftovers = [a for a in by_area if a not in AREA_NAMES]
    for area in leftovers:
        out += [f"## {area}", ""] + [f"- `{i['id']}` {cell(i.get('title'))} — {i['status']}" for i in by_area[area]] + [""]
    out += ["Legend: ⚠️ safety-critical · 🎯v1 required for v1.0.", ""]
    open(os.path.join(OUT, "MASTER_HARDWARE_ACCEPTANCE_LEDGER.md"), "w").write("\n".join(line.rstrip() for line in out) + "\n")


def write_audit(recs, issues, lineage, C):
    rc = collections.Counter()
    for r in recs.values():
        rc[(r["round"], r["result"])] += 1
    rounds = sorted({r["round"] for r in recs.values()}, key=lambda x: rkey(x))
    out = ["# Historical hardware QA audit, Rounds 1–12", "",
           "Owner request (Nov 1): *\"Stop losing bugs and features between hardware rounds.\"* This audit reads "
           "every hardware results document, checklist, worksheet, engineering intake and defect report from the "
           "first device tests (September) to Round 12, plus the owner's GitHub reports, and reconciles them "
           "against the current source. It changes no application code and no NRO; nothing is merged.", "",
           HEADER_NOTE, "",
           "## 1. Method", "",
           "1. **Extraction.** Every checklist item, owner result, owner comment, correction, request, decision, "
           "deferred item and requirement was extracted into one record each, with the owner's wording copied "
           "character for character (checked by script against the source file). 2,754 records from 6 slices: "
           "pre-round device tests (967), Rounds 1–3 (692), Round 4–6 + Oct 24–26 (429), Rounds 7–12 + all 332 "
           "GitHub issues/PRs and 800 comments (343), Round 11 worksheet + intake + Nov 1 QA findings (123), "
           "v1.0 requirements (200).",
           "2. **Deduplication.** The 997 issue-like records (FAIL, NOT TESTED/NOT VERIFIED, PARTIAL, owner request, "
           "correction, decision, deferred item, requirement, engineering finding) were grouped by underlying "
           "problem into **%d unique issues** with stable `PBX-<AREA>-NNN` IDs. Coverage check: every one of the "
           "997 records is listed under exactly one issue (or explicitly as noise with a reason); 0 missing, "
           "0 duplicated." % C["unique_issues"],
           "3. **Tracing.** Each issue was traced through git history (repair commits), the automated tests, the "
           "current source at `6c7c8e99` (file:line evidence) and every later hardware result.",
           "4. **Classification.** Exactly one of: Hardware Accepted · Fixed in Code — Awaiting Hardware · Open "
           "Defect · Missing Feature · Intentionally Unsupported · Needs Investigation.",
           "5. **Checklist lineage.** Every checklist item was followed across rounds to find items dropped without "
           "acceptance (§5).", "",
           "## 2. Rounds and candidates", "",
           "| Round | Candidate | Owner hardware results | Records PASS / FAIL / NOT TESTED / PARTIAL / n/a |",
           "|---|---|---|---|"]
    owner_res = {"PRE": "Sept–mid Oct device tests (multiple builds)", "R1": "42 PASS / 6 FAIL / 6 NOT VERIFIED",
                 "R2": "6 PASS / 6 NOT VERIFIED", "R3": "70 PASS / 37 FAIL / 3 NOT TESTED",
                 "R4": "Oct 23 retests: 7 FAIL, 3 NOT TESTED (on c60d4da0)",
                 "OCT24": "27 PASS / 6 FAIL / 4 unmarked + S06 not tested", "R5": "31 PASS / 8 FAIL / 3 NOT TESTED",
                 "OCT25": "10 owner PASS items", "R6": "49 PASS / 9 FAIL / 1 NOT TESTED (see §4: second 54/4/1 report)",
                 "OCT26": "19 owner PASS items", "R7": "**none recorded**", "R8": "**none recorded**",
                 "R9": "**none recorded**", "R10": "tested in R11", "R11": "34 PASS / 13 FAIL / 1 NOT TESTED (H01–H48) + 8 extra findings",
                 "R12": "**not device tested**", "GH": "GitHub-only owner notes", "REQ": "requirements",
                 "M1": "multilingual milestone defects"}
    for rd in rounds:
        nums = " / ".join(str(rc.get((rd, k), 0)) for k in ("PASS", "FAIL", "NOT TESTED", "PARTIAL", "n/a"))
        out.append(f"| {rd} | {CAND.get(rd, '—')} | {owner_res.get(rd, '—')} | {nums} |")
    out += ["", "Record counts include the same owner result repeated in several documents (results file, "
            "status page, regression gate); the unique-issue counts in §3 remove that duplication.", "",
            "## 3. Totals", "",
            "| Measure | Count |", "|---|---|",
            f"| Historical unique defects (defect + safety issues) | {C['historical_unique_defects']} |",
            f"| All unique issues (incl. requests, requirements, decisions) | {C['unique_issues']} |"]
    for s in STATUSES:
        out.append(f"| {s} | {C['by_status'][s]} |")
    out += [f"| Hardware Accepted but code changed since → R12 recheck | {C['hardware_accepted_needing_r12_recheck']} |",
            f"| Missing v1.0 features | {C['missing_v1_features']} |",
            f"| Safety-critical issues without hardware acceptance | {C['safety_critical_without_hw_acceptance']} |",
            f"| Untested safety-critical functions (no owner hardware PASS ever) | {C['untested_safety_functions']} |", "",
            "## 4. Record discrepancies the owner should know about", "",
            "- **Rounds 7, 8 and 9 have no owner hardware results anywhere** (repository and GitHub). Their "
            "repairs were only exercised through the Round 11 worksheet on the Round 10 candidate `3634d1e6`.",
            "- **Round 6 (`26186464`) has two owner reports.** The repository records 49 PASS / 9 FAIL / 1 NOT "
            "TESTED; `docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md` mentions a second report of "
            "54 / 4 / 1 for the same build that is not in the repository, and `ROUND6_RESULTS_26186464.md` was "
            "rebuilt from the owner's Round 7 directive. The original reports and screenshots are missing.",
            "- **Round numbering differs.** `tests/test_hw_round1_regression_gate.py` calls the Oct 24/25/26 "
            "results \"Round 5/6/7\". This audit uses candidate-based rounds (R5 = `1ca2faec`, R6 = `26186464`) "
            "and OCT24/OCT25/OCT26 for those lists. The gate's Oct 26 table also reuses R1 IDs (B01, C02 …) for "
            "different items.",
            "- **Reused IDs changed meaning** (B08, R2-REVIEWMOVE); results under them must not be merged.",
            "- **R11 H13 \"Japanese saves read-only\" is recorded PASS, but the owner wrote \"i didnt test a "
            "japenese save\".** It is not counted as hardware acceptance of Japanese saves.",
            "- **R11 H46 \"Export Copy reload and compare (OPTIONAL)\" is PASS without saying whether an emulator "
            "load or a PC compare was done**; S06 (export loads in an emulator) is therefore Needs Investigation.",
            "- **R1/R2 NROs are not archived** (GitHub HTTP 403); their artifacts expire 2027-01-14/16.", ""]
    # dropped
    dropped = lineage.get("dropped", [])
    out += ["## 5. Requirements dropped from a newer checklist", "",
            f"{lineage['counts']['dropped']} checklist items or owner findings disappeared from every later "
            f"checklist without ever being owner-PASSed; **{lineage['counts']['dropped_still_needed']} are still "
            "needed**. Two events caused most drops: the **Round 8 split** (commit `66f8a53f`, owner Oct 28: "
            "\"hardware acceptance must not depend on fixtures the owner does not own\") replaced Round 7's 66 "
            "never-tested items with one \"repeat a few favourite Round 7 checks\" line and moved exotic cases "
            "to Group A automation, and the **Round 11 Vault/Bank items** were routed to the separate Vault plan "
            "(owner safety clarification 4).", "",
            "| Item | Last round | Last result | Why it was dropped | Still needed | Owner words |", "|---|---|---|---|---|---|"]
    for d in sorted(dropped, key=lambda d: (not d.get("still_needed"), rkey(d.get("last_round")))):
        out.append("| " + " | ".join([cell(d.get("item"), 140), cell(d.get("last_round")), cell(d.get("last_result")),
                                       cell(d.get("reason"), 140) + (" — " + cell(d.get("evidence"), 220) if d.get("evidence") else ""),
                                       "**yes**" if d.get("still_needed") else "no", cell(d.get("owner_words"), 260)]) + " |")
    out += ["", "### 5a. Omitted from the Round 12 checklist (`RETEST_066e1db9.md`)", "",
            "| Item | Status before R12 | R12 coverage | Reason | Safety-critical |", "|---|---|---|---|---|"]
    for o in lineage.get("r12_omissions", []):
        out.append("| " + " | ".join([cell(o.get("item"), 150), cell(o.get("status_before_r12"), 160),
                                       cell(o.get("r12_coverage"), 200), cell(o.get("reason"), 100),
                                       "**yes**" if o.get("safety_critical") else ""]) + " |")
    unguarded = [p for p in lineage.get("passed_then_unlisted", []) if not p.get("guarded")]
    out += ["", "### 5b. Owner-accepted items that left the checklist with no permanent guard", "",
            f"{len(unguarded)} of {lineage['counts']['passed_then_unlisted']} accepted items that are no longer "
            "on a checklist are not guarded by `tests/test_hw_round1_regression_gate.py` (it has no Round 11 "
            "table). See `PERMANENT_REGRESSION_MATRIX.md` §3.", ""]
    for p in unguarded:
        out.append(f"- {cell(p.get('item'))} — last PASS {cell(p.get('last_pass'))}")
    # evidence record
    out += ["", "## 6. Evidence record (every unique issue, every occurrence)", "",
            "Owner words are verbatim. Occurrences list every historical record (round · candidate · test ID · "
            "result · source); none were deleted.", ""]
    for i in sorted(issues, key=lambda i: i["id"]):
        out += [f"### `{i['id']}` {cell(i.get('title'))}", "",
                f"- **Status:** {i['status']}" + (" (recheck in R12)" if i.get("rechecked_needed") else "")
                + f" · **Gen:** {gens(i)} · **Area:** {i.get('area')} · **Category:** {i.get('category')}"
                + (" · ⚠️ safety-critical" if i.get("safety_critical") else "") + (" · 🎯 v1.0 required" if i.get("v1_required") else ""),
                f"- **Last owner result:** {cell(i.get('last_owner_result'))}",
                f"- **Basis:** {cell(i.get('status_basis'))}"]
        if i.get("repair_commits"):
            out.append("- **Repair commits:** " + "; ".join(cell(x) for x in i["repair_commits"]))
        if i.get("automated_tests"):
            out.append("- **Automated tests:** " + "; ".join(cell(x) for x in i["automated_tests"]))
        out.append(f"- **Hardware acceptance:** {cell(i.get('hardware_acceptance_candidate'))}"
                   + (f" · R12 item {i['r12_checklist_item']}" if i.get("r12_checklist_item") else ""))
        out.append(f"- **Next action:** {cell(i.get('next_action'))}")
        for w in i.get("owner_words") or []:
            t = (w.get("text") or "").replace("\n", " ").strip()
            if t:
                out.append(f"- **Owner** ({w.get('rid')}): > {t}")
        occ = sorted(i.get("occurrences", []), key=lambda o: rkey(o.get("round")))
        out.append("- **Occurrences (%d):** " % len(occ) + "; ".join(
            f"{o.get('round')} {o.get('candidate') or ''} {o.get('item_id') or ''} {o.get('result') or ''} [{cell(o.get('src'), 70)}]".replace("  ", " ")
            for o in occ))
        out.append("")
    open(os.path.join(OUT, "HISTORICAL_ROUND_1_12_AUDIT.md"), "w").write("\n".join(line.rstrip() for line in out) + "\n")


MANDATORY = [
    ("1", "Move to Vault in the shared Pokémon Actions menu", ["PBX-ACT-002"]),
    ("2", "Vault deposit, withdrawal and persistent storage", ["PBX-VLT-001", "PBX-BNK-003", "PBX-BNK-005"]),
    ("3", "Gen I–V to later-generation transfer workflow", ["PBX-XFR-001", "PBX-XFR-002"]),
    ("4", "Vault Pokémon search", ["PBX-VLT-002"]),
    ("5", "Consistent Actions menus across generations", ["PBX-ACT-"]),  # PBX-ACT-002 is counted under #1
    ("6", "Clone, Move, Release and occupied-destination safety", ["PBX-CLN-", "PBX-CRY-", "PBX-REL-"]),
    ("7", "Complete Bag/PC item operations", ["PBX-BAG-", "PBX-PCI-", "PBX-BLK-"]),
    ("8", "Species-change level and history preservation", ["PBX-SPC-"]),
    ("9", "Correct move and encounter validation", ["PBX-MOV-", "PBX-MET-", "PBX-LEG-"]),
    ("10", "Language and name handling", ["PBX-LNG-"]),
    ("11", "Original-save immutability, backups and recovery", ["PBX-BKP-", "PBX-EXP-", "PBX-OTH-001", "PBX-OTH-002", "PBX-OTH-003"]),
    ("12", "Pokémon export/import and save serialization integrity", ["PBX-FIL-", "PBX-EXP-", "PBX-REV-"]),
]


def write_gaps(issues, C):
    byid = {i["id"]: i for i in issues}

    def match(prefixes):
        return [i for i in issues if any(i["id"] == p or (p.endswith("-") and i["id"].startswith(p)) for p in prefixes)]

    out = ["# v1.0 release feature gaps (Gen I–V)", "",
           "**Verdict: PokeBank NX is NOT ready for v1.0.** %d v1.0-required features are missing, %d v1.0-relevant "
           "defects are open, and %d safety-critical issues have no hardware acceptance. Nothing in this file "
           "counts a design document, an unused backend class, a model enum or a test as an implemented feature: "
           "a feature counts only if it is reachable from the shipped UI in the current source (`6c7c8e99` = "
           "candidate `066e1db9`)." % (C["missing_v1_features"],
                                       sum(1 for i in issues if i.get("v1_required") and i["status"] == "Open Defect"),
                                       C["safety_critical_without_hw_acceptance"]), "",
           HEADER_NOTE, "",
           "## 1. The twelve mandatory features", "",
           "| # | Feature | Overall | Issues (status) |", "|---|---|---|---|"]
    for n, name, pref in MANDATORY:
        ms = match(pref)
        if n == "5":
            ms = [i for i in ms if i["id"] != "PBX-ACT-002"]
        sts = collections.Counter(i["status"] for i in ms)
        if any(i["status"] == "Missing Feature" and i.get("v1_required") for i in ms):
            overall = "❌ **Missing**"
        elif any(i["status"] == "Open Defect" for i in ms):
            overall = "❌ **Open defect**"
        elif any(i["status"] in ("Fixed in Code — Awaiting Hardware", "Needs Investigation") for i in ms) or \
                any(i.get("rechecked_needed") for i in ms):
            overall = "⏳ Not hardware-accepted on the current candidate"
        elif ms:
            overall = "✅ Hardware accepted"
        else:
            overall = "❓ No traced issue"
        detail = "; ".join(f"`{i['id']}` {i['status']}" for i in sorted(ms, key=lambda i: i["id"])
                           if i["status"] != "Hardware Accepted" or i.get("rechecked_needed"))
        acc = sum(1 for i in ms if i["status"] == "Hardware Accepted" and not i.get("rechecked_needed"))
        if acc:
            detail += (" · " if detail else "") + f"{acc} accepted, unchanged since"
        out.append(f"| {n} | {name} | {overall} | {cell(detail, 900)} |")
    out += ["", "## 2. Missing v1.0 features (status Missing Feature, v1.0-required)", ""]
    for i in sorted([i for i in issues if i["status"] == "Missing Feature" and i.get("v1_required")], key=lambda i: i["id"]):
        out += [f"### `{i['id']}` {cell(i.get('title'))}", "", f"- Evidence: {cell(i.get('status_basis'))}",
                f"- Next action: {cell(i.get('next_action'))}", ""]
    out += ["## 3. Other missing features (not marked v1.0-required)", ""]
    for i in sorted([i for i in issues if i["status"] == "Missing Feature" and not i.get("v1_required")], key=lambda i: i["id"]):
        out.append(f"- `{i['id']}` {cell(i.get('title'))} — {cell(i.get('next_action'), 220)}")
    out += ["", "## 4. Open defects", ""]
    for i in sorted([i for i in issues if i["status"] == "Open Defect"], key=lambda i: i["id"]):
        out.append(f"- `{i['id']}`{' ⚠️' if i.get('safety_critical') else ''}{' 🎯v1' if i.get('v1_required') else ''} "
                   f"{cell(i.get('title'))} — {cell(i.get('next_action'), 260)}")
    out += ["", "## 5. Contradictory requirements that need an owner decision", "",
            "These come from the requirement extraction; each pair of documents states the requirement "
            "differently. Until the owner decides, neither version may be dropped silently.", "",
            "1. **v1.0 game scope** — `PRODUCT_DECISIONS.md` §1 (\"40 target v1.0 identities\", GB to 3DS/Switch) "
            "and `README.md` (Gen VI–IX before 1.0) vs. `CURRENT_STATUS.md` (\"Release scope remains Gen I–V\") "
            "and `V1_ROADMAP.md` (Gen VI–VIII \"not current release scope\").",
            "2. **Move to Vault presentation** — owner: in every generation's Actions menu; Round 11 intake: "
            "absent until the Vault exists; `UI_FLOW.md` / gate A11: greyed placeholder; `TRANSFER_MODEL.md`: "
            "only when verified.",
            "3. **Actions order and Review row** — `UI_FLOW.md` target order (Add to Master Vault, Add to Bank, "
            "Transfer to Game, Make Shiny) differs from the owner's list; owner list implies Review is always "
            "shown, code shows it only with pending changes; gate F02 wants a greyed empty Review.",
            "4. **Party actions** — `POKEMON_EDITOR_UI_CONTRACT.md` still says Party mutation is deferred; Gen I–III "
            "Party edit shipped and was accepted (R6 A21); owner wants Clone/Move/Release in Party too.",
            "5. **Gen I → Gen V transfer** — owner expects a route (R11 H38); intake: refuse or mark "
            "non-canonical; `FEATURE_FEASIBILITY_MATRIX.md` puts Transfer Lab in v1.x; `README.md` needs transfer "
            "for v1.0.",
            "6. **Vault timing** — `V1_ROADMAP.md`, intake and `RELEASE_CHECKLIST.md`: v1.0; "
            "`FUTURE_AFTER_GEN15_BACKLOG.md` #3 closed as \"not planned for the current Gen I–V gate\".",
            "7. **Moves on species change** — `MAIN_ACTIVE_HANDOFF.md` R2-BUG-004 (\"resets moves to the game's "
            "level-up moves\") vs. owner Round 11 (never auto-fill moves). Current code follows the owner.",
            "8. **Create validation** — owner Extra 6 \"universal rule\" and \"level 100 → 1–2 moves\" vs. the "
            "owner's own Oct 31 clarification (exact-game rules, never universal) and the intake (\"not a game "
            "rule\"). The 1–2-moves request is still unanswered by the owner.",
            "9. **Clone semantics** — `UI_FLOW.md` / `MASTER_VAULT_SPEC.md`: Clone creates a new Vault ID and never "
            "duplicates into a live slot; gate D12 and the shipped app clone into the staged save.",
            "10. **Japanese/Korean names** — owner wants native names and keyboards in every language (H27); "
            "`README.md` / consolidation keep Japanese Gen I/II saves read-only and their editing locked.",
            "11. **Species-change Level** — no specification existed; the owner's Nov 1 list makes it mandatory. "
            "Gen I–III reset Level to the encounter minimum, Gen IV keeps it, Gen V refuses a different growth "
            "group (`PBX-SPC-008`).", "",
            "## 6. Release blockers summary", "",
            f"- Missing v1.0 features: **{C['missing_v1_features']}**",
            f"- Open defects: **{C['still_open_defects']}**; needs investigation: **{C['needs_investigation']}**",
            f"- Fixed in code but awaiting hardware: **{C['fixed_awaiting_hardware']}**",
            f"- Accepted behaviour changed since acceptance (R12 recheck): **{C['hardware_accepted_needing_r12_recheck']}**",
            f"- Safety-critical issues without hardware acceptance: **{C['safety_critical_without_hw_acceptance']}**; "
            f"untested safety-critical functions: **{C['untested_safety_functions']}**",
            "", "v1.0 must not be declared while any line above for missing v1.0 features is non-zero.", ""]
    open(os.path.join(OUT, "V1_RELEASE_FEATURE_GAPS.md"), "w").write("\n".join(line.rstrip() for line in out) + "\n")


def write_matrix(issues, accepted, extras, lineage, C):
    out = ["# Permanent Gen I–V regression matrix", "",
           "Everything the owner has accepted on hardware, per generation, and how it is protected from "
           "silently regressing. Every accepted behaviour stays here permanently; a row is only removed by an "
           "explicit owner decision recorded in the ledger. A ✅ in *Accepted* is an owner hardware PASS on the "
           "named candidate, never a test or a commit.", "", HEADER_NOTE, "",
           "## 1. Accepted behaviour by generation", "",
           "| Gen | Feature | Latest owner PASS | Code changed since → recheck on current candidate | Evidence |",
           "|---|---|---|---|---|"]
    order = {"I": 0, "II": 1, "III": 2, "IV": 3, "V": 4, "all": 5, "app": 6}
    for a in sorted(accepted, key=lambda a: (order.get(str(a.get("gen")).split(",")[0].strip(), 7), str(a.get("feature")))):
        out.append("| " + " | ".join([cell(a.get("gen")), cell(a.get("feature"), 140), cell(a.get("latest_pass"), 90),
                                       "**yes**" if a.get("code_changed_since") else "no", cell(a.get("evidence"), 160)]) + " |")
    out += ["", "## 2. Accepted issues (ledger rows with status Hardware Accepted)", "",
            "| ID | Gen | Behaviour | Last owner PASS | Recheck needed |", "|---|---|---|---|---|"]
    for i in sorted([i for i in issues if i["status"] == "Hardware Accepted"], key=lambda i: i["id"]):
        out.append(f"| `{i['id']}` | {cell(gens(i))} | {cell(i.get('title'), 140)} | {cell(i.get('last_owner_result'), 90)} | "
                   f"{'**yes** (' + (i.get('r12_checklist_item') or 'not on R12') + ')' if i.get('rechecked_needed') else 'no'} |")
    unguarded = [p for p in lineage.get("passed_then_unlisted", []) if not p.get("guarded")]
    out += ["", "## 3. Regression guard coverage", "",
            "`tests/test_hw_round1_regression_gate.py` pins owner PASS items from Round 1 (42), Round 2 (6), "
            "Round 3 (30 new), Oct 24 (27), Oct 25 (10) and Oct 26 (19) to host tests and contracts and runs on "
            "every PR. **It has no Round 11 table**, so Round 11 PASS items that are not on the Round 12 checklist "
            "are not protected:", ""]
    for p in unguarded:
        out.append(f"- {cell(p.get('item'))} (last PASS {cell(p.get('last_pass'))})")
    out += ["", "Known gate caveats: the Oct 26 table keys rows by Round 6 sheet number (its B01/C02 are not the "
            "Round 1 items of the same name); the E02 guard pins the pre-Round-11 Actions order; the E12/C14 "
            "guards were updated for the owner's Round 11 reversal (Create starts with no moves).", "",
            "**Required next step:** add a Round 11 guard table (the 34 R11 PASS items) and a Round 12 table once "
            "the owner reports Round 12, so no accepted behaviour can leave a checklist unguarded.", ""]
    m = extras.get("item_ops_matrix")
    if m:
        ops = ["add", "remove", "quantity", "sort", "pc_items_view_edit", "top_up_add_missing"]
        out += ["## 4. Bag / PC item operations (mandatory feature 7)", "",
                "| Gen | " + " | ".join(o.replace("_", " ") for o in ops) + " |", "|---" * (len(ops) + 1) + "|"]
        for row in m:
            cells = []
            for o in ops:
                v = row.get(o) or next((row[k] for k in row if k.startswith(o.split("_")[0]) and isinstance(row[k], dict)), None)
                if not isinstance(v, dict):
                    cells.append("—")
                    continue
                ex = v.get("exists")
                ha = str(v.get("hardware_accepted", ""))
                cells.append(cell(("exists" if ex is True else ("n/a" if ex in (False, None, "n/a") else str(ex)))
                                  + " · accepted: " + (ha or "no"), 90))
            out.append(f"| {cell(row.get('gen'))} | " + " | ".join(cells) + " |")
        out.append("")
    usf = extras.get("untested_safety_functions", [])
    out += ["## 5. Safety-critical functions never accepted on hardware", "",
            "| Function | Gens | Evidence |", "|---|---|---|"]
    for u in usf:
        out.append(f"| {cell(u.get('function'), 160)} | {cell(u.get('gens'))} | {cell(u.get('evidence'), 260)} |")
    sc = sorted([i for i in issues if i.get("safety_critical") and i["status"] != "Hardware Accepted"], key=lambda i: i["id"])
    out += ["", "### Safety-critical ledger issues without hardware acceptance", ""]
    for i in sc:
        out.append(f"- `{i['id']}` {cell(i.get('title'), 160)} — {i['status']}")
    out += ["", "## 6. Every round's hardware checklist must include", "",
            "1. Every row in §1 with *recheck = yes* whose code changed since its last PASS.",
            "2. Every ledger row with status Fixed in Code — Awaiting Hardware (its acceptance candidate).",
            "3. Every safety function in §5 that an ordinary owner save can exercise.",
            "4. No row may be removed from a checklist unless it is owner-PASSed or the owner records a decision; "
            "the ledger records the reason.", ""]
    open(os.path.join(OUT, "PERMANENT_REGRESSION_MATRIX.md"), "w").write("\n".join(line.rstrip() for line in out) + "\n")


def main():
    recs, issues, accepted, extras, lineage = load()
    C = counts(issues, extras)
    os.makedirs(OUT, exist_ok=True)
    write_ledger(issues, C)
    write_audit(recs, issues, lineage, C)
    write_gaps(issues, C)
    write_matrix(issues, accepted, extras, lineage, C)
    json.dump(C, open(os.path.join(HERE, "counts.json"), "w"), indent=1)
    print(json.dumps(C, indent=1))


if __name__ == "__main__":
    main()
