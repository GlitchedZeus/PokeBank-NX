# Milestone plan

No milestones exist yet, and development sessions cannot create them (see
[`OWNER_CONFIGURATION_CHECKLIST.md`](OWNER_CONFIGURATION_CHECKLIST.md) §3). After the owner creates the four
milestones, assign issues and open PRs as follows. `tools/maintenance/prune_branches.sh --with-metadata` reads
this table.

| Milestone | Issues | Open PRs |
| --- | --- | --- |
| Gen I–V hardware acceptance | #313, #320, #322, #323, #324, #325, #326, #26, #15, #31 | #332, #333, #280, #122, #163 |
| v1.0 Vault | #29, #16, #55, #56, #57, #58, #59, #70, #89, #49, #21, #9, #17 | none (no Vault implementation PR exists yet) |
| Gen VI–IX parity | #30 | #309, #311, #312, #321, #331 |
| Legality integration | #329, #5, #50 | #314, #327, #328, #301, #273, #252, #230, #144 |

#122 is touchscreen parity work and #163 is Gen V foundation work (read-only PK5). Neither implements the Vault.
Both are under review for supersession by `main` before any closure.

Closed PRs (#103, #227–#229) are not listed; their work lives in #314 and #230.
