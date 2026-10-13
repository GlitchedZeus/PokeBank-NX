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
