// Feature A UI (owner Oct 28): Pokemon Actions -> "Import from file" / "Export to file" through the
// REAL TrainerViewScreen::update / draw (tests/host-ui harness) for Gen I, II, III, IV and V.
//
// Per generation, with real controller presses and touch taps:
//   * an occupied slot's Actions sheet lists "Export to file"; choosing it writes a NEW file under
//     sdmc:/switch/PokeBank-NX/pokemon/genN that decodes as that generation, posts "Exported to
//     <path>" and leaves the workspace exactly as it was; a second export gets " (2)";
//   * an empty slot's Actions sheet lists "Import from file"; it opens the shared file dialog on a
//     scripted SD folder, where an oversized file is refused before it is opened and a file of
//     another generation is refused with the editor's reason (nothing staged in either case);
//     D-pad + A opens a sub-folder, Y marks it a favourite, a tap on the exported file imports it:
//     the slot holds the record (Gen I / II: the box's first empty slot), the status says
//     "Imported <species> into Box n / Slot m (staged; the source save is unchanged)", the folder is
//     remembered and Review lists the slot; B at the start folder closes the dialog;
//   * '-' cancels the import from a folder above the start folder, from the favourites list and by
//     a tap on its footer badge: nothing read, staged or remembered (Round 11 H39).
// The SD card is a private mkdtemp working directory; every save is synthetic, built in code: Gen I /
// II / IV from the QA builders (tests/fuzz_synthetic_saves.h, gsc fixture), Gen III / V from the
// L06 harness fixtures (tests/host-ui/HostUiFixtures.h). No real save, ROM or Pokemon file is read.
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5PokemonFile.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Names/SpeciesNames.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/PokemonFileIo.h"
#include "UI/ReviewEntry.h"
#include "UI/ScreenChrome.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/Settings.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace G3 = PokeVault::Integration::Gen3;
namespace G5 = PokeVault::Integration::Gen5;
namespace Io = PokeBank::UIModel::PokemonFileIo;
namespace Entry = PokeBank::UIModel::ReviewEntry;
using View = UI::TrainerViewScreen::ViewMode;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

struct Driver {
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0, u64 held = 0) {
        PadState pad{down, held | down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) {
        frame(button);
        frame();
    }
    const UI::TrainerViewScreen::TouchButton* target(int id) const {
        for (const auto& b : screen.touchButtons)
            if (b.id == id) return &b;
        return nullptr;
    }
    // A completed tap on the footer badge of `button` (the nav-bar hits drawNavBar publishes).
    bool tapNav(u64 button) {
        for (const auto& h : UI::g_navHits) {
            if (h.button != button) continue;
            return tapAt(static_cast<u32>(h.x + h.w / 2), static_cast<u32>(h.y + h.h / 2));
        }
        return false;
    }
    bool tapAt(u32 x, u32 y) {
        HostUi::touch = {true, x, y};
        frame();
        HostUi::touch = {true, x, y};
        frame();
        HostUi::touch = {false, x, y};
        frame();
        frame();
        return true;
    }
    bool tap(int id) {
        const auto* b = target(id);
        if (!b) return false;
        const auto x = static_cast<u32>(b->x + b->w / 2), y = static_cast<u32>(b->y + b->h / 2);
        HostUi::touch = {true, x, y};
        frame();
        HostUi::touch = {true, x, y};
        frame();
        HostUi::touch = {false, x, y};
        frame();
        frame();
        return true;
    }
};

bool drawn(const std::string& text) {
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(), [&](const std::string& t) { return t == text; });
}
bool drawnContaining(const std::string& part) {
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                       [&](const std::string& t) { return t.find(part) != std::string::npos; });
}
std::string allTexts() {
    std::string out;
    for (const auto& t : HostUi::texts) out += "[" + t + "]";
    return out;
}

