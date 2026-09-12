from pathlib import Path
import runpy


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"missing patch marker: {label}")
    return text.replace(old, new, 1)

# Apply the already-prepared boxed-Pokemon UI exactly once.
overlay = Path('src/UI/TrainerViewScreenGSCOverlay.cpp')
if 'Generation II Pokemon Actions' not in overlay.read_text():
    runpy.run_path('tools/dev_gen2_ui_patch.py', run_name='__main__')

# The expanded staged editor now depends on the common Experience helpers in the bridge target too.
p = Path('Makefile.host.gsc')
s = p.read_text()
if 'src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp src/Pokemon/BaseStatsGen89.cpp' not in s:
    s = replace_once(
        s,
        'src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/BaseStatsGen89.cpp',
        'src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp src/Pokemon/BaseStatsGen89.cpp',
        'GSC bridge Experience linkage')
p.write_text(s)

# Capability model: machine move names are presentation capability, independent of mutation.
p = Path('include/Save/EditableSaveCapabilities.h')
s = p.read_text()
if 'MachineMoveNames' not in s:
    s = replace_once(
        s,
        '    PokemonShinyToggle= 1u << 17,\n',
        '    PokemonShinyToggle= 1u << 17,\n    MachineMoveNames   = 1u << 18,\n',
        'MachineMoveNames capability')
p.write_text(s)

p = Path('src/Integration/Gen2/Gen2StagedEditor.cpp')
s = p.read_text()
if '.add(SaveEdit::Capability::MachineMoveNames)' not in s:
    s = replace_once(
        s,
        '                 .add(SaveEdit::Capability::PokemonShinyToggle);',
        '                 .add(SaveEdit::Capability::PokemonShinyToggle)\n                 .add(SaveEdit::Capability::MachineMoveNames);',
        'Gen2 machine capability')
p.write_text(s)

# Add the exact Gen II TM01-TM50 + HM01-HM07 move sequence from PKHeX PersonalInfo2.MachineMoves.
p = Path('src/Names/TMMoves.cpp')
s = p.read_text()
if 'GSC_MACHINE_ITEMS' not in s:
    marker = '    // FireRed/LeafGreen  (PKHeX PersonalInfo3.MachineMovesTechnical / MachineMovesHidden)\n'
    insert = '''    // Gold/Silver/Crystal (PKHeX PersonalInfo2.MachineMoves).\n    // The Gen II bag stores one quantity byte for each machine in this exact item-id order.\n    // Indices 0-49 are TM01-TM50; indices 50-56 are HM01-HM07.\n    static const uint8_t GSC_MACHINE_ITEMS[] = {\n        191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,\n        212,213,214,215,216,217,218,219,221,222,223,224,225,226,227,228,229,230,231,232,\n        233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,\n    };\n    static const uint16_t GSC_MACHINE_MOVES[] = {\n        223, 29,174,205, 46, 92,192,249,244,237,\n        241,230,173, 59, 63,196,182,240,202,203,\n        218, 76,231,225, 87, 89,216, 91, 94,247,\n        189,104,  8,207,214,188,201,126,129,111,\n          9,138,197,156,213,168,211,  7,210,171,\n         15, 19, 57, 70,148,250,127,\n    };\n\n'''
    s = replace_once(s, marker, insert + marker, 'Gen2 machine tables')

if 'case GameVersion::GSC:' not in s:
    marker = '        switch (group) {\n'
    insert = '''        switch (group) {\n            // ---- Gold/Silver/Crystal ----\n            case GameVersion::GSC: {\n                constexpr size_t NI = sizeof(GSC_MACHINE_ITEMS) / sizeof(GSC_MACHINE_ITEMS[0]);\n                constexpr size_t NM = sizeof(GSC_MACHINE_MOVES) / sizeof(GSC_MACHINE_MOVES[0]);\n                static_assert(NI == NM);\n                for (size_t i = 0; i < NI; ++i)\n                    if (GSC_MACHINE_ITEMS[i] == itemId) return GSC_MACHINE_MOVES[i];\n                return 0;\n            }\n\n'''
    s = replace_once(s, marker, insert, 'Gen2 getTMMove switch')
