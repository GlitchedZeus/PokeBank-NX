#include "Utils/Settings.h"
#include "Utils/SettingsFile.h"
#include "Utils/AtomicTextFile.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "Globals.h"
#include "UI/Common.h"
#include "Utils/Logger.h"
#include "Utils/PokeBankPaths.h"

namespace Utils {
    static std::string settingsPath() {
        return PokeBank::Paths::settingsFile();
    }

    void loadSettings() {
        // Recover an interrupted promotion only when the authoritative file is absent.
        // A complete settings.cfg always wins over transaction artifacts.
        if (!AtomicTextFile::recoverPreviousIfNeeded(settingsPath())) {
            logErrorToFile("Failed to recover previous settings generation", settingsPath().c_str());
        }
        FILE* f = fopen(settingsPath().c_str(), "r");
        if (!f) return;  // no config yet -> keep compiled-in defaults

        // Read the whole (bounded) file and parse it line by line, never in fixed-size chunks: a
        // long line is handled whole, so its tail can never be read as another setting.
        std::string text;
        char chunk[512];
        size_t got = 0;
        while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
            text.append(chunk, got);
            if (text.size() > SettingsFile::kMaxFile) break;
        }
        fclose(f);
        if (text.size() > SettingsFile::kMaxFile) {
            logErrorToFile("Settings file is too large; defaults kept", settingsPath().c_str());
            return;
        }

        for (const auto& entry : SettingsFile::parse(text)) {
            const char* key = entry.key.c_str();
            const char* val = entry.value.c_str();

            if (strcmp(key, "theme") == 0) {
                UI::applyTheme(UI::themeModeFromKey(val));
            } else if (strcmp(key, "autoBackup") == 0) {
                // Retired setting. Opening a save never makes a backup now; backups are made only
                // by the "Create backup now" rows. Old settings.cfg files may still carry the key.
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
        text += "\nallowIllegal=";
        text += g_allowIllegalEdits ? "1" : "0";
        text += "\nmoveWarn=";
        text += g_moveWarn ? "1" : "0";
        // Keep writing the legacy key as zero so older builds also default to the safe state if the
        // same SD card is used, but never read it as authority in PokeBank NX.
        text += "\ninjectToGame=0";
        text += "\ndebugLogging=";
        text += g_debugLogging ? "1" : "0";
        text += "\n";

        if (!AtomicTextFile::replace(settingsPath(), text)) {
            logErrorToFile("Failed to replace settings file safely", settingsPath().c_str());
            return false;
        }
        return true;
    }
}
