// Owner hardware finding (2026-11-01): Gen VI-IX titles showed a second "(Save File)" / "... File"
// card beside the installed game (Shining Pearl File, Legends Arceus File, Scarlet File, Violet File,
// Shield File, Brilliant Diamond File), with Poke Ball art, "Choose save", and no path from the
// installed card into the shared editor.
//
// This test drives the REAL SaveSelectScreen (Product Home) on the host harness against a SCRIPTED
// console (tests/host-ui/switch.h, HostUi::consoleSaves) and REAL synthetic native saves for all 18
// modern titles (tests/fixtures/*), assigned through the real Modern::assignValidatedSource path.
//
// Reproduction: the pre-fix loadModernCards() appended one ModernAssignedFile card for EVERY modern
// id, including Switch ids whose installed SwitchTitle card already existed, so an installed Shining
// Pearl produced two cards. `oneCardPerGame` below fails on that code (checked by reverting
// loadModernCards locally); it passes on the fix. Input failures beyond that (e.g. "A did nothing")
// are exercised here through update(), but the harness cannot mount a console save, so the
// installed read-only snapshot itself is proven only up to the selection handed to UIManager.
//
// Evidence class: SYNTHETIC host UI. The harness has one account; "another profile" is modelled by
// console saves and bindings owned by a second uid that must never surface for the owner.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Globals.h"
#include "Games/GameIdentity.h"
#include "Integration/Modern/ModernSources.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen8a/Gen8aSyntheticSave.h"
#include "fixtures/gen8b/Gen8bSyntheticSave.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"
#include "fixtures/gen9za/Gen9ZaSyntheticSave.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace Modern = PokeVault::Integration::Modern;
using Screen = UI::SaveSelectScreen;
using Kind = Screen::SelectedSourceKind;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

