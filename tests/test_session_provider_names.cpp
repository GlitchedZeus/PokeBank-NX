// Round 3 Part 9: a session names its source's provider, never a generic word, and Product Home
// keeps the focused game across profile changes.
// 1. Trainer / Game Browser header (TrainerViewScreen::legacyProviderLabel -> sessionSourceName):
//    mGBA, RetroArch (+ its DeSmuME core), melonDS, DraStic, DraStic savestate, Manual each get their
//    own name; an installed Switch title is "Nintendo Switch"; an unidentified label is the honest
//    "Unknown provider", never "EXTERNAL" or a RetroArch default.
// 2. Leave dialog: "PokeBank NX closes and <who> starts." — "the game" for a Switch title, the
//    provider by launch descriptor id, "the emulator" only when unidentified; every line fits.
// 3. Profile focus: re-confirming the active profile changes nothing; switching away and back
//    returns to the card that was focused, for every card kind.
// Source checks pin the call sites (header, picker guard, Games-browser avatar touch, help row).
#include "Source/SaveProviderIdentity.h"
#include "UI/LeaveDecisionLayout.h"
#include "UI/ProfileTitleFocus.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace S = PokeVault::Source;
namespace L = PokeBank::UIModel::LeaveDecisionLayout;
using PokeBank::UIModel::ProfileTitleFocus;

