#!/usr/bin/env python3
"""Read-only remote-branch audit for PokeBank-NX (repository cleanup, Phase 1).

Classifies every branch under refs/remotes/<remote>/ against a protected set
(KEEP / KEEP-PAUSED branches plus every tag that exists on the remote) using
git ancestry, patch-id equivalence and tree equivalence, and writes a
machine-readable JSON report. It never writes to the remote: it only runs
read-only git plumbing (and, with --fetch-prs, unauthenticated GitHub REST
GETs).

Re-run before Phase 2 to detect drift:

    git fetch origin                      # never --prune
    git fetch origin 'refs/tags/*:refs/tags/*'
    python3 tools/qa/maintenance/branch_audit.py --repo . --fetch-prs \
        --out /tmp/branch_audit_new.json \
        --compare docs/maintenance/branch_audit_2026-11-02.json

--compare prints every branch whose tip SHA, PR set, classification or action
changed, plus branches that appeared or disappeared. Any drift means the
manifest must be regenerated and re-approved before deleting anything.
"""

from __future__ import annotations

import argparse
import collections
import datetime as dt
import json
import re
import subprocess
import sys
import urllib.request

OWNER_REPO = "GlitchedZeus/PokeBank-NX"

# Branches the owner named as active. Never deletion candidates.
ACTIVE_NAMED = {
    "main": "default branch / owner-accepted Gen I-V application",
    "development/gen6-plus": "Gen VI+ application line (paused behind Gen I-V acceptance)",
    "fix/round11-gen15-repairs": "PR #332, Round 11 repairs / Round 12 candidate 066e1db9",
    "fix/round12-gen15-repairs": "PR #333, Round 13 candidate 9c301c48 under owner hardware test",
    "claude/optimistic-euler-ph2z4f": "active Claude session branch",
    "claude/gen1-5-legality-campaign-w9mkom": "legality PR #314",
    "work/claude-gen15-legality-independent-20261030": "legality PR #327",
    "work/claude-gen69-legality-foundation-20261030": "legality PR #328",
}
ACTIVE_PATTERNS = [(re.compile(r"^r13/"), "Round 13 lane (r13/*)")]

# Gen VI-IX and legality work is PAUSED, not abandoned: preserved, never
# plain-deleted.
PAUSED_PATTERNS = [
    re.compile(r"legality"),
    re.compile(r"pkhex-parity"),
    re.compile(r"gen[6-9]-native"),
    re.compile(r"gen6-9|gen69|gen5-9|gen1-9"),
    re.compile(r"gen6-plus"),
]

EVIDENCE_RE = re.compile(
    r"(^docs/hardware/|^docs/qa/|^docs/qa-archives/|^docs/handoffs/|RETEST_|ROUND\d+_RESULTS|"
    r"OWNER_REPORT|owner-intake|screenshots?/|(DEVICE_TEST|HARDWARE)[^/]*\.(md|csv|html|txt)$)",
    re.IGNORECASE,
)
CANDIDATE_FILES = [
    "docs/qa/CANDIDATE_UNDER_AUDIT.env",
    "docs/hardware/NRO_ARCHIVE.md",
    ".github/workflows/archive-candidate-nro.yml",
]


def git(repo, *args, stdin=None):
    r = subprocess.run(
        ["git", "-C", repo, *args], input=stdin, capture_output=True, text=True
    )
    if r.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed: {r.stderr.strip()}")
    return r.stdout


def git_ok(repo, *args):
    return subprocess.run(["git", "-C", repo, *args], capture_output=True).returncode == 0


def fetch_prs():
    prs = []
    for page in range(1, 20):
        url = (
            f"https://api.github.com/repos/{OWNER_REPO}/pulls?state=all"
            f"&per_page=100&page={page}"
        )
        req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json"})
        with urllib.request.urlopen(req, timeout=60) as resp:
            batch = json.load(resp)
        if not batch:
            break
        prs.extend(batch)
    return prs


