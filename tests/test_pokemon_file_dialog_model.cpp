// Import from file dialog model (include/UI/PokemonFileDialogModel.h) and its SD-card half
// (include/UI/PokemonFileIo.h), owner Oct 28 Feature A UI. The SD card is a private mkdtemp working
// directory holding synthetic files only; the one record exported is built here in code.
#include "UI/PokemonFileDialogModel.h"
#include "UI/PokemonFileIo.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace B = PokeBank::UIModel::PokemonFileBrowser;
namespace D = PokeBank::UIModel::PokemonFileDialog;
namespace Io = PokeBank::UIModel::PokemonFileIo;
namespace P = Utils::FolderPreferences;
namespace fs = std::filesystem;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

D::State stateAt(const std::string& folder, const std::vector<B::RawEntry>& raw, const P::Preferences& prefs,
                 const std::string& start) {
    D::State s;
    s.startFolder = start;
    D::setListing(s, B::list(folder, raw, prefs));
    return s;
}

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// A plain stored PK4 (PKHeX plaintext layout): PID, species 25, GetFormat45 says Gen IV, checksum at 0x06.
std::vector<uint8_t> pk4(uint32_t pid) {
    std::vector<uint8_t> p(136, 0);
    for (int i = 0; i < 4; ++i) p[static_cast<std::size_t>(i)] = static_cast<uint8_t>(pid >> (8 * i));
    p[0x08] = 25;
    uint32_t sum = 0;
    for (std::size_t o = 8; o < 136; o += 2) sum += static_cast<uint32_t>(p[o] | (p[o + 1] << 8));
    p[6] = static_cast<uint8_t>(sum);
    p[7] = static_cast<uint8_t>(sum >> 8);
    return p;
}

void modelControls() {
    P::Preferences prefs;
    const std::vector<B::RawEntry> raw{{"saves", true, 0}, {"Box", true, 0}, {"a.pk3", false, 80},
                                       {"huge.pk4", false, 4096}, {"b.pk5", false, 136}};
    // entries: "..", Box, saves, a.pk3, b.pk5, huge.pk4
    auto s = stateAt("sdmc:/mons", raw, prefs, "sdmc:/mons");
    check(s.listing.entries.size() == 6 && s.cursor == 0, "listing with a parent row, cursor on the first entry");
    check(D::press(s, D::Key::Up, prefs, 4).command == D::Command::None && s.cursor == 5, "Up wraps to the last entry");
    check(D::press(s, D::Key::Down, prefs, 4).command == D::Command::None && s.cursor == 0, "Down wraps to the first");
    (void)D::press(s, D::Key::PageDown, prefs, 4);
    check(s.cursor == 4 && s.first == 1, "a page down moves one page and keeps the cursor visible");
    (void)D::press(s, D::Key::PageDown, prefs, 4);
    check(s.cursor == 5, "a page clamps at the end (no wrap)");
    auto out = D::press(s, D::Key::A, prefs, 4);
    check(out.command == D::Command::RefuseFile && out.path == "sdmc:/mons/huge.pk4" &&
              out.message.find("4096 bytes") != std::string::npos && out.message.find("236") != std::string::npos,
          "A on an oversized file refuses it with its size (" + out.message + ")");
    s.cursor = 3;
    out = D::press(s, D::Key::A, prefs, 4);
    check(out.command == D::Command::PickFile && out.path == "sdmc:/mons/a.pk3", "A on a file picks it");
    s.cursor = 1;
    out = D::press(s, D::Key::A, prefs, 4);
    check(out.command == D::Command::OpenFolder && out.path == "sdmc:/mons/Box", "A on a folder opens it");
    s.cursor = 0;
    out = D::press(s, D::Key::A, prefs, 4);
    check(out.command == D::Command::OpenFolder && out.path == "sdmc:/" && out.focus == "mons", "A on .. goes up");
    out = D::press(s, D::Key::B, prefs, 4);
    check(out.command == D::Command::Close, "B at the start folder closes the dialog");
    out = D::press(s, D::Key::Y, prefs, 4);
    check(out.command == D::Command::ToggleFavorite && out.path == "sdmc:/mons", "Y toggles the current folder");

    // Deeper than the start folder: B goes up and the cursor returns to the folder just left.
    auto deep = stateAt("sdmc:/mons/Box", {}, prefs, "sdmc:/mons");
    out = D::press(deep, D::Key::B, prefs, 4);
    check(out.command == D::Command::OpenFolder && out.path == "sdmc:/mons" && out.focus == "Box", "B goes up one folder");
    D::setListing(deep, B::list(out.path, raw, prefs), out.focus);
    check(deep.cursor == 1 && deep.listing.entries[1].name == "Box", "the folder just left is focused");
    // The SD root has no parent: B closes there even when the dialog opened deeper.
    auto root = stateAt("sdmc:/", raw, prefs, "sdmc:/mons");
    check(D::press(root, D::Key::B, prefs, 4).command == D::Command::Close, "B at the SD root closes");

    // Favourites list.
    check(prefs.toggleFavorite("sdmc:/mons") && prefs.toggleFavorite("sdmc:/saves"), "two favourites");
    (void)D::press(s, D::Key::X, prefs, 4);
    check(s.mode == D::Mode::Favorites && s.favoriteCursor == 0, "X shows the favourites");
    (void)D::press(s, D::Key::Down, prefs, 4);
    out = D::press(s, D::Key::Y, prefs, 4);
    check(out.command == D::Command::ToggleFavorite && out.path == "sdmc:/saves", "Y removes the focused favourite");
    out = D::press(s, D::Key::A, prefs, 4);
    check(out.command == D::Command::OpenFolder && out.path == "sdmc:/saves" && s.mode == D::Mode::Browse,
          "A opens the focused favourite");
    (void)D::press(s, D::Key::X, prefs, 4);
    check(D::press(s, D::Key::B, prefs, 4).command == D::Command::None && s.mode == D::Mode::Browse,
          "B leaves the favourites without closing the dialog");
    P::Preferences none;
    (void)D::press(s, D::Key::X, none, 4);
    check(D::press(s, D::Key::A, none, 4).command == D::Command::None, "A in an empty favourites list does nothing");

    // An empty folder: A does nothing.
    auto empty = stateAt("sdmc:/", {}, prefs, "sdmc:/");
    check(empty.listing.entries.empty() && D::press(empty, D::Key::A, prefs, 4).command == D::Command::None,
          "A in an empty listing does nothing");

    // Status lines.
    check(D::importedStatus("Pikachu", 0, 2) ==
              "Imported Pikachu into Box 1 / Slot 3 (staged; the source save is unchanged)", "import status");
    check(D::exportedStatus("sdmc:/x.pk3") == "Exported to sdmc:/x.pk3", "export status");

    // File ids: the DV word of a Gen I / II list, the PID of a Gen III-V record.
    std::vector<uint8_t> list(69, 0);
    list[3 + 0x1B] = 0xAB; list[3 + 0x1C] = 0xCD;
    check(D::fileId(1, list.data(), list.size()) == 0xABCD, "Gen I DV word");
    std::vector<uint8_t> list2(73, 0);
    list2[3 + 0x15] = 0x12; list2[3 + 0x16] = 0x34;
    check(D::fileId(2, list2.data(), list2.size()) == 0x1234, "Gen II DV word");
    const auto rec = pk4(0x89ABCDEFu);
    check(D::fileId(4, rec.data(), rec.size()) == 0x89ABCDEFu, "Gen IV PID");
    check(D::fileId(1, list.data(), 4) == 0, "a short buffer has no id");
}

