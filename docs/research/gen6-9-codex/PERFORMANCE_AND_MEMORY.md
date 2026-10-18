# Bounded parsing and Switch performance preparation

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Source-backed constraints

Switch runtime source pinned at libnx `20cbcfd90567f7bf84df7725f27e345e2164fa6b` (`nx/source/runtime/init.c::__libnx_initheap`, `nx/include/switch/runtime/env.h`). Heap may come from loader override, explicit __nx_heap_size, or svcGetInfo-derived availability before svcSetHeapSize. Therefore there is no justified universal free-memory number for this app, applet mode or return-host chain. Measure the actual launch mode.

File sizes come from SaveUtil in GAME_FORMAT_MATRIX and generated metadata. SV is several MiB including DLC; use the exact allowed predicate, not an arbitrary huge reader cap. Native source readers should stat and size-filter before allocating. Hash first; parse only within authenticated byte bounds (cryptographic hash here is integrity, not trusted origin).

## Current allocation shape: ORIGINAL + INFERRED

Gen8 decrypt copies encrypted payload then copies each block's decoded data into vectors, tracking keys in a std::set. Gen8Save retains original bytes and decoded blocks, so peak includes source, payload and decoded data plus metadata/quarantined parse temporaries. Export adds candidate and independent verifier state. This is an allocation inventory, not a measured peak multiplier or an assurance of fit on Switch. Gen6/7 source vectors and copy-based CRC/signature clearing also create temporary full saves. `Gen7Save::validBlockCount` clears on a private copy; repeated probes plus RSA leak are important.

## Proposed benchmark protocol

Measure source-read, hash, block decode, first-party render, first-box render, catalog query, stage, export-build and independent reparse separately. Record wall-time, heap baseline/peak, allocated bytes/block count, title/revision/file size, cold/warm state, launch mode, exact NRO and source/data SHAs. Repeat corrupt declared lengths, duplicate keys, many tiny blocks, all boxes occupied and corrupt slots. Keep frame-time percentiles for 1000+ species/item lists and controller/touch scrolling. On-device measurements remain pending.

Host smoke benchmark `evidence/host-benchmark.json` executes existing SWSH suite and Gen6 stats suite. Its `child_max_rss_kib_cumulative` is cumulative high-water across child runs, **not per-parser peak**; timing is whole suite, not single-save latency. It is reproducible baseline evidence only. No Switch responsiveness claim is made.

## Implementation guidance: INFERRED

- Cap number of SC blocks by both remaining serialized bytes (minimum five-byte block) and a reviewed schema/resource limit. Check count×stride using division before multiplication; avoid reserve from an unchecked length.
- Keep one immutable source and sparse staged modifications; avoid cloning full decoded storage for each UI keystroke. Materialize export candidates only on Review/Export, and release temporaries promptly.
- Index species/forms by exact game/revision once. Cache catalog search keys and use virtualized visible rows. Lazy-load sprites with bounded LRU; do not preload all forms/art.
- Decode box summaries lazily after whole-container integrity and required geometry checks where compatible with quarantine semantics. Cache by immutable source fingerprint. Do not rerun signature verification each frame or on every scroll.
- Keep unknown blocks byte-identical and preserve order; serialization can reuse untouched ciphertext where schema permits. Benchmark before optimizing crypto loops.
- Directory scan work must be bounded by known provider roots and depth. ZIP/decompression is unsupported; if later added, cap decompressed size/entries/ratio and disallow traversal before enabling it.

Acceptance is measured on the actual Switch NRO in its supported loader mode, with long-list touch/controller parity and malformed-save refusal. Host tests cannot substitute.
