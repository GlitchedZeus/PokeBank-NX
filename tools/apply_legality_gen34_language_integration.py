#!/usr/bin/env python3
"""Guarded one-shot migration for Gen III/IV language-domain legality.

Pinned reference: PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
(LanguageVerifier + Legal.GetMaxLanguageID).

This script is intentionally exact-string guarded and idempotent. It only wires the
already-reviewed Gen34LanguageEvidence helper into Legality::analyze(); it never
changes save/editor/write behavior.
"""

from pathlib import Path

PATH = Path("src/Legality/Legality.cpp")

OLD_INCLUDE = '#include "Legality/Gen34EggMoveEvidence.h"\n'
NEW_INCLUDE = (
    '#include "Legality/Gen34EggMoveEvidence.h"\n'
    '#include "Legality/Gen34LanguageEvidence.h"\n'
)

OLD_BLOCK = '''        // ---- L1: ball / language ranges (skip when unwired == 0) ----
        if (pk.ball() != 0 && pk.ball() > 37)
            add(r, Severity::Invalid, "Ball id out of range (" + std::to_string(pk.ball()) + ")");
        if (pk.language() != 0 && (pk.language() > 10 || pk.language() == 6))
            add(r, Severity::Invalid, "Invalid language id (" + std::to_string(pk.language()) + ")");
'''

NEW_BLOCK = '''        // ---- L1: ball / language ranges (skip when unwired == 0) ----
        if (pk.ball() != 0 && pk.ball() > 37)
            add(r, Severity::Invalid, "Ball id out of range (" + std::to_string(pk.ball()) + ")");

        const uint8_t language = pk.language();
        if (sourceProfile && (exactGeneration == 3 || exactGeneration == 4)) {
            if (Gen34Language::isInvalid(exactGeneration, language)) {
                add(r, Severity::Invalid,
                    "Language id " + std::to_string(language) +
                    " cannot exist in Generation " + std::to_string(exactGeneration),
                    CheckIdentifier::Trainer);
            }
        } else if (language != 0 && (language > 10 || language == 6)) {
            // Preserve the historical generic-format fallback where exact source identity
            // is unavailable. Zero remains the project's unwired/unknown sentinel.
            add(r, Severity::Invalid,
                "Invalid language id (" + std::to_string(language) + ")");
        }
'''


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    if new in text:
        return text, False
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one old pattern, found {count}")
    return text.replace(old, new, 1), True


def main() -> None:
    text = PATH.read_text(encoding="utf-8")
    changed = False

    text, did = replace_once(text, OLD_INCLUDE, NEW_INCLUDE, "language include")
    changed |= did
    text, did = replace_once(text, OLD_BLOCK, NEW_BLOCK, "language analyzer block")
    changed |= did

    if changed:
        PATH.write_text(text, encoding="utf-8")
        print("Applied guarded Gen III/IV language-domain analyzer integration.")
    else:
        print("Gen III/IV language-domain analyzer integration already present.")


if __name__ == "__main__":
    main()
