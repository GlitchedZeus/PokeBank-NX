#include "Utils/PokeBankPaths.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

int main() {
    using namespace PokeBank::Paths;
    assert(root() == "sdmc:/switch/PokeBank-NX");
    assert(exportsRoot() == root() + "/exports");
    assert(gen2ExportsRoot() == root() + "/exports/gen2");
    assert(backupsRoot() == root() + "/backups");
    assert(configRoot() == root() + "/config");
    assert(logsRoot() == root() + "/logs");
    assert(legacyBankRoot() == root() + "/banks/legacy-pkse");
    assert(settingsFile() == root() + "/config/settings.cfg");
    assert(legacySourceBindingsFile() == root() + "/config/legacy_source_bindings.cfg");
    assert(traceFile() == root() + "/logs/trace.log");

    assert(isOwnedPath(root()));
    assert(isOwnedPath(gen2ExportsRoot()));
    assert(!isOwnedPath("sdmc:/PKSE"));
    assert(!isOwnedPath("sdmc:/switch/PokeBank-NX-evil/file"));

    assert(isSafeComponent("gold_gbc"));
    assert(!isSafeComponent("../PKSE"));
    assert(!isSafeComponent("a/b"));
    assert(child(root(), "safe") == root() + "/safe");
    assert(child(root(), "../escape").empty());
    assert(sanitizeComponent("Pokemon Gold/../bad") == "Pokemon_Gold_.._bad");
    assert(sanitizeComponent("..") == ".._");
    assert(isSafeTimestamp("20260912_173200"));
    assert(!isSafeTimestamp("../../escape000"));
    assert(gen2ExportDirectory("gold_gbc", "20260912_173200") ==
 root() + "/exports/gen2/gold_gbc_20260912_173200");
    assert(gen2ExportDirectory("../gold", "20260912_173200") ==
 root() + "/exports/gen2/.._gold_20260912_173200");
    assert(gen2ExportDirectory("gold_gbc", "bad").empty());

    const std::filesystem::path base = std::filesystem::temp_directory_path() /
        ("pokebank-path-test-" + std::to_string(static_cast<long long>(getpid())));
    std::filesystem::remove_all(base);
    std::string error;
    assert(ensureDirectoryTree((base / "switch/PokeBank-NX/config").string(), &error));
    assert(std::filesystem::is_directory(base / "switch/PokeBank-NX/config"));
    assert(ensureDirectoryTree((base / "switch/PokeBank-NX/config").string(), &error));

    std::ofstream blocker(base / "file");
    blocker << "x";
    blocker.close();
    error.clear();
    assert(!ensureDirectoryTree((base / "file/child").string(), &error));
    assert(!error.empty());
    assert(!std::filesystem::exists(base / "PKSE"));
    std::filesystem::remove_all(base);
    std::cout << "PokeBank path ownership tests passed\n";
}