def remote_tags(repo, remote):
    tags = {}
    for line in git(repo, "ls-remote", "--tags", remote).splitlines():
        sha, ref = line.split("\t")
        name = ref[len("refs/tags/"):]
        if name.endswith("^{}"):
            tags[name[:-3]] = sha  # peeled commit wins
        else:
            tags.setdefault(name, sha)
    return tags


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", default=".")
    ap.add_argument("--remote", default="origin")
    ap.add_argument("--prs", help="JSON file of GET /pulls?state=all (all pages concatenated)")
    ap.add_argument("--fetch-prs", action="store_true", help="fetch PRs from the public REST API")
    ap.add_argument("--recent-days", type=int, default=7,
                    help="unique, non-protected branches with a commit this recent are kept for owner review")
    ap.add_argument("--now", default=None, help="audit date (YYYY-MM-DD), default today UTC")
    ap.add_argument("--out", required=True)
    ap.add_argument("--compare", help="earlier JSON report to diff against")
    a = ap.parse_args()
    repo, remote = a.repo, a.remote

    if a.fetch_prs:
        prs = fetch_prs()
    elif a.prs:
        prs = json.load(open(a.prs))
    else:
        sys.exit("need --prs FILE or --fetch-prs")

    now = dt.datetime.strptime(a.now, "%Y-%m-%d").replace(tzinfo=dt.timezone.utc) if a.now \
        else dt.datetime.now(dt.timezone.utc)

    # ---- branches -------------------------------------------------------
    fmt = "%(refname:lstrip=3)%00%(objectname)%00%(committerdate:iso-strict)%00%(authorname)%00%(subject)"
    branches = {}
    for line in git(repo, "for-each-ref", f"--format={fmt}", f"refs/remotes/{remote}").splitlines():
        name, sha, date, author, subject = line.split("\0")
        if name == "HEAD":
            continue
        branches[name] = dict(name=name, tip=sha, date=date, author=author, subject=subject)
    main_ref = f"{remote}/main"
    main_sha = branches["main"]["tip"]

    tags = remote_tags(repo, remote)
    # make sure each remote tag commit exists locally
    tag_commits = {}
    for t, sha in tags.items():
        if git_ok(repo, "cat-file", "-e", f"{sha}^{{commit}}"):
            tag_commits[t] = git(repo, "rev-parse", f"{sha}^{{commit}}").strip()

    # ---- PR maps --------------------------------------------------------
    pr_by_head = collections.defaultdict(list)
    open_bases = collections.defaultdict(list)
    for p in prs:
        if p["head"]["repo"] and p["head"]["repo"]["full_name"] != OWNER_REPO:
            continue
        rec = dict(
            number=p["number"], state=p["state"], merged=bool(p.get("merged_at")),
            draft=p.get("draft", False), base=p["base"]["ref"], head_sha=p["head"]["sha"],
            title=p["title"],
        )
        pr_by_head[p["head"]["ref"]].append(rec)
        if p["state"] == "open":
            open_bases[p["base"]["ref"]].append(p["number"])

    # ---- candidate NRO commits -----------------------------------------
    hist = git(repo, "log", f"--remotes={remote}", "-p", "--format=", "--", *CANDIDATE_FILES)
    cand_shas = sorted(set(re.findall(r"^\+CODE_SHA=([0-9a-f]{40})", hist, re.M)))
    cand_named = [b for b in branches if b != "main" and re.search(r"(?<![\w/-])" + re.escape(b) + r"(?![\w/-])", hist)]

    # ---- protected set (iterative: candidate holders) --------------------
    keep = {}  # name -> (status, reason)
    for b in branches:
        reasons, status = [], None
        if b in ACTIVE_NAMED:
            reasons.append(ACTIVE_NAMED[b]); status = "ACTIVE"
        for rx, why in ACTIVE_PATTERNS:
            if rx.search(b):
                reasons.append(why); status = "ACTIVE"
        open_heads = [p["number"] for p in pr_by_head.get(b, []) if p["state"] == "open"]
        if open_heads:
            reasons.append("head of open PR " + ", ".join(f"#{n}" for n in open_heads))
        if b in open_bases:
            reasons.append("base of open PR " + ", ".join(f"#{n}" for n in open_bases[b]))
        if b in cand_named:
            reasons.append("named in candidate/NRO archive history")
        if branches[b]["tip"] in cand_shas:
            reasons.append("tip is a candidate NRO commit")
        paused = any(rx.search(b) for rx in PAUSED_PATTERNS)
        if reasons:
            if status is None:
                status = "PAUSED" if paused else "ACTIVE"
            keep[b] = (status, reasons)

    def protected_refs():
        return [branches[b]["tip"] for b in keep] + list(tag_commits.values())

    # every candidate commit must stay reachable from the protected set
    for c in cand_shas:
        if not git_ok(repo, "cat-file", "-e", f"{c}^{{commit}}"):
            continue
        prot = protected_refs()
        if any(git_ok(repo, "merge-base", "--is-ancestor", c, r) for r in set(prot)):
            continue
        holders = git(repo, "for-each-ref", "--contains", c, "--format=%(refname:lstrip=3)",
                      f"refs/remotes/{remote}").split()
        for h in holders:
            if h != "HEAD":
                keep.setdefault(h, ("ACTIVE", []))[1].append(f"only holder of candidate {c[:8]}")

    prot = sorted(set(protected_refs()))

    # ---- patch-ids / trees of protected history ---------------------------
    pid_out = git(repo, "log", "--no-merges", "-p", "--format=commit %H", f"--remotes={remote}", "--tags")
    pid_map = {}
    r = subprocess.run(["git", "-C", repo, "patch-id", "--stable"], input=pid_out,
                       capture_output=True, text=True)
    for line in r.stdout.splitlines():
        pid, c = line.split()
        pid_map[c] = pid
    prot_commits = set(git(repo, "rev-list", "--stdin", stdin="\n".join(prot) + "\n").split())
    prot_pids = {pid_map[c] for c in prot_commits if c in pid_map}
    prot_trees = set(git(repo, "log", "--format=%T", "--stdin", stdin="\n".join(prot) + "\n").split())
    prot_blobs = set()
    for line in git(repo, "rev-list", "--objects", "--stdin", stdin="\n".join(prot) + "\n").splitlines():
        prot_blobs.add(line.split(" ", 1)[0])

    # containment helpers: which protected/other branch contains this tip
    tip_to_names = collections.defaultdict(list)
    for b, info in branches.items():
        tip_to_names[info["tip"]].append(b)

    keep_names_sorted = sorted(keep, key=lambda n: (n != "main", n))

    def container_of(sha, candidates):
        for n in candidates:
            if git_ok(repo, "merge-base", "--is-ancestor", sha, branches[n]["tip"]):
                return n
        return None

    recent_cut = now - dt.timedelta(days=a.recent_days)
    rows = []
    for b in sorted(branches):
        info = branches[b]
        tip = info["tip"]
        ahead, behind = (int(x) for x in git(repo, "rev-list", "--left-right", "--count",
                                             f"{tip}...{main_sha}").split())
        in_main = git_ok(repo, "merge-base", "--is-ancestor", tip, main_sha)
        prs_here = pr_by_head.get(b, [])
        paused = any(rx.search(b) for rx in PAUSED_PATTERNS)
        row = dict(
            branch=b, tip=tip, last_commit=info["date"], author=info["author"],
            subject=info["subject"], ahead_of_main=ahead, behind_main=behind,
            in_main=in_main,
            prs=[dict(number=p["number"], state=p["state"], merged=p["merged"], base=p["base"],
                      head_sha_matches_tip=(p["head_sha"] == tip)) for p in prs_here],
            open_pr_base_of=open_bases.get(b, []),
            same_tip_as=[n for n in tip_to_names[tip] if n != b],
        )
        # unique commits relative to protected set, excluding the branch itself
        prot_ex = [s for s in prot if s != tip] if b in keep else prot
        neg = "".join(f"^{s}\n" for s in prot_ex)
        uniq = git(repo, "rev-list", tip, "--stdin", stdin=neg).split()
        merges = set(git(repo, "rev-list", "--merges", tip, "--stdin", stdin=neg).split()) if uniq else set()
        nonmerge = [c for c in uniq if c not in merges]
        if b in keep and uniq:
            # compare a retained branch against the OTHER retained refs only
            others = git(repo, "rev-list", "--stdin", stdin="\n".join(prot_ex) + "\n").split()
            cmp_pids = {pid_map[c] for c in others if c in pid_map}
            cmp_trees = set(git(repo, "log", "--format=%T", "--stdin",
                                stdin="\n".join(prot_ex) + "\n").split())
        else:
            cmp_pids, cmp_trees = prot_pids, prot_trees
        not_equiv = [c for c in nonmerge if pid_map.get(c) not in cmp_pids]
        # empty commits have no patch-id; treat them as equivalent (no content)
        not_equiv = [c for c in not_equiv if c in pid_map]
        tree = git(repo, "rev-parse", f"{tip}^{{tree}}").strip()
        tree_equiv = tree in cmp_trees
        row.update(unique_commits=len(uniq), unique_nonmerge=len(nonmerge),
                   unique_merges=len(merges), unique_not_patch_equivalent=len(not_equiv),
                   tip_tree_in_protected=tree_equiv)

        # describe unique content
        files = []
        if not_equiv or (uniq and not tree_equiv):
            src = not_equiv or uniq
            out = git(repo, "show", "--no-renames", "--name-only", "--format=", *src[:400])
            files = sorted(set(f for f in out.splitlines() if f))
        cats = collections.Counter()
        for f in files:
            if EVIDENCE_RE.search(f):
                cats["hardware/QA evidence"] += 1
            elif f.startswith(".github/workflows/"):
                cats["workflows"] += 1
            elif f.startswith("tests/") or "/test" in f or f.startswith("test"):
                cats["tests"] += 1
            elif f.startswith(("source/", "src/", "include/", "Makefile", "romfs/", "data/", "tools/")):
                cats["code/data"] += 1
            elif f.endswith((".md", ".txt", ".csv", ".html")) or f.startswith("docs/"):
                cats["docs/research"] += 1
            else:
                cats["other"] += 1
        top_dirs = collections.Counter("/".join(f.split("/")[:2]) for f in files).most_common(6)
        row["unique_files"] = len(files)
        row["unique_categories"] = dict(cats)
        row["unique_top_paths"] = [f"{d} ({n})" for d, n in top_dirs]
        row["unique_evidence_files"] = [f for f in files if EVIDENCE_RE.search(f)][:40]

        # evidence blobs at the tip: present in protected history?
        ev_missing = []
        if uniq:
            for line in git(repo, "ls-tree", "-r", tip).splitlines():
                meta, path = line.split("\t", 1)
                if EVIDENCE_RE.search(path):
                    blob = meta.split()[2]
                    if blob not in prot_blobs:
                        ev_missing.append(path)
        row["evidence_blobs_not_in_protected"] = ev_missing[:60]
        row["evidence_blobs_not_in_protected_count"] = len(ev_missing)

        recent = dt.datetime.fromisoformat(info["date"]) >= recent_cut
        open_pr = any(p["state"] == "open" for p in prs_here) or bool(open_bases.get(b))

        # ---- content status (every branch) ----
        if not uniq:
            cls = "MERGED"
        elif not not_equiv and (not merges or tree_equiv):
            cls = "SUPERSEDED"
        elif tree_equiv and not ev_missing:
            cls = "SUPERSEDED"
        else:
            cls = "ABANDONED-UNIQUE"
        content_superseded = cls == "SUPERSEDED"
        dups = row["same_tip_as"]
        if cls != "MERGED" and dups:
            cls = "DUPLICATE"
        row["content_status"] = "UNIQUE" if cls == "ABANDONED-UNIQUE" else cls
        if cls == "MERGED":
            holder = "main" if in_main else container_of(
                tip, [n for n in keep_names_sorted if n != b])
            if holder:
                row["contained_in"] = f"{holder} @ {branches[holder]['tip'][:8]}"
            else:
                t = next((t for t, s2 in sorted(tag_commits.items())
                          if git_ok(repo, "merge-base", "--is-ancestor", tip, s2)), None)
                row["contained_in"] = f"tag {t}"
        elif b not in keep:
            holders = [h for h in git(repo, "for-each-ref", "--contains", tip,
                                      "--format=%(refname:lstrip=3)", f"refs/remotes/{remote}").split()
                       if h not in ("HEAD", b)]
            row["tip_also_reachable_from"] = holders[:8]

        # ---- classification / action ----
        if b in keep:
            status, reasons = keep[b]
            row["keep_reasons"] = reasons
            named = b in ACTIVE_NAMED or any(rx.search(b) for rx, _ in ACTIVE_PATTERNS) \
                or any(r.startswith(("named in candidate", "tip is a candidate", "only holder")) for r in reasons)
            if named:
                row["classification"] = "ACTIVE"; row["action"] = "KEEP"
            elif paused:
                row["classification"] = "PAUSED"; row["action"] = "KEEP-PAUSED"
            else:
                # kept only because an open PR uses it (head or base)
                row["classification"] = cls
                row["action"] = "KEEP"
                row["keep_reasons"] = reasons + ["kept while an open PR uses it; see PR proposals"]
            row["preservation"] = "retained branch"
        else:
            row["classification"] = "PAUSED" if paused else cls
            if paused:
                row["action"] = "ARCHIVE-TAG-THEN-DELETE"
            elif cls in ("MERGED", "SUPERSEDED") and not ev_missing:
                row["action"] = "DELETE-SAFE"
            elif cls == "DUPLICATE" and content_superseded and not ev_missing:
                row["action"] = "DELETE-SAFE"
            elif recent and cls in ("ABANDONED-UNIQUE", "DUPLICATE"):
                row["classification"] = "ACTIVE"
                row["action"] = "KEEP"
                row["keep_reasons"] = [f"unique commits within the last {a.recent_days} days; owner review before archiving"]
            else:
                row["action"] = "ARCHIVE-TAG-THEN-DELETE"
            if row["action"] == "KEEP":
                row["preservation"] = "retained branch"
            elif row["action"] == "DELETE-SAFE":
                if cls == "MERGED":
                    row["preservation"] = f"contained in {row['contained_in']}"
                else:
                    row["preservation"] = ("all commits patch-equivalent to retained history"
                                           + ("; tip tree identical to a retained commit" if tree_equiv else ""))
            else:
                row["preservation"] = f"tag archive/{b} @ {tip[:8]} (proposed)"
        row["open_pr"] = open_pr
        row["recent"] = recent
        row["phase2_status"] = "PENDING OWNER APPROVAL"
        rows.append(row)

    cand_holders = {}
    for c in cand_shas:
        if not git_ok(repo, "cat-file", "-e", f"{c}^{{commit}}"):
            cand_holders[c] = ["MISSING LOCALLY"]
            continue
        refs = git(repo, "for-each-ref", "--contains", c, "--format=%(refname)",
                   f"refs/remotes/{remote}", "refs/tags").split()
        hs = []
        for ref in refs:
            if ref.startswith(f"refs/remotes/{remote}/"):
                n = ref[len(f"refs/remotes/{remote}/"):]
                if n in keep:
                    hs.append(n)
            elif ref.startswith("refs/tags/"):
                t = ref[len("refs/tags/"):]
                if t in tags and not t.startswith("archive/local-reflog"):
                    hs.append("tag " + t)
        cand_holders[c] = sorted(hs, key=lambda x: (x != "main", x.startswith("tag "), x))

    report = dict(
        generated=now.strftime("%Y-%m-%dT%H:%M:%SZ"),
        remote=remote,
        main_sha=main_sha,
        branch_count=len(rows),
        protected_tip_count=len(prot),
        remote_tags={t: tag_commits.get(t, s) for t, s in sorted(tags.items())},
        candidate_code_shas=cand_shas,
        candidate_named_branches=cand_named,
        candidate_holders=cand_holders,
        branches=rows,
        pull_requests=[
            dict(number=p["number"], title=p["title"], state=p["state"],
                 merged=bool(p.get("merged_at")), draft=p.get("draft", False),
                 head=p["head"]["ref"], head_sha=p["head"]["sha"], base=p["base"]["ref"],
                 head_branch_exists=p["head"]["ref"] in branches,
                 head_branch_tip_matches=(branches.get(p["head"]["ref"], {}).get("tip") == p["head"]["sha"]),
                 created_at=p["created_at"], closed_at=p.get("closed_at"), merged_at=p.get("merged_at"))
            for p in sorted(prs, key=lambda x: -x["number"])
        ],
    )
    with open(a.out, "w") as fh:
        json.dump(report, fh, indent=1, sort_keys=False)
        fh.write("\n")
    c1 = collections.Counter(r["classification"] for r in rows)
    c2 = collections.Counter(r["action"] for r in rows)
    print(f"branches={len(rows)} protected_tips={len(prot)} tags={len(tags)}")
    print("classification:", dict(sorted(c1.items())))
    print("action:", dict(sorted(c2.items())))

    if a.compare:
        old = {r["branch"]: r for r in json.load(open(a.compare))["branches"]}
        new = {r["branch"]: r for r in rows}
        drift = 0
        for b in sorted(set(old) | set(new)):
            if b not in new:
                print(f"DRIFT gone: {b}"); drift += 1; continue
            if b not in old:
                print(f"DRIFT new: {b} {new[b]['tip'][:8]} {new[b]['action']}"); drift += 1; continue
            for k in ("tip", "classification", "action"):
                if old[b][k] != new[b][k]:
                    print(f"DRIFT {b}: {k} {old[b][k]} -> {new[b][k]}"); drift += 1
            if sorted(p["number"] for p in old[b]["prs"]) != sorted(p["number"] for p in new[b]["prs"]) or \
               sorted(p["state"] for p in old[b]["prs"]) != sorted(p["state"] for p in new[b]["prs"]):
                print(f"DRIFT {b}: PR set/state changed"); drift += 1
        print(f"drift items: {drift}")
        return 1 if drift else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
