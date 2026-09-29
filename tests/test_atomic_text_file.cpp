#include "Utils/AtomicTextFile.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

static std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

static void writeText(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out << text;
    out.close();
    assert(out);
}

int main() {
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-settings-atomic-" + std::to_string(static_cast<long long>(getpid())));
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path cfg = root / "settings.cfg";

    const std::string oldText = "theme=oled\nautoBackup=1\n";
    const std::string newText = "theme=light\nautoBackup=0\n";

    writeText(cfg, oldText);
    assert(Utils::AtomicTextFile::replace(cfg.string(), newText));
    assert(readText(cfg) == newText);
    assert(!fs::exists(cfg.string() + ".tmp"));
    assert(!fs::exists(cfg.string() + ".previous"));

    for (const auto hooks : {
             Utils::AtomicTextFile::Hooks{true, false, false, false},
             Utils::AtomicTextFile::Hooks{false, true, false, false},
             Utils::AtomicTextFile::Hooks{false, false, true, false},
             Utils::AtomicTextFile::Hooks{false, false, false, true},
         }) {
        writeText(cfg, oldText);
        assert(!Utils::AtomicTextFile::replace(cfg.string(), newText, hooks));
        assert(readText(cfg) == oldText);
        assert(!fs::exists(cfg.string() + ".tmp"));
    }

    // Simulate power loss after old-final rotation and before promotion.
    fs::remove(cfg);
    writeText(cfg.string() + ".previous", oldText);
    assert(Utils::AtomicTextFile::recoverPreviousIfNeeded(cfg.string()));
    assert(readText(cfg) == oldText);
    assert(!fs::exists(cfg.string() + ".previous"));

    // A complete final always wins; stale previous evidence is not promoted over it.
    writeText(cfg, newText);
    writeText(cfg.string() + ".previous", oldText);
    assert(Utils::AtomicTextFile::recoverPreviousIfNeeded(cfg.string()));
    assert(readText(cfg) == newText);

    fs::remove_all(root);
    std::cout << "Atomic settings replacement: PASS\n";
}
