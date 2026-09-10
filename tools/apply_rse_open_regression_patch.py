#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


TEST = "tests/test_rse_gen3_adapter.cpp"
replace_once(
    TEST,
    """    auto invalidItemResult = parse(invalidItem, SourceGame::RubyGBA);\n    assert(invalidItemResult && invalidItemResult.error == SaveError::None);\n""",
    """    const auto invalidItemUntouched = invalidItem;\n    auto invalidItemResult = parse(invalidItem, SourceGame::RubyGBA);\n    assert(invalidItemResult && invalidItemResult.error == SaveError::None);\n    assert(invalidItem == invalidItemUntouched);\n""",
)
replace_once(
    TEST,
    """    auto impossibleCountResult = parse(impossibleCount, SourceGame::SapphireGBA);\n    assert(impossibleCountResult && impossibleCountResult.error == SaveError::None);\n""",
    """    const auto impossibleCountUntouched = impossibleCount;\n    auto impossibleCountResult = parse(impossibleCount, SourceGame::SapphireGBA);\n    assert(impossibleCountResult && impossibleCountResult.error == SaveError::None);\n    assert(impossibleCount == impossibleCountUntouched);\n""",
)
replace_once(
    TEST,
    """    assert(impossibleCountResult.save->boxes().size() == 2);\n    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);\n\n    // Current PKHeX Gen III pouch semantics load fixed-width slots and treat count-zero entries\n""",
    """    assert(impossibleCountResult.save->boxes().size() == 2);\n    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);\n\n    // Repeat the optional-inventory failure contract on Emerald, including keyed quantity storage.\n    auto invalidEmeraldItem = emerald;\n    for (uint8_t slot = 0; slot < 2; ++slot)\n        overwriteInventoryEntry(invalidEmeraldItem, slot, 0x0560, 377,\n                                static_cast<uint16_t>(1 ^ 0xC3D4));\n    const auto invalidEmeraldUntouched = invalidEmeraldItem;\n    auto invalidEmeraldResult = parse(invalidEmeraldItem, SourceGame::EmeraldGBA);\n    assert(invalidEmeraldResult && invalidEmeraldResult.error == SaveError::None);\n    assert(invalidEmeraldItem == invalidEmeraldUntouched);\n    assert(invalidEmeraldResult.save->inventory().empty());\n    assert(invalidEmeraldResult.save->trainer().name == \"WILL\");\n    assert(invalidEmeraldResult.save->party().size() == 1);\n    assert(invalidEmeraldResult.save->lastEnumerationError() == SaveError::None);\n    assert(invalidEmeraldResult.save->boxes().size() == 2);\n    assert(invalidEmeraldResult.save->lastEnumerationError() == SaveError::None);\n\n    // Current PKHeX Gen III pouch semantics load fixed-width slots and treat count-zero entries\n""",
)

print("RSE source-immutability and Emerald nonfatal-inventory regressions staged")
