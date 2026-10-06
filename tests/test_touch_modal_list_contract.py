#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing touch contract source: {rel}")
    return path.read_text(encoding="utf-8")

def require(text: str, rel: str, *needles: str) -> None:
    for needle in needles:
        if needle not in text:
            raise AssertionError(f"{rel}: missing touch contract token: {needle}")

touch_h = read("include/UI/TouchInput.h")
touch_cpp = read("src/UI/TouchInput.cpp")
require(touch_h, "include/UI/TouchInput.h",
        "bool justTouchedDown() const",
        "bool justReleased() const",
        "bool justPressed() const { return justReleased() && !dragged(); }",
        "bool justTapped() const { return justPressed(); }",
        "int startX() const", "int startY() const")
require(touch_cpp, "src/UI/TouchInput.cpp",
        "maxDistanceSquared = std::max(maxDistanceSquared",
        "constexpr int kTapSlop = 22",
        "return maxDistanceSquared > (kTapSlop * kTapSlop);")

base = read("src/UI/TrainerViewScreenBase.inc")
require(base, "src/UI/TrainerViewScreenBase.inc",
        "int TrainerViewScreen::touchedButtonDownId",
        "if (!touch.justTouchedDown()) return -1;",
        "int TrainerViewScreen::touchedButtonId",
        "if (!touch.justTapped()) return -1;",
        "if (details.legalityOverlay)",
        "if (details.ribbonOverlay)",
        "touch.justReleased() && touch.dragged()",
        "details.legalityScroll",
        "details.ribbonScroll")
release_hit = base[base.index("int TrainerViewScreen::touchedButtonId"):
                   base.index("int TrainerViewScreen::touchedButtonId") + 700]
assert "touch.x()" in release_hit and "touch.y()" in release_hit
assert "startX()" not in release_hit and "startY()" not in release_hit, \
    "release activation must use release coordinates so release-outside cancels"

details = read("src/UI/Modals/PokemonDetailsModal.cpp")
require(details, "src/UI/Modals/PokemonDetailsModal.cpp",
        "Legality report: clipped touch-scroll surface with explicit close.",
        "screen.details.legalityScroll = std::clamp",
        "screen.details.ribbonScroll = std::clamp",
        "fb.setClipRect",
        "drawScrollbar",
        "screen.touchButtons.push_back({96, closeX, closeY, closeW, closeH});")

backup = read("src/UI/BackupSelectionScreen.cpp")
require(backup, "src/UI/BackupSelectionScreen.cpp",
        "touch.justReleased() && touch.dragged()",
        "touch.justReleased() && !touch.dragged()",
        "kSwipeDistance")

inventory = read("src/UI/ClassicInventoryOverlay.cpp")
require(inventory, "src/UI/ClassicInventoryOverlay.cpp",
        "bool handleInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch)",
        "touch.justTouchedDown()",
        "touch.justReleased() && touch.dragged()",
        "screen.touchButtons.push_back")

save_confirm = read("src/UI/Dialogs/SaveConfirmDialog.cpp")
require(save_confirm, "src/UI/Dialogs/SaveConfirmDialog.cpp",
        "drawEditChoiceButton(screen, fb",
        "\"B\", \"Cancel\", 90",
        "\"A\", \"Save\", 91")

packed = read("src/UI/ClassicPackedMoveOverlay.inc")
require(packed, "src/UI/ClassicPackedMoveOverlay.inc",
        "bool touchArmed = false;",
        "bool touchSelecting = false;",
        "cellAtPoint",
        "touch.isDown() && touch.dragged()",
        "state.touchArmed && touch.justReleased()")

# Modal glyph buttons (Back/Discard/Save, Release/Cancel, transfer confirms, etc.) are not
# decoration. drawGlyphButton must publish its exact rectangle immediately so a button drawn after
# the footer is still directly tappable on the next input frame through navTouchButton().
chrome = read("include/UI/ScreenChrome.h")
require(chrome, "include/UI/ScreenChrome.h",
        "const TouchGlyphHit hit{bx, by, bw, bh, glyph};",
        "g_touchGlyphAccum.push_back(hit);",
        "g_touchGlyphHits.push_back(hit);",
        "inline uint64_t navTouchButton(const TouchInput& touch)")

print("touch modal/list contract: PASS")
