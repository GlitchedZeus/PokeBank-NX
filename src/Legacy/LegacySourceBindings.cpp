#include "Legacy/LegacySourceBindings.h"

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

namespace PokeVault::Legacy {
    namespace {
        char hexDigit(unsigned value) noexcept {
            return static_cast<char>(value < 10 ? '0' + value : 'a' + value - 10);
        }

        int hexValue(char value) noexcept {
            if (value >= '0' && value <= '9') return value - '0';
            if (value >= 'a' && value <= 'f') return value - 'a' + 10;
            if (value >= 'A' && value <= 'F') return value - 'A' + 10;
            return -1;
        }

        std::string hexEncode(std::string_view value) {
            std::string result;
            result.reserve(value.size() * 2);
            for (unsigned char byte : value) {
                result.push_back(hexDigit(byte >> 4));
                result.push_back(hexDigit(byte & 0x0F));
            }
            return result;
        }

        bool hexDecode(std::string_view value, std::string& output) {
            if ((value.size() & 1) != 0) return false;
            output.clear();
            output.reserve(value.size() / 2);
            for (size_t index = 0; index < value.size(); index += 2) {
                const int high = hexValue(value[index]);
                const int low = hexValue(value[index + 1]);
                if (high < 0 || low < 0) return false;
                output.push_back(static_cast<char>((high << 4) | low));
            }
            return !output.empty();
        }
    }

    LegacySourceBindings::LegacySourceBindings(std::string storagePath)
        : storagePath_(std::move(storagePath)) {}

    bool LegacySourceBindings::load() {
        owners_.clear();
        if (storagePath_.empty()) return false;
        FILE* file = std::fopen(storagePath_.c_str(), "rb");
        if (!file) return true; // First run: a missing binding file is an empty, valid database.

        char line[4096];
        bool valid = true;
        while (std::fgets(line, sizeof(line), file)) {
            std::string row(line);
            while (!row.empty() && (row.back() == '\n' || row.back() == '\r')) row.pop_back();
            if (row.empty() || row.front() == '#') continue;
            const size_t separator = row.find('\t');
            if (separator == std::string::npos || row.find('\t', separator + 1) != std::string::npos) {
                valid = false;
                continue;
            }
            std::string source;
            std::string profile;
            if (!hexDecode(std::string_view(row).substr(0, separator), source) ||
                !hexDecode(std::string_view(row).substr(separator + 1), profile)) {
                valid = false;
                continue;
            }
            owners_.insert_or_assign(std::move(source), std::move(profile));
        }
        if (std::ferror(file)) valid = false;
        if (std::fclose(file) != 0) valid = false;
        return valid;
    }

    bool LegacySourceBindings::save() const {
        if (storagePath_.empty()) return false;
        const std::string temporary = storagePath_ + ".tmp";
        FILE* file = std::fopen(temporary.c_str(), "wb");
        if (!file) return false;

        bool valid = std::fputs("# PokeBank NX legacy source bindings v1\n", file) >= 0;
        std::vector<std::pair<std::string, std::string>> ordered(owners_.begin(), owners_.end());
        std::sort(ordered.begin(), ordered.end());
        for (const auto& [source, profile] : ordered) {
            const std::string row = hexEncode(source) + "\t" + hexEncode(profile) + "\n";
            if (valid && std::fwrite(row.data(), 1, row.size(), file) != row.size()) valid = false;
        }
        if (std::fflush(file) != 0) valid = false;
        if (std::fclose(file) != 0) valid = false;
        if (!valid || std::rename(temporary.c_str(), storagePath_.c_str()) != 0) {
            std::remove(temporary.c_str());
            return false;
        }
        return true;
    }

    bool LegacySourceBindings::assign(std::string_view sourceIdentity,
                                      std::string_view profileIdentity) {
        if (sourceIdentity.empty() || profileIdentity.empty()) return false;
        owners_.insert_or_assign(std::string(sourceIdentity), std::string(profileIdentity));
        return true;
    }

    bool LegacySourceBindings::unassign(std::string_view sourceIdentity) {
        return owners_.erase(std::string(sourceIdentity)) != 0;
    }

    bool LegacySourceBindings::isAssigned(std::string_view sourceIdentity) const {
        return owners_.contains(std::string(sourceIdentity));
    }

    bool LegacySourceBindings::isVisibleTo(std::string_view sourceIdentity,
                                           std::string_view profileIdentity) const {
        const auto found = owners_.find(std::string(sourceIdentity));
        return found != owners_.end() && found->second == profileIdentity;
    }

    std::string LegacySourceBindings::assignedProfile(std::string_view sourceIdentity) const {
        const auto found = owners_.find(std::string(sourceIdentity));
        return found == owners_.end() ? std::string{} : found->second;
    }
}
