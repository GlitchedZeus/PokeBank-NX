# Extraction schema (PokeBank NX historical QA reconciliation)

Write ONE JSON file (UTF-8, a JSON array) at the path your brief names. One object per record:

{
  "src": "repo-relative path (or GitHub ref like 'issue #307 comment 2026-10-17 by GlitchedZeus')",
  "round": "R1..R12 | PRE (Sept/early-Oct device tests before Round 1) | OCT24 | OCT25 | OCT26 | (other label as used by the doc)",
  "candidate": "8-char code SHA the result is for, or null",
  "date": "YYYY-MM-DD if stated, else null",
  "item_id": "the ID exactly as the document uses it (e.g. 'H07', 'R2-BUG-004', 'C14', 'AUD-09', 'Extra 6'), or null",
  "title": "short title as written",
  "gen": ["I","II","III","IV","V","all","app"],   // which generations it concerns
  "games": "games named, if any (e.g. 'FireRed, Crystal')",
  "area": "one of: install-home, save-discovery, open-save, pokemon-view, pokemon-edit, create, species-change, moves, encounters-met, stats, clone, move-carry, release, actions-menu, import-export-file, bag-items, pc-items, bulk-topup, trainer, review-keep-discard, export-copy, backups-recovery, bank-storage, vault, transfer, search, language-names, visuals-sprites, help-footer-controls, touch, launch-quit, settings, legality, other",
  "kind": "checklist-item | owner-fail | owner-not-tested | owner-pass | owner-correction | owner-request | owner-decision | deferred | engineering-finding | requirement",
  "result": "PASS | FAIL | NOT TESTED | NO FIXTURE | PARTIAL | n/a",
  "owner_words": "VERBATIM owner wording (comment/result text) exactly as written, or null if none",
  "doc_note": "short paraphrase of what the doc says (expected behaviour, steps, engineering note)",
  "later_refs": "any fix commit / PR / later round the SAME doc mentions for it, else null"
}

Rules:
- Extract EVERY checklist item (with its result if the doc has one), EVERY FAIL / NOT TESTED / PARTIAL,
  every owner correction, additional request, decision and every deferred / follow-up item. Do not
  summarize multiple items into one. Do not invent results: a template or unfilled checklist item is
  result "n/a" with kind "checklist-item".
- owner_words must be copied character-for-character (no fixes to spelling). If long, keep the whole
  sentence(s) relevant to the item.
- Read the files fully (use offset/limit for large ones). Do not modify the repository.