p.write_text(s)

# Reusable game-aware machine descriptor / presentation / filtering abstraction.
Path('include/Names/MachineDisplay.h').write_text(r'''#ifndef NAMES_MACHINE_DISPLAY_H
#define NAMES_MACHINE_DISPLAY_H

#include <cstdint>
#include <string>
#include <string_view>

#include "Enums/GameVersion.h"

namespace Names {

enum class MachineKind : uint8_t { None, TM, HM, TR };

struct MachineDescriptor {
    MachineKind kind = MachineKind::None;
    uint16_t number = 0;
    uint8_t numberWidth = 2;
    uint16_t moveId = 0;

    constexpr explicit operator bool() const noexcept {
        return kind != MachineKind::None && moveId != 0;
    }
};

// Interpret a stored item id in the context of the exact game/group. The item id is never mutated.
MachineDescriptor getMachineDescriptor(Enums::GameVersion game, uint16_t itemId) noexcept;
std::string machineIdentifier(const MachineDescriptor& machine);
std::string machineDisplayLabel(Enums::GameVersion game, uint16_t itemId, std::string_view fallbackLabel);

// Reusable case-insensitive filter helper for machine lists. Matches identifier/number/move/full label.
bool machineDisplayMatches(Enums::GameVersion game, uint16_t itemId,
                           std::string_view fallbackLabel, std::string_view query);

} // namespace Names

#endif
''')

Path('src/Names/MachineDisplay.cpp').write_text(r'''#include "Names/MachineDisplay.h"

#include "Names/MoveNames.h"
#include "Names/TMMoves.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace Names {
namespace {

MachineDescriptor make(Enums::GameVersion game, uint16_t itemId, MachineKind kind,
                       uint16_t number, uint8_t width) noexcept {
    const uint16_t move = getTMMove(game, itemId);
    if (move == 0) return {};
    return {kind, number, width, move};
}

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

MachineDescriptor getMachineDescriptor(Enums::GameVersion game, uint16_t itemId) noexcept {
    using Enums::GameVersion;
    switch (game) {
        case GameVersion::GSC: {
            static constexpr uint8_t ids[] = {
                191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
                212,213,214,215,216,217,218,219,221,222,223,224,225,226,227,228,229,230,231,232,
                233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,
            };
            for (uint16_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
                if (ids[i] != itemId) continue;
                if (i < 50) return make(game, itemId, MachineKind::TM, static_cast<uint16_t>(i + 1), 2);
                return make(game, itemId, MachineKind::HM, static_cast<uint16_t>(i - 49), 2);
            }
            return {};
        }

        case GameVersion::FR:
        case GameVersion::LG:
        case GameVersion::FRLG:
            if (itemId >= 289 && itemId <= 338) return make(game, itemId, MachineKind::TM, itemId - 288, 2);
            if (itemId >= 339 && itemId <= 346) return make(game, itemId, MachineKind::HM, itemId - 338, 2);
            return {};

        case GameVersion::GP:
        case GameVersion::GE:
        case GameVersion::GG:
            if (itemId >= 328 && itemId <= 387) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            return {};

        case GameVersion::SW:
        case GameVersion::SH:
        case GameVersion::SWSH:
            if (itemId == 1230) return make(game, itemId, MachineKind::TM, 0, 2);
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 2);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 2);
            if (itemId >= 1130 && itemId <= 1229) return make(game, itemId, MachineKind::TR, itemId - 1130, 2);
            return {};

        case GameVersion::BD:
        case GameVersion::SP:
        case GameVersion::BDSP:
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            if (itemId >= 420 && itemId <= 427) return make(game, itemId, MachineKind::TM, 93 + (itemId - 420), 3);
            return {};

        case GameVersion::SL:
        case GameVersion::VL:
        case GameVersion::SV:
            if (itemId == 1230) return make(game, itemId, MachineKind::TM, 0, 3);
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 3);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 3);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 3);
            if (itemId >= 2160 && itemId <= 2289) return make(game, itemId, MachineKind::TM, 100 + (itemId - 2160), 3);
            return {};

        case GameVersion::ZA:
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 3);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 3);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 3);
            if (itemId == 2160) return make(game, itemId, MachineKind::TM, 100, 3);
            if (itemId >= 2162 && itemId <= 2221) return make(game, itemId, MachineKind::TM, 101 + (itemId - 2162), 3);
            return {};

        case GameVersion::PLA:
        case GameVersion::RBY:
        default:
            return {};
    }
}

std::string machineIdentifier(const MachineDescriptor& machine) {
    if (!machine) return {};
    const char* prefix = machine.kind == MachineKind::TM ? "TM" :
                         machine.kind == MachineKind::HM ? "HM" :
                         machine.kind == MachineKind::TR ? "TR" : "";
    std::ostringstream out;
    out << prefix << std::setw(machine.numberWidth) << std::setfill('0') << machine.number;
    return out.str();
}

std::string machineDisplayLabel(Enums::GameVersion game, uint16_t itemId, std::string_view fallbackLabel) {
    const auto machine = getMachineDescriptor(game, itemId);
    if (!machine) return std::string(fallbackLabel);
    return machineIdentifier(machine) + " — " + getMoveName(machine.moveId);
}

bool machineDisplayMatches(Enums::GameVersion game, uint16_t itemId,
                           std::string_view fallbackLabel, std::string_view query) {
    if (query.empty()) return true;
    const std::string q = lower(query);
    const std::string display = lower(machineDisplayLabel(game, itemId, fallbackLabel));
    if (display.find(q) != std::string::npos) return true;
    const auto machine = getMachineDescriptor(game, itemId);
    if (!machine) return lower(fallbackLabel).find(q) != std::string::npos;
    return std::to_string(machine.number).find(q) != std::string::npos ||
           lower(getMoveName(machine.moveId)).find(q) != std::string::npos;
}

} // namespace Names
''')

