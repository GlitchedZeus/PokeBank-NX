#!/usr/bin/env python3
"""Round 11 H27: Hangul must render, not "[][][][][]".

PKSEFramebuffer registers the console's shared fonts through UI::SystemFontFallbacks (host-tested
by tests/test_system_font_fallbacks.cpp): Standard (Japanese) and then Korean (PlSharedFontType_KO),
both as fallbacks of the primary Nunito face, after the two romfs symbol fonts, with system-owned
memory (freeData = 0). The Gen V name display sites use the shared display decoder, while the edit
and replay paths keep the ASCII write gate.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def ordered(text: str, *parts: str) -> bool:
    pos = -1
    for part in parts:
        nxt = text.find(part, pos + 1)
        if nxt < 0:
            return False
        pos = nxt
    return True


def main() -> None:
    fb = read("src/UI/PKSEFramebuffer.cpp")
    header = read("include/UI/PKSEFramebuffer.h")
    fallbacks = read("include/UI/SystemFontFallbacks.h")

    assert '#include "UI/SystemFontFallbacks.h"' in fb
    assert "PlSharedFontType_KO" in fb, "the Korean shared font is never requested"
    assert "PlSharedFontType_Standard" in fb
    assert "SystemFontFallbacks::registerAll(" in fb
    # Symbol fonts first, then the console fonts, all on fontSans.
    assert ordered(fb, "nvgAddFallbackFontId(vg, fontSans, fontSym)",
                   "nvgAddFallbackFontId(vg, fontSans, fontSym2)",
                   "SystemFontFallbacks::registerAll(")
    assert "nvgAddFallbackFontId(vg, fontSans, id)" in fb
    # System-owned font memory: never freed by fontstash.
    assert "static_cast<int>(data.size), 0)" in fb
    assert "int fontKorean = -1;" in header
    assert ordered(fallbacks, "{SharedFont::Standard, \"jp\"}", "{SharedFont::Korean, \"ko\"}")

    # Gen V display sites use the display decoder; the edit / replay gates keep asciiName5.
    surface = read("src/UI/Gen5SharedReadOnlySurface.inc")
    assert surface.count("displayName5(") >= 6
    assert "Native glyphs" not in surface and "(native glyphs)" not in surface
    chrome = surface[surface.index("static EditorSurface::ChromeInfo chrome("):]
    chrome = chrome[:chrome.index("info.level")]
    assert "displayName5(p.decodedBytes(), true)" in chrome and "asciiName5(" not in chrome
    edit = surface[surface.index("case Nickname: case OriginalTrainer: {\n                const bool nickname"):]
    assert "asciiName5(p.decodedBytes(), nickname)" in edit[:1200]
    assert "displayName5" not in edit[:1200]
    for rel in ("include/Integration/Gen5/Gen5SharedPokemonSession.h",
                "include/Integration/Gen5/Gen5SharedReview.h",
                "include/Integration/Gen5/Gen5SharedScreenState.h"):
        text = read(rel)
        assert "asciiName5(" in text and "displayName5(" not in text, rel
    trainer = read("include/Integration/Gen5/Gen5TrainerName.h")
    assert "return displayGen5Text(raw,7);" in trainer
    native = read("include/Integration/Gen5/Gen5NativeText.h")
    assert "return displayGen5Text(raw,nameMaxLength5(nickname));" in native
    print("Korean shared font fallback and Gen V display decoder contract: PASS")


if __name__ == "__main__":
    main()
