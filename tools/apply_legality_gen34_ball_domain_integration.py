#!/usr/bin/env python3
from pathlib import Path

path = Path("src/Legality/Legality.cpp")
text = path.read_text()

include_anchor = '#include "Legality/Gen34LanguageEvidence.h"\n'
include_insert = include_anchor + '#include "Legality/Gen34BallDomainEvidence.h"\n'
if '#include "Legality/Gen34BallDomainEvidence.h"' not in text:
    count = text.count(include_anchor)
    if count != 1:
        raise RuntimeError(f"{path}: expected one language include anchor, found {count}")
    text = text.replace(include_anchor, include_insert, 1)

old = '''        // ---- L1: ball / language ranges (skip when unwired == 0) ----
        if (pk.ball() != 0 && pk.ball() > 37)
            add(r, Severity::Invalid, "Ball id out of range (" + std::to_string(pk.ball()) + ")");

        const uint8_t language = pk.language();
'''
new = '''        // ---- L1: ball / language ranges (skip when unwired == 0) ----
        const uint8_t ball = pk.ball();
        if (sourceProfile && (exactGeneration == 3 || exactGeneration == 4)) {
            if (Gen34BallDomain::isInvalid(exactGeneration, ball)) {
                add(r, Severity::Invalid,
                    "Ball id " + std::to_string(ball) +
                    " cannot exist in Generation " + std::to_string(exactGeneration),
                    CheckIdentifier::Items);
            }
        } else if (ball != 0 && ball > 37) {
            // Preserve the historical generic-format fallback where exact source identity
            // is unavailable. Zero remains the project's unwired/unknown sentinel.
            add(r, Severity::Invalid,
                "Ball id out of range (" + std::to_string(ball) + ")");
        }

        const uint8_t language = pk.language();
'''
if old in text:
    text = text.replace(old, new, 1)
elif new not in text:
    raise RuntimeError(f"{path}: guarded ball-language anchor not found")

path.write_text(text)
