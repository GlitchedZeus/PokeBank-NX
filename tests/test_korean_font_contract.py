#!/usr/bin/env python3
"""Round 11 H27: Hangul must render, not "[][][][][]".

PKSEFramebuffer registers the console's shared fonts through UI::SystemFontFallbacks (host-tested
by tests/test_system_font_fallbacks.cpp): Standard (Japanese) and then Korean (PlSharedFontType_KO),
both as fallbacks of the primary Nunito face, after the two romfs symbol fonts, with system-owned
memory (freeData = 0). The Gen V name display sites use the shared display decoder, while the edit
and replay paths keep the ASCII write gate.

Keyboards (Round 15, owner: "Implement shared Japanese/Korean/Latin input with native encoding
validation"): a Korean-language Gen IV or Gen V Pokemon's Nickname / OT prompts open the Hangul
page through the ONE name prompt (Utils::promptName with the record's field, whose keyboard()
profile starts on Hangul for Korean); the writer still validates through the Gen 4 table / the
Gen V rule (Text::Gen5NameField). Gen III has no Korean, and Japanese Gen I / II editing stays
behind JapaneseEditingGate.h.
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
    # Round 15: the edit seed is the writer's own read (editableName5), never the display decoder.
    assert "editableName5(p.decodedBytes(), nickname," in edit[:1400]
    assert "displayName5" not in edit[:1400]
    for rel in ("include/Integration/Gen5/Gen5SharedPokemonSession.h",
                "include/Integration/Gen5/Gen5SharedReview.h",
                "include/Integration/Gen5/Gen5SharedScreenState.h"):
        text = read(rel)
        assert ("asciiName5(" in text or "editableName5(" in text) and "displayName5(" not in text, rel
    trainer = read("include/Integration/Gen5/Gen5TrainerName.h")
    assert "return displayGen5Text(raw,7);" in trainer
    native = read("include/Integration/Gen5/Gen5NativeText.h")
    assert "return displayGen5Text(raw,nameMaxLength5(nickname));" in native
    # Hangul keyboard wiring.
    gen4 = read("src/UI/Gen4SharedPokemonSurface.inc")
    assert "PokeBank::Text::Format::Gen4, kind, mon.language(), false" in gen4
    assert ordered(gen4, "Utils::promptName(*field, \"Nickname\"", "mon.setNickname(Utils::utf8ToUtf16(result.text))")
    assert ordered(gen4, "Utils::promptName(*field, \"Original Trainer\"",
                   "mon.setOriginalTrainerName(Utils::utf8ToUtf16(r.text))")
    gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc")
    assert "p.language(), false});" in gen5 and "Utils::promptName(*field, nickname ? \"Nickname\"" in gen5
    for rel in ("src/UI/Gen5SharedReadOnlySurface.inc", "src/UI/Gen3SharedPokemonSurface.inc",
                "src/UI/Gen4SharedPokemonSurface.inc"):
        assert "promptKoreanName" not in read(rel), rel
    capability = read("include/Text/TextFieldCapability.h")
    assert "if (language_ == kLanguageKorean) return koreanProfile();" in capability
    keyboard = read("src/Utils/Keyboard.cpp")
    body = keyboard[keyboard.index("KeyboardResult promptKoreanName("):]
    body = body[:body.index("NumberResult promptNumber(")]
    assert "request.koreanHangul = true;" in body and "promptSystemText(" in body
    dialog = read("src/UI/Dialogs/KeyboardDialog.cpp")
    assert ": profile.start == Start::Hangul ? KeyboardPage::Hangul" in dialog
    assert "profile = T::koreanProfile();" in dialog
    assert "H::compose(" in dialog
    # Mojibake guard: the round-trip check in Pokemon4Mutable stays the storage rule. Round 14 P0-2:
    # it now compares against the REQUESTED text; the old copy was cut to the cap first ("bounded"),
    # so an over-long name passed the check as its own prefix.
    mutable = read("src/Pokemon/Pokemon4Mutable.cpp")
    assert "if (Utils::decodeGen4Field(encoded) != value) return false;" in mutable
    assert "value.substr(0, std::min(value.size(), maxCharacters))" not in mutable
    print("Korean shared font fallback and Gen V display decoder contract: PASS")


if __name__ == "__main__":
    main()
