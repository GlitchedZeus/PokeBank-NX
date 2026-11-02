#!/usr/bin/env python3
"""Render docs/maintenance/BRANCH_ARCHIVE_MANIFEST.md from branch_audit.py JSON.

    python3 tools/qa/maintenance/render_branch_manifest.py \
        docs/maintenance/branch_audit_2026-11-02.json \
        --releases releases.json \
        --out docs/maintenance/BRANCH_ARCHIVE_MANIFEST.md

--releases is optional: the JSON of GET /repos/{owner}/{repo}/releases.
Output has no trailing whitespace and ends with a single newline.
"""

import argparse
import collections
import json

REPO_URL = "https://github.com/GlitchedZeus/PokeBank-NX"
ACTION_ORDER = ["KEEP", "KEEP-PAUSED", "DELETE-SAFE", "ARCHIVE-TAG-THEN-DELETE"]
ACTION_TEXT = {
    "KEEP": "Retained working branches (active lanes, open-PR heads/bases, candidate holders, recent unique work).",
    "KEEP-PAUSED": "Retained paused Gen VI-IX / legality branches used by an open PR.",
    "DELETE-SAFE": "Every commit is already in a retained branch (or patch-equivalent with an identical tree); no open PR; no hardware/QA evidence that is missing from retained history.",
    "ARCHIVE-TAG-THEN-DELETE": "Create annotated tag `archive/<branch>` at the exact tip first, verify, then delete. Includes every paused legality / Gen VI-IX branch without an open PR (never plain-deleted) and every branch with unique commits.",
}


def esc(s):
    return str(s).replace("|", "\\|").replace("\n", " ").strip()


def pr_links(r):
    out = []
    for p in r["prs"]:
        st = "merged" if p["merged"] else p["state"]
        out.append(f"[#{p['number']}]({REPO_URL}/pull/{p['number']}) {st}")
    if r["open_pr_base_of"]:
        out.append("base of open " + ", ".join(f"#{n}" for n in r["open_pr_base_of"]))
    return "<br>".join(out) if out else "none"


