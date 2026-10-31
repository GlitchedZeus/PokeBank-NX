#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FONTSTASH = (ROOT / "nanovg" / "fontstash.h").read_text(encoding="utf-8")
NANOVG = (ROOT / "nanovg" / "nanovg.c").read_text(encoding="utf-8")


def ordered(text: str, *parts: str) -> bool:
    pos = -1
    for part in parts:
        nxt = text.find(part, pos + 1)
        if nxt < 0:
            return False
        pos = nxt
    return True


def function_body(text: str, signature: str) -> str:
    # Headers may contain a prototype before the implementation; use the last
    # matching signature so ordering checks inspect the actual function body.
    start = text.rindex(signature)
    brace = text.index("{", start)
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:i]
    raise AssertionError(f"unterminated function: {signature}")


def main() -> None:
    # Glyph-array realloc must not overwrite live ownership or advance capacity on failure.
    assert "FONSglyph* newGlyphs = (FONSglyph*)realloc" in FONTSTASH
    assert ordered(
        FONTSTASH,
        "FONSglyph* newGlyphs = (FONSglyph*)realloc",
        "if (newGlyphs == NULL) return NULL;",
        "font->glyphs = newGlyphs;",
        "font->cglyphs = newCapacity;",
    )

    # fons__getGlyph must stop before dereferencing a failed glyph allocation.
    assert ordered(
        FONTSTASH,
        "glyph = fons__allocGlyph(font);",
        "if (glyph == NULL) return NULL;",
        "glyph->codepoint = codepoint;",
    )

    # Atlas CPU allocation must succeed before renderer/atlas state is mutated.
    reset = function_body(FONTSTASH, "int fonsResetAtlas(FONScontext* stash, int width, int height)")
    assert ordered(
        reset,
        "newTexData = (unsigned char*)realloc",
        "if (newTexData == NULL) return 0;",
        "fons__flush(stash);",
        "stash->params.renderResize",
        "stash->texData = newTexData;",
        "fons__atlasReset(stash->atlas, width, height);",
    )

    # NanoVG must propagate reset failure instead of claiming a new atlas is usable.
    alloc = function_body(NANOVG, "static int nvg__allocTextAtlas(NVGcontext* ctx)")
    assert "if (!fonsResetAtlas(ctx->fs, iw, ih))" in alloc
    assert ordered(
        alloc,
        "++ctx->fontImageIdx;",
        "if (!fonsResetAtlas(ctx->fs, iw, ih))",
        "--ctx->fontImageIdx;",
        "return 0;",
    )

    print("Fontstash allocation-failure contract: PASS")


if __name__ == "__main__":
    main()
