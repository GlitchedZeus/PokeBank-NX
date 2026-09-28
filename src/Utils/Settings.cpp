#include "Utils/Settings.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "Globals.h"
#include "UI/Common.h"
#include "Utils/Logger.h"
#include "Utils/DurableFile.h"
#include "Utils/PokeBankPaths.h"

namespace Utils {
    static std::string settingsPath() {
        return PokeBank::Paths::settingsFile();
    }

    void loadSettings() {
        FILE* f = fopen(settingsPath().c_str(), "r");
        if (!f) return;  // no config yet -> keep compiled-in defaults

        char line[128];
        while (fgets(line, sizeof(line), f)) {
            // Trim the trailing newline / carriage return.
            char* nl = strpbrk(line, "\r\n");
            if (nl) *nl = '\0';

            char* eq = strchr(line, '=');
            if (!eq) continue;
            *eq = '\0';
            const char* key = line;
            const char* val = eq + 1;

            if (strcmp(key, "theme") == 0) {
                UI::applyTheme(UI::themeModeFromKey(val));
            } else if (strcmp(key, "autoBackup") == 0) {
                g_autoBackupEnabled = (strcmp(val, "0") != 0);
            } else if (strcmp(key, "allowIllegal") == 0) {
                g_allowIllegalEdits = (strcmp(val, "0") != 0);
            } else if (strcmp(key, "moveWarn") == 0 || strcmp(key, "lgpeMoveWarn") == 0) {
                // "lgpeMoveWarn" is the old key for the same toggle, still read so an existing
                // settings.cfg doesn't silently revert the user's choice to the default. Only the
                // new key is written back, so it ages out on the first save.
                g_moveWarn = (strcmp(val, "0") != 0);
            } else if (strcmp(key, "injectToGame") == 0) {
                // Legacy PKSE setting. PokeBank NX intentionally ignores it: live title writes
                // are a compile-time policy lock, not a user-configurable option.
            } else if (strcmp(key, "debugLogging") == 0) {
                g_debugLogging = (strcmp(val, "0") != 0);
            }
        }
        fclose(f);
    }

    bool saveSettings() {
        std::string pathError;
        if (!PokeBank::Paths::ensureConfigRoot(&pathError)) {
            logErrorToFile("Failed to create PokeBank NX config directory", pathError.c_str());
            return false;
        }

        const std::string_view themeKey = UI::themeModeKey(UI::g_themeMode);
        std::string text;
        text.reserve(160);
        text += "theme=";
        text.append(themeKey.data(), themeKey.size());
        text += "\n";
        text += "autoBackup=" + std::to_string(g_autoBackupEnabled ? 1 : 0) + "\n";
        text += "allowIllegal=" + std::to_string(g_allowIllegalEdits ? 1 : 0) + "\n";
        text += "moveWarn=" + std::to_string(g_moveWarn ? 1 : 0) + "\n";
        // Keep writing the legacy key as zero so older builds also default to the safe state if the
        // same SD card is used, but never read it as authority in PokeBank NX.
        text += "injectToGame=0\n";
        text += "debugLogging=" + std::to_string(g_debugLogging ? 1 : 0) + "\n";

        const std::vector<uint8_t> bytes(text.begin(), text.end());
        const auto validator = [text](std::span<const uint8_t> candidate, std::string& error) {
            if (candidate.size() != text.size() ||
                !std::equal(candidate.begin(), candidate.end(), text.begin())) {
                error = "settings reread differs from requested configuration";
                return false;
            }
            const std::string reread(candidate.begin(), candidate.end());
            if (reread.find("injectToGame=0\n") == std::string::npos) {
                error = "settings safety lock is missing";
                return false;
            }
            return true;
        };

        const auto durable = PokeBank::Storage::DurableFile::replace(
            settingsPath(), std::span<const uint8_t>(bytes.data(), bytes.size()), validator);
        if (!durable.ok) {
            logErrorToFile("Failed to persist settings durably", durable.error.c_str());
            return false;
        }
        return true;
    }
}
