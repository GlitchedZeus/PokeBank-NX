#!/usr/bin/env python3
"""Apply guarded WC3 BACD_R_A event-ribbon legality integration.

This migration is intentionally narrow. PKHeX's pinned EncounterGift3 catalog and
RibbonVerifierEvent3 require the fixed Gen III event-ribbon state to match WC3
templates. PokeBank NX already reads the Gen III ribbons word for fateful; expose
the fixed Event3 ribbon bits as read-only legality evidence and thread them through
BACD_R_A matching. Earth Ribbon is intentionally not fixed here because it can be
earned later through supported Gen III GameCube cross-transfer history.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    target = ROOT / path
    text = target.read_text(encoding="utf-8")
    if new in text:
        return
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one guarded anchor, found {count}")
    target.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def main() -> int:
    # Original National Ribbon migration. These guarded replacements are kept so
    # a fresh checkout can still deterministically reproduce the full tranche.
    replace_once(
        "include/Pokemon/Pokemon.h",
        '''        /** Fateful-encounter ("obtained in a fateful encounter") flag; false / no-op where unwired. */
        virtual bool isFatefulEncounter() const noexcept { return false; }
        virtual void setFatefulEncounter(bool value) noexcept { (void)value; }
''',
        '''        /** Fateful-encounter ("obtained in a fateful encounter") flag; false / no-op where unwired. */
        virtual bool isFatefulEncounter() const noexcept { return false; }
        virtual void setFatefulEncounter(bool value) noexcept { (void)value; }

        /** Gen III National Ribbon persistent state; false where the format/accessor is unwired. */
        virtual bool ribbonNational() const noexcept { return false; }
''',
    )

    replace_once(
        "include/Pokemon/Pokemon3FRLG.h",
        '''        bool isFatefulEncounter() const noexcept override { return (rd32(0x4C) & 0x80000000u) != 0; }
''',
        '''        bool isFatefulEncounter() const noexcept override { return (rd32(0x4C) & 0x80000000u) != 0; }
        // PK3 National Ribbon is bit 24 of the same persisted ribbon word.
        bool ribbonNational() const noexcept override { return (rd32(0x4C) & 0x01000000u) != 0; }
''',
    )

    replace_once(
        "include/Legality/Gen3BacdRaEventTemplate.h",
        '''    uint8_t otGenderRule;
    bool fateful;
    std::u16string_view otName;
''',
        '''    uint8_t otGenderRule;
    bool fateful;
    bool ribbonNational;
    std::u16string_view otName;
''',
    )
    replace_once(
        "include/Legality/Gen3BacdRaEventTemplate.h",
        '''    bool isEgg = false;
    bool fateful = false;
    bool shiny = false;
''',
        '''    bool isEgg = false;
    bool fateful = false;
    bool ribbonNational = false;
    bool shiny = false;
''',
    )
    replace_once(
        "include/Legality/Gen3BacdRaEventTemplate.h",
        '''        row.level != c.metLevel || row.fateful != c.fateful ||
        row.otName != c.otName)
''',
        '''        row.level != c.metLevel || row.fateful != c.fateful ||
        row.ribbonNational != c.ribbonNational || row.otName != c.otName)
''',
    )

    # Scope the runtime replacement to Gen3BacdRaEvent::match so the similar
    # Negai candidate initializer cannot be mistaken for the WC3 event path.
    replace_once(
        "src/Legality/Legality.cpp",
        '''                const auto restrictedAntiEvent =
                    Gen3BacdRaEvent::match(
                        {
                            species, pk.tid16(), pk.sid16(), pk.originGame(),
                            pk.language(), pk.otGender(), pk.metLevel(),
                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
''',
        '''                const auto restrictedAntiEvent =
                    Gen3BacdRaEvent::match(
                        {
                            species, pk.tid16(), pk.sid16(), pk.originGame(),
                            pk.language(), pk.otGender(), pk.metLevel(),
                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
                        },
                        bacd);
''',
    )

    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''        70, 255, 4,
        false, false, false,
        u"10ANNIV"
''',
        '''        70, 255, 4,
        false, false, false, false,
        u"10ANNIV"
''',
    )
    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''    auto unrestricted = rng;
    unrestricted.restrictedSeed = false;
    assert(!match(charizard, unrestricted).matched);
''',
        '''    auto unrestricted = rng;
    unrestricted.restrictedSeed = false;
    assert(!match(charizard, unrestricted).matched);

    // PKHeX RibbonVerifierEvent3 requires National Ribbon to exactly match the
    // WC3 template. The pinned BACD_R_A catalog has exactly two such rows:
    // Japanese FESTA Metang and English ROCKS Metang (both TID 02005).
    std::size_t nationalRibbonRows = 0;
    for (const auto& row : kEntries) {
        if (!row.ribbonNational)
            continue;
        ++nationalRibbonRows;
        assert(row.species == 375);
        assert(row.tid == 2005);

        Candidate metang{
            row.species, row.tid, row.sid,
            2, row.language, 0,
            row.level, 255, 4,
            false, row.fateful, true, false,
            row.otName
        };
        assert(persistentFieldsMatch(row, metang, 0));
        metang.ribbonNational = false;
        assert(!persistentFieldsMatch(row, metang, 0));
    }
    assert(nationalRibbonRows == 2);
''',
    )
    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''    std::cout << "Gen III BACD_R_A event-template + initial-move metadata evidence: PASS\\n";
''',
        '''    std::cout << "Gen III BACD_R_A event-template + initial-move + National Ribbon evidence: PASS\\n";
''',
    )

    replace_once(
        "docs/LEGALITY_WC3_PROGRESS.md",
        '''- fateful flag
- the exact four-move distribution-time payload
''',
        '''- fateful flag
- National Ribbon state (including the required FESTA/ROCKS Metang ribbon)
- the exact four-move distribution-time payload
''',
    )
    replace_once(
        "docs/LEGALITY_WC3_PROGRESS.md",
        '''The remaining WC3 parity work is increasingly about template depth and historical constraints rather than simply recognizing another PID method: source ribbons, mutable-vs-immutable move provenance, held-item derivation where applicable, evolved-event reconstruction, recipient/trade history, and other fields that can still be proven from a surviving Pokémon.
''',
        '''The remaining WC3 parity work is increasingly about template depth and historical constraints rather than simply recognizing another PID method: the remaining source ribbon classes beyond the now-wired National Ribbon, mutable-vs-immutable move provenance, held-item derivation where applicable, evolved-event reconstruction, recipient/trade history, and other fields that can still be proven from a surviving Pokémon.
''',
    )

    # Keep the deterministic generator as the single source of truth for all
    # 114 BACD_R_A rows. The migration teaches it to retain RibbonNational.
    replace_once(
        "tools/gen_legality_gen3_bacd_ra_events.py",
        '''            bool(re.search(r"\\bFatefulEncounter\\s*=\\s*true", fields)),
            ot_match.group(1),
            moves,
''',
        '''            bool(re.search(r"\\bFatefulEncounter\\s*=\\s*true", fields)),
            bool(re.search(r"\\bRibbonNational\\s*=\\s*true", fields)),
            ot_match.group(1),
            moves,
''',
    )
    replace_once(
        "tools/gen_legality_gen3_bacd_ra_events.py",
        '''    for species, tid, sid, level, language, gender_rule, fateful, ot, moves in rows:
        lines.append(
            "    {%d, %d, %d, %d, %d, %d, %s, %s, {%d, %d, %d, %d}}," %
            (species, tid, sid, level, language, gender_rule,
             "true" if fateful else "false", cxx_u16(ot), *moves)
        )
''',
        '''    for species, tid, sid, level, language, gender_rule, fateful, ribbon_national, ot, moves in rows:
        lines.append(
            "    {%d, %d, %d, %d, %d, %d, %s, %s, %s, {%d, %d, %d, %d}}," %
            (species, tid, sid, level, language, gender_rule,
             "true" if fateful else "false",
             "true" if ribbon_national else "false", cxx_u16(ot), *moves)
        )
''',
    )
    replace_once(
        "tools/gen_legality_gen3_bacd_ra_events.py",
        '''    print("wrote %d BACD_R_A event rows with moves to %s" % (len(rows), OUT))
''',
        '''    print("wrote %d BACD_R_A event rows with moves and National Ribbon state to %s" % (len(rows), OUT))
''',
    )

    # Extend the already-wired National Ribbon evidence to the other fixed
    # IRibbonSetEvent3 fields used by PKHeX's pinned EncounterGift3 verifier.
    # These four ribbons are false for the entire pinned BACD_R_A catalog. They
    # are event-distribution state, unlike Earth Ribbon which can be earned later.
    replace_once(
        "include/Pokemon/Pokemon.h",
        '''        /** Gen III National Ribbon persistent state; false where the format/accessor is unwired. */
        virtual bool ribbonNational() const noexcept { return false; }
''',
        '''        /** Gen III National Ribbon persistent state; false where the format/accessor is unwired. */
        virtual bool ribbonNational() const noexcept { return false; }

        /** Additional fixed Gen III event-ribbon states used by WC3 legality evidence. */
        virtual bool ribbonChampionBattle() const noexcept { return false; }
        virtual bool ribbonChampionRegional() const noexcept { return false; }
        virtual bool ribbonChampionNational() const noexcept { return false; }
        virtual bool ribbonCountry() const noexcept { return false; }
''',
    )

    replace_once(
        "include/Pokemon/Pokemon3FRLG.h",
        '''        // PK3 National Ribbon is bit 24 of the same persisted ribbon word.
        bool ribbonNational() const noexcept override { return (rd32(0x4C) & 0x01000000u) != 0; }
''',
        '''        // PK3 event-ribbon bits 20-24 of the same persisted ribbon word.
        bool ribbonChampionBattle() const noexcept override { return (rd32(0x4C) & 0x00100000u) != 0; }
        bool ribbonChampionRegional() const noexcept override { return (rd32(0x4C) & 0x00200000u) != 0; }
        bool ribbonChampionNational() const noexcept override { return (rd32(0x4C) & 0x00400000u) != 0; }
        bool ribbonCountry() const noexcept override { return (rd32(0x4C) & 0x00800000u) != 0; }
        bool ribbonNational() const noexcept override { return (rd32(0x4C) & 0x01000000u) != 0; }
''',
    )

    replace_once(
        "include/Legality/Gen3BacdRaEventTemplate.h",
        '''    bool fateful = false;
    bool ribbonNational = false;
    bool shiny = false;
''',
        '''    bool fateful = false;
    bool ribbonNational = false;
    bool ribbonCountry = false;
    bool ribbonChampionBattle = false;
    bool ribbonChampionRegional = false;
    bool ribbonChampionNational = false;
    bool shiny = false;
''',
    )

    replace_once(
        "include/Legality/Gen3BacdRaEventTemplate.h",
        '''        row.level != c.metLevel || row.fateful != c.fateful ||
        row.ribbonNational != c.ribbonNational || row.otName != c.otName)
''',
        '''        row.level != c.metLevel || row.fateful != c.fateful ||
        row.ribbonNational != c.ribbonNational || c.ribbonCountry ||
        c.ribbonChampionBattle || c.ribbonChampionRegional ||
        c.ribbonChampionNational || row.otName != c.otName)
''',
    )

    replace_once(
        "src/Legality/Legality.cpp",
        '''                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
''',
        '''                            pk.metLocation(), pk.ball(), pk.isEgg(),
                            pk.isFatefulEncounter(), pk.ribbonNational(),
                            pk.ribbonCountry(), pk.ribbonChampionBattle(),
                            pk.ribbonChampionRegional(), pk.ribbonChampionNational(),
                            pk.isShiny(pk.id32(), {}), pk.otName()
''',
    )

    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''        70, 255, 4,
        false, false, false, false,
        u"10ANNIV"
''',
        '''        70, 255, 4,
        false, false, false,
        false, false, false, false,
        false,
        u"10ANNIV"
''',
    )

    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''    auto wrongOrigin = charizard;
    wrongOrigin.originGame = 1;
    assert(!match(wrongOrigin, rng).matched);

    auto wrongShiny = charizard;
''',
        '''    auto wrongOrigin = charizard;
    wrongOrigin.originGame = 1;
    assert(!match(wrongOrigin, rng).matched);

    // The pinned EncounterGift3 templates have no Country/Champion event ribbons.
    // Unlike Earth Ribbon, these cannot be acquired later through normal Gen III
    // gameplay, so their presence contradicts a direct BACD_R_A template match.
    auto countryRibbon = charizard;
    countryRibbon.ribbonCountry = true;
    assert(!match(countryRibbon, rng).matched);

    auto battleRibbon = charizard;
    battleRibbon.ribbonChampionBattle = true;
    assert(!match(battleRibbon, rng).matched);

    auto regionalRibbon = charizard;
    regionalRibbon.ribbonChampionRegional = true;
    assert(!match(regionalRibbon, rng).matched);

    auto championNationalRibbon = charizard;
    championNationalRibbon.ribbonChampionNational = true;
    assert(!match(championNationalRibbon, rng).matched);

    auto wrongShiny = charizard;
''',
    )

    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''            row.level, 255, 4,
            false, row.fateful, true, false,
            row.otName
''',
        '''            row.level, 255, 4,
            false, row.fateful, true,
            false, false, false, false,
            false,
            row.otName
''',
    )

    replace_once(
        "tests/test_gen3_bacd_ra_event_template.cpp",
        '''    std::cout << "Gen III BACD_R_A event-template + initial-move + National Ribbon evidence: PASS\\n";
''',
        '''    std::cout << "Gen III BACD_R_A event-template + initial-move + fixed Event3 ribbon evidence: PASS\\n";
''',
    )

    replace_once(
        "docs/LEGALITY_WC3_PROGRESS.md",
        '''- National Ribbon state (including the required FESTA/ROCKS Metang ribbon)
- the exact four-move distribution-time payload
''',
        '''- National Ribbon state (including the required FESTA/ROCKS Metang ribbon)
- absence of Country / Champion Battle / Champion Regional / Champion National ribbons for the pinned `BACD_R_A` gifts
- the exact four-move distribution-time payload
''',
    )

    replace_once(
        "docs/LEGALITY_WC3_PROGRESS.md",
        '''The remaining WC3 parity work is increasingly about template depth and historical constraints rather than simply recognizing another PID method: the remaining source ribbon classes beyond the now-wired National Ribbon, mutable-vs-immutable move provenance, held-item derivation where applicable, evolved-event reconstruction, recipient/trade history, and other fields that can still be proven from a surviving Pokémon.
''',
        '''The remaining WC3 parity work is increasingly about template depth and historical constraints rather than simply recognizing another PID method: extending fixed event-ribbon parity beyond the now-covered `BACD_R_A` family, mutable-vs-immutable move provenance, held-item derivation where applicable, evolved-event reconstruction, recipient/trade history, and other fields that can still be proven from a surviving Pokémon. Earth Ribbon remains separate because a Gen III Pokémon can acquire it later through GameCube cross-transfer history.
''',
    )

    print("applied guarded WC3 BACD_R_A fixed event-ribbon legality integration")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
