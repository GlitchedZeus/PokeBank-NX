#!/usr/bin/env python3
"""Keep BACD_R_A source moves out of immutable encounter-identity matching.

Pinned PKHeX EncounterGift3 stores the distribution moves for generation and move
provenance, but IsMatchExact does not require a non-egg gift's current moves to
remain unchanged. This guarded migration removes the temporary current-moves
argument if present and otherwise verifies the expected source shape.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "Legality" / "Legality.cpp"

START = "                const auto restrictedAntiEvent =\n"
END = "                const Gen3NegaiBoshiEvent::Candidate negaiCandidate{\n"
WITH_MOVES = """                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName(),
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
"""
WITHOUT_MOVES = """                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
"""


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    start = text.find(START)
    end = text.find(END, start + 1)
    if start < 0 or end < 0:
        raise SystemExit("BACD_R_A candidate anchors do not match expected source")

    segment = text[start:end]
    if segment.count(WITH_MOVES) == 1:
        segment = segment.replace(WITH_MOVES, WITHOUT_MOVES, 1)
        text = text[:start] + segment + text[end:]
        TARGET.write_text(text, encoding="utf-8", newline="\n")
        return 0
    if segment.count(WITHOUT_MOVES) == 1:
        return 0
    raise SystemExit("BACD_R_A candidate payload does not match expected source")


if __name__ == "__main__":
    raise SystemExit(main())
