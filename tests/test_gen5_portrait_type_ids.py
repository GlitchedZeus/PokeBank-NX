#!/usr/bin/env python3
"""Round 16 guard (owner: Gen V Ninjask shown as Ghost/Flying).

The shared Gen IV/V editor portrait draws SpriteManager::getTypeSprite(), which takes CANONICAL
type ids (Bug 6, Ghost 7). Gen V's portraitTypes() must pass canonical ids (personalTypes5), never
the classic-id conversion that only the species picker preview (ClassicTypeBadges) understands.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
gen5 = (ROOT / "src/UI/Gen5SharedReadOnlySurface.inc").read_text(encoding="utf-8")
shared = (ROOT / "src/UI/SharedPokemonEditorSurface.inc").read_text(encoding="utf-8")
failures = []
m = re.search(r"static std::array<uint8_t,2> portraitTypes\([^)]*\)\s*\{(.*?)\n    \}", gen5, re.S)
if not m:
    failures.append("Gen V portraitTypes not found")
else:
    body = m.group(1)
    if "personalTypes5" not in body or "classicTypes5" in body or "canonicalToClassicType" in body:
        failures.append("Gen V portraitTypes must return canonical personalTypes5 ids: " + body.strip())
if "A::portraitTypes(screen, *raw)" not in shared or "SpriteManager::getTypeSprite(types[i])" not in shared:
    failures.append("shared portrait no longer draws canonical ids via getTypeSprite; re-check every portraitTypes")
if failures:
    print("\n".join(failures))
    sys.exit(1)
print("Gen V portrait type ids are canonical (Ninjask Bug/Flying): PASS")