// The narrow Pokemon Actions sheet (NarrowPokemonActionMenu geometry: 440 px centred, labels 18 px
// inside the 10 px row inset) draws its row labels at x = 420 + 10 + 18.
std::vector<std::string> sheetRows() {
    std::vector<std::string> rows;
    for (const auto& t : HostUi::visibleText)
        if (t.x == 448 && t.style == 1) rows.push_back(t.text);
    return rows;
}
int sheetRow(const std::string& label) {
    const auto rows = sheetRows();
    const auto it = std::find(rows.begin(), rows.end(), label);
    return it == rows.end() ? -1 : static_cast<int>(it - rows.begin());
}
std::string joined(const std::vector<std::string>& rows) {
    std::string out;
    for (const auto& r : rows) out += (out.empty() ? "" : ", ") + r;
    return out;
}
// D-pad to the labelled row (the sheet opens on its first row), then A.
bool chooseAction(Driver& d, const std::string& label) {
    const int row = sheetRow(label);
    if (row < 0) return false;
    for (int i = 0; i < row; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    return true;
}

// The file dialog (MenuType::FileBrowser: 1000 px centred, rows inset 20 px, labels 18 px in).
bool dialogShown(int generation) { return drawn("Import from file: Generation " + std::to_string(generation)); }
std::vector<std::string> dialogRows() {
    std::vector<std::string> rows;
    for (const auto& t : HostUi::visibleText)
        if (t.x == 178 && t.style == 1) rows.push_back(t.text);
    return rows;
}
constexpr int kDialogRowTouchBase = 88000;

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// Synthetic plain stored PK4 / PK5 records (PKHeX plaintext: species 25, checksum at 0x06; a PK5 has
// 0x5F >= 0x10 so PKHeX GetFormat45 reads it as Generation V). Used only as "another generation".
std::vector<uint8_t> pk45(int generation) {
    std::vector<uint8_t> p(136, 0);
    p[0] = 0x78; p[1] = 0x56; p[2] = 0x34; p[3] = 0x12;
    p[0x08] = 25;
    if (generation == 5) p[0x5F] = 0x10;
    uint32_t sum = 0;
    for (std::size_t o = 8; o < 136; o += 2) sum += static_cast<uint32_t>(p[o] | (p[o + 1] << 8));
    p[6] = static_cast<uint8_t>(sum);
    p[7] = static_cast<uint8_t>(sum >> 8);
    return p;
}

// A complete plain stored PK5 (the record tests/test_pokemon_file_gen5.cpp pk5() builds: PKHeX PK5.cs
// offsets, Pikachu with Tackle, met on a native BW route, OT ASH 12345 / 54321, checksum at 0x06).
std::vector<uint8_t> pk5() {
    namespace C = PokeVault::Integration::Gen5::Crypto;
    std::vector<uint8_t> b(C::StoredSize, 0);
    const auto put32 = [&](size_t o, uint32_t v) {
        C::write16(b, o, static_cast<uint16_t>(v));
        C::write16(b, o + 2, static_cast<uint16_t>(v >> 16));
    };
    put32(0, 0x2468ACE1u);
    C::write16(b, 0x08, 25);
    C::write16(b, 0x0C, 12345);
    C::write16(b, 0x0E, 54321);
    put32(0x10, 1000);
    b[0x14] = 70;
    b[0x15] = 9;
    b[0x17] = 2;
    b[0x18] = 4;
    C::write16(b, 0x28, 33);
    b[0x30] = 35;
    put32(0x38, 0x0012345u);
    b[0x41] = 3;
    const char16_t name[] = u"SPARK";
    for (size_t i = 0; i < 5; ++i) C::write16(b, 0x48 + i * 2, name[i]);
    C::write16(b, 0x48 + 10, 0xFFFF);
    b[0x5F] = 20;
    const char16_t ot[] = u"ASH";
    for (size_t i = 0; i < 3; ++i) C::write16(b, 0x68 + i * 2, ot[i]);
    C::write16(b, 0x68 + 6, 0xFFFF);
    b[0x7B] = 11; b[0x7C] = 3; b[0x7D] = 9;
    C::write16(b, 0x80, 4);
    b[0x83] = 4;
    b[0x84] = 5;
    C::write16(b, 0x06, C::checksum(b));
    return b;
}

// What one generation's test needs from its save.
struct Case {
    int generation = 0;
    std::string game;
    UI::TrainerViewScreen* screen = nullptr;
    // Export source: a Boxes slot, or a Party slot when exportParty.
    bool exportParty = false;
    int exportBox = 0, exportSlot = 0;
    // Import: the empty visual cell the sheet is opened on, and where the record must land.
    int importBox = 1, importCell = 0, importSlot = 0;
    // Second empty cell for the "B closes" check.
    int spareBox = 2, spareCell = 0;
    std::function<std::vector<uint8_t>()> snapshot;   // the workspace (pending changes + staged bytes)
    std::function<std::optional<uint16_t>(int box, int slot)> speciesAt;
};

std::string ext(int generation) { return std::string(PokeBank::PokemonFile::plainExtension(generation)); }

void openSheet(Driver& d, View mode, int box, int slot) {
    d.screen.goBack = false;
    d.screen.helpOverlayActive = false;
    d.screen.detailViewActive = true;
    d.screen.selectedMode = mode;
    if (mode == View::Party) d.screen.selectedPartyIndex = slot;
    else {
        d.screen.selectedBoxIndex = box;
        d.screen.selectedItemIndex = slot;
    }
    d.frame();
    d.frame();
    d.press(HidNpadButton_A);
}

void run(Case& c) {
    const std::string g = "Gen " + std::to_string(c.generation) + " " + c.game;
    Driver d{*c.screen, {}, {}};
    d.frame();
    const std::string folder = "sdmc:/switch/PokeBank-NX/pokemon/gen" + std::to_string(c.generation);

    // ---- Export (twice) ------------------------------------------------------------------------
    const auto before = c.snapshot();
    std::vector<std::string> exported;
    for (int round = 0; round < 2; ++round) {
        openSheet(d, c.exportParty ? View::Party : View::Boxes, c.exportBox, c.exportSlot);
        const auto rows = sheetRows();
        check(sheetRow("Export to file") >= 0 && sheetRow("Import from file") < 0,
              g + ": the occupied slot's Actions list Export to file (rows: " + joined(rows) + ")");
        if (round == 0 && sheetRow("Origin & Legality") >= 0)
            check(sheetRow("Export to file") == sheetRow("Origin & Legality") + 1,
                  g + ": Export to file follows Origin & Legality (shared order)");
        if (!chooseAction(d, "Export to file")) { check(false, g + ": Export to file row"); return; }
        const std::string status = d.screen.storageStatus;
        check(status.rfind("Exported to " + folder + "/", 0) == 0 && status.size() > 4 &&
                  status.substr(status.size() - 4) == ext(c.generation),
              g + ": export status names the new file (" + status + ")");
        const std::string path = status.rfind("Exported to ", 0) == 0 ? status.substr(12) : std::string{};
        exported.push_back(path);
        std::string error;
        const auto decoded = Io::loadPokemonFile(path, error);
        check(decoded && decoded->generation == c.generation && !decoded->wasEncrypted,
              g + ": the exported file decodes as a plain Generation " + std::to_string(c.generation) + " record " + error);
        check(c.snapshot() == before, g + ": Export leaves the workspace unchanged (no new pending change)");
        check(!sheetRows().size() || sheetRow("Export to file") < 0, g + ": the sheet closed after Export");
    }
    check(exported.size() == 2 && exported[0] != exported[1] &&
              exported[1].find(" (2)" + ext(c.generation)) != std::string::npos,
          g + ": a second export of the same Pokemon gets (2): " + (exported.size() > 1 ? exported[1] : ""));

    // ---- Scripted SD folder ----------------------------------------------------------------------
    const std::string mons = "sdmc:/mons" + std::to_string(c.generation);
    fs::create_directories(mons + "/ok");
    fs::copy_file(exported[0], mons + "/ok/" + fs::path(exported[0]).filename().string());
    writeFile(mons + "/big" + ext(c.generation), std::vector<uint8_t>(5000, 0x42));
    const int otherGeneration = c.generation == 4 ? 5 : 4;
    writeFile(mons + "/wrong" + ext(otherGeneration), pk45(otherGeneration));
    Utils::g_folderPreferences = {};
    check(Utils::g_folderPreferences.remember(Utils::FolderPreferences::Operation::ImportPokemonFile, mons),
          g + ": the scripted folder is the remembered import folder");

    // ---- Import: refusals, navigation, favourite, the real import ------------------------------
    openSheet(d, View::Boxes, c.importBox, c.importCell);
    check(sheetRow("Import from file") == 1 && sheetRow("Add Pokemon") == 0 && sheetRow("Export to file") < 0,
          g + ": the empty slot's Actions list Add Pokemon, then Import from file (rows: " + joined(sheetRows()) + ")");
    if (!chooseAction(d, "Import from file")) { check(false, g + ": Import from file row"); return; }
    check(dialogShown(c.generation) && drawn(mons), g + ": the file dialog opens on the remembered folder " + allTexts());
    const std::vector<std::string> expected{"..", "ok/", "big" + ext(c.generation), "wrong" + ext(otherGeneration)};
    check(dialogRows() == expected, g + ": the folder lists .., the sub-folder, then the files (" + joined(dialogRows()) + ")");
    // Round 11 H39: the B badge names what B does here (Close at the start folder, Up Folder deeper)
    // and '-' Cancel is always listed (deliberately replaces the old single "B Back" label).
    check(drawn("Open") && drawn("Favourite") && drawn("Favourites") && drawn("Close") && drawn("Cancel") &&
              !drawn("Up Folder"),
          g + ": the dialog footer names A Open / Y Favourite / X Favourites / B Close / - Cancel");

    const auto staged0 = c.snapshot();
    const std::size_t opened0 = Io::openedFileCount();
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);   // big.pkN
    check(Io::openedFileCount() == opened0, g + ": the oversized file is refused before it is opened");
    check(d.screen.storageStatus.find("5000 bytes") != std::string::npos && dialogShown(c.generation),
          g + ": the oversized refusal says why and the dialog stays open (" + d.screen.storageStatus + ")");
    check(c.snapshot() == staged0, g + ": nothing staged for the oversized file");

    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);   // wrong.pkM
    check(Io::openedFileCount() == opened0 + 1, g + ": the other generation's file is read");
    check(!d.screen.storageStatus.empty() && d.screen.storageStatus.rfind("Imported", 0) != 0 &&
              dialogShown(c.generation) && drawn(d.screen.storageStatus),
          g + ": another generation's file is refused with the editor's reason, shown in the dialog (" +
              d.screen.storageStatus + ")");
    check(c.snapshot() == staged0, g + ": nothing staged for the refused file");

    d.press(HidNpadButton_Up);
    d.press(HidNpadButton_Up);
    d.press(HidNpadButton_A);   // ok/
    check(drawn(mons + "/ok") && dialogRows().size() == 2 && dialogRows()[1] == fs::path(exported[0]).filename().string(),
          g + ": A opens the sub-folder (" + joined(dialogRows()) + ")");
    check(drawn("Up Folder") && drawn("Cancel") && !drawn("Close"),
          g + ": in a sub-folder the footer says B Up Folder and - Cancel");
    d.press(HidNpadButton_Y);
    check(Utils::g_folderPreferences.isFavorite(mons + "/ok") && drawn("Unfavourite"),
          g + ": Y marks the folder a favourite");
    d.press(HidNpadButton_X);
    check(drawn("Favourite folders") && drawn(mons + "/ok"), g + ": X lists the favourite folders");
    d.press(HidNpadButton_B);
    check(dialogShown(c.generation), g + ": B leaves the favourites list, the dialog stays open");

    const std::size_t opened1 = Io::openedFileCount();
    check(d.tap(kDialogRowTouchBase + 1), g + ": the exported file's row is tappable");
    check(Io::openedFileCount() == opened1 + 1 && !dialogShown(c.generation), g + ": the tap picked the file and closed the dialog");
    const std::string imported = d.screen.storageStatus;
    const auto species = c.speciesAt(c.importBox, c.importSlot);
    const std::string name = species ? Names::getSpeciesName(*species) : "?";
    check(species.has_value() && imported == "Imported " + name + " into Box " + std::to_string(c.importBox + 1) +
              " / Slot " + std::to_string(c.importSlot + 1) + " (staged; the source save is unchanged)",
          g + ": the record is staged in the slot and the status says so (" + imported + ")");
    check(c.snapshot() != staged0, g + ": the import is a staged change");
    check(Utils::g_folderPreferences.lastImport == mons + "/ok", g + ": the import folder is remembered");

    // Review lists it.
    d.screen.selectedMode = View::Boxes;
    d.frame();
    const auto key = Entry::footerHint(Entry::Screen::Boxes, c.generation).glyph[0] == 'Y' ? HidNpadButton_Y
                                                                                          : HidNpadButton_Plus;
    d.press(key);
    check(drawn("Review Pending Changes") &&
              drawnContaining("Box " + std::to_string(c.importBox + 1) + " / Slot " + std::to_string(c.importSlot + 1)),
          g + ": Review lists the imported slot " + allTexts());
    d.press(HidNpadButton_B);

    // B at the start folder closes the dialog; nothing staged.
    const auto staged1 = c.snapshot();
    openSheet(d, View::Boxes, c.spareBox, c.spareCell);
    if (!chooseAction(d, "Import from file")) { check(false, g + ": Import from file row (spare)"); return; }
    check(dialogShown(c.generation) && drawnContaining(mons + "/ok"), g + ": the next import opens on the remembered folder");
    d.press(HidNpadButton_B);
    check(!dialogShown(c.generation) && c.snapshot() == staged1, g + ": B at the start folder closes, nothing staged");

    // Round 11 H39: '-' cancels the import from any folder or list. Nothing is read, staged or
    // remembered, and '-' does not reach the screen's Help while the dialog is open.
    const std::string remembered = Utils::g_folderPreferences.lastImport;
    const std::size_t opened2 = Io::openedFileCount();
    const auto cancelFrom = [&](const std::string& where, const std::function<void()>& navigate,
                                const std::function<void()>& cancel) {
        openSheet(d, View::Boxes, c.spareBox, c.spareCell);
        if (!chooseAction(d, "Import from file")) { check(false, g + ": Import from file row (" + where + ")"); return; }
        navigate();
        check(dialogShown(c.generation) || drawn("Favourite folders"), g + ": dialog open before Cancel (" + where + ")");
        cancel();
        check(!dialogShown(c.generation) && !drawn("Favourite folders"), g + ": '-' closes the dialog from " + where);
        check(c.snapshot() == staged1 && Io::openedFileCount() == opened2 &&
                  Utils::g_folderPreferences.lastImport == remembered,
              g + ": Cancel from " + where + " stages nothing, opens no file, remembers no folder");
        check(!d.screen.helpOverlayActive, g + ": '-' in the dialog does not open Help (" + where + ")");
    };
    const auto minus = [&] { d.press(HidNpadButton_Minus); };
    // ".." from the start folder lands above it, where B would only go up a folder.
    cancelFrom("a folder above the start folder", [&] {
        d.press(HidNpadButton_A);
        check(drawn(mons) && drawn("Up Folder"), g + ": .. went above the start folder " + allTexts());
    }, minus);
    cancelFrom("the favourites list", [&] { d.press(HidNpadButton_X); }, minus);
    cancelFrom("a footer tap on - Cancel", [&] {}, [&] {
        check(d.tapNav(HidNpadButton_Minus), g + ": the - Cancel footer badge is tappable");
    });

    // ---- Round 11 X-IMPEXP: X on the Boxes grid ----------------------------------------------
    // The imported slot is occupied in every generation: its footer names X Export and X writes a
    // new file without touching the workspace (no sheet in between).
    const auto boxGrid = [&](int box, int slot) {
        d.screen.goBack = false;
        d.screen.helpOverlayActive = false;
        d.screen.detailViewActive = true;
        d.screen.selectedMode = View::Boxes;
        d.screen.selectedBoxIndex = box;
        d.screen.selectedItemIndex = slot;
        d.frame();
        d.frame();
    };
    const auto staged2 = c.snapshot();
    boxGrid(c.importBox, c.importSlot);
    check(drawn("Export") && !drawn("Import") && !drawn("Add"),
          g + ": an occupied box slot's footer names X Export (no X Add) " + allTexts());
    d.press(HidNpadButton_X);
    const std::string xStatus = d.screen.storageStatus;
    check(xStatus.rfind("Exported to " + folder + "/", 0) == 0 && !dialogShown(c.generation) && sheetRows().empty(),
          g + ": X on an occupied box slot exports it (" + xStatus + ")");
    check(c.snapshot() == staged2, g + ": X Export leaves the workspace unchanged");
    // An empty cell: X Import opens the same file dialog ('-' cancels it, nothing staged).
    boxGrid(c.spareBox, c.spareCell);
    check(drawn("Import") && !drawn("Export"), g + ": an empty box slot's footer names X Import " + allTexts());
    d.press(HidNpadButton_X);
    check(dialogShown(c.generation), g + ": X on an empty box slot opens Import from file, never Create");
    d.press(HidNpadButton_Minus);
    check(!dialogShown(c.generation) && c.snapshot() == staged2, g + ": cancelled X Import stages nothing");
    // Create stays reachable: the empty slot's A sheet still starts with Add Pokemon.
    openSheet(d, View::Boxes, c.spareBox, c.spareCell);
    check(sheetRow("Add Pokemon") == 0, g + ": Add Pokemon stays first on the empty slot's Actions");
    d.press(HidNpadButton_B);

    // ---- Round 11 ACTIONS: one order, Move and Review in every generation ---------------------
    openSheet(d, View::Boxes, c.importBox, c.importSlot);
    const std::vector<std::string> order{"View", "Edit", "Clone", "Move", "Release", "Origin & Legality",
                                         "Export to file", "Review Pending Changes", "Cancel"};
    check(sheetRows() == order, g + ": the occupied box Actions follow the shared order (" + joined(sheetRows()) + ")");
    if (chooseAction(d, "Move")) {
        check(d.screen.storageStatus.find("picked up") != std::string::npos && sheetRows().empty(),
              g + ": Actions -> Move picks the Pokemon up (" + d.screen.storageStatus + ")");
        d.press(HidNpadButton_B);
        check(c.snapshot() == staged2, g + ": B cancels the Move and the box is unchanged");
    } else {
        check(false, g + ": Move row");
    }
    // Party (where the save has one): Review Pending Changes is listed while changes are staged and
    // opens the shared Review.
    if (d.screen.trainer.getPartySize() > 0) {
        openSheet(d, View::Party, 0, 0);
        const auto rows = sheetRows();
        check(rows.size() >= 2 && rows[rows.size() - 2] == "Review Pending Changes" && rows.back() == "Cancel",
              g + ": the Party Actions list Review Pending Changes before Cancel (" + joined(rows) + ")");
        if (chooseAction(d, "Review Pending Changes"))
            check(drawn("Review Pending Changes") && sheetRows().empty(), g + ": Party Review opens the shared Review");
        d.press(HidNpadButton_B);
        check(c.snapshot() == staged2, g + ": opening Review from Party changes nothing");
    }
    std::cout << g << ": Export / Import from file checked\n";
}

