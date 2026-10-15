// PokeBank NX application QA: helpers for export-safety and source-immutability contracts
// (Thread 4, QA-owned). Host-only; uses temporary directories, never real saves.
#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace qacontract {

namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;

inline int& failures() {
    static int count = 0;
    return count;
}

// Contract checks record every violation and keep going so one run reports all of them.
#define QA_EXPECT(cond, what)                                                                  \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            ++::qacontract::failures();                                                        \
            std::fprintf(stderr, "CONTRACT VIOLATION [%s] %s (%s:%d)\n", ::qacontract::currentCase().c_str(), \
                         std::string(what).c_str(), __FILE__, __LINE__);                       \
        }                                                                                      \
    } while (0)

inline std::string& currentCase() {
    static std::string name = "setup";
    return name;
}

inline Bytes slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), {});
}

inline void spit(const fs::path& path, std::span<const uint8_t> bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// Byte content plus size and nanosecond mtime of one file.
struct FileState {
    Bytes bytes;
    off_t size = -1;
    int64_t mtimeSec = 0, mtimeNsec = 0;
    bool operator==(const FileState&) const = default;
};
inline FileState fileState(const fs::path& path) {
    FileState s;
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return s;
    if (S_ISREG(st.st_mode)) s.bytes = slurp(path);
    s.size = st.st_size;
    s.mtimeSec = st.st_mtim.tv_sec;
    s.mtimeNsec = st.st_mtim.tv_nsec;
    return s;
}

// Recursive snapshot of a directory: relative path -> state ("<dir>" entries for directories).
using TreeState = std::map<std::string, FileState>;
inline TreeState treeState(const fs::path& root) {
    TreeState out;
    if (!fs::exists(root)) return out;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        const auto rel = fs::relative(entry.path(), root).string();
        if (entry.is_directory()) out[rel + "/<dir>"] = {};
        else out[rel] = fileState(entry.path());
    }
    return out;
}

// A synthetic "source" save on disk with a back-dated mtime, so a rewrite in the same clock tick
// is still detectable.
inline void writeSource(const fs::path& path, std::span<const uint8_t> bytes) {
    fs::create_directories(path.parent_path());
    spit(path, bytes);
    struct timespec times[2] = {{1000000000, 0}, {1000000000, 0}};
    ::utimensat(AT_FDCWD, path.c_str(), times, 0);
}

struct Range {
    size_t begin = 0, end = 0;  // [begin, end)
};
// Every changed byte must fall inside one of the whitelisted ranges. Returns the first offender.
inline bool onlyWhitelistedChanges(std::span<const uint8_t> before, std::span<const uint8_t> after,
                                   const std::vector<Range>& allowed, size_t* offender = nullptr,
                                   size_t* changed = nullptr) {
    if (before.size() != after.size()) return false;
    size_t count = 0;
    for (size_t i = 0; i < before.size(); ++i) {
        if (before[i] == after[i]) continue;
        ++count;
        const bool ok = std::any_of(allowed.begin(), allowed.end(),
                                    [&](const Range& r) { return i >= r.begin && i < r.end; });
        if (!ok) {
            if (offender) *offender = i;
            if (changed) *changed = count;
            return false;
        }
    }
    if (changed) *changed = count;
    return true;
}

inline std::string hex(size_t value) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "0x%zX", value);
    return buffer;
}

inline fs::path scratchRoot(const std::string& label) {
    const auto root = fs::temp_directory_path() / ("pokebank-qa-" + label + "-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

} // namespace qacontract
