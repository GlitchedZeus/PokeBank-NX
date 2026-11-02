#!/usr/bin/env bash
# prune_branches.sh - owner-run remote branch prune for PokeBank-NX (repository cleanup, Phase 2).
#
# The owner runs this locally with their own git credentials. Claude sessions cannot delete remote
# refs or push tags (the session git proxy answers HTTP 403), so this script is the only executor.
#
# DEFAULT IS A DRY RUN. Nothing on the remote changes unless --execute is given (and confirmed).
#
# Requirements: bash, git, curl, python3. gh is optional and used only when `gh auth status` succeeds
# (for a higher GitHub API rate limit and for --with-metadata). No token is ever printed.
#
# Plan source: docs/maintenance/branch_audit_2026-11-02.json (Phase 1 audit, owner-approved Phase 2).
#
# Groups (owner approval of 2026-11-02):
#   contained           DELETE-SAFE branches whose tip is contained in a preserved ref (64 at audit).
#                       Live `git merge-base --is-ancestor` check, then delete. No tag.
#   equivalent-archive  DELETE-SAFE branches that are only patch/tree-equivalent (6). Treated as
#                       archive: annotated tag archive/branches/<name> first, then delete.
#   paused-contained    PAUSED legality / Gen VI-IX branches fully contained in preserved history (160).
#                       No tag: live containment check, log original SHA + surviving ref, delete.
#   unique-archive      Unique-history branches (8 paused + 39 abandoned) + 1 patch-equivalent paused
#                       branch (48). Tag archive/branches/<name>, push, verify, then delete.
#   consolidation       integration/main-gen15-consolidation only: verify PR #330 is merged and its
#                       tip is contained in origin/main, then handled like `contained`.
#   legality-base       integration/legality-gen14-major-20261012 only, and only with
#                       --allow-legality-base: no open PR may use it as head or base (live check);
#                       then handled like `paused-contained`.
#   all (default)       contained + equivalent-archive + paused-contained + unique-archive.
#   Aliases: delete-safe = contained + equivalent-archive; archive = paused-contained + unique-archive.
# New tags in a full run: 6 + 48 = 54.
#
# Safety rules enforced for every branch, in every mode:
#   * a full verified backup bundle (git clone --mirror + git bundle --all) is made first;
#   * branch_audit.py --compare is re-run; tip / PR / KEEP drift on a targeted branch aborts;
#   * protected names are never touched (see is_protected below), nor any open-PR head or base;
#   * immediately before EACH deletion: live tip == recorded tip, open PRs re-read (abort the batch
#     on API failure), containment re-verified for no-tag groups, tag verified on the remote for
#     tag groups; delete with --force-with-lease pinned to the recorded tip;
#   * batches of at most 25; after each batch protected branches and open-PR heads/bases must still
#     exist on the remote.
#
# Usage:
#   tools/maintenance/prune_branches.sh [options]
#     --execute                 perform tags/deletions (asks you to type the confirmation phrase)
#     --yes                     skip the interactive confirmation (only with --execute)
#     --group NAME              repeatable; see groups above (default: all)
#     --batch-size N            1..25 (default 25)
#     --bundle-dir DIR          where the backup bundle goes (default: parent of the repository)
#     --bundle-source URL|PATH  what to mirror for the bundle (default: the remote's URL)
#     --skip-bundle             dry run only: skip the backup step (faster listing)
#     --skip-drift-check        dry run only: skip the branch_audit.py --compare step
#     --allow-legality-base     permit the legality-base group (still requires no open PR uses it)
#     --with-metadata           with gh logged in: milestones, repo description/topics, milestone plan
#     --audit-json PATH         default docs/maintenance/branch_audit_2026-11-02.json
#     --remote NAME             default origin
#     -h | --help
#
# Recovery: docs/maintenance/RECOVERY.md. Log: docs/maintenance/BRANCH_DELETION_LOG.md (+ .tsv).

set -Eeuo pipefail

OWNER_REPO="GlitchedZeus/PokeBank-NX"
API="https://api.github.com/repos/${OWNER_REPO}"
MAX_BATCH=25

EXECUTE=0
ASSUME_YES=0
GROUPS_REQ=()
BATCH_SIZE=25
BUNDLE_DIR=""
BUNDLE_SOURCE=""
SKIP_BUNDLE=0
SKIP_DRIFT=0
ALLOW_LEGALITY_BASE=0
WITH_METADATA=0
REMOTE="origin"
AUDIT_JSON=""

usage() { sed -n '2,/^set -Eeuo/p' "$0" | sed -e '$d' -e 's/^# \{0,1\}//'; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --execute) EXECUTE=1 ;;
    --yes) ASSUME_YES=1 ;;
    --group) GROUPS_REQ+=("${2:?--group needs a value}"); shift ;;
    --batch-size) BATCH_SIZE="${2:?}"; shift ;;
    --bundle-dir) BUNDLE_DIR="${2:?}"; shift ;;
    --bundle-source) BUNDLE_SOURCE="${2:?}"; shift ;;
    --skip-bundle) SKIP_BUNDLE=1 ;;
    --skip-drift-check) SKIP_DRIFT=1 ;;
    --allow-legality-base) ALLOW_LEGALITY_BASE=1 ;;
    --with-metadata) WITH_METADATA=1 ;;
    --audit-json) AUDIT_JSON="${2:?}"; shift ;;
    --remote) REMOTE="${2:?}"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1 (see --help)" >&2; exit 2 ;;
  esac
  shift
done

die() { echo "ABORT: $*" >&2; exit 1; }
note() { echo "[$(date -u +%H:%M:%S)] $*"; }

