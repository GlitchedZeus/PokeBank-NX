#pragma once

// ONE save gate in front of every game workspace, Generation I-V (Round 14, owner H01 / R02 on
// 9c301c48: "i shouldnt be able to open a game save or the editor thing if the save isnt selected
// first ... it should pop up with the choose save file thing or say no save accessible" and "it
// shouldnt even go to that menu it should have the file menu pop up to assign a save since there
// is none").
//
// Product Home asks this before it opens ANY workspace surface for a game card: the loaded-game hub
// (Overview / Party / Boxes / Items / Trainer / Editor / Create), the "+" Game Workspace menu that
// lists those destinations, Items from the dock, Quick Games and the full Games browser. Every one
// of those entries goes through SaveSelectScreen::selectCurrentTitle / selectCurrentTitleForItems /
// the "+" menu, and each asks decide() with what the generation's own validator reported:
//
//   ConsoleManaged  Nintendo Switch savedata the console enumerated for this profile. The console
//                   owns it; the real open still mounts and parses it (unchanged).
//   DiscoveredFile  Gen I-III emulator battery saves (RetroArch / mGBA / ...) and the FireRed /
//                   LeafGreen HOME forwarders that open them: a card has one only when a strict
//                   parse found a READY save for that exact game and profile.
//   AssignedFile    Gen IV and Gen V DS saves: an explicit profile + exact-game assignment that
//                   must resolve AND pass strict validation of the exact bytes in this open.
//
// Anything else never opens a workspace: Product Home opens the save chooser for that game instead
// (Save Instances, then the save-file browser when nothing was found), with one plain sentence.
// Pure: no I/O. Generation-specific codecs/validation stay underneath; this is the shared rule.

#include <cstdint>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::SaveOpenGate {

enum class Source : uint8_t { ConsoleManaged, DiscoveredFile, AssignedFile };
enum class Assignment : uint8_t { None, Ready, Missing, Unreadable, Ambiguous };
enum class Validation : uint8_t { NotChecked, Valid, Invalid };
enum class Action : uint8_t { OpenWorkspace, ChooseSave };

struct Input {
    Source source = Source::AssignedFile;
    Assignment assignment = Assignment::None;
    Validation validation = Validation::NotChecked;
    std::string_view gameTitle;   // "White", "Crystal" ... (empty: "this game")
};

struct Decision {
    Action action = Action::ChooseSave;
    std::string message;          // empty when the workspace may open
    [[nodiscard]] bool open() const noexcept { return action == Action::OpenWorkspace; }
};

[[nodiscard]] inline std::string gameName(std::string_view title) {
    return title.empty() ? std::string("this game") : "Pokémon " + std::string(title);
}

[[nodiscard]] inline Decision decide(const Input& in) {
    if (in.source == Source::ConsoleManaged) return {Action::OpenWorkspace, {}};
    const std::string name = gameName(in.gameTitle);
    switch (in.assignment) {
        case Assignment::None:
            return {Action::ChooseSave, "No save assigned for " + name + ". Choose a save file."};
        case Assignment::Missing:
            return {Action::ChooseSave, "The save for " + name + " is missing or moved. Choose a save file."};
        case Assignment::Unreadable:
            return {Action::ChooseSave, "The save for " + name + " can't be read. Choose a save file."};
        case Assignment::Ambiguous:
            return {Action::ChooseSave, "More than one save is remembered for " + name + ". Choose a save file."};
        case Assignment::Ready:
            break;
    }
    if (in.validation != Validation::Valid)
        return {Action::ChooseSave, "The save for " + name + " did not pass validation. Choose a save file."};
    return {Action::OpenWorkspace, {}};
}

// The save-file browser lists folders and only files a battery-save reader can inspect: raw
// .sav / .srm and footer-declared .dsv containers. Savestates (.dss, .state, .st0...) are never
// saves and are not offered. Case-insensitive.
[[nodiscard]] inline bool saveFileNameOffered(std::string_view name) noexcept {
    const auto dot = name.find_last_of('.');
    if (dot == std::string_view::npos || name.size() - dot != 4) return false;
    char ext[3];
    for (int i = 0; i < 3; ++i) {
        char c = name[dot + 1 + static_cast<size_t>(i)];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        ext[i] = c;
    }
    const std::string_view e(ext, 3);
    return e == "sav" || e == "srm" || e == "dsv";
}

} // namespace PokeBank::UIModel::SaveOpenGate
