#!/usr/bin/env bash
# Create (or update) the "PokeBank NX" GitHub Projects board from tools/github/project_board_items.tsv.
#
# Additive only: it creates the project and its two single-select fields if they are missing, adds
# each listed issue / PR and sets its Stage and Resolution. It never closes, deletes or edits an issue,
# PR or branch. Re-running it is safe: existing items are updated in place.
#
# Needs the GitHub CLI logged in as the repository owner with the project scope:
#   gh auth refresh -s project
#   tools/github/setup_project_board.sh            # owner GlitchedZeus, repo PokeBank-NX
set -euo pipefail

OWNER="${OWNER:-GlitchedZeus}"
REPO="${REPO:-PokeBank-NX}"
TITLE="${TITLE:-PokeBank NX}"
ITEMS="$(dirname "$0")/project_board_items.tsv"
STAGES="Backlog,To Do,In Progress,Review,Done"
RESOLUTIONS="Active,Completed,Superseded,Deferred"

command -v gh >/dev/null || { echo "gh (GitHub CLI) is required" >&2; exit 1; }
command -v jq >/dev/null || { echo "jq is required" >&2; exit 1; }

number="$(gh project list --owner "$OWNER" --format json --limit 100 \
  | jq -r --arg t "$TITLE" '.projects[] | select(.title == $t) | .number' | head -1)"
if [ -z "$number" ]; then
  number="$(gh project create --owner "$OWNER" --title "$TITLE" --format json | jq -r .number)"
  echo "created project #$number"
fi
project_id="$(gh project view "$number" --owner "$OWNER" --format json | jq -r .id)"
gh project link "$number" --owner "$OWNER" --repo "$OWNER/$REPO" >/dev/null 2>&1 || true

ensure_field() {  # name options
  if ! gh project field-list "$number" --owner "$OWNER" --format json \
      | jq -e --arg n "$1" '.fields[] | select(.name == $n)' >/dev/null; then
    gh project field-create "$number" --owner "$OWNER" --name "$1" \
      --data-type SINGLE_SELECT --single-select-options "$2" >/dev/null
    echo "created field $1"
  fi
}
ensure_field "Stage" "$STAGES"
ensure_field "Resolution" "$RESOLUTIONS"

fields="$(gh project field-list "$number" --owner "$OWNER" --format json)"
field_id() { jq -r --arg n "$1" '.fields[] | select(.name == $n) | .id' <<<"$fields"; }
option_id() { jq -r --arg n "$1" --arg o "$2" '.fields[] | select(.name == $n) | .options[] | select(.name == $o) | .id' <<<"$fields"; }
stage_field="$(field_id Stage)"
resolution_field="$(field_id Resolution)"

while IFS=$'\t' read -r kind num stage resolution note; do
  [[ -z "$kind" || "$kind" == \#* ]] && continue
  case "$kind" in
    issue) url="https://github.com/$OWNER/$REPO/issues/$num" ;;
    pr)    url="https://github.com/$OWNER/$REPO/pull/$num" ;;
    *) echo "skipping unknown kind $kind" >&2; continue ;;
  esac
  item="$(gh project item-add "$number" --owner "$OWNER" --url "$url" --format json | jq -r .id)"
  gh project item-edit --id "$item" --project-id "$project_id" --field-id "$stage_field" \
    --single-select-option-id "$(option_id Stage "$stage")" >/dev/null
  gh project item-edit --id "$item" --project-id "$project_id" --field-id "$resolution_field" \
    --single-select-option-id "$(option_id Resolution "$resolution")" >/dev/null
  printf '%-6s #%-4s %-12s %-11s %s\n' "$kind" "$num" "$stage" "$resolution" "$note"
done < "$ITEMS"

echo "Board: https://github.com/users/$OWNER/projects/$number (group the table or board view by Stage)"
