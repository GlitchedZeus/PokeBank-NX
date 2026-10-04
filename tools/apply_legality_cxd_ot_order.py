#!/usr/bin/env python3
"""Move the CXD OT-gender invariant ahead of all Gen III PID early returns.

Pinned PKHeX CXDVerifier applies the OT-gender rule to every GameCube-origin PK3.
This guarded migration is idempotent and fails closed if the known source shape changes.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "Legality" / "Legality.cpp"

BLOCK = '''            if (pk.originGame() == 15 && pk.otGender() != 0) {\n                add(r, Severity::Invalid,\n                    "Pokemon Colosseum/XD-origin PK3 cannot have a female OT gender",\n                    CheckIdentifier::Trainer);\n            }\n\n'''

ANCHOR = '''            const std::array<uint8_t, 6> ivs{\n                pk.ivHP(), pk.ivATK(), pk.ivDEF(), pk.ivSPE(), pk.ivSPA(), pk.ivSPD()\n            };\n'''


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    if BLOCK not in text:
        raise SystemExit("expected CXD OT-gender block not found")
    if ANCHOR not in text:
        raise SystemExit("Gen III PID evidence anchor not found")

    block_at = text.index(BLOCK)
    anchor_at = text.index(ANCHOR)
    desired_at = anchor_at + len(ANCHOR)

    # Already ordered correctly: the invariant immediately follows IV extraction,
    # before handheld Method 1/2/3/4 and every other early-return branch.
    if block_at == desired_at:
        return 0

    # Guard against accidentally moving an unrelated duplicate.
    if text.count(BLOCK) != 1:
        raise SystemExit("unexpected duplicate CXD OT-gender blocks")
    if block_at < desired_at:
        raise SystemExit("CXD OT-gender block is already earlier than expected anchor")

    text = text[:block_at] + text[block_at + len(BLOCK):]
    anchor_at = text.index(ANCHOR)
    desired_at = anchor_at + len(ANCHOR)
    text = text[:desired_at] + BLOCK + text[desired_at:]
    TARGET.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
