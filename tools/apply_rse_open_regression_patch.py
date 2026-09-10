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
    """    auto invalidItemResult = parse(invalidItem, SourceGame::RubyGBA);\n    assert(!invalidItemResult && invalidItemResult.error == SaveError::InvalidInventory);\n\n""",
    """    auto invalidItemResult = parse(invalidItem, SourceGame::RubyGBA);\n    assert(invalidItemResult && invalidItemResult.error == SaveError::None);\n    assert(invalidItemResult.save->inventory().empty());\n    assert(invalidItemResult.save->trainer().name == \"WILL\");\n    assert(invalidItemResult.save->party().size() == 1);\n    assert(invalidItemResult.save->lastEnumerationError() == SaveError::None);\n    assert(invalidItemResult.save->boxes().size() == 2);\n    assert(invalidItemResult.save->lastEnumerationError() == SaveError::None);\n\n""",
)
replace_once(
    TEST,
    """    auto impossibleCountResult = parse(impossibleCount, SourceGame::SapphireGBA);\n    assert(!impossibleCountResult && impossibleCountResult.error == SaveError::InvalidInventory);\n\n""",
    """    auto impossibleCountResult = parse(impossibleCount, SourceGame::SapphireGBA);\n    assert(impossibleCountResult && impossibleCountResult.error == SaveError::None);\n    assert(impossibleCountResult.save->inventory().empty());\n    assert(impossibleCountResult.save->trainer().money == 500000);\n    assert(impossibleCountResult.save->party().size() == 1);\n    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);\n    assert(impossibleCountResult.save->boxes().size() == 2);\n    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);\n\n    // Current PKHeX Gen III pouch semantics load fixed-width slots and treat count-zero entries\n    // as unowned/clearable state. Preserve realistic stale slots instead of rejecting inventory.\n    auto staleZero = rs;\n    for (uint8_t slot = 0; slot < 2; ++slot)\n        overwriteInventoryEntry(staleZero, slot, 0x0564, 600, 0);\n    auto staleZeroResult = parse(staleZero, SourceGame::RubyGBA);\n    assert(staleZeroResult);\n    assert(staleZeroResult.save->inventory().size() == 6);\n    assert(staleZeroResult.save->inventory()[0].items.size() == 1);\n    assert(staleZeroResult.save->inventory()[0].items[0].itemId == 13);\n    assert(staleZeroResult.save->inventory()[0].items[0].count == 3);\n\n    auto staleEmerald = emerald;\n    for (uint8_t slot = 0; slot < 2; ++slot)\n        overwriteInventoryEntry(staleEmerald, slot, 0x0564, 600, 0xC3D4);\n    auto staleEmeraldResult = parse(staleEmerald, SourceGame::EmeraldGBA);\n    assert(staleEmeraldResult);\n    assert(staleEmeraldResult.save->inventory().size() == 6);\n    assert(staleEmeraldResult.save->inventory()[0].items.size() == 1);\n\n""",
)

for path in (
    "src/Integration/Gen3/PKSMGen3Adapter.cpp",
    "src/Integration/Gen3/PKSMGen3NativeAdapter.cpp",
):
    replace_once(
        path,
        'case SaveError::InvalidInventory: return "invalid FireRed/LeafGreen inventory";',
        'case SaveError::InvalidInventory: return "invalid Generation III inventory";',
    )

print("RSE open-regression patch staged")
