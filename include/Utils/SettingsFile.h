#pragma once
// Bounded, whole-line parsing of settings.cfg (key=value per line).
//
// The loader used to read fixed 128-byte chunks with fgets, so a longer line was split and its tail
// parsed as a separate line (PKSE f4fd79a raised the buffer to 1024 for folder paths). Here a line is
// always handled whole: an over-long line or value is dropped as a unit and never truncated or split,
// keys are plain identifiers, and control bytes refuse the line. Repeated keys are kept in order.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Utils::SettingsFile {

inline constexpr std::size_t kMaxLine = 4096;   // a whole line, key included
inline constexpr std::size_t kMaxKey = 64;
inline constexpr std::size_t kMaxValue = 1024;  // room for a deep SD-card folder path
inline constexpr std::size_t kMaxFile = 64 * 1024;

struct Entry {
    std::string key;
    std::string value;
};

inline bool plainKey(std::string_view key) noexcept {
    if (key.empty() || key.size() > kMaxKey) return false;
    for (char c : key)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    return true;
}

inline std::vector<Entry> parse(std::string_view text) {
    std::vector<Entry> out;
    if (text.size() > kMaxFile) text = {};  // an oversized file is not a settings file
    while (!text.empty()) {
        const std::size_t nl = text.find('\n');
        std::string_view line = text.substr(0, nl);
        text = nl == std::string_view::npos ? std::string_view{} : text.substr(nl + 1);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.size() > kMaxLine) continue;
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos) continue;
        const std::string_view key = line.substr(0, eq), value = line.substr(eq + 1);
        if (!plainKey(key) || value.size() > kMaxValue) continue;
        bool clean = true;
        for (unsigned char c : value) if (c < 0x20 || c == 0x7F) { clean = false; break; }
        if (!clean) continue;
        out.push_back({std::string(key), std::string(value)});
    }
    return out;
}

}  // namespace Utils::SettingsFile
