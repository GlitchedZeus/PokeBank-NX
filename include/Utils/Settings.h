#ifndef UTILS_SETTINGS_H
#define UTILS_SETTINGS_H

namespace Utils {
    // Persisted app settings live in sdmc:/switch/PokeBank-NX/config/settings.cfg (simple key=value text).
    // Currently: theme (OLED Black/Dark/Light) and app safety/preferences.

    // Load settings into the runtime globals (UI::g_themeMode via applyTheme,
    // ::g_autoBackupEnabled). Safe when the file is missing — defaults are kept.
    // Call once at startup, before any screen draws.
    void loadSettings();

    // Durably replace the current settings.cfg after a setting changes.
    // Returns false if the new image could not be written, reread, validated, or promoted.
    bool saveSettings();
}

#endif