# Unified inventory rendering: every known machine becomes "identifier — move" without changing its id.
p = Path('src/UI/Panels/ItemsPanel.cpp')
s = p.read_text()
if '#include "Names/MachineDisplay.h"' not in s:
    s = s.replace('#include "Names/TMMoves.h"\n', '#include "Names/TMMoves.h"\n#include "Names/MachineDisplay.h"\n', 1)
old = '''            std::string gen2Name;
            const char* itemName = nullptr;
            if (gameGroup == GameVersion::RBY) {
                itemName = Names::getItemNameG1(item.itemId);
            } else if (gameGroup == GameVersion::GSC) {
                gen2Name = std::string(PokeVault::Integration::Gen2::gen2ItemName(
                    static_cast<uint8_t>(item.itemId)));
                itemName = gen2Name.c_str();
            } else if (gameGroup == GameVersion::FRLG) {
                itemName = Names::getItemNameG3(item.itemId);
            } else {
                itemName = getItemName(item.itemId);
            }
            fb.drawText(nx, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2,
                        itemName, nameCol, TextStyle::Body);

            // Raw Gen I/II machine ids have their own namespaces; never reinterpret them through a
            // later-generation TM table. Their exact TM01..TM50/HM01..HM07 labels come from gen2ItemName.
            if (gameGroup != GameVersion::RBY && gameGroup != GameVersion::GSC) {
                if (uint16_t tmMove = Names::getTMMove(gameGroup, item.itemId)) {
                    int iw, ih; fb.measureText(itemName, iw, ih, TextStyle::Body);
                    const Color moveCol = selected ? Colors::PrimaryText : Colors::TextDim;
                    fb.drawText(nx + iw + 14, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2,
                                Names::getMoveName(tmMove), moveCol, TextStyle::Body);
                }
            }
'''
new = '''            std::string baseName;
            if (gameGroup == GameVersion::RBY) {
                baseName = Names::getItemNameG1(item.itemId);
            } else if (gameGroup == GameVersion::GSC) {
                baseName = std::string(PokeVault::Integration::Gen2::gen2ItemName(
                    static_cast<uint8_t>(item.itemId)));
            } else if (gameGroup == GameVersion::FRLG) {
                baseName = Names::getItemNameG3(item.itemId);
            } else {
                baseName = getItemName(item.itemId);
            }
            const std::string displayName = Names::machineDisplayLabel(gameGroup, item.itemId, baseName);
            fb.drawText(nx, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2,
                        displayName, nameCol, TextStyle::Body);
'''
if old in s:
    s = s.replace(old, new, 1)
