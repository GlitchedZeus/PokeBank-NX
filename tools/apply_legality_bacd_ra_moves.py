#!/usr/bin/env python3
"""Keep BACD_R_A source moves out of immutable encounter-identity matching.

Pinned PKHeX EncounterGift3 stores the distribution moves for generation and move
provenance, but IsMatchExact does not require a non-egg gift's current moves to
remain unchanged. This guarded migration removes the temporary current-moves
argument if present and otherwise verifies one of the known source shapes.

The BACD_R_A candidate gained persistent ribbon evidence after this migration was
first introduced. Keep this older cleanup idempotent across the pre-ribbon,
National-Ribbon, and fixed Event3-ribbon layouts so deterministic regeneration can
run before or after those later migrations without weakening the shape guard.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "src" / "Legality" / "Legality.cpp"

START = "                const auto restrictedAntiEvent =\n"
END = "                const Gen3NegaiBoshiEvent::Candidate negaiCandidate{\n"
MOVE_PAYLOAD = """,
                            {pk.move(0), pk.move(1), pk.move(2), pk.move(3)}
"""

KNOWN_TAILS = (
    # Original candidate layout.
    """                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
""",
    # National Ribbon evidence added by the first WC3 ribbon tranche.
    """                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
""",
    # Fixed Event3 ribbon evidence. Earth Ribbon is intentionally not part of
    # this layout because it can be earned later through Gen III GameCube play.
    """                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.ribbonCountry(), pk.ribbonChampionBattle(),
                            pk.ribbonChampionRegional(), pk.ribbonChampionNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
""",
)


def main() -> int:
    text = TARGET.read_text(encoding="utf-8")
    start = text.find(START)
    end = text.find(END, start + 1)
    if start < 0 or end < 0:
        raise SystemExit("BACD_R_A candidate anchors do not match expected source")

    segment = text[start:end]
    matched = [tail for tail in KNOWN_TAILS if segment.count(tail) == 1]
    if len(matched) != 1:
        raise SystemExit("BACD_R_A candidate payload does not match a known source shape")

    tail = matched[0]
    with_moves = tail.rstrip("\n") + MOVE_PAYLOAD
    if segment.count(with_moves) == 1:
        segment = segment.replace(with_moves, tail.rstrip("\n") + "\n", 1)
        text = text[:start] + segment + text[end:]
        TARGET.write_text(text, encoding="utf-8", newline="\n")
        return 0

    if "{pk.move(0), pk.move(1), pk.move(2), pk.move(3)}" in segment:
        raise SystemExit("BACD_R_A move payload is present in an unexpected source shape")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