// ---- Gen I / II / IV: synthetic saves through the real strict parsers (openBackupTrainer). ----
struct Opened {
    std::unique_ptr<Trainer::Trainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Opened> keepAlive;   // surfaces key per-screen state by address: never reuse one

Opened* openClassic(const qasynth::Bytes& bytes, const std::string& game, PokeVault::Safety::SourceKind kind,
                    const std::string& provider) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    char pattern[] = "/tmp/pokebank-file-menus-save-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, game + ": temp dir");
    if (!dir) return nullptr;
    const std::string path = std::string(dir) + "/" + game + ".sav";
    writeFile(path, bytes);
    std::string error;
    S::File file;
    check(S::read(path, dir, {}, file, error), game + ": read synthetic save: " + error);
    auto trainer = PokeVault::Legacy::openBackupTrainer(file, game, error);
    check(trainer != nullptr, game + ": strict parse + trainer: " + error);
    fs::remove_all(dir);
    if (!trainer) return nullptr;
    auto& opened = keepAlive.emplace_back();
    opened.trainer = std::move(trainer);
    opened.screen = std::make_unique<UI::TrainerViewScreen>(*opened.trainer, "Pokemon " + game, path, 0,
                                                           AccountUid{}, kind, game, provider);
    return &opened;
}

template <class Editor>
std::vector<uint8_t> classicSnapshot(const Editor* e) {
    if (!e) return {};
    const auto bytes = e->stagedBytes();
    std::vector<uint8_t> out(bytes.begin(), bytes.end());
    out.push_back(static_cast<uint8_t>(e->pendingChanges().size()));
    return out;
}

