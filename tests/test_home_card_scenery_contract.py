#!/usr/bin/env python3
"""Round 11 (HOME card): the region scenery at the top of the HOME selected-game card is masked by
the card's rounded corners (UI/CardScenery.h), never drawn through a rectangular clip that lets the
artwork protrude past the radius-18 corners and the focus ring."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
src = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")

start = src.index("SystemIcons::regionBackdrop(regionKey)")
block = src[start:start + 2500]
end = block.find("withAlpha(Colors::Divider")
assert end > 0, "region scenery block not found"
block = block[:end]

assert "CardScenery::drawTopBand(" in block, "region scenery must use the rounded band helper"
for forbidden in ("setClipRect", "drawImageScaled", "drawImage(", "drawFilledRect("):
    assert forbidden not in block, f"region scenery must not use {forbidden} (rectangular corners)"
# The focus ring is redrawn above the scenery, with the card's own radius.
assert re.search(r"if \(gameFocused\)\s*fb\.drawRoundedRect\(DETAIL_X, HUB_Y, DETAIL_W, HUB_H, CardScenery::kHomeCardRadius", block)
# One radius for card, ring and scenery.
assert "drawFocusedCard(fb, DETAIL_X, HUB_Y, DETAIL_W, HUB_H, gameFocused, CardScenery::kHomeCardRadius)" in src

helper = (ROOT / "include/UI/CardScenery.h").read_text(encoding="utf-8")
assert "drawImageRoundedCover" in helper and "drawFilledRoundedRectVarying" in helper
assert "setClipRect" not in helper

fb = (ROOT / "src/UI/PKSEFramebuffer.cpp").read_text(encoding="utf-8")
body = fb[fb.index("void PKSEFramebuffer::drawImageRoundedCover"):]
body = body[:body.index("\n    }\n")]
# Native single-path mask: one rounded path filled with an image pattern; no strips, no scissor.
assert body.count("nvgRoundedRectVarying") == 1 and "nvgImagePattern" in body
assert "nvgScissor" not in body and "for (" not in body

print("PASS test_home_card_scenery_contract")