for tool in git curl python3; do
  command -v "$tool" >/dev/null 2>&1 || die "$tool is required"
done
if ! [[ "$BATCH_SIZE" =~ ^[0-9]+$ ]] || (( BATCH_SIZE < 1 || BATCH_SIZE > MAX_BATCH )); then
  die "--batch-size must be 1..$MAX_BATCH"
fi
if (( EXECUTE )); then
  (( SKIP_BUNDLE )) && die "--skip-bundle is only allowed in a dry run"
  (( SKIP_DRIFT )) && die "--skip-drift-check is only allowed in a dry run"
fi
(( ASSUME_YES && ! EXECUTE )) && die "--yes only makes sense with --execute"

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || die "run inside the PokeBank-NX clone"
cd "$REPO_ROOT"
AUDIT_JSON="${AUDIT_JSON:-docs/maintenance/branch_audit_2026-11-02.json}"
[[ -f "$AUDIT_JSON" ]] || die "audit JSON not found: $AUDIT_JSON"
[[ -f tools/maintenance/branch_audit.py ]] || die "tools/maintenance/branch_audit.py not found"
git remote get-url "$REMOTE" >/dev/null 2>&1 || die "no git remote named $REMOTE"

RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/pokebank-prune.${RUN_ID}.XXXXXX")"
LOG_MD="docs/maintenance/BRANCH_DELETION_LOG.md"
LOG_TSV="docs/maintenance/BRANCH_DELETION_LOG.tsv"
if (( EXECUTE )); then MODE="EXECUTE"; else MODE="DRY-RUN"; fi
PLAN_OUT="$WORK/plan-${RUN_ID}.tsv"    # every decision of this run (dry run and execute)
DONE_FILE="$WORK/done.tsv"             # actions actually performed on the remote this run
: > "$DONE_FILE"
BUNDLE_PATH=""

# ---------------------------------------------------------------- error handling / summary ---
summary() {
  local tags deletes
  tags=$(awk -F'\t' '$1=="TAG"' "$DONE_FILE" | wc -l)
  deletes=$(awk -F'\t' '$1=="DELETE"' "$DONE_FILE" | wc -l)
  echo
  echo "================ prune_branches.sh summary ($MODE, run $RUN_ID) ================"
  echo "remote changes made this run: $tags tag(s) pushed, $deletes branch(es) deleted"
  [[ -n "$BUNDLE_PATH" ]] && echo "backup bundle: $BUNDLE_PATH (keep it OFF the repository, permanently)"
  echo "decision log of this run: $PLAN_OUT"
  if (( EXECUTE )); then echo "deletion log: $REPO_ROOT/$LOG_MD and $REPO_ROOT/$LOG_TSV"; fi
  if (( deletes > 0 )); then
    echo
    echo "Recovery commands for branches deleted in this run (see docs/maintenance/RECOVERY.md):"
    awk -F'\t' -v r="$REMOTE" '$1=="DELETE"{printf "  git push %s %s:refs/heads/%s\n", r, $3, $2}' "$DONE_FILE"
  fi
}
on_err() {
  local rc=$? line=${1:-?}
  trap - ERR
  echo >&2
  echo "STOPPED SAFELY: command failed (exit $rc) at line $line; no further remote changes." >&2
  summary >&2
  exit "$rc"
}
trap 'on_err $LINENO' ERR
trap 'rm -rf "$WORK"/mirror.git "$WORK"/verify.git 2>/dev/null || true' EXIT

# ---------------------------------------------------------------- GitHub API helpers ---------
GH_OK=0
if command -v gh >/dev/null 2>&1 && gh auth status >/dev/null 2>&1; then GH_OK=1; fi

# GET a GitHub API path (relative to /repos/OWNER/REPO) into a file. Uses gh when logged in,
# otherwise curl (unauthenticated unless GITHUB_TOKEN is set; the token is passed via stdin config,
# never on the command line and never printed). Returns non-zero on any HTTP/API failure.
api_get() {
  local path="$1" out="$2" code
  if (( GH_OK )); then
    gh api -H "Accept: application/vnd.github+json" "repos/${OWNER_REPO}/${path}" > "$out" 2>"$WORK/api.err" || return 1
    return 0
  fi
  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    code=$(printf 'header = "Authorization: Bearer %s"\n' "$GITHUB_TOKEN" |
      curl -sS --config - -o "$out" -w '%{http_code}' -H "Accept: application/vnd.github+json" "${API}/${path}" 2>"$WORK/api.err") || return 1
  else
    code=$(curl -sS -o "$out" -w '%{http_code}' -H "Accept: application/vnd.github+json" "${API}/${path}" 2>"$WORK/api.err") || return 1
  fi
  [[ "$code" == "200" ]] || { echo "GitHub API HTTP $code for $path" > "$WORK/api.err"; return 1; }
}

# Write every open PR as "number<TAB>head<TAB>base" to $1 (all pages). Non-zero on failure.
fetch_open_prs() {
  local out="$1" page=1 n
  : > "$out"
  while :; do
    api_get "pulls?state=open&per_page=100&page=${page}" "$WORK/prs.page.json" || return 1
    n=$(python3 - "$WORK/prs.page.json" "$out" <<'PY'
import json, sys
data = json.load(open(sys.argv[1]))
if not isinstance(data, list):
    sys.exit(1)
with open(sys.argv[2], "a") as f:
    for p in data:
        f.write(f"{p['number']}\t{p['head']['ref']}\t{p['base']['ref']}\n")
print(len(data))
PY
) || return 1
    (( n < 100 )) && break
    page=$((page + 1))
  done
}

