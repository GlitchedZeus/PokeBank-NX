#!/usr/bin/env python3
"""Wire exact BACD_R_A event moves into the central Gen III legality candidate."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "Legality" / "Legality.cpp"

START = "                const auto restrictedAntiEvent =\n"
END = "                const Gen3NegaiBoshiEvent::Candidate negaiCandidate{\n"
OLD = """                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
"""
NEW = """                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName(),
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
"""


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    start = text.find(START)
    end = text.find(END, start + 1)
    if start < 0 or end < 0:
        raise SystemExit("BACD_R_A candidate anchors do not match expected source")

    segment = text[start:end]
    if "pk.move(0), pk.move(1), pk.move(2), pk.move(3)" in segment:
        return 0
    if segment.count(OLD) != 1:
        raise SystemExit("BACD_R_A candidate payload does not match expected source")

    segment = segment.replace(OLD, NEW, 1)
    text = text[:start] + segment + text[end:]
    TARGET.write_text(text, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
