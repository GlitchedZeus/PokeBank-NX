#ifndef UTILS_ATOMIC_TEXT_FILE_H
#define UTILS_ATOMIC_TEXT_FILE_H

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace Utils::AtomicTextFile {

struct Hooks {
    bool failWrite = false;
    bool failFlush = false;
    bool failClose = false;
    bool failPromote = false;
};

inline bool fileExists(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

inline bool readExact(const std::string& path, std::string& out) {
    out.clear();
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[512];
    while (true) {
        const size_t n = std::fread(buf, 1, sizeof(buf), f);
        if (n) out.append(buf, n);
        if (n < sizeof(buf)) {
            if (std::ferror(f)) { std::fclose(f); return false; }
            break;
        }
    }
    return std::fclose(f) == 0;
}

// App-owned narrow file transaction:
//  1) write + flush + close a sibling temp,
//  2) byte-for-byte readback,
//  3) rotate prior final,
//  4) promote temp,
//  5) restore prior final if promotion fails.
// Hooks exist only so host regression tests can deterministically exercise failure paths.
inline bool replace(const std::string& finalPath, std::string_view text,
                    Hooks hooks = {}) {
    if (finalPath.empty()) return false;
    const std::string temp = finalPath + ".tmp";
    const std::string previous = finalPath + ".previous";

    (void)std::remove(temp.c_str());

    FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) return false;

    bool ok = true;
    const size_t expected = text.size();
    size_t written = 0;
    if (!hooks.failWrite && expected != 0)
        written = std::fwrite(text.data(), 1, expected, f);
    if (hooks.failWrite || written != expected) ok = false;
    if (hooks.failFlush || std::fflush(f) != 0) ok = false;
    const int closeResult = std::fclose(f);
    if (hooks.failClose || closeResult != 0) ok = false;

    std::string verify;
    if (ok && (!readExact(temp, verify) || verify != text)) ok = false;
    if (!ok) {
        (void)std::remove(temp.c_str());
        return false;
    }

    const bool hadFinal = fileExists(finalPath);
    if (hadFinal) {
        (void)std::remove(previous.c_str());
        if (std::rename(finalPath.c_str(), previous.c_str()) != 0) {
            (void)std::remove(temp.c_str());
            return false;
        }
    }

    const bool promoted = !hooks.failPromote &&
        std::rename(temp.c_str(), finalPath.c_str()) == 0;
    if (!promoted) {
        if (hadFinal) (void)std::rename(previous.c_str(), finalPath.c_str());
        (void)std::remove(temp.c_str());
        return false;
    }

    if (hadFinal) (void)std::remove(previous.c_str());
    return true;
}

// If an interruption happened after rotating the prior file but before promotion,
// restore the prior authoritative generation. A complete final always wins.
inline bool recoverPreviousIfNeeded(const std::string& finalPath) {
    if (finalPath.empty() || fileExists(finalPath)) return true;
    const std::string previous = finalPath + ".previous";
    if (!fileExists(previous)) return true;
    return std::rename(previous.c_str(), finalPath.c_str()) == 0;
}

} // namespace Utils::AtomicTextFile

#endif