void gen1() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(qasynth::gen1::save(), "red_gb", Kind::RetroArchLegacy, "RetroArch");
    if (!opened) return;
    auto* rby = dynamic_cast<PokeVault::Legacy::RBYReadOnlyTrainer*>(opened->trainer.get());
    check(rby && rby->stagedPokemon(), "Gen 1: staged editor");
    if (!rby || !rby->stagedPokemon()) return;
    auto* e = rby->stagedPokemon();
    // The synthetic save has empty boxes: stage one Pokemon (Box 1) to export.
    std::string error;
    check(e->stageAdd(0, 0, PokeVault::Integration::Gen1::BoxPokemonCreate{}, error) &&
              rby->refreshBoxesFromStagedPokemon(error), "Gen 1: a Pokemon to export: " + error);
    Case c;
    c.generation = 1;
    c.game = "red_gb";
    c.screen = opened->screen.get();
    c.importBox = 1; c.importCell = 3; c.importSlot = 0;   // packed list: the box's first empty slot
    c.spareBox = 2; c.spareCell = 0;
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](int box, int slot) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    run(c);
}

void gen2() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(fixture(GS), "gold_gbc", Kind::RetroArchLegacy, "RetroArch");
    if (!opened) return;
    auto* gsc = dynamic_cast<PokeVault::Legacy::GSCReadOnlyTrainer*>(opened->trainer.get());
    check(gsc && gsc->stagedEditor(), "Gen 2: staged editor");
    if (!gsc || !gsc->stagedEditor()) return;
    auto* e = gsc->stagedEditor();
    Case c;
    c.generation = 2;
    c.game = "gold_gbc";
    c.screen = opened->screen.get();
    c.exportBox = 0; c.exportSlot = 0;                      // the fixture's boxed Pikachu
    c.importBox = 2; c.importCell = 4; c.importSlot = 0;   // packed list: the box's first empty slot
    c.spareBox = 3; c.spareCell = 0;
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](int box, int slot) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    run(c);
}