# Names (heads and bases) of open PRs, one per line, from a fetch_open_prs file.
pr_refs() { awk -F'\t' '{print $2; print $3}' "$1" | sort -u; }

# ---------------------------------------------------------------- remote state helpers -------
live_heads() {  # "sha<TAB>branch" for every remote branch
  git ls-remote --heads "$REMOTE" | sed 's#\trefs/heads/#\t#'
}
live_tip() {  # live tip SHA of one branch (empty if absent)
  git ls-remote "$REMOTE" "refs/heads/$1" | awk -v r="refs/heads/$1" '$2==r{print $1}'
}
remote_tag_commit() {  # peeled commit of a remote tag (empty if absent)
  local t="refs/tags/$1" out
  out=$(git ls-remote "$REMOTE" "$t" "$t^{}")
  awk -v t="$t^{}" '$2==t{print $1}' <<<"$out"
}

# ---------------------------------------------------------------- plan from the audit JSON ---
PLAN_ALL="$WORK/plan_all.tsv"     # group, branch, tip, recorded surviving ref, classification
KEEP_SET="$WORK/keep.txt"         # KEEP / KEEP-PAUSED names at audit time
EXCL_PR_HEADS="$WORK/excluded_pr_heads.txt"
python3 - "$AUDIT_JSON" "$PLAN_ALL" "$KEEP_SET" "$EXCL_PR_HEADS" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
special = {"integration/main-gen15-consolidation": "consolidation",
           "integration/legality-gen14-major-20261012": "legality-base"}
rows, keep = [], []
for b in d["branches"]:
    name, act, cls, st = b["branch"], b["action"], b["classification"], b["content_status"]
    surv = (b.get("contained_in") or "").split(" @ ")[0]
    if name in special:
        g = special[name]
    elif act in ("KEEP", "KEEP-PAUSED"):
        keep.append(name); continue
    elif act == "DELETE-SAFE":
        g = "contained" if st == "MERGED" else "equivalent-archive"
    elif act == "ARCHIVE-TAG-THEN-DELETE":
        g = "paused-contained" if (cls == "PAUSED" and st == "MERGED") else "unique-archive"
    else:
        sys.exit(f"unknown action {act} for {name}")
    rows.append((g, name, b["tip"], surv or "-", cls))
    if act in ("KEEP", "KEEP-PAUSED"):
        keep.append(name)
with open(sys.argv[2], "w") as f:
    for r in rows:
        f.write("\t".join(r) + "\n")
with open(sys.argv[3], "w") as f:
    f.write("\n".join(sorted(set(keep))) + "\n")
# Heads of PRs the owner reviews individually (#122, #163, #280, #227-#230, #309-#312).
excl = {122, 163, 280, 227, 228, 229, 230, 309, 310, 311, 312}
with open(sys.argv[4], "w") as f:
    for p in d["pull_requests"]:
        if p["number"] in excl:
            f.write(p["head"] + "\n")
PY
note "audit plan: $(wc -l < "$PLAN_ALL") candidate branches, $(wc -l < "$KEEP_SET") KEEP/KEEP-PAUSED"

# Resolve requested groups.
[[ ${#GROUPS_REQ[@]} -eq 0 ]] && GROUPS_REQ=(all)
SELECTED=()
for g in "${GROUPS_REQ[@]}"; do
  case "$g" in
    all) SELECTED+=(contained paused-contained equivalent-archive unique-archive) ;;
    delete-safe) SELECTED+=(contained equivalent-archive) ;;
    archive) SELECTED+=(paused-contained unique-archive) ;;
    contained|paused-contained|equivalent-archive|unique-archive|consolidation) SELECTED+=("$g") ;;
    legality-base)
      (( ALLOW_LEGALITY_BASE )) || die "group legality-base requires --allow-legality-base"
      SELECTED+=("$g") ;;
    *) die "unknown group: $g" ;;
  esac
done
# de-duplicate, keep the order (no-tag groups first, then tag groups, then the special ones)
ORDERED=()
for g in contained paused-contained equivalent-archive unique-archive consolidation legality-base; do
  for s in "${SELECTED[@]}"; do [[ "$s" == "$g" ]] && { ORDERED+=("$g"); break; }; done
done
note "mode: $MODE; groups: ${ORDERED[*]}; batch size: $BATCH_SIZE; gh: $([[ $GH_OK == 1 ]] && echo authenticated || echo 'not used')"

needs_tag() { [[ "$1" == "equivalent-archive" || "$1" == "unique-archive" ]]; }

