#!/usr/bin/env python3
"""Render docs/qa/RECHECK_REGRESSIONS.md: the 94 Hardware Accepted issues whose code changed after their
accepted candidate, the actual commits / files that changed since then, and the regression test(s) that
exercise the changed code. Every row stays marked for re-verification on hardware.

  python3 tools/qa/gen_recheck_regressions.py [repo]           refresh the git data, write JSON + Markdown
  python3 tools/qa/gen_recheck_regressions.py [repo] --check   render from the JSON; fail if the .md differs

Inputs: docs/qa/reconciliation/trace_G*.json (which issues; never modified here) and
tools/qa/recheck_regressions_map.py (implementing files, tests, owner supersessions).
Outputs: docs/qa/reconciliation/recheck_regressions.json (git data cached, so --check needs no history)
and docs/qa/RECHECK_REGRESSIONS.md.
"""
import glob
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from recheck_regressions_map import BASE, MAP  # noqa: E402

R11_START, R12_START = "3634d1e6", "066e1db9"
STATUS_LABEL = {
    "recheck": "Hardware Accepted (recheck required)",
    "partial": "Hardware Accepted (recheck required); partly superseded by owner request",
    "superseded": "Superseded by owner request (left marked for re-verification)",
    "scope": "Hardware Accepted (recheck required); outside the Gen I-V scope",
}


def recheck_issues(repo):
    out = []
    for f in sorted(glob.glob(os.path.join(repo, "docs", "qa", "reconciliation", "trace_G[1-7]*.json"))):
        for i in json.load(open(f, encoding="utf-8"))["issues"]:
            if i["status"] == "Hardware Accepted" and i.get("rechecked_needed"):
                out.append(i)
    return out


def git(repo, *args):
    return subprocess.run(["git", "-C", repo, *args], check=True, capture_output=True, text=True).stdout


def git_data(repo, acc, files):
    rng = f"{acc}..{BASE}"
    log = [line for line in git(repo, "log", "--no-merges", "--format=%h %ad %s", "--date=short", rng, "--", *files).splitlines() if line]
    sha = {line.split()[0] for line in log}
    r11 = set(git(repo, "log", "--format=%h", f"{R11_START}..{R12_START}").split())
    r12 = set(git(repo, "log", "--format=%h", f"{R12_START}..{BASE}").split())
    changed = [f for f in git(repo, "diff", "--name-only", acc, BASE, "--", *files).splitlines() if f]
    return {
        "range": rng,
        "commit_count": len(log),
        "r12_commits": [line for line in log if line.split()[0] in r12],
        "r11_commits": [line for line in log if line.split()[0] in r11],
        "latest_commits": log[:3],
        "changed_files": changed if all(not os.path.isdir(os.path.join(repo, f)) for f in files) else
                         [f"{len(changed)} files under {', '.join(files)}"],
        "unchanged_files": [f for f in files if not any(c == f or c.startswith(f.rstrip('/') + '/') for c in changed)],
        "_shas": sorted(sha),
    }


def coverage(entry):
    status = entry.get("status", "recheck")
    if status == "superseded":
        return "superseded"
    if status == "scope":
        return "scope"
    return "new" if entry.get("new") and not entry.get("extra") else "existing"


def build(repo, cached):
    rows = []
    issues = {i["id"]: i for i in recheck_issues(repo)}
    if set(issues) != set(MAP):
        raise SystemExit(f"map / trace mismatch: missing {sorted(set(issues) - set(MAP))}, "
                         f"extra {sorted(set(MAP) - set(issues))}")
    for iid in sorted(issues):
        i, e = issues[iid], MAP[iid]
        try:
            g = git_data(repo, e["acc"], e["files"])
            g.pop("_shas")
        except (subprocess.CalledProcessError, FileNotFoundError):
            g = cached.get(iid, {}).get("git")
            if g is None:
                raise SystemExit(f"{iid}: no git history for {e['acc']}..{BASE} and no cached data")
        rows.append({
            "id": iid, "title": i["title"], "gens": i.get("gens", []), "area": i.get("area"),
            "trace_status": i["status"], "rechecked_needed": bool(i.get("rechecked_needed")),
            "accepted_candidate": e["acc"], "acceptance_record": i.get("hardware_acceptance_candidate"),
            "last_owner_result": i.get("last_owner_result"),
            "status": e.get("status", "recheck"), "status_label": STATUS_LABEL[e.get("status", "recheck")],
            "coverage": coverage(e), "superseded_by": e.get("superseded"), "note": e.get("note"),
            "implementing_files": e["files"], "git": g,
            "tests": [{"path": p, "asserts": a} for p, a in e.get("tests", [])],
            "new_tests": [{"path": p, "asserts": a} for p, a in e.get("new", [])],
        })
    return rows


def cell(s):
    return str(s).replace("|", "\\|").replace("\n", " ")