void gen4() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(qasynth::gen4::save(qasynth::gen4::Kind::Platinum), "platinum_nds",
                               Kind::ExternalLegacy, "DraStic");
    if (!opened) return;
    auto* g4 = dynamic_cast<PokeVault::Legacy::Gen4ReadOnlyTrainer*>(opened->trainer.get());
    check(g4 && g4->stagedPokemon(), "Gen 4: staged editor");
    if (!g4 || !g4->stagedPokemon()) return;
    auto* e = g4->stagedPokemon();
    // Box 1 / Slot 1 is exported; when the synthetic save leaves it empty, stage a Piplup there first.
    std::string error;
    if (!e->boxedPokemon(0, 0)) {
        auto draft = e->createBoxDraft(0, 0, 393, &error);
        check(draft && e->stageCreateBoxPokemon(0, 0, *draft, &error) && g4->refreshStagedPokemonPresentation(error),
              "Gen 4: a Pokemon to export: " + error);
    }
    Case c;
    c.generation = 4;
    c.game = "platinum_nds";
    c.screen = opened->screen.get();
    c.importBox = 1; c.importCell = 4; c.importSlot = 4;
    c.spareBox = 1; c.spareCell = 9;
    c.snapshot = [e] { return e->stagedBytes(); };
    c.speciesAt = [e](int box, int slot) -> std::optional<uint16_t> {
        const auto p = e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot));
        return p ? std::optional<uint16_t>(p->species()) : std::nullopt;
    };
    run(c);
}