def summary(r):
    cs = r["content_status"]
    if cs == "MERGED":
        return f"none: every commit in {r.get('contained_in', 'retained history')}"
    n, ne = r["unique_commits"], r["unique_not_patch_equivalent"]
    if cs == "SUPERSEDED":
        return f"{n} commit(s) outside retained refs, all patch-equivalent" + (
            "; tip tree identical to a retained commit" if r["tip_tree_in_protected"] else "")
    cats = ", ".join(f"{k} {v}" for k, v in sorted(r["unique_categories"].items()))
    paths = ", ".join(r["unique_top_paths"][:3])
    s = f"{n} unique commit(s), {ne} not patch-equivalent; files: {cats or 'none'}"
    if paths:
        s += f"; top: {paths}"
    if r["unique_evidence_files"]:
        s += "; QA evidence: " + ", ".join(f"`{f}`" for f in r["unique_evidence_files"][:3])
    if cs == "DUPLICATE":
        s += "; same tip as " + ", ".join(f"`{x}`" for x in r["same_tip_as"])
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("audit_json")
    ap.add_argument("--releases")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    d = json.load(open(a.audit_json))
    rels = {r["tag_name"]: r for r in json.load(open(a.releases))} if a.releases else {}
    B = d["branches"]
    L = []
    w = L.append
    w("# Branch archive manifest")
    w("")
    w(f"**Snapshot:** {d['generated']} (remote `{d['remote']}`, `main` @ `{d['main_sha'][:8]}`), "
      f"{d['branch_count']} remote branches, {len(d['remote_tags'])} remote tags, "
      f"{len(d['pull_requests'])} pull requests.")
    w("")
    w("**Status: PHASE 1 AUDIT ONLY.** Nothing listed here has been deleted, tagged, closed or edited. "
      "Every row's Phase 2 status is **PENDING OWNER APPROVAL**. Companion report: "
      "[REPO_CLEANUP_AUDIT_2026-11-02.md](REPO_CLEANUP_AUDIT_2026-11-02.md); machine-readable data: "
      "[branch_audit_2026-11-02.json](branch_audit_2026-11-02.json); generator: "
      "`tools/qa/maintenance/branch_audit.py` + `tools/qa/maintenance/render_branch_manifest.py`.")
    w("")
    w("## Method")
    w("")
    w("- **Retained set** = every KEEP / KEEP-PAUSED branch tip plus every tag that exists on the remote "
      f"({d['protected_tip_count']} distinct commits).")
    w("- **Unique commits** = `git rev-list <tip> --not <retained set>` (a retained branch is compared "
      "against the *other* retained refs).")
    w("- **Patch equivalence** = `git patch-id --stable` of each unique non-merge commit against all "
      "retained history; **tree equivalence** = the tip's tree equals the tree of a retained commit.")
    w("- **Content status:** MERGED (fully contained), SUPERSEDED (all patch-equivalent), DUPLICATE "
      "(same tip as another branch), UNIQUE (at least one commit with no equivalent).")
    w("- **Classification:** ACTIVE, PAUSED (Gen VI-IX / legality: preserved, never plain-deleted), MERGED, "
      "SUPERSEDED, DUPLICATE, ABANDONED-UNIQUE.")
    w("- **Evidence check:** for every non-retained branch, each blob at its tip under `docs/hardware/`, "
      "`docs/qa/`, `docs/qa-archives/`, `docs/handoffs/`, or named `RETEST_*`, `ROUND*_RESULTS*`, owner "
      "intake, screenshots or `*DEVICE_TEST*` / `*HARDWARE*` documents must exist in retained history, "
      "otherwise the branch cannot be DELETE-SAFE.")
    w("- GitHub keeps `refs/pull/<n>/head` for every PR even after a branch is deleted. That is an extra "
      "safety net, **not** counted as preservation here.")
    w("")
    c = collections.Counter(r["action"] for r in B)
    k = collections.Counter(r["classification"] for r in B)
    w("## Counts")
    w("")
    w("| Action | Branches |")
    w("|---|---:|")
    for act in ACTION_ORDER:
        w(f"| {act} | {c.get(act, 0)} |")
    w(f"| **Total** | **{len(B)}** |")
    w("")
    w("| Classification | Branches |")
    w("|---|---:|")
    for cl, n in sorted(k.items()):
        w(f"| {cl} | {n} |")
    w("")
    for act in ACTION_ORDER:
        rows = [r for r in B if r["action"] == act]
        w(f"## {act} ({len(rows)})")
        w("")
        w(ACTION_TEXT[act])
        w("")
        w("| Old branch name | Tip SHA | Last activity | Related PR(s) | Classification | Unique-work summary | Recommended action | Proposed preservation location | Phase 2 status |")
        w("|---|---|---|---|---|---|---|---|---|")
        for r in rows:
            cl = r["classification"]
            if cl in ("ACTIVE", "PAUSED", "ABANDONED-UNIQUE") and r["content_status"] not in ("UNIQUE",):
                cl += f" (content: {r['content_status']})"
            reasons = r.get("keep_reasons")
            pres = r["preservation"] + (" — " + "; ".join(reasons) if reasons else "")
            w(f"| `{r['branch']}` | `{r['tip']}` | {r['last_commit'][:10]} ({esc(r['author'])}) | "
              f"{pr_links(r)} | {cl} | {esc(summary(r))} | {r['action']} | {esc(pres)} | "
              f"{r['phase2_status']} |")
        w("")

    # tags / releases
    w("## Tags and releases (all KEEP)")
    w("")
    w("No tag is proposed for deletion. `nro/*` are the archived candidate NRO pre-releases; "
      "`evidence/*` is an evidence draft pre-release; `archive/local-reflog-20261102/*` preserve commits "
      "recovered from local reflogs on November 2.")
    w("")
    w("| Tag | Commit | Release | Meaning | Action |")
    w("|---|---|---|---|---|")
    for t, sha in sorted(d["remote_tags"].items()):
        if t.startswith("archive/local-reflog-20261102/"):
            continue
        rel = rels.get(t)
        rtxt = ("draft pre-release" if rel and rel["draft"] else "pre-release" if rel and rel["prerelease"]
                else "release" if rel else "none")
        meaning = ("candidate NRO archive" if t.startswith("nro/") else
                   "evidence snapshot (legality #314 head)" if t.startswith("evidence/") else "archive")
        w(f"| `{t}` | `{sha[:8]}` | {rtxt} | {meaning} | KEEP |")
    ref = sorted(t for t in d["remote_tags"] if t.startswith("archive/local-reflog-20261102/"))
    if ref:
        w(f"| `archive/local-reflog-20261102/*` ({len(ref)} tags) | various | none | recovered local-reflog commits | KEEP |")
    w("")
    w("Proposed new tags: one annotated `archive/<old-branch-name>` per ARCHIVE-TAG-THEN-DELETE row "
      f"({c.get('ARCHIVE-TAG-THEN-DELETE', 0)} tags), each at the exact tip SHA shown above.")
    w("")

    # NRO archive
    w("## NRO candidate archive")
    w("")
    w("Candidate application commits ever named in `docs/qa/CANDIDATE_UNDER_AUDIT.env`, with the retained "
      "refs that keep each one reachable (see `docs/hardware/NRO_ARCHIVE.md` on `main`).")
    w("")
    w("| Candidate commit | `nro/<sha8>` tag | Reachable from |")
    w("|---|---|---|")
    for sha in d["candidate_code_shas"]:
        tag = f"nro/{sha[:8]}"
        has = tag in d["remote_tags"]
        holders = d.get("candidate_holders", {}).get(sha, [])
        shown = holders[:6] + ([f"+{len(holders) - 6} more"] if len(holders) > 6 else [])
        w(f"| `{sha}` | {'`' + tag + '` present' if has else 'none yet'} | "
          f"{', '.join('`' + h + '`' for h in shown) or 'NOT REACHABLE'} |")
    w("")

    # PRs
    prs = d["pull_requests"]
    w(f"## Pull requests ({len(prs)})")
    w("")
    w("| PR | Title | State | Merged | Head branch | Base | Head branch still exists |")
    w("|---|---|---|---|---|---|---|")
    for p in prs:
        w(f"| [#{p['number']}]({REPO_URL}/pull/{p['number']}) | {esc(p['title'])} | {p['state']}"
          f"{' (draft)' if p['draft'] else ''} | {'yes' if p['merged'] else 'no'} | `{p['head']}` | "
          f"`{p['base']}` | {'yes' if p['head_branch_exists'] else 'no'} |")
    w("")
    text = "\n".join(line.rstrip() for line in L).rstrip("\n") + "\n"
    open(a.out, "w").write(text)


if __name__ == "__main__":
    main()