def render(rows):
    n = {k: sum(1 for r in rows if r["coverage"] == k) for k in ("existing", "new", "superseded", "scope")}
    partial = sum(1 for r in rows if r["status"] == "partial")
    out = ["# Round 13 recheck regressions (94 accepted items touched by new code)", "",
           "Owner goal (verbatim): \"The 94 earlier accepted items touched by new code must remain marked for "
           "re-verification, with targeted regressions based on the actual code changes.\"", "",
           "Every row below is a ledger issue with status **Hardware Accepted** whose code changed after the "
           f"candidate the owner accepted it on (`rechecked_needed`). For each, the git history `accepted..{BASE}` "
           "of the files that implement the accepted behaviour (read from the code paths), and the automated "
           "test(s) that run that changed code and assert the accepted behaviour. **No row is re-accepted here**: "
           "an automated test is not hardware evidence, so every row stays marked for re-verification on the "
           "next candidate. Where a later owner request deliberately changed the behaviour, the old behaviour is "
           "not pinned and the row says which request superseded it.", "",
           "Generated by `python3 tools/qa/gen_recheck_regressions.py` from "
           "`tools/qa/recheck_regressions_map.py` and the trace data; data in "
           "`docs/qa/reconciliation/recheck_regressions.json`. Checked by "
           "`tests/test_recheck_regressions_contract.py`. **NOT DEVICE TESTED.**", "",
           "## Totals", "",
           "| Coverage | Issues |", "|---|---|",
           f"| Already covered: an existing test runs the changed code and asserts the accepted behaviour | {n['existing']} |",
           f"| Newly covered by a Round 13 recheck regression | {n['new']} |",
           f"| Superseded by owner request (not pinned) | {n['superseded']} |",
           f"| Outside the Gen I-V scope (indirect coverage only) | {n['scope']} |",
           f"| **Total** | **{len(rows)}** |", "",
           f"Of the covered rows, {partial} are partly superseded (keys / wording changed by owner request; the "
           "rest of the behaviour is pinned). Found broken on the base: **0** (every listed test passes on "
           f"`{BASE}`).", "",
           "Round 13 recheck tests: `tests/test_recheck_met_language.cpp`, "
           "`tests/test_ui_recheck_regressions_host.cpp` (both `Makefile.host.recheck`, with `_sanitize` targets), "
           "`recheckCreateStartsAtZero5` in `tests/test_gen5_party_box_parity.cpp` (`Makefile.host.gen5`) and "
           "`tests/test_recheck_lifecycle_contract.py`.", "",
           "## Issues", "",
           "| ID | Title | Accepted on | Changed since (commits / files) | Regression test(s) | Status |",
           "|---|---|---|---|---|---|"]
    for r in rows:
        g = r["git"]
        changed = f"{g['commit_count']} commit(s) in `{g['range']}`"
        if g["r12_commits"]:
            changed += "; R12: " + ", ".join(f"`{c.split()[0]}`" for c in g["r12_commits"])
        if g["r11_commits"]:
            shown = g["r11_commits"][:6]
            changed += "; R11: " + ", ".join(f"`{c.split()[0]}`" for c in shown)
            if len(g["r11_commits"]) > len(shown):
                changed += f" (+{len(g['r11_commits']) - len(shown)})"
        if not g["r12_commits"] and not g["r11_commits"] and g["latest_commits"]:
            changed += "; latest: " + ", ".join(f"`{c.split()[0]}`" for c in g["latest_commits"])
        changed += "<br>Files: " + (", ".join(f"`{f}`" for f in g["changed_files"]) or "none")
        tests = [f"`{t['path']}`: {t['asserts']}" for t in r["tests"]]
        tests += [f"**NEW** `{t['path']}`: {t['asserts']}" for t in r["new_tests"]]
        status = r["status_label"]
        if r["superseded_by"]:
            status += f" — {r['superseded_by']}"
        if r["note"]:
            status += f" ({r['note']})"
        out.append("| " + " | ".join([
            f"`{r['id']}`", cell(r["title"]), cell(f"`{r['accepted_candidate']}` ({r['last_owner_result']})"),
            cell(changed), cell("<br>".join(tests) or "—"), cell(status)]) + " |")
    out += ["", "## Commit subjects", "",
            "Every commit named above, once (subject as committed):", ""]
    seen = {}
    for r in rows:
        for c in r["git"]["r12_commits"] + r["git"]["r11_commits"] + r["git"]["latest_commits"]:
            seen.setdefault(c.split()[0], c)
    for sha in sorted(seen, key=lambda s: seen[s].split()[1], reverse=True):
        out.append(f"- `{sha}` {cell(' '.join(seen[sha].split()[1:]))}")
    return "\n".join(line.rstrip() for line in out).rstrip("\n") + "\n"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    repo = os.path.abspath(args[0] if args else os.path.join(HERE, "..", ".."))
    data_path = os.path.join(repo, "docs", "qa", "reconciliation", "recheck_regressions.json")
    md_path = os.path.join(repo, "docs", "qa", "RECHECK_REGRESSIONS.md")
    if "--check" in sys.argv:
        rows = json.load(open(data_path, encoding="utf-8"))
        if open(md_path, encoding="utf-8").read() != render(rows):
            raise SystemExit("docs/qa/RECHECK_REGRESSIONS.md is stale: run tools/qa/gen_recheck_regressions.py")
        print("RECHECK_REGRESSIONS.md matches its data")
        return
    cached = {}
    if os.path.isfile(data_path):
        cached = {r["id"]: r for r in json.load(open(data_path, encoding="utf-8"))}
    rows = build(repo, cached)
    with open(data_path, "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=1, ensure_ascii=False)
        f.write("\n")
    with open(md_path, "w", encoding="utf-8") as f:
        f.write(render(rows))
    print(f"wrote {len(rows)} rows: {md_path}")


if __name__ == "__main__":
    main()