// ---- Gen III (FireRed, one party Pikachu) and Gen V (Black, one party Pikachu). ----
struct Gen3Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::FRLGReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen3Opened> gen3Alive;

void gen3() {
    auto& out = gen3Alive.emplace_back();
    out.bytes = HostUiFixtures::gen3Fixture(0);
    auto parsed = G3::parse(out.bytes, G3::SourceGame::FireRedGBA);
    check(static_cast<bool>(parsed), "Gen III fixture parses: " + parsed.detail);
    if (!parsed) return;
    std::string error;
    out.trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    check(out.trainer != nullptr && out.trainer->stagedPokemon(), "Gen III trainer: " + error);
    if (!out.trainer || !out.trainer->stagedPokemon()) return;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon FireRed", "/host/firered.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    auto* e = out.trainer->stagedPokemon();
    Case c;
    c.generation = 3;
    c.game = "firered_gba";
    c.screen = out.screen.get();
    c.exportParty = true; c.exportSlot = 0;                // the shared Gen I-III Party sheet
    c.importBox = 0; c.importCell = 5; c.importSlot = 5;
    c.spareBox = 0; c.spareCell = 6;
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](int box, int slot) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    run(c);
}

struct Gen5Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::Gen5ReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen5Opened> gen5Alive;

void gen5() {
    auto& out = gen5Alive.emplace_back();
    out.bytes = HostUiFixtures::gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(out.bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
    check(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return;
    out.trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    check(out.trainer != nullptr, "Gen V trainer: " + error);
    if (!out.trainer) return;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon Black", "/host/black.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    auto* trainer = out.trainer.get();
    // The fixture's party Pikachu is a bare record the Gen V import rules refuse (it reads as Gen IV
    // content), so Box 1 / Slot 1 first gets a complete PK5 through the same backend Import uses.
    namespace PF = PokeBank::PokemonFile;
    const auto record = pk5();
    const auto decoded = PF::decode(record, "seed.pk5", error);
    check(decoded && G5::importPokemonFile5(trainer->stagedPokemon(), *decoded, 0, 0, &error) &&
              trainer->refreshStagedPokemonPresentation(error), "Gen V: a Pokemon to export: " + error);
    Case c;
    c.generation = 5;
    c.game = "black_nds";
    c.screen = out.screen.get();
    c.exportBox = 0; c.exportSlot = 0;
    c.importBox = 0; c.importCell = 2; c.importSlot = 2;
    c.spareBox = 0; c.spareCell = 3;
    c.snapshot = [trainer] {
        const auto& ws = trainer->stagedPokemon();
        std::vector<uint8_t> out{static_cast<uint8_t>(ws.changedRecordCount()),
                                 static_cast<uint8_t>(ws.pendingReview().size())};
        for (size_t slot = 0; slot < 6; ++slot)
            if (const auto p = ws.viewBox(0, slot); p && p->valid())
                out.insert(out.end(), p->decodedBytes().begin(), p->decodedBytes().end());
        return out;
    };
    c.speciesAt = [trainer](int box, int slot) -> std::optional<uint16_t> {
        const auto p = trainer->stagedPokemon().viewBox(static_cast<size_t>(box), static_cast<size_t>(slot));
        return p && p->valid() && !p->empty() ? std::optional<uint16_t>(p->species()) : std::nullopt;
    };
    run(c);

    // Party: Export to file writes the party record (220 bytes) and changes nothing.
    Driver d{*out.screen, {}, {}};
    const auto before = c.snapshot();
    openSheet(d, View::Party, 0, 0);
    check(chooseAction(d, "Export to file"), "Gen 5 Party: the Actions list Export to file (" + joined(sheetRows()) + ")");
    const std::string status = d.screen.storageStatus;
    const std::string path = status.rfind("Exported to ", 0) == 0 ? status.substr(12) : std::string{};
    const auto party = Io::loadPokemonFile(path, error);
    check(party && party->generation == 5 && party->party && c.snapshot() == before,
          "Gen 5 Party: the party record is exported and the workspace is unchanged (" + status + ")");
}

}  // namespace

int main() {
    // The app's sdmc:/ root is a private mkdtemp working directory (synthetic files only).
    char pattern[] = "/tmp/pokebank-file-menus-XXXXXX";
    const char* made = ::mkdtemp(pattern);
    if (!made) {
        std::cerr << "mkdtemp failed\n";
        return EXIT_FAILURE;
    }
    const std::string root = made;
    const fs::path previous = fs::current_path();
    fs::current_path(root);
    fs::create_directory("sdmc:");

    gen1();
    gen2();
    gen3();
    gen4();
    gen5();

    fs::current_path(previous);
    fs::remove_all(root);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Import from file / Export to file through the real TrainerViewScreen, Gen I-V (" << checks
              << " checks): PASS\n";
    return EXIT_SUCCESS;
}
