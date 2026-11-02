// R2-BUG-001 parity for Gen V (Lane C fixed the Gen I-IV Trainer footer in cccec27b):
// the Gen V Trainer page follows the same footer rules.
//   * "A: Edit" appears only while the FOCUSED row is editable in this session (Name / Money /
//     Play Time); the Pokedex row says "A: Open Pokedex"; inside the Pokedex list there is no A;
//   * a new-game save (no trainer written yet; beginEdit refuses) never advertises A: Edit;
//   * B (Back) is always there;
//   * the page draws its own footer through the shared glyph renderer (drawNavBar), never a
//     read-only override, and the device surface uses the session-aware footer(ws).
#include "Integration/Gen5/Gen5TrainerScreen.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace T = PokeVault::Integration::Gen5::TrainerLayout5;
using Screen = G::Gen5TrainerScreen;
using Row = Screen::Row;

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

// Synthetic SAV5 (primary copy). trainer=false leaves PlayerData as a fresh new game:
// no name, TID = SID = 0, play time 0:00:00 (Gen5TrainerName.h newGameNoTrainer5).
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version, bool trainer) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t player = G::trainerBlock5(family, T::PlayerBlockIndex).offset;
    const size_t misc = G::trainerBlock5(family, T::MiscBlockIndex).offset;
    if (trainer) {
        C::write16(sav, player + 0x04, 'N');
        C::write16(sav, player + 0x06, 'X');
        C::write16(sav, player + 0x08, 0xFFFF);
        C::write16(sav, player + 0x14, 12345);
        C::write16(sav, player + 0x16, 54321);
        C::write16(sav, player + 0x24, 12);
        sav[player + 0x26] = 34;
        sav[player + 0x27] = 56;
    } else {
        C::write16(sav, player + 0x04, 0xFFFF);
    }
    sav[player + 0x1E] = 2;
    sav[player + 0x1F] = version;
    C::write16(sav, misc + 0x00, 1000);
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

bool has(const std::vector<Screen::FooterEntry>& footer, std::string_view glyph, std::string_view label = {}) {
    return std::any_of(footer.begin(), footer.end(), [&](const auto& e) {
        return glyph == e.glyph && (label.empty() || label == e.label);
    });
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "missing " << path << '\n';
        std::exit(EXIT_FAILURE);
    }
    return {std::istreambuf_iterator<char>(in), {}};
}

void exercise(G::SaveFamily family, std::string_view game, uint8_t version) {
    const std::string tag = std::string(game) + ": ";
    std::string error;
    {
        const auto sav = fixture(family, version, true);
        const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
        check(parsed.has_value(), tag + "trainer fixture parses (" + error + ")");
        if (!parsed) return;
        G::Gen5StagedPokemonWorkspace ws(*parsed);
        check(ws.trainerPresentation() && !Screen::newGame(*ws.trainerPresentation()), tag + "a trainer exists");
        Screen screen;
        check(screen.enter(ws, error), tag + "Trainer page opens (" + error + ")");
        // Walk every focusable row: A: Edit exactly when the focused row is editable.
        for (size_t step = 0; step < Screen::kRows.size(); ++step) {
            const Row row = screen.selectedRow();
            const auto footer = screen.footer(ws);
            const std::string name = Screen::label(row);
            check(has(footer, "B", "Back"), tag + name + ": B Back always shown");
            if (Screen::editable(row))
                check(has(footer, "A", "Edit") && screen.beginEdit(ws, error),
                      tag + name + ": editable focused row shows A: Edit and A edits");
            else
                check(!has(footer, "A", "Edit") && !screen.beginEdit(ws, error),
                      tag + name + ": non-editable focused row never shows A: Edit");
            if (row == Row::Pokedex) {
                check(has(footer, "A", "Open Pokedex"), tag + "Pokedex row: A opens the list");
                if (screen.openPokedex(ws, error)) {
                    check(!has(screen.footer(ws), "A") && has(screen.footer(ws), "B", "Back"),
                          tag + "inside the Pokedex list: no A, B Back");
                    screen.dex().close();
                }
            }
            screen.moveRow(1);
        }
        for (size_t i = 0; i < Screen::kRows.size(); ++i)
            if (!Screen::focusable(Screen::kRows[i]))
                check(!screen.tapRow(i), tag + "read-only rows are never focused");
    }
    {
        const auto sav = fixture(family, version, false);
        const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
        check(parsed.has_value(), tag + "new-game fixture parses (" + error + ")");
        if (!parsed) return;
        G::Gen5StagedPokemonWorkspace ws(*parsed);
        const auto t = ws.trainerPresentation();
        check(t && Screen::newGame(*t), tag + "the fixture is a new-game save (no trainer yet)");
        Screen screen;
        if (!screen.enter(ws, error)) {
            // A page that does not open shows no Edit at all.
            check(!has(screen.footer(ws), "A", "Edit"), tag + "new game: unopened page has no A: Edit");
            return;
        }
        for (size_t step = 0; step < Screen::kRows.size(); ++step) {
            const auto footer = screen.footer(ws);
            check(!has(footer, "A", "Edit") && has(footer, "B", "Back"),
                  tag + "new game: " + Screen::label(screen.selectedRow()) + " never advertises A: Edit");
            if (Screen::editable(screen.selectedRow()))
                check(!screen.beginEdit(ws, error), tag + "new game: A is refused, matching the footer");
            screen.moveRow(1);
        }
    }
}

} // namespace

int main() {
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21);
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22);

    // The device page: its own footer, session-aware, through the shared glyph renderer.
    const auto surface = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto begin = surface.find("void drawTrainerEditor(");
    const auto body = begin == std::string::npos ? std::string{} : surface.substr(begin, surface.find("\n} // namespace", begin) - begin);
    check(!body.empty(), "drawTrainerEditor exists");
    check(body.find("for(const auto& entry:editor.footer(ws))hints.push_back({entry.glyph,entry.label});") !=
                  std::string::npos &&
              body.find("editor.footer())") == std::string::npos,
          "the Trainer page uses the session-aware footer(ws) everywhere");
    check(body.find("drawNavBar(fb,hints);") != std::string::npos,
          "the footer goes through the shared glyph renderer (drawNavBar)");
    check(body.find("readOnlyForFooterOverride") == std::string::npos &&
              body.find("ReadOnlyFooter") == std::string::npos,
          "no read-only override replaces the Trainer page footer");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen V Trainer footer parity with Gen I-IV (R2-BUG-001, B/W/B2/W2, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
