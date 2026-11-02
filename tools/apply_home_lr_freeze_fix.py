#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str) -> None:
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one replacement, found {count}\nOLD:\n{old}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# Switch FireRed/LeafGreen are 2026 Switch releases, not the original 2004 GBA releases.
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    '''            if (id == "firered_gba" || id == "firered_switch") return 200401290;\n            if (id == "leafgreen_gba" || id == "leafgreen_switch") return 200401291;\n            if (id == "emerald_gba") return 200409160;''',
    '''            if (id == "firered_gba") return 200401290;\n            if (id == "leafgreen_gba") return 200401291;\n            if (id == "emerald_gba") return 200409160;''')
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    '''            if (id == "legends_za_switch") return 202510160;\n            // Unknown/future identities sort after known releases instead of jumping in front.''',
    '''            if (id == "legends_za_switch") return 202510160;\n            if (id == "firered_switch") return 202602270;\n            if (id == "leafgreen_switch") return 202602271;\n            // Unknown/future identities sort after known releases instead of jumping in front.''')

# Trainer3FRLG gets a presentation-only constructor mode. It validates the active sector table and
# parses only trainer + party, avoiding the expensive 420 box-slot parse on Product Home navigation.
replace_once(
    "include/Trainer/Trainer3FRLG.h",
    '''        explicit Trainer3FRLG(std::vector<uint8_t> data, std::string fileName);''',
    '''        explicit Trainer3FRLG(std::vector<uint8_t> data, std::string fileName,\n                              bool previewOnly = false);''')
replace_once(
    "src/Trainer/Trainer3FRLG.cpp",
    '''    Trainer3FRLG::Trainer3FRLG(std::vector<uint8_t> data, std::string fileName)\n        : Trainer(std::vector<Block>{}), saveData(std::move(data)), m_fileName(std::move(fileName))''',
    '''    Trainer3FRLG::Trainer3FRLG(std::vector<uint8_t> data, std::string fileName, bool previewOnly)\n        : Trainer(std::vector<Block>{}), saveData(std::move(data)), m_fileName(std::move(fileName))''')
replace_once(
    "src/Trainer/Trainer3FRLG.cpp",
    '''        parseTrainer();\n        parseParty();\n        parseBoxes();\n        parseBoxNames();\n        parseItems();''',
    '''        parseTrainer();\n        parseParty();\n        // Product Home only needs identity + active party. Parsing all 420 box slots on the UI\n        // input thread made L/R appear frozen on the native Switch FRLG wrappers. Full editor opens\n        // keep the default path and still parse boxes, names and inventory.\n        if (previewOnly) return;\n        parseBoxes();\n        parseBoxNames();\n        parseItems();''')

replace_once(
    "include/Save/GetSaveFileContents.h",
    '''    Trainer3FRLG readTrainerInfoFRLG(const char* backupDir);''',
    '''    Trainer3FRLG readTrainerInfoFRLG(const char* backupDir, bool previewOnly = false);''')
replace_once(
    "src/Save/GetSaveFileContents.cpp",
    '''    Trainer3FRLG readTrainerInfoFRLG(const char* backupDir) {''',
    '''    Trainer3FRLG readTrainerInfoFRLG(const char* backupDir, bool previewOnly) {''')
replace_once(
    "src/Save/GetSaveFileContents.cpp",
    '''        return Trainer3FRLG(std::move(data), fileName);''',
    '''        return Trainer3FRLG(std::move(data), fileName, previewOnly);''')

# FRLG used to bypass read validation entirely. Make the common open preflight prove the wrapper save
# has a valid 128 KiB sector table before any full editor parser is constructed.
replace_once(
    "src/Save/GetSaveFileContents.cpp",
    '''        if (group != GameVersion::PLA) return true;''',
    '''        if (group == GameVersion::FRLG) {\n            auto preview = readTrainerInfoFRLG(backupDir, true);\n            if (!preview.isValid()) {\n                error = "FireRed/LeafGreen save is missing, truncated, or has an invalid active sector table.";\n                return false;\n            }\n            return true;\n        }\n\n        if (group != GameVersion::PLA) return true;''')

