// Pokemon file browser model (include/UI/PokemonFileBrowser.h), owner Oct 28 Feature A UI.
#include "UI/PokemonFileBrowser.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace B = PokeBank::UIModel::PokemonFileBrowser;
namespace P = Utils::FolderPreferences;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
}  // namespace

int main() {
    P::Preferences prefs;
    // Listing: parent first, folders then files, A-Z case-insensitive; hidden, unsupported and
    // odd names left out; an oversized file is listed but flagged.
    const std::vector<B::RawEntry> raw{
        {"zeta.pk3", false, 80},  {"Alpha.PK4", false, 136}, {"notes.txt", false, 10}, {".hidden.pk5", false, 136},
        {"saves", true, 0},       {"Banks", true, 0},        {".git", true, 0},         {"big.pk5", false, 4096},
        {"mew.ek5", false, 220},  {"red.pk1", false, 69},    {"weird.ek1", false, 69},  {"", false, 0},
    };
    const auto l = B::list("sdmc://pokemon//", raw, prefs);
    check(l.folder == "sdmc:/pokemon", "the folder is canonical (" + l.folder + ")");
    std::vector<std::string> names;
    for (const auto& e : l.entries) names.push_back(e.name);
    const std::vector<std::string> expected{"..", "Banks", "saves", "Alpha.PK4", "big.pk5", "mew.ek5", "red.pk1", "zeta.pk3"};
    check(names == expected, "listing order and filtering");
    for (const auto& e : l.entries)
        if (e.name == "big.pk5") check(e.tooLarge, "an oversized file is flagged");
        else check(!e.tooLarge, "regular files are not flagged: " + e.name);
    check(!l.truncated && !l.favorite, "short, not a favourite");
    // The SD root has no parent row.
    const auto root = B::list("sdmc:/", raw, prefs);
    check(!root.entries.empty() && root.entries.front().name != "..", "the root lists no parent");

    // Navigation never leaves the SD card.
    check(B::parentOf("sdmc:/a/b") == std::optional<std::string>("sdmc:/a"), "parent of a/b");
    check(B::parentOf("sdmc:/a") == std::optional<std::string>("sdmc:/"), "parent of a is the root");
    check(!B::parentOf("sdmc:/"), "the root has no parent");
    check(B::childOf("sdmc:/a", "b") == std::optional<std::string>("sdmc:/a/b"), "child");
    check(!B::childOf("sdmc:/a", "..") && !B::childOf("sdmc:/a", "x/y") && !B::childOf("sdmc:/a", ""),
          "no traversal through a child name");

    // Truncation.
    std::vector<B::RawEntry> many;
    for (int i = 0; i < 700; ++i) many.push_back({"f" + std::to_string(i) + ".pk5", false, 136});
    const auto big = B::list("sdmc:/x", many, prefs);
    check(big.truncated && big.entries.size() == B::kMaxEntries, "a huge folder is cut and says so");

    // Favourite flag.
    check(prefs.toggleFavorite("sdmc:/Pokemon"), "favourite added");
    check(B::list("sdmc:/pokemon", raw, prefs).favorite, "favourite matched case-insensitively (FAT)");

    // Start folder: remembered import folder if present, else the app folder, else the root.
    std::set<std::string> present{"sdmc:/switch/PokeBank-NX/pokemon"};
    const auto exists = [&](const std::string& p) { return present.count(p) > 0; };
    check(B::startFolder(prefs, exists) == "sdmc:/switch/PokeBank-NX/pokemon", "app folder by default");
    check(prefs.remember(P::Operation::ImportPokemonFile, "sdmc:/my/mons"), "remember import folder");
    check(B::startFolder(prefs, exists) == "sdmc:/switch/PokeBank-NX/pokemon", "a missing remembered folder is skipped");
    present.insert("sdmc:/my/mons");
    check(B::startFolder(prefs, exists) == "sdmc:/my/mons", "the remembered folder when present");
    present.clear();
    P::Preferences none;
    check(B::startFolder(none, exists) == "sdmc:/", "the root when nothing exists");

    // Export names: filename-safe, never overwrite.
    check(B::exportFolder(3) == "sdmc:/switch/PokeBank-NX/pokemon/gen3", "export folder");
    check(B::baseName("Mr. Mime", 0x1234ABCDu) == "Mr__Mime_1234ABCD", "base name (" + B::baseName("Mr. Mime", 0x1234ABCDu) + ")");
    check(B::baseName("Flabébé", 1) == "Flabb_00000001", "non-ASCII letters are left out of the file name");
    check(B::baseName("", 0) == "Pokemon_00000000", "empty species");
    std::set<std::string> files{"sdmc:/x/Pikachu_00000001.pk3", "sdmc:/x/Pikachu_00000001 (2).pk3"};
    const auto has = [&](const std::string& p) { return files.count(p) > 0; };
    check(B::freeExportPath("sdmc:/x", "Pikachu_00000001", ".pk3", has) ==
              std::optional<std::string>("sdmc:/x/Pikachu_00000001 (3).pk3"), "first free name");
    check(B::freeExportPath("sdmc:/x", "Eevee_00000002", ".pk4", has) ==
              std::optional<std::string>("sdmc:/x/Eevee_00000002.pk4"), "no clash, plain name");
    const auto always = [](const std::string&) { return true; };
    check(!B::freeExportPath("sdmc:/x", "A", ".pk5", always), "gives up rather than overwrite");

    if (failures) { std::cerr << failures << " file browser check(s) failed\n"; return 1; }
    std::cout << "Pokemon file browser model: listing, navigation, start folder, export naming PASS\n";
    return 0;
}