# Protected = never touched by this script, whatever the plan says.
# $1 branch, $2 group being processed, $3 file of live open-PR refs
is_protected() {
  local b="$1" g="$2" prrefs="$3"
  case "$b" in
    main|development/*|fix/round11-gen15-repairs|fix/round12-gen15-repairs|claude/*|r13/*) return 0 ;;
    # claude/tender-gates-bkl5jq is covered by claude/* above.
    archive/qa-campaigns-2026-10-28|audit/gen12-independent-20261013) return 0 ;;
    feature/ds-melonds-link-emulator-20261101) return 0 ;;
    integration/main-gen15-consolidation) [[ "$g" == "consolidation" ]] || return 0 ;;
    integration/legality-gen14-major-20261012)
      [[ "$g" == "legality-base" && $ALLOW_LEGALITY_BASE == 1 ]] || return 0 ;;
  esac
  if [[ "$b" != "integration/main-gen15-consolidation" && "$b" != "integration/legality-gen14-major-20261012" ]]; then
    grep -Fxq -- "$b" "$KEEP_SET" && return 0
  fi
  grep -Fxq -- "$b" "$EXCL_PR_HEADS" && return 0
  grep -Fxq -- "$b" "$prrefs" && return 0
  return 1
}
protect_reason() {
  local b="$1" g="$2" prrefs="$3"
  if grep -Fxq -- "$b" "$prrefs"; then echo "head/base of an open PR (live)";
  elif grep -Fxq -- "$b" "$EXCL_PR_HEADS"; then echo "head of an owner-review PR";
  elif grep -Fxq -- "$b" "$KEEP_SET"; then echo "KEEP/KEEP-PAUSED in the audit";
  else echo "protected name"; fi
}

# ---------------------------------------------------------------- step 0: fetch ------------
note "step 0: git fetch $REMOTE (branches and tags; no prune)"
git fetch --quiet "$REMOTE"
git fetch --quiet "$REMOTE" 'refs/tags/*:refs/tags/*'

LIVE0="$WORK/live0.tsv"
live_heads > "$LIVE0"
note "remote branches now: $(wc -l < "$LIVE0")"

OPEN0="$WORK/open0.tsv"
fetch_open_prs "$OPEN0" || die "could not read open PRs from the GitHub API ($(cat "$WORK/api.err" 2>/dev/null))"
PRREFS0="$WORK/prrefs0.txt"
pr_refs "$OPEN0" > "$PRREFS0"
note "open PRs: $(wc -l < "$OPEN0") (heads+bases: $(wc -l < "$PRREFS0"))"

# Snapshot of everything that must survive the run: protected names, KEEP branches and open-PR
# heads/bases that exist right now.
MUST_SURVIVE="$WORK/must_survive.txt"
{
  awk -F'\t' '{print $2}' "$LIVE0" | while read -r b; do
    case "$b" in
      main|development/*|fix/round11-gen15-repairs|fix/round12-gen15-repairs|claude/*|r13/*) echo "$b" ;;
    esac
  done
  cat "$KEEP_SET" "$PRREFS0" "$EXCL_PR_HEADS"
} | sort -u | grep -Fxf <(awk -F'\t' '{print $2}' "$LIVE0") > "$MUST_SURVIVE" || true
# The two gated branches may legitimately go when their group is selected.
for special in integration/main-gen15-consolidation integration/legality-gen14-major-20261012; do
  for g in "${ORDERED[@]}"; do
    if [[ ( "$g" == consolidation && "$special" == integration/main-gen15-consolidation ) ||
          ( "$g" == legality-base && "$special" == integration/legality-gen14-major-20261012 ) ]]; then
      grep -Fxv -- "$special" "$MUST_SURVIVE" > "$MUST_SURVIVE.new" || true
      mv "$MUST_SURVIVE.new" "$MUST_SURVIVE"
    fi
  done
done
note "branches that must survive this run: $(wc -l < "$MUST_SURVIVE")"

# ---------------------------------------------------------------- step 1: backup bundle ----
make_bundle() {
  local src dir date manifest sha
  src="${BUNDLE_SOURCE:-$(git remote get-url "$REMOTE")}"
  dir="${BUNDLE_DIR:-$(dirname "$REPO_ROOT")}"
  mkdir -p "$dir"
  dir="$(cd "$dir" && pwd -P)"
  case "$dir/" in "$(cd "$REPO_ROOT" && pwd -P)/"*) die "--bundle-dir must be outside the repository" ;; esac
  date="$(date -u +%Y%m%d)"
  BUNDLE_PATH="$dir/pokebank-nx-full-${date}.bundle"
  [[ -e "$BUNDLE_PATH" ]] && BUNDLE_PATH="$dir/pokebank-nx-full-${RUN_ID}.bundle"
  note "step 1: mirror clone of the bundle source (credentials are not printed)"
  git clone --quiet --mirror "$src" "$WORK/mirror.git"
  note "step 1: git bundle create $BUNDLE_PATH --all"
  git -C "$WORK/mirror.git" bundle create "$BUNDLE_PATH" --all 2>"$WORK/bundle.err" \
    || { cat "$WORK/bundle.err" >&2; die "git bundle create failed"; }
  git bundle verify "$BUNDLE_PATH" >"$WORK/bundle.verify" 2>&1 \
    || { cat "$WORK/bundle.verify" >&2; die "git bundle verify failed"; }
  note "step 1: bundle verified; cloning it back to compare every ref"
  git clone --quiet --mirror "$BUNDLE_PATH" "$WORK/verify.git"
  git -C "$WORK/mirror.git" for-each-ref --format='%(objectname) %(refname)' | sort > "$WORK/refs.mirror"
  git -C "$WORK/verify.git" for-each-ref --format='%(objectname) %(refname)' | sort > "$WORK/refs.bundle"
  diff -q "$WORK/refs.mirror" "$WORK/refs.bundle" >/dev/null \
    || die "bundle refs differ from the mirror refs (see $WORK/refs.mirror vs refs.bundle)"
  # Every live remote branch tip must be present in the bundle as a commit.
  local missing=0 sha b
  while IFS=$'\t' read -r sha b; do
    git -C "$WORK/verify.git" cat-file -e "${sha}^{commit}" 2>/dev/null || { echo "  missing from bundle: $b $sha" >&2; missing=$((missing+1)); }
  done < "$LIVE0"
  (( missing == 0 )) || die "$missing live branch tip(s) are not in the bundle"
  sha="$(sha256sum "$BUNDLE_PATH" | cut -d' ' -f1)"
  manifest="${BUNDLE_PATH%.bundle}.manifest.tsv"
  {
    printf '# PokeBank-NX preservation manifest, run %s\n' "$RUN_ID"
    printf '# bundle\t%s\n# sha256\t%s\n# refs in bundle\t%s\n' "$(basename "$BUNDLE_PATH")" "$sha" "$(wc -l < "$WORK/refs.bundle")"
    printf '# branch\ttip\tgroup\tpreserved-by\n'
    awk -F'\t' 'NR==FNR{g[$2]=$1; s[$2]=$4; next}
      { grp = ($2 in g) ? g[$2] : "retained";
        pres = "bundle";
        if (grp=="contained"||grp=="paused-contained"||grp=="consolidation"||grp=="legality-base") pres="bundle; contained in " s[$2];
        else if (grp=="equivalent-archive"||grp=="unique-archive") pres="bundle; tag archive/branches/" $2;
        else pres="bundle; branch retained";
        printf "%s\t%s\t%s\t%s\n", $2, $1, grp, pres }' "$PLAN_ALL" "$LIVE0"
  } > "$manifest"
  printf '%s  %s\n' "$sha" "$(basename "$BUNDLE_PATH")" > "$BUNDLE_PATH.sha256"
  note "step 1: OK - $(wc -l < "$WORK/refs.bundle") refs, sha256 $sha"
  note "  bundle:   $BUNDLE_PATH"
  note "  manifest: $manifest"
  note "  KEEP THIS BUNDLE OUTSIDE THE REPOSITORY, PERMANENTLY (restore: docs/maintenance/RECOVERY.md)."
}
if (( SKIP_BUNDLE )); then
  note "step 1: SKIPPED (--skip-bundle, dry run only)"
else
  make_bundle
fi

# ---------------------------------------------------------------- step 2: drift check -------
TARGETS="$WORK/targets.tsv"
: > "$TARGETS"
for g in "${ORDERED[@]}"; do awk -F'\t' -v g="$g" '$1==g' "$PLAN_ALL" >> "$TARGETS"; done

already_deleted() {  # branch deleted by an earlier run of this script (from the TSV log)?
  [[ -f "$LOG_TSV" ]] && awk -F'\t' -v b="$1" '$4==b && $9=="DELETE"{f=1} END{exit !f}' "$LOG_TSV"
}

if (( SKIP_DRIFT )); then
  note "step 2: SKIPPED (--skip-drift-check, dry run only)"
else
  note "step 2: re-running branch_audit.py --compare (read-only; this takes a few minutes)"
  rc=0   # exit 1 just means "some drift"; the lines are judged below (|| keeps the ERR trap quiet)
  python3 tools/maintenance/branch_audit.py --repo . --remote "$REMOTE" --fetch-prs \
    --out "$WORK/branch_audit_live.json" --compare "$AUDIT_JSON" > "$WORK/drift.txt" 2>&1 || rc=$?
  grep -q '^drift items:' "$WORK/drift.txt" || { cat "$WORK/drift.txt" >&2; die "branch_audit.py failed (exit $rc)"; }
  bad=0
  while read -r line; do
    b=""; kind=""
    case "$line" in
      "DRIFT gone: "*) b="${line#DRIFT gone: }"; kind=gone ;;
      "DRIFT new: "*) continue ;;   # new branches are never targets
      "DRIFT "*": tip "*) b="${line#DRIFT }"; b="${b%%: tip *}"; kind=tip ;;
      "DRIFT "*": PR set/state changed") b="${line#DRIFT }"; b="${b%%: PR set*}"; kind="pr" ;;
      "DRIFT "*": action "*) b="${line#DRIFT }"; b="${b%%: action *}"; kind=action ;;
      "DRIFT "*": classification "*) b="${line#DRIFT }"; b="${b%%: classification *}"; kind=class ;;
      *) continue ;;
    esac
    awk -F'\t' -v b="$b" '$2==b{f=1} END{exit !f}' "$TARGETS" || continue
    case "$kind" in
      gone) if already_deleted "$b"; then continue; fi
            echo "  drift: targeted branch $b vanished outside this script" >&2; bad=$((bad+1)) ;;
      tip) echo "  drift: $line" >&2; bad=$((bad+1)) ;;
      pr)
        # The two gated branches are expected to change PRs (e.g. #314 retarget); their own live
        # gates (PR #330 merged / no open PR uses the branch) decide. Everything else stops.
        if [[ "$b" == integration/main-gen15-consolidation || "$b" == integration/legality-gen14-major-20261012 ]]; then
          echo "  note (live gate decides): $line"
        else echo "  drift: $line" >&2; bad=$((bad+1)); fi ;;
      action)
        # Tags created by an earlier run can move ARCHIVE <-> DELETE-SAFE; the recorded (stricter)
        # plan still applies. Anything that now says KEEP must stop.
        if [[ "$line" == *"-> KEEP"* ]]; then echo "  drift: $line" >&2; bad=$((bad+1));
        else echo "  note (tolerated, recorded plan applies): $line"; fi ;;
      class) echo "  note (tolerated, recorded plan applies): $line" ;;
    esac
  done < "$WORK/drift.txt"
  (( bad == 0 )) || die "$bad drift item(s) on targeted branches; regenerate and re-approve the audit"
  note "step 2: no blocking drift ($(grep '^drift items:' "$WORK/drift.txt"))"
fi

# ---------------------------------------------------------------- confirmation --------------
if (( EXECUTE && ! ASSUME_YES )); then
  echo
  echo "About to tag/delete up to $(wc -l < "$TARGETS") branches on $REMOTE ($OWNER_REPO), groups: ${ORDERED[*]}."
  read -r -p "Type 'prune $OWNER_REPO' to continue: " answer
  [[ "$answer" == "prune $OWNER_REPO" ]] || die "not confirmed"
fi

if (( EXECUTE )); then
  mkdir -p "$(dirname "$LOG_MD")"
  if [[ ! -f "$LOG_TSV" ]]; then
    printf 'utc_time\trun\tbatch\tbranch\ttip\tgroup\tpreserved_by\ttag\taction\n' > "$LOG_TSV"
  fi
  if [[ ! -f "$LOG_MD" ]]; then
    printf '# Branch deletion log\n\nAppended by tools/maintenance/prune_branches.sh.\n\n| UTC time | Run | Batch | Branch | Tip | Group | Preserved by | Tag | Action |\n|---|---|---|---|---|---|---|---|---|\n' > "$LOG_MD"
  fi
fi
log_action() {  # batch branch tip group preserved tag action
  local t; t="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'PLAN\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$t" "$1" "$2" "$3" "$4" "$5" "$6" "$7" >> "$PLAN_OUT"
  if (( EXECUTE )) && [[ "$7" == DELETE || "$7" == TAG ]]; then
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$t" "$RUN_ID" "$1" "$2" "$3" "$4" "$5" "$6" "$7" >> "$LOG_TSV"
    printf '| %s | %s | %s | `%s` | `%s` | %s | %s | %s | %s |\n' "$t" "$RUN_ID" "$1" "$2" "$3" "$4" "$5" "${6:--}" "$7" >> "$LOG_MD"
  fi
}

# ---------------------------------------------------------------- containment ---------------
# Echo "ref@sha" of a preserved ref containing $tip, or nothing. Preserved = recorded surviving
# ref if it is protected/retained, then main, then any other branch that must survive, then tags.
find_container() {
  local tip="$1" recorded="$2" r sha
  local cands=()
  [[ "$recorded" != "-" ]] && cands+=("$recorded")
  cands+=(main)
  while read -r r; do cands+=("$r"); done < "$MUST_SURVIVE"
  for r in "${cands[@]}"; do
    grep -Fxq -- "$r" "$MUST_SURVIVE" || [[ "$r" == main ]] || continue
    sha="$(awk -F'\t' -v b="$r" '$2==b{print $1}' "$LIVE_BATCH")"
    [[ -n "$sha" ]] || continue
    git cat-file -e "${sha}^{commit}" 2>/dev/null || continue   # local copy must be current
    if git merge-base --is-ancestor "$tip" "$sha" 2>/dev/null; then echo "$r@$sha"; return 0; fi
  done
  return 1
}

# ---------------------------------------------------------------- tagging -------------------
ensure_tag() {  # branch tip -> 0 when archive/branches/<branch> is verified on the remote at tip
  local b="$1" tip="$2" tag="archive/branches/$1" local_c remote_c
  remote_c="$(remote_tag_commit "$tag")"
  if [[ -n "$remote_c" ]]; then
    [[ "$remote_c" == "$tip" ]] && return 0
    echo "  remote tag $tag points to $remote_c, not $tip" >&2; return 1
  fi
  if git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
    local_c="$(git rev-parse "refs/tags/$tag^{commit}")"
    [[ "$local_c" == "$tip" ]] || { echo "  local tag $tag points to $local_c, not $tip" >&2; return 1; }
  else
    git tag -a "$tag" "$tip" -m "Archive of branch $b at $tip (PokeBank-NX cleanup Phase 2, run $RUN_ID).
Restore: git push $REMOTE refs/tags/$tag^{commit}:refs/heads/$b"
  fi
  git push --quiet "$REMOTE" "refs/tags/$tag" || return 1
  remote_c="$(remote_tag_commit "$tag")"
  [[ "$remote_c" == "$tip" ]] || { echo "  pushed tag $tag verifies as '$remote_c', expected $tip" >&2; return 1; }
  printf 'TAG\t%s\t%s\t%s\n' "$b" "$tip" "$tag" >> "$DONE_FILE"
  return 0
}

# ---------------------------------------------------------------- per-branch processing -----
BATCH_ABORT=""
COUNT_DELETED=0; COUNT_WOULD=0; COUNT_SKIP=0; COUNT_TAGGED=0; COUNT_WOULD_TAG=0
declare -A GROUP_OK GROUP_SKIP

process_branch() {  # batch group branch tip recorded_surv
  local batch="$1" g="$2" b="$3" tip="$4" surv="$5" live cont tag="" pres prrefs
  prrefs="$PRREFS_BATCH"
  skip() { log_action "$batch" "$b" "$tip" "$g" "-" "-" "SKIP: $1"; echo "  SKIP  $b: $1"; COUNT_SKIP=$((COUNT_SKIP+1)); GROUP_SKIP[$g]=$(( ${GROUP_SKIP[$g]:-0} + 1 )); }

  if is_protected "$b" "$g" "$prrefs"; then skip "$(protect_reason "$b" "$g" "$prrefs")"; return 0; fi

  if (( EXECUTE )); then
    live="$(live_tip "$b")"
  else
    live="$(awk -F'\t' -v b="$b" '$2==b{print $1}' "$LIVE_BATCH")"
  fi
  if [[ -z "$live" ]]; then
    if already_deleted "$b"; then skip "already deleted by an earlier run"; else skip "not on the remote any more"; fi
    return 0
  fi
  [[ "$live" == "$tip" ]] || { skip "tip changed: recorded ${tip:0:12}, live ${live:0:12}"; return 0; }

  if (( EXECUTE )); then
    # Re-read open PRs immediately before this deletion; any API failure aborts the batch.
    fetch_open_prs "$WORK/open_now.tsv" || { BATCH_ABORT="open-PR re-check failed ($(cat "$WORK/api.err" 2>/dev/null))"; return 0; }
    pr_refs "$WORK/open_now.tsv" > "$WORK/prrefs_now.txt"
    prrefs="$WORK/prrefs_now.txt"
    if is_protected "$b" "$g" "$prrefs"; then skip "$(protect_reason "$b" "$g" "$prrefs")"; return 0; fi
  fi

  case "$g" in
    consolidation)
      api_get "pulls/330" "$WORK/pr330.json" || { BATCH_ABORT="cannot read PR #330"; return 0; }
      python3 -c 'import json,sys; sys.exit(0 if json.load(open(sys.argv[1])).get("merged") else 1)' "$WORK/pr330.json" \
        || { skip "PR #330 is not merged"; return 0; }
      ;;
    legality-base)
      if grep -Fxq -- "$b" "$prrefs"; then skip "still used by an open PR"; return 0; fi
      ;;
  esac

  if needs_tag "$g"; then
    tag="archive/branches/$b"
    if (( EXECUTE )); then
      ensure_tag "$b" "$tip" || { BATCH_ABORT="tag $tag could not be verified on the remote"; return 0; }
      COUNT_TAGGED=$((COUNT_TAGGED+1))
      log_action "$batch" "$b" "$tip" "$g" "tag $tag" "$tag" "TAG"
    else
      local rc_tag; rc_tag="$(remote_tag_commit "$tag")"
      if [[ -n "$rc_tag" && "$rc_tag" != "$tip" ]]; then skip "remote tag $tag exists at a different commit"; return 0; fi
      COUNT_WOULD_TAG=$((COUNT_WOULD_TAG+1))
      log_action "$batch" "$b" "$tip" "$g" "tag $tag" "$tag" "WOULD-TAG"
    fi
    pres="tag $tag"
  else
    cont="$(find_container "$tip" "$surv")" || { skip "containment not verified live (recorded: $surv)"; return 0; }
    pres="contained in $cont"
  fi

  if (( EXECUTE )); then
    [[ "$(live_tip "$b")" == "$tip" ]] || { skip "tip changed just before deletion"; return 0; }
    git push --quiet "--force-with-lease=refs/heads/$b:$tip" "$REMOTE" --delete "$b" \
      || { BATCH_ABORT="delete of $b was refused"; return 0; }
    [[ -z "$(live_tip "$b")" ]] || { BATCH_ABORT="$b still exists after delete"; return 0; }
    printf 'DELETE\t%s\t%s\t%s\n' "$b" "$tip" "$pres" >> "$DONE_FILE"
    log_action "$batch" "$b" "$tip" "$g" "$pres" "$tag" "DELETE"
    echo "  DELETED $b ${tip:0:12} ($pres)"
    COUNT_DELETED=$((COUNT_DELETED+1))
  else
    log_action "$batch" "$b" "$tip" "$g" "$pres" "$tag" "WOULD-DELETE"
    echo "  would delete $b ${tip:0:12} ($pres)"
    COUNT_WOULD=$((COUNT_WOULD+1))
  fi
  GROUP_OK[$g]=$(( ${GROUP_OK[$g]:-0} + 1 ))
  return 0
}

post_batch_check() {
  local live missing=0 b
  live="$WORK/live_after.tsv"
  live_heads > "$live"
  while read -r b; do
    awk -F'\t' -v b="$b" '$2==b{f=1} END{exit !f}' "$live" || { echo "  MISSING protected/open-PR branch: $b" >&2; missing=$((missing+1)); }
  done < "$MUST_SURVIVE"
  if fetch_open_prs "$WORK/open_after.tsv"; then
    while read -r b; do
      awk -F'\t' -v b="$b" '$2==b{f=1} END{exit !f}' "$live" || { echo "  MISSING open-PR head/base: $b" >&2; missing=$((missing+1)); }
    done < <(pr_refs "$WORK/open_after.tsv")
  else
    echo "  could not re-read open PRs after the batch" >&2; missing=$((missing+1))
  fi
  (( missing == 0 )) || die "post-batch verification failed ($missing problem(s)); stop and review"
  note "post-batch check OK; remote branches now: $(wc -l < "$live")"
}

# ---------------------------------------------------------------- step 3: batches ----------
note "step 3: processing $(wc -l < "$TARGETS") targeted branches in batches of <= $BATCH_SIZE ($MODE)"
batch=0
mapfile -t TARGET_LINES < "$TARGETS"
for g in "${ORDERED[@]}"; do
  mapfile -t GLINES < <(printf '%s\n' "${TARGET_LINES[@]}" | awk -F'\t' -v g="$g" '$1==g')
  [[ ${#GLINES[@]} -eq 0 || -z "${GLINES[0]}" ]] && { note "group $g: no branches"; continue; }
  note "group $g: ${#GLINES[@]} branch(es)"
  i=0
  while (( i < ${#GLINES[@]} )); do
    batch=$((batch+1))
    # Re-fetch and re-read live state before each batch.
    git fetch --quiet "$REMOTE"
    LIVE_BATCH="$WORK/live_batch.tsv"
    live_heads > "$LIVE_BATCH"
    PRREFS_BATCH="$WORK/prrefs_batch.txt"
    fetch_open_prs "$WORK/open_batch.tsv" || die "batch $batch: open-PR read failed ($(cat "$WORK/api.err" 2>/dev/null))"
    pr_refs "$WORK/open_batch.tsv" > "$PRREFS_BATCH"
    # Abort the batch on drift: every branch of the batch still on the remote must have its tip.
    drift=0
    for line in "${GLINES[@]:i:BATCH_SIZE}"; do
      IFS=$'\t' read -r _ b tip _ _ <<<"$line"
      live="$(awk -F'\t' -v b="$b" '$2==b{print $1}' "$LIVE_BATCH")"
      if [[ -n "$live" && "$live" != "$tip" ]]; then echo "  batch $batch drift: $b ${tip:0:12} -> ${live:0:12}" >&2; drift=$((drift+1)); fi
    done
    if (( drift > 0 && ! EXECUTE )); then
      echo "  (dry run: continuing so the full listing is visible; --execute would abort this batch)" >&2
    elif (( drift > 0 )); then
      die "batch $batch: $drift branch tip(s) moved since the audit; regenerate and re-approve the audit"
    fi
    note "batch $batch ($g): branches $((i+1))..$(( i + BATCH_SIZE < ${#GLINES[@]} ? i + BATCH_SIZE : ${#GLINES[@]} ))"
    for line in "${GLINES[@]:i:BATCH_SIZE}"; do
      IFS=$'\t' read -r grp b tip surv _ <<<"$line"
      BATCH_ABORT=""
      process_branch "$batch" "$grp" "$b" "$tip" "$surv"
      [[ -z "$BATCH_ABORT" ]] || die "batch $batch aborted at $b: $BATCH_ABORT"
    done
    if (( EXECUTE )); then post_batch_check; fi
    i=$((i + BATCH_SIZE))
  done
done

# ---------------------------------------------------------------- metadata (optional) -------
apply_metadata() {
  local meta="docs/maintenance/REPO_METADATA.md" plan="docs/maintenance/MILESTONE_PLAN.md"
  if (( ! GH_OK )); then note "metadata: skipped (gh is not authenticated)"; return 0; fi
  note "metadata: milestones"
  gh api "repos/${OWNER_REPO}/milestones?state=all&per_page=100" --jq '.[].title' > "$WORK/milestones.txt"
  local m
  for m in "Gen I–V hardware acceptance" "v1.0 Vault" "Gen VI–IX parity" "Legality integration"; do
    if grep -Fxq -- "$m" "$WORK/milestones.txt"; then echo "  exists: $m"
    elif (( EXECUTE )); then gh api -X POST "repos/${OWNER_REPO}/milestones" -f title="$m" >/dev/null; echo "  created: $m"
    else echo "  would create: $m"; fi
  done
  if [[ -f "$meta" ]]; then
    local desc topics
    desc="$(sed -n 's/^Description: //p' "$meta" | head -1)"
    topics="$(sed -n 's/^Topics: //p' "$meta" | head -1 | tr -d ' ')"
    if [[ -n "$desc" ]]; then
      if (( EXECUTE )); then gh repo edit "$OWNER_REPO" --description "$desc" >/dev/null; echo "  description set"
      else echo "  would set description: $desc"; fi
    fi
    if [[ -n "$topics" ]]; then
      if (( EXECUTE )); then gh repo edit "$OWNER_REPO" --add-topic "$topics" >/dev/null; echo "  topics added: $topics"
      else echo "  would add topics: $topics"; fi
    fi
  else
    echo "  $meta not found; description/topics unchanged"
  fi
  if [[ -f "$plan" ]]; then
    # Table rows "| #123 | Milestone title | ..." (first two columns). Unknown milestones are skipped.
    gh api "repos/${OWNER_REPO}/milestones?state=all&per_page=100" --jq '.[] | "\(.number)\t\(.title)"' > "$WORK/ms.tsv"
    local num title msnum
    while IFS=$'\t' read -r num title; do
      msnum="$(awk -F'\t' -v t="$title" '$2==t{print $1}' "$WORK/ms.tsv")"
      if [[ -z "$msnum" ]]; then echo "  skip #$num: milestone '$title' not found"; continue; fi
      if (( EXECUTE )); then gh api -X PATCH "repos/${OWNER_REPO}/issues/$num" -F milestone="$msnum" >/dev/null; echo "  #$num -> $title"
      else echo "  would assign #$num -> $title"; fi
    done < <(python3 - "$plan" <<'PY'
import re, sys
for line in open(sys.argv[1], encoding="utf-8"):
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    if len(cells) >= 2:
        m = re.fullmatch(r"#?(\d+)", cells[0])
        if m and cells[1]:
            print(f"{m.group(1)}\t{cells[1]}")
PY
)
  else
    echo "  $plan not found; no milestone assignments"
  fi
}
if (( WITH_METADATA )); then apply_metadata; fi

# ---------------------------------------------------------------- report --------------------
echo
echo "Per-group result ($MODE):"
for g in "${ORDERED[@]}"; do
  printf '  %-20s %s %4d   skipped %4d\n' "$g" "$( (( EXECUTE )) && echo deleted || echo 'would delete')" "${GROUP_OK[$g]:-0}" "${GROUP_SKIP[$g]:-0}"
done
if (( EXECUTE )); then
  echo "  tags pushed+verified: $COUNT_TAGGED; deleted: $COUNT_DELETED; skipped: $COUNT_SKIP"
else
  echo "  tags that would be created: $COUNT_WOULD_TAG; would delete: $COUNT_WOULD; skipped: $COUNT_SKIP"
fi
cp "$PLAN_OUT" "${TMPDIR:-/tmp}/prune-plan-${RUN_ID}.tsv"
PLAN_OUT="${TMPDIR:-/tmp}/prune-plan-${RUN_ID}.tsv"
summary
