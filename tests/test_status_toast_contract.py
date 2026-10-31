#!/usr/bin/env python3
"""B08 (round 3 owner FAIL, "Export does nothing across generations").

The Gen II-V shared surfaces own their frames and return before the base
draw/update, which held the only postStatus painter and countdown. Export
results (success path or refusal) were posted but never painted. These
contracts keep one painter that every draw path runs last, and one countdown
per frame on every update path.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def body_of(text: str, signature: str) -> str:
    start = text.index(signature)
    depth = 0
    for i in range(text.index("{", start), len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    raise AssertionError(f"unterminated body: {signature}")


composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
base = read("src/UI/TrainerViewScreenBase.inc")
header = read("include/UI/TrainerViewScreenBase.h")

draw = body_of(composite, "void TrainerViewScreen::draw(PKSEFramebuffer& fb) {")
guard = draw.index("~StatusOnTop() { screen.drawTransientStatus(fb); }")
first_surface = draw.index("Gen5SharedReadOnlySurface::draw(*this, fb)")
assert guard < first_surface, "the status painter guard must exist before any surface can return"
for surface in ("Gen5SharedReadOnlySurface::draw", "Gen4SharedEditorSurface::draw",
                "Gen3SharedEditorSurface::draw", "Gen2PokemonEditor::drawFinalGen2Surface"):
    assert surface in draw, surface

update = body_of(composite, "void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {")
expiry = update.index("~StatusExpiry() { if (frames > 0 && frames == atStart) --frames; }")
assert expiry < update.index("Gen4SharedEditorSurface::handleInput"), \
    "the per-frame countdown guard must be armed before a surface can return"

painter = body_of(base, "void TrainerViewScreen::drawTransientStatus(PKSEFramebuffer& fb) {")
assert "storageStatusFrames <= 0 || storageStatus.empty()" in painter
assert "fb.drawText(bx + padX, by + 7, storageStatus, Colors::Text);" in painter
assert "void drawTransientStatus(PKSEFramebuffer& fb);" in header
# Only one painter: no second copy of the toast drawing code.
assert len(re.findall(r"fb\.drawText\([^;]*storageStatus,", base)) == 1

# Each generation's export reports through postStatus (now always visible).
for rel, needle in (
        ("src/UI/Gen4SharedPokemonSurface.inc", 'screen.postStatus("Verified copy exported: "'),
        ("src/UI/Gen3SharedPokemonSurface.inc", "screen.postStatus("),
        ("src/UI/Gen5SharedReadOnlySurface.inc", 'screen.postStatus("Gen V export blocked: "')):
    assert needle in read(rel), (rel, needle)

print("B08 status toast contracts: PASS")
