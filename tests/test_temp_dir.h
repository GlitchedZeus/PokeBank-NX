// Fresh per-run scratch directories for host tests.
//
// A name built from getpid() alone is reused when the OS recycles a PID, and a test that does not
// clear it first then meets a previous run's files (a stale move transaction made engine.prepare()
// refuse on an otherwise green gate). mkdtemp() creates a new, empty directory with a random suffix,
// so every run starts clean. removeTempFamily() deletes that directory and its "<base>-x" / "<base>.x"
// siblings that some tests derive from it.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace PokeBankTest {

inline std::string freshTempBase(const std::string& stem) {
    std::string pattern = (std::filesystem::temp_directory_path() / (stem + "-XXXXXX")).string();
    if (::mkdtemp(pattern.data()) == nullptr) {
        std::perror("mkdtemp");
        std::abort();
    }
    return pattern;
}

inline void removeTempFamily(const std::string& base) {
    namespace fs = std::filesystem;
    const fs::path path(base);
    const std::string name = path.filename().string();
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(path.parent_path(), ec)) {
        const std::string candidate = entry.path().filename().string();
        if (candidate == name || candidate.rfind(name + "-", 0) == 0 || candidate.rfind(name + ".", 0) == 0)
            fs::remove_all(entry.path(), ec);
    }
}

} // namespace PokeBankTest