elif 'machineDisplayLabel(gameGroup' not in s:
    raise SystemExit('ItemsPanel machine-rendering marker missing')
p.write_text(s)

# Pouch/held-item pickers share the same game-aware label layer.
p = Path('src/UI/Dialogs/PickerDialog.cpp')
s = p.read_text()
if '#include "Names/MachineDisplay.h"' not in s:
    s = s.replace('#include "Names/ItemNames.h"', '#include "Names/ItemNames.h"\n#include "Names/MachineDisplay.h"', 1)
marker = '''            } else {
                label = pickerOptionLabel(kind, val);
            }
            fb.drawText(px + 28, ry + (rowH - 4 - fb.lineHeight(TextStyle::Body)) / 2, label, col);'''
replacement = '''            } else {
                label = pickerOptionLabel(kind, val);
            }
            static std::string machineLabel;
            if (kind == PickerKind::Item || kind == PickerKind::ItemG3 ||
                kind == PickerKind::PouchItem || kind == PickerKind::PouchItemG3) {
                machineLabel = Names::machineDisplayLabel(screen.trainer.getGameGroup(),
                                                         static_cast<uint16_t>(val), label);
                label = machineLabel.c_str();
            }
            fb.drawText(px + 28, ry + (rowH - 4 - fb.lineHeight(TextStyle::Body)) / 2, label, col);'''
if marker in s:
    s = s.replace(marker, replacement, 1)
elif 'machineDisplayLabel(screen.trainer.getGameGroup()' not in s:
    raise SystemExit('PickerDialog label marker missing')
p.write_text(s)

# Expose PP Ups through the existing Move PP rows without adding a fake independent save field.
p = overlay
s = p.read_text()
old = '''        case 6: case 8: case 10: case 12: {
            const int slot = (row - 6) / 2;
            auto pp = pokemon->pp;
            const auto r = Utils::promptNumber("Gen II Move PP", pp[slot], 0, 63);
            if (!r.accepted) return false;
            pp[slot] = static_cast<uint8_t>(r.value);
            edit.pp = pp;
            break;
        }'''
new = '''        case 6: case 8: case 10: case 12: {
            const int slot = (row - 6) / 2;
            auto pp = pokemon->pp;
            auto ppUps = pokemon->ppUps;
            const auto r = Utils::promptNumber("Gen II Move PP", pp[slot], 0, 63);
            if (!r.accepted) return false;
            const auto ups = Utils::promptNumber("Gen II PP Ups (0-3)", ppUps[slot], 0, 3);
            if (!ups.accepted) return false;
            pp[slot] = static_cast<uint8_t>(r.value);
            ppUps[slot] = static_cast<uint8_t>(ups.value);
            edit.pp = pp;
            edit.ppUps = ppUps;
            break;
        }'''
if old in s:
    s = s.replace(old, new, 1)