constexpr u64 kOwner[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
constexpr u64 kOther[2] = {0x1111111111111111ULL, 0x2222222222222222ULL};

std::string profileOf(const u64 (&uid)[2]) {
    std::ostringstream o;
    o << std::hex << std::setfill('0') << std::setw(16) << uid[0] << std::setw(16) << uid[1];
    return o.str();
}

struct Title {
    std::string id;
    std::function<std::vector<uint8_t>()> make;
};

std::vector<Title> titles() {
    namespace L6 = PokeVault::Integration::Gen6::Layout;
    namespace L7 = PokeVault::Integration::Gen7::Layout;
    using G6 = Gen6Fixture::SaveSpec;
    using G7 = Gen7Fixture::SaveSpec;
    return {
        {"x_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionX}); }},
        {"y_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionY}); }},
        {"omegaruby_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionOmegaRuby}); }},
        {"alphasapphire_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionAlphaSapphire}); }},
        {"sun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionSun}); }},
        {"moon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionMoon}); }},
        {"ultrasun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraSun}); }},
        {"ultramoon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraMoon}); }},
        {"letsgo_pikachu_switch", [] { return Gen7bFixture::makeSave(); }},
        {"letsgo_eevee_switch", [] { Gen7bFixture::Options o; o.version = 43; return Gen7bFixture::makeSave(o); }},
        {"sword_switch", [] { return Gen8Fixture::makeSave(); }},
        {"shield_switch", [] { Gen8Fixture::Options o; o.version = 45; return Gen8Fixture::makeSave(o); }},
        {"brilliant_diamond_switch", [] { return Gen8bFixture::makeSave(); }},
        {"shining_pearl_switch", [] { Gen8bFixture::Options o; o.romCode = 1; return Gen8bFixture::makeSave(o); }},
        {"legends_arceus_switch", [] { return Gen8aFixture::makeSave(); }},
        {"scarlet_switch", [] { return Gen9SvFixture::makeSave(); }},
        {"violet_switch", [] { Gen9SvFixture::Options o; o.game = 51; return Gen9SvFixture::makeSave(o); }},
        {"legends_za_switch", [] { return Gen9ZaFixture::makeSave(); }},
    };
}

bool isSwitch(const std::string& id) { return Modern::sourceVariantFor(id) == Modern::kSourceVariantSwitchMain; }
u64 switchTitleId(const std::string& id) {
    const auto* g = PokeVault::Games::findGame(id);
    return g ? g->switchTitleId : 0;
}

void addConsoleSave(u64 title, const u64 (&user)[2]) {
    auto& e = HostUi::consoleSaveEntries[HostUi::consoleSaveCount];
    e = {};
    e.application_id = title;
    e.save_data_type = FsSaveDataType_Account;
    e.uid[0] = user[0];
    e.uid[1] = user[1];
    HostUi::consoleSaveSpace[HostUi::consoleSaveCount++] = FsSaveDataSpaceId_User;
}

void resetConsole() {
    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
}

std::vector<uint8_t> slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void spit(const std::string& p, const std::vector<uint8_t>& b) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
}

// One private working directory (the app's sdmc:/ root) per scenario.
struct Sandbox {
    fs::path previous, dir;
    Sandbox() {
        char templ[] = "/tmp/pokebank-modern-cards-XXXXXX";
        const char* made = ::mkdtemp(templ);
        check(made != nullptr, "sandbox directory");
        previous = fs::current_path();
        dir = made ? made : "";
        fs::current_path(dir);
        fs::create_directories("sdmc:/switch/PokeBank-NX");
        fs::create_directories(dir / "exports");
    }
    ~Sandbox() {
        fs::current_path(previous);
        fs::remove_all(dir);
    }
    std::string bindingsPath() const { return (dir / "sdmc:/switch/PokeBank-NX/bindings-test.tsv").string(); }
    std::string exportPath(const std::string& id) const { return (dir / "exports" / (id + ".sav")).string(); }
};

struct Driver {
    explicit Driver(Screen& s) : screen(s) {}
    Screen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
    void frames(int n) { for (int i = 0; i < n; ++i) frame(); }
    bool drawn(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t.find(text) != std::string::npos) return true;
        return false;
    }
};

size_t cardsFor(const Screen& s, const std::string& id) {
    size_t n = 0;
    for (const auto& c : s.cardsForInspection()) n += c.gameId == id;
    return n;
}
const Screen::CardInspection* card(const Screen& s, const std::string& id, std::vector<Screen::CardInspection>& keep) {
    keep = s.cardsForInspection();
    for (const auto& c : keep)
        if (c.gameId == id) return &c;
    return nullptr;
}
bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// The one-card rule and the "no File card" rule for the whole HOME grid.
void oneCardPerGame(const Screen& s, const std::vector<Title>& all, const std::string& where,
                    const std::function<bool(const std::string&)>& expectCard) {
    for (const auto& c : s.cardsForInspection()) {
        check(c.name.find("(Save File)") == std::string::npos && !endsWith(c.name, " File") &&
                  !endsWith(c.label, " File"),
              where + ": no separate File card (" + c.name + " / " + c.label + ")");
        if (Modern::isModernAssignedId(c.gameId))
            check(c.sourceKind == Kind::ModernAssignedFile, where + ": " + c.gameId + " is the one shared modern card kind");
    }
    for (const auto& t : all) {
        const size_t n = cardsFor(s, t.id);
        check(n == (expectCard(t.id) ? 1u : 0u),
              where + ": " + t.id + " has " + std::to_string(n) + " card(s), expected " + (expectCard(t.id) ? "1" : "0"));
    }
}

// --------------------------------------------------------------------------------------------
// Scenario 1: every Switch title installed for this profile, nothing assigned.
// --------------------------------------------------------------------------------------------
void installedNothingAssigned(const std::vector<Title>& all) {
    Sandbox box;
    resetConsole();
    for (const auto& t : all)
        if (isSwitch(t.id)) {
            addConsoleSave(switchTitleId(t.id), kOwner);
            addConsoleSave(switchTitleId(t.id), kOwner);  // a second save index: still one card
        }
    addConsoleSave(switchTitleId("scarlet_switch"), kOther); // another profile: no effect on the owner
    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    PokeVault::Legacy::LegacySourceBindings bindings(box.bindingsPath());
    Screen screen(legacy, bindings);
    Driver d{screen};
    d.frame();
    oneCardPerGame(screen, all, "installed", [](const std::string&) { return true; });

    for (const auto& t : all) {
        std::vector<Screen::CardInspection> keep;
        const auto* c = card(screen, t.id, keep);
        if (!c) continue;
        const std::string w = "installed/" + t.id;
        if (isSwitch(t.id)) {
            check(c->titleId == switchTitleId(t.id), w + ": the card carries the installed title id (cover art, Launch)");
            check(c->sourceLabel == "INSTALLED SAVE", w + ": source label INSTALLED SAVE, got " + c->sourceLabel);
        } else {
            check(c->titleId == 0, w + ": 3DS card has no Switch title id");
            check(c->sourceLabel == "CHOOSE SAVE", w + ": 3DS card shows Choose Save, got " + c->sourceLabel);
        }

        // Focus and sorting: the focus lands on exactly this card.
        check(screen.focusGameForInspection(t.id), w + ": focusable");
        check(screen.cardsForInspection()[static_cast<size_t>(screen.focusedIndexForInspection())].gameId == t.id,
              w + ": focus is on the game");
        d.frame();
        check(d.drawn(isSwitch(t.id) ? "Installed save. A opens it read-only" : "Choose Save: no save is assigned yet"),
              w + ": HOME explains what A does");
        check(d.drawn("Save"), w + ": HOME footer offers X Save");
        const auto launch = screen.launchForInspection().state;
        if (isSwitch(t.id))
            check(launch != UI::GameLaunchState::Unavailable, w + ": installed Switch game keeps a Launch target");
        else
            check(launch == UI::GameLaunchState::Unavailable, w + ": 3DS game offers no Launch");

        // A: installed -> read-only snapshot of the installed save in the shared editor; 3DS -> Save Instances.
        d.press(HidNpadButton_A);
        if (isSwitch(t.id)) {
            check(screen.hasSelectedTitle(), w + ": A opens the game");
            check(screen.getSelectedSourceKind() == Kind::ModernInstalledSnapshot,
                  w + ": A opens the installed save as a read-only snapshot");
            check(screen.getSelectedTitleId() == switchTitleId(t.id) && screen.getSelectedGameId() == t.id,
                  w + ": the exact installed game is handed to the shared editor");
            check(HostUi::consoleSaves.mountsRefused == 0, w + ": selection itself mounts nothing");
            screen.resumeAfterEditor();                 // return HOME after editing / discarding
            d.frame();
            check(screen.overlayForInspection() == "None" && !screen.hasSelectedTitle(), w + ": back on HOME");
            check(cardsFor(screen, t.id) == 1, w + ": still one card after returning HOME");
            check(screen.cardsForInspection()[static_cast<size_t>(screen.focusedIndexForInspection())].gameId == t.id,
                  w + ": focus stays on the game after returning HOME");
        } else {
            check(!screen.hasSelectedTitle(), w + ": A does not open an unassigned 3DS game");
            check(screen.overlayForInspection() == "SaveInstancesSetup" || screen.overlayForInspection() == "SaveInstances",
                  w + ": A opens Save Instances, got " + std::string(screen.overlayForInspection()));
            check(screen.noticeForInspection().find("No save is assigned to") != std::string::npos,
                  w + ": Save Instances explains why: " + screen.noticeForInspection());
            d.press(HidNpadButton_B);
            check(screen.overlayForInspection() == "None", w + ": B returns to HOME");
        }

        // X: change / assign the save under the same card.
        check(screen.focusGameForInspection(t.id), w + ": refocus");
        d.press(HidNpadButton_X);
        check(screen.overlayForInspection() == "SaveInstancesSetup" || screen.overlayForInspection() == "SaveInstances",
              w + ": X opens Save Instances under this card, got " + std::string(screen.overlayForInspection()));
        check(!screen.hasSelectedTitle(), w + ": X never opens the editor");
        d.press(HidNpadButton_B);
        check(screen.overlayForInspection() == "None", w + ": B cancels Save Instances");
        check(cardsFor(screen, t.id) == 1, w + ": Save Instances never adds a card");

        // ZR: Launch only for an installed game.
        check(screen.focusGameForInspection(t.id), w + ": refocus for ZR");
        d.press(HidNpadButton_ZR);
        if (isSwitch(t.id)) {
            check(screen.overlayForInspection() == "LaunchDecision", w + ": ZR opens the launch decision, got " +
                                                                          std::string(screen.overlayForInspection()));
            d.press(HidNpadButton_B);
            check(screen.overlayForInspection() == "None" && !screen.hasRequestedAppExit(), w + ": B stays in PokeBank NX");
        } else {
            check(screen.overlayForInspection() == "None" && !screen.hasRequestedAppExit(), w + ": ZR on a 3DS game launches nothing");
        }

        // The Games grid (Y) shows the same one card; grid A matches HOME A.
        check(screen.focusGameForInspection(t.id), w + ": refocus for grid");
        d.press(HidNpadButton_Y);
        d.frames(40);                                   // the grid slides in before it takes input
        check(screen.overlayForInspection() == "GamesDrawer", w + ": Y opens the Games grid");
        d.press(HidNpadButton_A);
        if (isSwitch(t.id)) {
            check(screen.getSelectedSourceKind() == Kind::ModernInstalledSnapshot && screen.getSelectedGameId() == t.id,
                  w + ": grid A opens the same installed snapshot; overlay " + std::string(screen.overlayForInspection()) +
                      " selected=" + std::to_string(screen.hasSelectedTitle()) + " game=" + screen.getSelectedGameId());
            screen.resumeAfterEditor();
        } else {
            check(screen.overlayForInspection() == "SaveInstancesSetup" || screen.overlayForInspection() == "SaveInstances",
                  w + ": grid A opens Save Instances");
            d.press(HidNpadButton_B);
        }
        d.frame();
    }
    check(HostUi::consoleSaves.mountsRefused == 0, "installed: the host never attempted a save mount from HOME");
}

// --------------------------------------------------------------------------------------------
// Scenario 2: nothing installed, nothing assigned: Switch titles have no card, 3DS titles one each.
// --------------------------------------------------------------------------------------------
void nothingInstalled(const std::vector<Title>& all) {
    Sandbox box;
    resetConsole();
    for (const auto& t : all)
        if (isSwitch(t.id)) addConsoleSave(switchTitleId(t.id), kOther); // installed for ANOTHER profile only
    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    PokeVault::Legacy::LegacySourceBindings bindings(box.bindingsPath());
    Screen screen(legacy, bindings);
    Driver d{screen};
    d.frame();
    oneCardPerGame(screen, all, "not-installed", [](const std::string& id) { return !isSwitch(id); });
}

// --------------------------------------------------------------------------------------------
// Scenario 3: a validated exported save assigned to every title (installed or not), then the
// assignment goes missing, corrupted, changed; another profile's assignment never surfaces.
// --------------------------------------------------------------------------------------------
void assignedSaves(const std::vector<Title>& all) {
    Sandbox box;
    const std::string owner = profileOf(kOwner), other = profileOf(kOther);
    std::vector<std::vector<uint8_t>> originals;
    {
        PokeVault::Legacy::LegacySourceBindings bindings(box.bindingsPath());
        for (const auto& t : all) {
            originals.push_back(t.make());
            spit(box.exportPath(t.id), originals.back());
            const auto row = Modern::inspectSourceFile(box.exportPath(t.id), "Exported copy", t.id);
            check(row.ready(), t.id + ": fixture validates as a Save Instance: " + row.diagnostic);
            const auto r = Modern::assignValidatedSource(bindings, row, owner, t.id);
            check(r.success, t.id + ": assigned to the owner: " + r.diagnostic);
        }
    }
    resetConsole();
    // Half the Switch titles are installed as well: still ONE card, now showing the assigned save.
    size_t i = 0;
    for (const auto& t : all)
        if (isSwitch(t.id) && (i++ % 2 == 0)) addConsoleSave(switchTitleId(t.id), kOwner);

    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    PokeVault::Legacy::LegacySourceBindings bindings(box.bindingsPath());
    check(bindings.load(), "assigned: bindings reload");
    Screen screen(legacy, bindings);
    Driver d{screen};
    d.frame();
    oneCardPerGame(screen, all, "assigned", [](const std::string&) { return true; });

    for (size_t k = 0; k < all.size(); ++k) {
        const auto& t = all[k];
        const std::string w = "assigned/" + t.id;
        std::vector<Screen::CardInspection> keep;
        const auto* c = card(screen, t.id, keep);
        if (!c) continue;
        check(c->linkedSave && c->sourceLabel.rfind("REMEMBERED", 0) == 0,
              w + ": the card shows the assigned save, got " + c->sourceLabel);
        check(screen.focusGameForInspection(t.id), w + ": focusable");
        d.frame();
        d.press(HidNpadButton_A);
        check(screen.hasSelectedTitle() && screen.getSelectedSourceKind() == Kind::ModernAssignedFile &&
                  screen.getSelectedGameId() == t.id,
              w + ": A opens the assigned validated save in the shared editor");
        check(screen.getSelectedTitleId() == 0, w + ": the assigned save, not the installed one, is opened");
        screen.resumeAfterEditor();
        d.frame();
        check(cardsFor(screen, t.id) == 1 && screen.overlayForInspection() == "None", w + ": back on HOME, one card");
        check(slurp(box.exportPath(t.id)) == originals[k], w + ": the assigned original is byte-identical");
    }

    // Missing / corrupted / changed sources: A never opens another save, it opens Save Instances
    // with the reason; the card is still the one card.
    const std::string missing = "shining_pearl_switch", corrupt = "scarlet_switch", changed = "y_3ds";
    fs::remove(box.exportPath(missing));
    {
        auto bytes = slurp(box.exportPath(corrupt));
        bytes[bytes.size() / 2] ^= 0x5A;
        spit(box.exportPath(corrupt), bytes);
    }
    // The file is replaced by a different, internally valid save of ANOTHER game (X in Y's place).
    spit(box.exportPath(changed), slurp(box.exportPath("x_3ds")));
    // Another profile's assignment of a fresh file must not change the owner's card.
    {
        const auto otherPath = (box.dir / "exports" / "other-violet.sav").string();
        spit(otherPath, all[16].make());
        const auto row = Modern::inspectSourceFile(otherPath, "Exported copy", "violet_switch");
        (void)Modern::assignValidatedSource(bindings, row, other, "violet_switch");
    }
    for (const auto& id : {missing, corrupt, changed}) {
        const std::string w = "stale/" + id;
        // The HOME catalog was built before the file changed; A re-validates the exact bytes.
        check(screen.focusGameForInspection(id), w + ": focusable");
        d.frame();
        d.press(HidNpadButton_A);
        check(!screen.hasSelectedTitle(), w + ": a failing assignment opens no editor (never the installed save)");
        check(screen.overlayForInspection() == "SaveInstancesSetup" || screen.overlayForInspection() == "SaveInstances",
              w + ": A opens Save Instances, got " + std::string(screen.overlayForInspection()));
        check(screen.noticeForInspection().find("No save is assigned") == std::string::npos,
              w + ": the reason names the failing assignment, not 'nothing assigned'");
        d.press(HidNpadButton_B);
        if (screen.overlayForInspection() != "None") d.press(HidNpadButton_B);
        d.frame();
        check(cardsFor(screen, id) == 1, w + ": still one card");
    }
    // A fresh HOME (next launch / profile switch) shows the failing assignment on the same one card.
    Screen fresh(legacy, bindings);
    Driver f{fresh};
    f.frame();
    oneCardPerGame(fresh, all, "stale-fresh", [](const std::string&) { return true; });
    for (const auto& id : {missing, corrupt, changed}) {
        const std::string w = "stale-fresh/" + id;
        std::vector<Screen::CardInspection> keep;
        const auto* c = card(fresh, id, keep);
        check(c && c->sourceLabel != "REMEMBERED" && c->sourceLabel != "CHOOSE SAVE" && c->sourceLabel != "INSTALLED SAVE",
              w + ": the card shows the assignment problem, got " + (c ? c->sourceLabel : std::string("no card")));
        check(fresh.focusGameForInspection(id), w + ": focusable");
        f.frame();
        // MISSING says so on HOME; a file that still exists but no longer validates (RELINK SAVE)
        // shows the exact validation failure on HOME instead of a trainer or party.
        const std::string reason = id == missing ? "The assigned save needs attention"
                                 : id == corrupt ? "SHA-256 does not match" : "reassign the source explicitly";
        if (!f.drawn(reason) && std::getenv("HOSTUI_DUMP"))
            for (const auto& t : HostUi::texts) std::cerr << "   | " << t << '\n';
        check(f.drawn(reason), w + ": HOME states why the assigned save cannot open (" +
                                   (c ? c->sourceLabel : std::string()) + ")");
        const auto launch = fresh.launchForInspection().state;
        if (!isSwitch(id)) check(launch == UI::GameLaunchState::Unavailable, w + ": 3DS game offers no Launch");
    }
    std::vector<Screen::CardInspection> keep;
    const auto* violet = card(fresh, "violet_switch", keep);
    check(violet && violet->linkedSave && violet->sourceLabel.rfind("REMEMBERED", 0) == 0,
          "other profile: the owner's Violet card still shows the owner's save");
}
} // namespace

int main() {
    const auto all = titles();
    check(all.size() == 18, "18 modern titles");
    size_t registered = 0;
    for (const auto& g : PokeVault::Games::allGameDescriptors()) registered += Modern::isModernAssignedId(g.id);
    check(registered == 18, "the game catalog registers exactly the 18 modern titles");
    for (const auto& t : all)
        check(!isSwitch(t.id) || switchTitleId(t.id) != 0, t.id + ": Switch title id known");

    installedNothingAssigned(all);
    nothingInstalled(all);
    assignedSaves(all);

    std::cout << "Modern game cards (real SaveSelectScreen, scripted console, 18 titles): " << checks
              << " checks, " << failures << " failure(s). SYNTHETIC evidence only.\n";
    return failures == 0 ? 0 : 1;
}
