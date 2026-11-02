# Branch deletion log

`tools/qa/maintenance/prune_branches.sh --execute` appends one row to this table for every archive tag it pushes and every branch it deletes. It writes the same rows to `BRANCH_DELETION_LOG.tsv`. Commit both files after each owner run. [RECOVERY.md](RECOVERY.md) explains how to restore any branch listed here.

No branch has been deleted yet. The table below is empty until the first owner run.

| UTC time | Run | Batch | Branch | Tip | Group | Preserved by | Tag | Action |
|---|---|---|---|---|---|---|---|---|