rows = [
    ('{"Move 1 PP", std::to_string(pokemon->pp[0])}', '{"Move 1 PP", std::to_string(pokemon->pp[0]) + " / PP Ups " + std::to_string(pokemon->ppUps[0])}'),
    ('{"Move 2 PP", std::to_string(pokemon->pp[1])}', '{"Move 2 PP", std::to_string(pokemon->pp[1]) + " / PP Ups " + std::to_string(pokemon->ppUps[1])}'),
    ('{"Move 3 PP", std::to_string(pokemon->pp[2])}', '{"Move 3 PP", std::to_string(pokemon->pp[2]) + " / PP Ups " + std::to_string(pokemon->ppUps[2])}'),
    ('{"Move 4 PP", std::to_string(pokemon->pp[3])}', '{"Move 4 PP", std::to_string(pokemon->pp[3]) + " / PP Ups " + std::to_string(pokemon->ppUps[3])}'),
]
for a, b in rows:
    s = s.replace(a, b)
p.write_text(s)

# Clear installed-save UX: behavior remains discard/return only; no writeback is enabled.
p = Path('src/UI/Dialogs/SaveConfirmDialog.cpp')
s = p.read_text()
old = '''        if (screen.exitingWithUnsavedChanges) {
            constexpr int h = 248;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            int cy = drawDialogFrame(fb, x, y, w, h, "Unsaved Changes", Colors::Warning);
            fb.drawText(x + 24, cy,      "You have unsaved changes.", Colors::Text);
            fb.drawText(x + 24, cy + 28, "Changes will be lost if you continue.", Colors::TextDim);

            // Buttons carry their glyph (id 0 = Cancel/B, id 1 = Discard & Exit/A), no guide line.
            screen.touchButtons.clear();
            const int cbh = TouchTargetMin, cby = y + h - cbh - 16;
            const int cbw = (w - 48 - 16) / 2;
            drawEditChoiceButton(screen, fb, x + 24,           cby, cbw, cbh, "B", "Cancel",         0);
            drawEditChoiceButton(screen, fb, x + w - 24 - cbw, cby, cbw, cbh, "A", "Discard & Exit", 1);
            return;
        }'''
new = '''        if (screen.exitingWithUnsavedChanges) {
            const bool installedPreview = screen.sourceKind == PokeVault::Safety::SourceKind::InstalledGame;
            constexpr int h = 270;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            int cy = drawDialogFrame(fb, x, y, w, h,
                                     installedPreview ? "SAVE WRITING NOT YET ENABLED" : "Unsaved Changes",
                                     Colors::Warning);
            if (installedPreview) {
                fb.drawText(x + 24, cy,      "Edits to this installed game are preview-only in this build.", Colors::Text);
                fb.drawText(x + 24, cy + 28, "PokeBank NX will not modify the installed game save.", Colors::TextDim);
                fb.drawText(x + 24, cy + 56, "Discard the preview changes, or return to the editor.", Colors::TextDim);
            } else {
                fb.drawText(x + 24, cy,      "You have unsaved changes.", Colors::Text);
                fb.drawText(x + 24, cy + 28, "Changes will be lost if you continue.", Colors::TextDim);
            }

            // Same behavior and button ids as before; only the installed-source wording is clearer.
            screen.touchButtons.clear();
            const int cbh = TouchTargetMin, cby = y + h - cbh - 16;
            const int cbw = (w - 48 - 16) / 2;
            drawEditChoiceButton(screen, fb, x + 24, cby, cbw, cbh, "B",
                                 installedPreview ? "Return to Editor" : "Cancel", 0);
            drawEditChoiceButton(screen, fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                                 installedPreview ? "Discard Changes" : "Discard & Exit", 1);
            return;
        }'''
if old in s:
    s = s.replace(old, new, 1)
elif 'SAVE WRITING NOT YET ENABLED' not in s:
    raise SystemExit('SaveConfirm installed-warning marker missing')
p.write_text(s)

# The immediate installed-source guard must also describe the lock as an intentional product state.
p = Path('include/UI/TrainerViewScreen.h')
s = p.read_text()
s = s.replace(
    ': "Installed source is read-only. Open a backup workspace explicitly to edit.", 300);',
    ': "SAVE WRITING NOT YET ENABLED. Installed-game saves remain read-only in this build.", 300);')
