# QA regression corpus (synthetic only)

`corpus/<fuzz target>/` holds minimised inputs that once tripped a fuzz target (a product defect or a
harness gap). Both runners replay them on every run (`make qa-fuzz-smoke`, `make qa-libfuzzer-run`).
Every file here is synthetic: no real save, ROM or key is ever committed.

| File | Origin |
| --- | --- |
| `corpus/fuzz_pk_records/pk4-*-all-zero-blank.bin` | libFuzzer soak 2026-10-14: all-zero PK4 records are the documented PKHeX/PKSM blank-slot convention (valid + empty, never encrypted), so they are exempt from the re-encryption round trip. Harness gap, not a product defect. |
