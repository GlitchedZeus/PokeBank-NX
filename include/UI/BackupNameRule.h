#ifndef POKEBANK_UI_BACKUP_NAME_RULE_H
#define POKEBANK_UI_BACKUP_NAME_RULE_H
// The one rule for a typed "New backup" folder name (Round 14 P1, SHARED_UX_REQUIREMENTS.md §1).
//
// The name becomes a folder on the SD card, so it is kept to characters that are safe on FAT32 and
// cannot escape the profile/game directory ('/' or ".."): ASCII letters, digits, space, '-' and
// '_'. The prompt used to DROP every other character silently, so "Run #2 (ÉVA)" quietly became
// "Run 2 VA". A name with anything else is now refused with the reason, and nothing is created;
// the user retypes it. Pure: no filesystem access, host-testable.
#include <cstddef>
#include <string>

namespace PokeBank::UIModel {

struct BackupNameVerdict {
    bool ok = false;
    std::string folder;   // the exact folder name to create (== the typed name) when ok
    std::string reason;   // one user-facing sentence when refused
};

[[nodiscard]] inline bool backupNameCharacterAllowed(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == ' ' || c == '-' || c == '_';
}

[[nodiscard]] inline BackupNameVerdict backupFolderName(const std::string& typed) {
    BackupNameVerdict verdict;
    if (typed.empty()) {
        verdict.reason = "Enter a name for the new backup.";
        return verdict;
    }
    for (const char c : typed) {
        if (!backupNameCharacterAllowed(c)) {
            verdict.reason = "Backup names can use only letters A-Z, numbers, spaces, - and _. "
                             "Nothing was created.";
            return verdict;
        }
    }
    if (typed.front() == ' ' || typed.back() == ' ') {
        verdict.reason = "A backup name cannot start or end with a space. Nothing was created.";
        return verdict;
    }
    verdict.ok = true;
    verdict.folder = typed;
    return verdict;
}

/// The SUGGESTED name the prompt opens with: our own title text reduced to allowed characters,
/// so the default the user sees is a name the rule accepts. Never applied to typed text.
[[nodiscard]] inline std::string backupNameSuggestion(const std::string& title) {
    std::string out;
    for (const char c : title)
        if (backupNameCharacterAllowed(c) && !(c == ' ' && (out.empty() || out.back() == ' ')))
            out += c;
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out.empty() ? std::string("Backup") : out;
}

}  // namespace PokeBank::UIModel

#endif
