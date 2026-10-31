# PokeBank NX project board

One GitHub Projects board, **PokeBank NX**, tracks the open issues and pull requests. Its definition is version-controlled:

- `docs/project-board/items.tsv`: every tracked issue / PR with its **Stage** and **Resolution**;
- `docs/project-board/setup_project_board.sh`: creates or updates the board from that file (owner runs it once with `gh auth refresh -s project`). It is additive: it never closes, deletes or edits an issue, PR or branch.

## Stage (the board columns)

| Stage | Meaning |
|---|---|
| **Backlog** | Planned, deferred or waiting on another line (v1.0 trackers, legacy lanes, legality lineage) |
| **To Do** | Agreed next work that has not started (for example the Gen VI+ retargets once `development/gen6-plus` exists) |
| **In Progress** | Actively developed or under owner hardware test (#313, #314, #327, #328, #329) |
| **Review** | Done in code and waiting for the owner: hardware verification on the current candidate, or the consolidation merge |
| **Done** | Closed after the owner's verification or acceptance |

## Resolution (keeps completed work apart from the rest)

| Resolution | Meaning |
|---|---|
| **Active** | Still being worked on or tracked |
| **Completed** | The work is done (fixed / implemented); for hardware items, pending verification until Done |
| **Superseded** | Contained in a successor line (`main` or #314); closed only after that successor is accepted |
| **Deferred** | Unique work kept for later review; never deleted without an archive ref and owner approval |

A hardware issue moves to **Done** only after the owner verifies it on the current candidate; results from older rounds alone keep it in **Review**. See `docs/MAIN_CONSOLIDATION_20261031.md` for the evidence behind each classification.
