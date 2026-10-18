"""Game Boy text encoding for the differential corpora, read from the pinned PKHeX source.

The character tables come straight out of PKHeX.Core's StringConverter1.cs (Gen I) and
StringConverter2.cs (Gen II), so the corpus encodes names with the same table PKHeX
decodes them with, independently of PokeBank NX's own decoders.
"""

import re

TERMINATOR = 0x50
TRADE_OT = 0x5D


def read_table(pkhex, generation, name):
    """The 256-entry table `name` (TableEN / TableJP) as a list; None marks a byte with
    no plain character (terminator, control codes and PKHeX's named placeholders)."""
    src = (pkhex / f"PKHeX.Core/PKM/Strings/StringConverter{generation}.cs").read_text(encoding="utf-8")
    start = src.index(f"ReadOnlySpan<char> {name} =>")
    body = src[src.index("[", start) + 1:src.index("];", start)]
    body = re.sub(r"//[^\n]*", "", body)
    table = []
    for quoted, named in re.findall(r"'(\\'|[^'])'|\b([A-Z][A-Z0-9]{2})\b", body):
        table.append(("'" if quoted == "\\'" else quoted) if quoted else None)
    if len(table) != 256:
        raise SystemExit(f"StringConverter{generation}.{name}: expected 256 entries, found {len(table)}")
    return table


def read_nul_mask(pkhex, generation, name):
    """For the 256-entry table `name`: True where PKHeX's entry is NUL (no character at
    all, where its decoders stop), False for any character or named placeholder."""
    src = (pkhex / f"PKHeX.Core/PKM/Strings/StringConverter{generation}.cs").read_text(encoding="utf-8")
    start = src.index(f"ReadOnlySpan<char> {name} =>")
    body = src[src.index("[", start) + 1:src.index("];", start)]
    body = re.sub(r"//[^\n]*", "", body)
    mask = [named == "NUL" for quoted, named in re.findall(r"'(\\'|[^'])'|\b([A-Z][A-Z0-9]{2})\b", body)]
    if len(mask) != 256:
        raise SystemExit(f"StringConverter{generation}.{name}: expected 256 entries, found {len(mask)}")
    return mask


class Encoder:
    def __init__(self, table):
        self.index = {}
        for i, c in enumerate(table):
            if c is not None and c not in self.index:
                self.index[c] = i

    def encode(self, text, length, terminate=True):
        """`text` in a `length`-byte field, padded with terminators. With terminate=False
        the text may fill the whole field (used for over-long names)."""
        out = [self.index[c] for c in text]
        room = length if not terminate else length - 1
        if len(out) > room:
            raise ValueError(f"{text!r} does not fit a {length}-byte field")
        return bytes(out + [TERMINATOR] * (length - len(out)))


def japanese_names(encoder):
    """A few ordinary katakana/hiragana names, checked against the table."""
    names = ["サトシ", "シゲル", "カスミ", "タケシ", "ゴールド", "あかね", "ミナキ"]
    for n in names:
        for c in n:
            if c not in encoder.index:
                raise SystemExit(f"{c!r} is not in the Japanese table")
    return names


def read_korean_tables(pkhex):
    """Korean Gold/Silver text from PKHeX.Core's StringConverter2KOR.cs: {page: table} where
    page 0 is the one-byte table and pages 1-11 are the second byte of a two-byte character
    whose first byte is the page number. None marks a byte with no character."""
    src = (pkhex / "PKHeX.Core/PKM/Strings/StringConverter2KOR.cs").read_text(encoding="utf-8")
    tables = {}
    for page, name in enumerate(["Table0"] + [f"Table{c}" for c in "123456789AB"]):
        start = src.index(f"ReadOnlySpan<char> {name} =>")
        body = src[src.index("[", start) + 1:src.index("];", start)]
        body = re.sub(r"//[^\n]*", "", body)
        table = [quoted or None for quoted, _named in re.findall(r"'([^'])'|\b(NULL|NUL|RET)\b", body)]
        if len(table) > 256:
            raise SystemExit(f"StringConverter2KOR.{name}: {len(table)} entries")
        tables[page] = table
    return tables


class KoreanEncoder:
    """StringConverter2KOR.SetString: Hangul as (page, index) pairs, the rest one byte."""

    def __init__(self, tables):
        self.index = {}
        for page in range(1, 12):
            for i, c in enumerate(tables[page]):
                if c is not None and c not in self.index:
                    self.index[c] = bytes((page, i))
        for i, c in enumerate(tables[0]):
            if c is not None and c not in self.index:
                self.index[c] = bytes((i,))

    def hangul(self, text):
        return all(c in self.index and len(self.index[c]) == 2 for c in text)

    def encode(self, text, length, terminate=True):
        out = b"".join(self.index[c] for c in text)
        if len(out) > (length - 1 if terminate else length):
            raise ValueError(f"{text!r} does not fit a {length}-byte field")
        return out + bytes([TERMINATOR] * (length - len(out)))


def korean_names(encoder):
    """A few ordinary Hangul names, checked against the table."""
    names = ["한별", "민수", "지훈", "서연", "철수", "영희", "골드"]
    for n in names:
        if not encoder.hangul(n):
            raise SystemExit(f"{n!r} is not in the Korean tables")
    return names


def korean_table_cpp(pkhex):
    """gb_diff.cpp's Korean decoding table as C++: kKoreanTable[page][byte] in UTF-8,
    "" where PKHeX has no character (decoding stops there, as StringConverter2KOR does)."""
    tables = read_korean_tables(pkhex)
    lines = ["// Generated by gb_text.py from PKHeX.Core StringConverter2KOR.cs; not committed.",
             "static const char* const kKoreanTable[12][256] = {"]
    for page in range(12):
        row = tables[page] + [None] * (256 - len(tables[page]))
        cells = ['"' + (c.replace("\\", "\\\\").replace('"', '\\"') if c else "") + '"' for c in row]
        lines.append("    {" + ", ".join(cells) + "},")
    lines.append("};")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    import sys
    from pathlib import Path
    if len(sys.argv) != 3 or sys.argv[1] != "korean-table":
        raise SystemExit("usage: gb_text.py korean-table PKHEX_CHECKOUT")
    sys.stdout.write(korean_table_cpp(Path(sys.argv[2])))