namespace {

std::string slurp(const char* path) {
    std::ifstream in(path);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string body(const std::string& text, const std::string& head) {
    const std::size_t at = text.find(head);
    assert(at != std::string::npos);
    std::size_t open = text.find('{', at), depth = 0, i = open;
    for (; i < text.size(); ++i) {
        if (text[i] == '{') ++depth;
        else if (text[i] == '}' && --depth == 0) break;
    }
    return text.substr(at, i - at + 1);
}

void headerNames() {
    struct Case { const char* raw; const char* shown; };
    const Case cases[] = {
        {"mGBA", "mGBA"},
        {"RetroArch", "RetroArch"},
        {"RETROARCH", "RetroArch"},                 // historical upper-cased classic label
        {"RetroArch/DeSmuME", "RetroArch (DeSmuME)"}, // RetroArch with its DeSmuME core
        {"melonDS", "melonDS"},
        {"DraStic", "DraStic"},
        {"DraStic Savestate", "DraStic (savestate)"},
        {"Tico", "Tico"},
        {"Manual", "Manual file"},
        {"Source", "Unknown provider"},             // FRLG browser placeholder
        {"Remembered", "Unknown provider"},         // remembered pre-provider binding
        {"", "Unknown provider"},
    };
    for (const auto& c : cases) {
        const std::string shown = S::sessionSourceName(c.raw, false);
        if (shown != c.shown) std::cerr << c.raw << " -> " << shown << "\n";
        assert(shown == c.shown);
        assert(shown != "EXTERNAL" && shown != "RETROARCH");
    }
    assert(S::sessionSourceName("", true) == "Nintendo Switch");
    assert(S::sessionSourceName("DraStic", true) == "Nintendo Switch");
    // Distinct providers never collapse to one name.
    assert(S::sessionSourceName("mGBA", false) != S::sessionSourceName("RetroArch", false));
    assert(S::sessionSourceName("melonDS", false) != S::sessionSourceName("RetroArch/DeSmuME", false));
}

void launchDescriptorIds() {
    using S::ProviderIdentity;
    assert(S::identifyProviderId("retroarch") == ProviderIdentity::RetroArch);
    assert(S::identifyProviderId("retroarch-desmume") == ProviderIdentity::RetroArchDeSmuME);
    assert(S::identifyProviderId("drastic") == ProviderIdentity::DraStic);
    assert(S::identifyProviderId("drastic-savestate") == ProviderIdentity::DraSticSavestate);
    assert(S::identifyProviderId("melonds") == ProviderIdentity::MelonDS);
    assert(S::identifyProviderId("mgba") == ProviderIdentity::MGBA);
    assert(S::identifyProviderId("source-choice") == ProviderIdentity::Unknown);
    assert(S::identifyProviderId("") == ProviderIdentity::Unknown);
    // Every id is exactly providerIdFor() of the discovery label it came from.
    for (const char* label : {"DraStic", "melonDS", "RetroArch", "RetroArch/DeSmuME", "mGBA", "Tico"})
        assert(S::identifyProviderId(S::providerIdFor(label)) == S::identifyProvider(label));
}

void leaveDialog() {
    assert(L::launchStarter(true, "DraStic") == "the game");
    assert(L::launchStarter(false, "DraStic") == "DraStic");
    assert(L::launchStarter(false, "RetroArch") == "RetroArch");
    assert(L::launchStarter(false, "Unknown provider") == "the emulator");
    assert(L::launchStarter(false, "") == "the emulator");
    const L::Text sw = L::build(false, "Pokemon Legends: Arceus", "", L::launchStarter(true, ""));
    assert(sw.lines[0] == "PokeBank NX closes and the game starts.");
    assert(sw.lines[0].find("emulator") == std::string::npos);
    for (const char* provider : {"DraStic", "DraStic (savestate)", "melonDS", "RetroArch", "RetroArch (DeSmuME)",
                                 "mGBA", "Tico", "Manual file"}) {
        const L::Text t = L::build(false, "Pokemon SoulSilver", "", L::launchStarter(false, provider));
        assert(t.fits);
        assert(t.lines[0] == "PokeBank NX closes and " + std::string(provider) + " starts.");
    }
}

void profileFocus() {
    ProfileTitleFocus focus;
    const std::vector<std::string> a = {"red_gb|0|1", "pearl|0|3", "|0100ABCD|0"};
    const std::vector<std::string> b = {"crystal_gbc|0|1"};
    assert(!focus.indexFor("A", a));                 // nothing remembered: first card
    focus.remember("A", a[2]);                       // Switch title focused on profile A
    focus.remember("B", b[0]);
    assert(focus.indexFor("A", a) == 2u);            // back on A: the same Switch card
    assert(focus.indexFor("B", b) == 0u);
    focus.remember("A", a[1]);                       // a Gen IV card this time
    assert(focus.indexFor("A", a) == 1u);
    assert(!focus.indexFor("A", b));                 // card no longer listed: fall back
    focus.remember("A", "");                         // nothing valid focused: forget
    assert(!focus.indexFor("A", a));
    focus.remember("", "x");                         // no profile identity: ignored
    assert(!focus.indexFor("", {"x"}));
    assert(!PokeBank::UIModel::profileChoiceChangesUser(2, 2));
    assert(PokeBank::UIModel::profileChoiceChangesUser(1, 2));
}

void callSites() {
    const std::string trainer = slurp("include/UI/TrainerViewScreenBase.h");
    const std::string label = body(trainer, "std::string legacyProviderLabel() const");
    assert(label.find("PokeVault::Source::sessionSourceName(sourceProviderLabel, false)") != std::string::npos);
    assert(label.find("EXTERNAL") == std::string::npos && label.find("RETROARCH") == std::string::npos);
    assert(trainer.find("\"External source is read-only.") == std::string::npos);

    const std::string home = slurp("src/UI/SaveSelectScreen.cpp");
    assert(home.find("profileChoiceChangesUser(profilePickerIndex, userIndex)") != std::string::npos);
    const std::string setUser = body(home, "void SaveSelectScreen::setUser(int idx)");
    assert(setUser.find("profileTitleFocus.remember(") != std::string::npos);
    assert(setUser.find("profileTitleFocus.indexFor(") != std::string::npos);
    // Games browser: the avatar's headerRects are hit-tested inside the classic block, before L/R.
    const std::size_t classic = home.find("        if (classicGamesActive) {\n            if (kDown & HidNpadButton_B) {");
    const std::size_t lr = home.find("if (kDown & HidNpadButton_L) setUser(userIndex - 1);", classic);
    const std::size_t avatar = home.find("if (r.idx == 0 && tappedHit(r)) {", classic);
    assert(classic != std::string::npos && avatar != std::string::npos && lr != std::string::npos && avatar < lr);
    // The L/R profile help row exists only with more than one profile.
    const std::size_t row = home.find("rows.push_back({\"L/R\", \"Switch profile\"});");
    assert(row != std::string::npos && home.rfind("if (users.size() > 1)", row) + 40 > row);
    // The leave dialog says who starts; no literal generic consequence line remains.
    assert(home.find("quitting, pendingLaunchName, pendingLaunchNotice, pendingLaunchStarter)") != std::string::npos);
    assert(slurp("include/UI/LeaveDecisionLayout.h").find("\"PokeBank NX closes and the emulator starts.\"") ==
           std::string::npos);
}

} // namespace

int main() {
    headerNames();
    launchDescriptorIds();
    leaveDialog();
    profileFocus();
    callSites();
    std::cout << "Part 9: each provider (mGBA, RetroArch + DeSmuME core, melonDS, DraStic, Switch title) named "
                 "in the header and leave dialog; profile focus kept across switches; Games-browser avatar "
                 "touch PASS\n";
}
