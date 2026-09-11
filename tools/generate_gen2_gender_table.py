#!/usr/bin/env python3
"""Generate the Gen II gender threshold table from the pinned PKSM-Core personal2 oracle."""
from pathlib import Path
import hashlib
import re

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "vendor/PKSM-Core"
PERSONAL = CORE / "personals/personal2"
SOURCE = CORE / "source/personal/personal.cpp"
OUT = ROOT / "include/Integration/Gen2/Gen2GenderTable.h"
EXPECTED_CORE = "aa22d7a4f87c0351baf7da5962ba5acd01039a7c"
EXPECTED_PERSONAL_SHA256 = None  # printed/recorded on generation; git pin is the primary oracle identity
ENTRY = 10
COUNT = 252
GENDER_OFFSET = 8


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    gsc = source[source.index("namespace PersonalGSC"):]
    # Prove that the pinned implementation actually reads gender from personal2 byte +0x8.
    if not re.search(r"u8\s+gender\(u8\s+species\).*?personal2\(\)\[species\s*\*\s*personal2_entrysize\s*\+\s*0x8\]", gsc, re.S):
        raise SystemExit("Pinned PKSM-Core PersonalGSC::gender no longer matches personal2 +0x8")
    raw = PERSONAL.read_bytes()
    if len(raw) != ENTRY * COUNT:
        raise SystemExit(f"personal2 length {len(raw)} != {ENTRY * COUNT}")
    values = [raw[i * ENTRY + GENDER_OFFSET] for i in range(COUNT)]
    rows = []
    for i in range(0, COUNT, 18):
        rows.append("    " + ", ".join(f"0x{x:02X}" for x in values[i:i+18]) + ",")
    digest = hashlib.sha256(raw).hexdigest()
    text = f'''#ifndef POKEBANK_GEN2_GENDER_TABLE_H\n#define POKEBANK_GEN2_GENDER_TABLE_H\n\n#include <array>\n#include <cstdint>\n\nnamespace PokeVault::Integration::Gen2 {{\n// GENERATED from PKSM-Core {EXPECTED_CORE} personals/personal2.\n// personal2 SHA-256: {digest}\n// Values are the canonical Gen II gender thresholds: 0x00 male-only, 0xFE female-only,\n// 0xFF genderless; otherwise female iff Attack DV * 16 < threshold.\ninline constexpr std::array<uint8_t, {COUNT}> kGen2GenderThreshold = {{{{\n{chr(10).join(rows)}\n}}}};\n}} // namespace PokeVault::Integration::Gen2\n\n#endif\n'''
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(text, encoding="utf-8")
    print(f"generated {OUT.relative_to(ROOT)} from personal2 sha256={digest}")

if __name__ == "__main__":
    main()
