# Historical QA reconciliation data (Rounds 1–12)

The source data behind `docs/qa/MASTER_HARDWARE_ACCEPTANCE_LEDGER.md`,
`HISTORICAL_ROUND_1_12_AUDIT.md`, `V1_RELEASE_FEATURE_GAPS.md` and `PERMANENT_REGRESSION_MATRIX.md`
(Nov 1, owner request "Mandatory Historical QA Reconciliation").

- `all_records.json`: 2,754 records extracted from every hardware document and GitHub thread
  (schema: `SCHEMA.md`); owner wording verbatim.
- `trace_G1`…`trace_G7_*.json`: the 276 unique issues (`PBX-<AREA>-NNN`), traced and classified per
  `TRACE_BRIEF.md`; `trace_G8_checklist_lineage.json`: checklist items followed across rounds.
- `counts.json`: the totals the documents report.

Regenerate the four documents (never hand-edit them): `python3 tools/qa/gen_historical_qa_docs.py .`
To change a status, edit the issue in its trace file with evidence, then regenerate. Never delete an
issue or an occurrence; a resolved issue changes status.