void sdCard() {
    char pattern[] = "/tmp/pokebank-file-dialog-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, "temp dir");
    if (!dir) return;
    const fs::path previous = fs::current_path();
    fs::current_path(dir);
    fs::create_directories("sdmc:/mons/sub");
    writeFile("sdmc:/mons/ok.pk4", pk4(0x01020304u));
    writeFile("sdmc:/mons/huge.pk4", std::vector<uint8_t>(5000, 0x11));
    writeFile("sdmc:/mons/notes.txt", {1, 2, 3});

    std::string error;
    const auto raw = Io::listFolder("sdmc:/mons", &error);
    check(error.empty() && raw.size() == 4, "listFolder sees every entry (" + std::to_string(raw.size()) + ")");
    const auto listing = B::list("sdmc:/mons", raw, P::Preferences{});
    check(listing.entries.size() == 4, "the browser lists .., sub, huge.pk4, ok.pk4 (no .txt)");
    check(Io::listFolder("sdmc:/missing", &error).empty(), "a missing folder lists nothing");
    check(!error.empty(), "a missing folder reports an error");

    // Oversized: refused from its size alone, never opened.
    const std::size_t opened = Io::openedFileCount();
    check(!Io::readPokemonFile("sdmc:/mons/huge.pk4", error) && error.find("5000 bytes") != std::string::npos,
          "an oversized file is refused (" + error + ")");
    check(Io::openedFileCount() == opened, "an oversized file is never opened");
    const auto decoded = Io::loadPokemonFile("sdmc:/mons/ok.pk4", error);
    check(decoded && decoded->generation == 4 && Io::openedFileCount() == opened + 1, "a valid file reads and decodes");

    // Export: a new file under the app's pokemon folder (created), never replacing one.
    const auto bytes = pk4(0x0A0B0C0Du);
    const auto first = Io::exportPokemonFile(4, "Mr. Mime", bytes);
    check(first.ok && first.path == "sdmc:/switch/PokeBank-NX/pokemon/gen4/Mr__Mime_0A0B0C0D.pk4",
          "first export path (" + first.path + first.error + ")");
    const auto second = Io::exportPokemonFile(4, "Mr. Mime", bytes);
    check(second.ok && second.path == "sdmc:/switch/PokeBank-NX/pokemon/gen4/Mr__Mime_0A0B0C0D (2).pk4",
          "a second export of the same Pokemon gets (2) (" + second.path + second.error + ")");
    std::vector<uint8_t> back;
    {
        std::ifstream in(first.path, std::ios::binary);
        back.assign(std::istreambuf_iterator<char>(in), {});
    }
    check(back == bytes, "the exported file holds exactly the record");
    std::size_t files = 0;
    for (const auto& e : fs::directory_iterator("sdmc:/switch/PokeBank-NX/pokemon/gen4")) (void)e, ++files;
    check(files == 2, "only the two exported files are in the folder (no temp / previous left)");
    // A record decode() refuses is never written.
    auto broken = bytes;
    broken[0x08] ^= 0xFF;
    const auto refused = Io::exportPokemonFile(4, "Broken", broken);
    check(!refused.ok && !fs::exists("sdmc:/switch/PokeBank-NX/pokemon/gen4/Broken_0A0B0C0D.pk4"),
          "an unverifiable record is not exported (" + refused.error + ")");

    fs::current_path(previous);
    fs::remove_all(dir);
}

}  // namespace

int main() {
    modelControls();
    sdCard();
    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Pokemon file dialog model + SD-card I/O: PASS\n";
    return EXIT_SUCCESS;
}