p.write_text(s)

# Focused permanent mapping tests.
Path('tests/test_machine_display.cpp').write_text(r'''#include "Names/MachineDisplay.h"
#include "Names/MoveNames.h"
#include "Names/TMMoves.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    using Enums::GameVersion;
    using Names::MachineKind;

    // Gen II exact PKHeX PersonalInfo2 mapping: TM01 Dynamic Punch, HM03 Surf.
    assert(Names::getTMMove(GameVersion::GSC, 191) == 223);
    assert(Names::getTMMove(GameVersion::GSC, 245) == 57);
    assert(Names::getTMMove(GameVersion::GSC, 195) == 0); // unused gap must not alias a machine

    auto tm01gsc = Names::getMachineDescriptor(GameVersion::GSC, 191);
    assert(tm01gsc && tm01gsc.kind == MachineKind::TM && tm01gsc.number == 1 && tm01gsc.moveId == 223);
    auto hm03gsc = Names::getMachineDescriptor(GameVersion::GSC, 245);
    assert(hm03gsc && hm03gsc.kind == MachineKind::HM && hm03gsc.number == 3 && hm03gsc.moveId == 57);
    assert(Names::machineIdentifier(tm01gsc) == "TM01");
    assert(Names::machineIdentifier(hm03gsc) == "HM03");

    // Same machine number, different generation/game mapping.
    assert(Names::getTMMove(GameVersion::FRLG, 289) == 264);
    assert(Names::getTMMove(GameVersion::FRLG, 289) != Names::getTMMove(GameVersion::GSC, 191));

    // Existing reliable project mappings remain available through the global descriptor layer.
    auto tr15 = Names::getMachineDescriptor(GameVersion::SWSH, 1145);
    assert(tr15 && tr15.kind == MachineKind::TR && tr15.number == 15 && tr15.moveId == 126);
    assert(Names::getMachineDescriptor(GameVersion::PLA, 328).kind == MachineKind::None);

    const uint16_t stored = 191;
    const std::string display = Names::machineDisplayLabel(GameVersion::GSC, stored, "TM01");
    assert(display.find("TM01") != std::string::npos);
    assert(display.find(Names::getMoveName(223)) != std::string::npos);
    assert(stored == 191); // presentation must never rewrite the save-format item id

    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "TM01"));
    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "1"));
    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "dynamic"));
    assert(!Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "surf"));

    std::cout << "machine display tests passed\n";
    return 0;
}
''')

Path('Makefile.host.machine').write_text(r'''# Global game-aware machine-name presentation gates.
MACHINE_DISPLAY_SOURCES := tests/test_machine_display.cpp \
	src/Names/MachineDisplay.cpp src/Names/TMMoves.cpp src/Names/MoveNames.cpp

MACHINE_HOST_TESTS := $(HOST_BUILD)/test_machine_display
MACHINE_SANITIZE_TESTS := $(HOST_BUILD)/test_machine_display_sanitize

HOST_TESTS += $(MACHINE_HOST_TESTS)
HOST_SANITIZE_TESTS += $(MACHINE_SANITIZE_TESTS)
host-test: $(MACHINE_HOST_TESTS)
host-sanitize: $(MACHINE_SANITIZE_TESTS)

$(HOST_BUILD)/test_machine_display: $(MACHINE_DISPLAY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_machine_display_sanitize: $(MACHINE_DISPLAY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
''')

p = Path('Makefile.host')
s = p.read_text()
if 'include Makefile.host.machine' not in s:
    s += '\ninclude Makefile.host.machine\n'
p.write_text(s)

# Extend existing focused editor test assertions for the new capability and text glyph hardening.
p = Path('tests/test_gsc_pokemon_editor.cpp')
s = p.read_text()
if 'Capability::MachineMoveNames' not in s:
    s = replace_once(
        s,
        '    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonShinyToggle));\n',
        '    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonShinyToggle));\n    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::MachineMoveNames));\n',
        'machine capability test')
p.write_text(s)