# Product Home's native Switch FRLG preview uses the lightweight parser and returns before the generic
# variant path can construct the full 420-slot editor model.
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    '''            std::string error;\n            if (Save::validateTrainerSaveForOpen("pbpreview:", title.titleId, error)) {\n                auto trainer = Save::readTrainerInfo("pbpreview:", title.titleId);''',
    '''            const GameVersion previewGroup = getGameGroup(getGameVersion(title.titleId));\n            if (previewGroup == GameVersion::FRLG) {\n                auto parsed = Save::readTrainerInfoFRLG("pbpreview:", true);\n                if (parsed.isValid()) {\n                    previewTrainerName = parsed.trainerName;\n                    previewTrainerGender = parsed.trainerGender;\n                    previewTrainerGenderKnown = true;\n                    const size_t count = std::min<size_t>(parsed.party.size(), partyPreview.size());\n                    for (size_t i = 0; i < count; ++i) {\n                        if (!parsed.party[i]) continue;\n                        const auto* pokemon = parsed.party[i].get();\n                        addParty(i, pokemon->speciesID(), pokemon->level(), pokemon->form(),\n                                 pokemon->isShiny(pokemon->id32(), pokemon->species()));\n                    }\n                    partyPreviewStatus = count == 0 ? "No active party Pokémon." : "Current save party";\n                } else {\n                    partyPreviewStatus = "FireRed/LeafGreen preview could not validate this save.";\n                }\n                fsdevUnmountDevice("pbpreview");\n                return;\n            }\n\n            std::string error;\n            if (Save::validateTrainerSaveForOpen("pbpreview:", title.titleId, error)) {\n                auto trainer = Save::readTrainerInfo("pbpreview:", title.titleId);''')

# Lock the regression into the Product Home contract suite.
test_path = ROOT / "tests/test_game_hub_contract.py"
test = test_path.read_text(encoding="utf-8")
anchor = '''game_identity = (ROOT / "src/Games/GameIdentity.cpp").read_text(encoding="utf-8")\n'''
if anchor not in test:
    raise SystemExit("tests/test_game_hub_contract.py: source-variable anchor missing")
test = test.replace(anchor, anchor + '''trainer3_header = (ROOT / "include/Trainer/Trainer3FRLG.h").read_text(encoding="utf-8")\ntrainer3_source = (ROOT / "src/Trainer/Trainer3FRLG.cpp").read_text(encoding="utf-8")\nsave_reader = (ROOT / "src/Save/GetSaveFileContents.cpp").read_text(encoding="utf-8")\n''', 1)

anchor = '''require("HidNpadButton_L" in source and "HidNpadButton_R" in source,\n        "L/R must switch the selected game")\n'''
if anchor not in test:
    raise SystemExit("tests/test_game_hub_contract.py: L/R anchor missing")
test = test.replace(anchor, anchor + '''require('readTrainerInfoFRLG("pbpreview:", true)' in source,\n        "Product Home native FRLG preview must use the lightweight parser")\nrequire("bool previewOnly = false" in trainer3_header and "if (previewOnly) return;" in trainer3_source,\n        "FRLG parser must keep a lightweight trainer+party presentation mode")\nrequire("group == GameVersion::FRLG" in save_reader and\n        "readTrainerInfoFRLG(backupDir, true)" in save_reader,\n        "FRLG common open preflight must validate the active sector table before full parsing")\n''', 1)

anchor = '''        'if (id == "letsgo_pikachu_switch") return 201811160;' in source and\n'''
if anchor not in test:
    raise SystemExit("tests/test_game_hub_contract.py: release-order anchor missing")
test = test.replace(anchor, anchor + '''        'if (id == "firered_switch") return 202602270;' in source and\n        'if (id == "leafgreen_switch") return 202602271;' in source and\n        'firered_gba" || id == "firered_switch' not in source and\n''', 1)
test_path.write_text(test, encoding="utf-8")

print("Applied Product Home native FRLG L/R freeze fix.")
