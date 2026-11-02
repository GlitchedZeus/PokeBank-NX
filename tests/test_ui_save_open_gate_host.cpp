// Round 14 (owner H01 / R02 on 9c301c48, P0). Verbatim:
//  H01 "gen 5 i dont have a save for white 2 on purpose, i shouldnt be able to open a game save or
//      the editor thing if the save isnt selected first, like i dont have a save for pokemon white
//      but im able to click A on the main card and it opens the editor with parrty storage and all
//      that, it should pop up with the choose save file thing or say no save accessible"
//  R02 "it shouldnt even go to that menu it should have the file menu pop up to assign a save since
//      there is none"
//
// Through the REAL SaveSelectScreen (Product Home) on the host harness with one scripted account:
// for every Gen IV and Gen V game card x {no save assigned, assigned to a sibling game's save,
// assigned to a junk file, assigned to a valid save}:
//   - A on the card opens the workspace ONLY for the valid save; otherwise it opens the save-file
//     chooser for that game (never the intermediate setup menu, never the hub);
//   - "+" (the Game Workspace menu listing Overview / Party / Boxes / Trainer / Editor) opens ONLY for
//     the valid save; otherwise the same save chooser;
//   - Items (dock) and Quick Games A go through the same gate.
// Gen I-III: a RetroArch card exists only for a strictly parsed READY save (an invalid battery file
// yields no card), and the FireRed HOME forwarder with no linked GBA save never opens the "+" menu.
// The shared rule itself (UI/SaveOpenGate.h) is checked for every source kind.
//
// Evidence class: SYNTHETIC host files; no emulator save is written (the original files are compared
// byte for byte at the end).
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Utils/CRC16.h"
#include "Encryption/Encryption4.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveOpenGate.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"
#include "Utils/Gen4TextCodec.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;
namespace L5 = PokeVault::Integration::Gen5::LayoutInfo;
namespace Gate = PokeBank::UIModel::SaveOpenGate;
using Kind = UI::SaveSelectScreen::SelectedSourceKind;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

constexpr u64 kOwner[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
const std::string kProfile = "0123456789abcdef0fedcba987654321";

std::vector<uint8_t> diskBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { check(false, "write " + path); return; }
    std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);
}

// ---- Gen V synthetic save (same construction as tests/test_gen5_source_e2e.cpp) ----
void stampCopy5(std::vector<uint8_t>& sav, size_t base, G5::SaveFamily family) {
    const auto blocks = family == G5::SaveFamily::BlackWhite ? std::span<const L5::SaveBlock>(L5::BlackWhite)
                                                             : std::span<const L5::SaveBlock>(L5::Black2White2);
    const size_t size = family == G5::SaveFamily::BlackWhite ? L5::BlackWhiteCopySize : L5::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(base, size);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C5::write16(copy, b.checksumOffset, crc);
        C5::write16(copy, b.mirrorOffset, crc);
    }
}
void writeCopy5(std::vector<uint8_t>& sav, size_t base, G5::SaveFamily family, uint8_t version) {
    const size_t player = base + 0x19400;
    C5::write16(sav, player + 0x04, 'P');
    C5::write16(sav, player + 0x06, 'X');
    C5::write16(sav, player + 0x08, 0xFFFF);
    C5::write16(sav, player + 0x14, 12345);
    C5::write16(sav, player + 0x16, 54321);
    sav[player + 0x1F] = version;
    C5::write16(sav, player + 0x24, 5);
    std::vector<uint8_t> pk(C5::PartySize, 0);
    C5::write16(pk, 0, 0x1357);
    C5::write16(pk, 8, 25);
    C5::write16(pk, 12, 12345);
    C5::write16(pk, 14, 54321);
    pk[0x8C] = 20;
    const auto party = C5::encryptCandidate(pk);
    sav[base + L5::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L5::PartyOffset + 8));
    stampCopy5(sav, base, family);
}
std::vector<uint8_t> gen5Save(const std::string& game) {
    const bool bw = game == "black_nds" || game == "white_nds";
    const auto family = bw ? G5::SaveFamily::BlackWhite : G5::SaveFamily::Black2White2;
    const uint8_t version = game == "white_nds" ? 20 : game == "black_nds" ? 21 : game == "white2_nds" ? 22 : 23;
    std::vector<uint8_t> raw(L5::FullSaveSize, 0);
    const size_t copySize = bw ? L5::BlackWhiteCopySize : L5::Black2White2CopySize;
    writeCopy5(raw, 0, family, version);
    writeCopy5(raw, copySize, family, version);
    return raw;
}

// ---- Gen IV synthetic Diamond save (same construction as tests/test_home_card_gen4_snapshot.cpp) ----
constexpr size_t SAVE4 = 0x80000, PARTITION4 = 0x40000;
constexpr size_t GENERAL4 = 0xC100, STORAGE4_START = 0xC100, STORAGE4 = 0x121E0;
void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) { b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8); }
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>(v >> (8 * i)); }
void put(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i) dst[off + i] = static_cast<uint8_t>(src[i]);
}
void footer4(std::vector<uint8_t>& save, size_t off, size_t len, uint32_t major, uint32_t minor) {
    const size_t end = off + len;
    w32(save, end - 0x14, major); w32(save, end - 0x10, minor);
    w32(save, end - 0x0C, static_cast<uint32_t>(len)); w32(save, end - 0x08, 0x20060623);
    w16(save, end - 2, Utils::crc16ccitt(save.data() + off, len - 0x14));
}
std::vector<uint8_t> diamondSave() {
    const uint8_t language = static_cast<uint8_t>(Enums::LanguageID::English);
    std::vector<uint8_t> save(SAVE4, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    for (int p = 0; p < 2; ++p) {
        const size_t base = static_cast<size_t>(p) * PARTITION4;
        std::fill(save.begin() + base, save.begin() + base + GENERAL4, 0);
        const size_t st = base + STORAGE4_START;
        std::fill(save.begin() + st, save.begin() + st + STORAGE4, 0);
        const size_t tr = base + 0x64;
        put(save, tr, Utils::encodeGen4Field(u"ASH", 8, 7, language));
        w16(save, tr + 0x10, 12345); w16(save, tr + 0x12, 54321); w32(save, tr + 0x14, 999);
        save[tr + 0x18] = 0; save[tr + 0x19] = language; save[tr + 0x1A] = 8; save[tr + 0x1C] = 7;
        save[base + 0x98 - 4] = 0;
        for (size_t box = 0; box < 18; ++box)
            for (size_t slot = 0; slot < 30; ++slot)
                put(save, st + 4 + box * 0xFF0 + slot * Encryption::SIZE_STORED4, blank);
        for (size_t box = 0; box < 18; ++box)
            put(save, st + 4 + 18 * 0xFF0 + box * 40, Utils::encodeGen4Field(u"BOX", 20, 8, language));
        footer4(save, base, GENERAL4, p == 0 ? 20 : 10, 1);
        footer4(save, st, STORAGE4, p == 0 ? 30 : 15, 2);
    }
    return save;
}

struct Driver {
    UI::SaveSelectScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        if (!screen.hasSelectedTitle() && !screen.shouldExit()) screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
    bool drawn(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t.find(text) != std::string::npos) return true;
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

enum class Case { None, Sibling, Junk, Valid };
const char* caseName(Case c) {
    return c == Case::None ? "no save assigned" : c == Case::Sibling ? "sibling game's save"
         : c == Case::Junk ? "junk file" : "valid save";
}

struct Paths {
    std::string root, junk;
    std::string gen5[4];   // white, black, white2, black2
    std::string diamond;
};

std::string gen5Path(const Paths& p, const std::string& game) {
    if (game == "white_nds") return p.gen5[0];
    if (game == "black_nds") return p.gen5[1];
    if (game == "white2_nds") return p.gen5[2];
    return p.gen5[3];
}
std::string siblingOf(const std::string& game) {
    if (game == "white_nds") return "black_nds";
    if (game == "black_nds") return "white_nds";
    if (game == "white2_nds") return "black2_nds";
    if (game == "black2_nds") return "white2_nds";
    if (game == "diamond_nds" || game == "pearl_nds") return "platinum_nds";
    return "diamond_nds";
}
bool isGen5(const std::string& game) { return game.find("black") == 0 || game.find("white") == 0; }
std::string familyOf(const std::string& game) {
    if (game == "black_nds" || game == "white_nds") return "BW";
    if (game == "black2_nds" || game == "white2_nds") return "B2W2";
    if (game == "diamond_nds" || game == "pearl_nds") return "DP";
    if (game == "platinum_nds") return "PT";
    return "HGSS";
}

// Writes the binding for `game` in a fresh database and returns its path.
std::string bindingsFor(const Paths& p, const std::string& game, Case c, int serial) {
    const std::string db = p.root + "/bindings-" + game + "-" + std::to_string(serial);
    PokeVault::Legacy::LegacySourceBindings bindings(db);
    check(bindings.load(), db + ": fresh bindings database");
    if (c == Case::None) return db;
    std::string path;
    if (c == Case::Junk) path = p.junk;
    else if (isGen5(game)) path = gen5Path(p, c == Case::Valid ? game : siblingOf(game));
    else path = p.diamond;   // Gen IV: Diamond's save; for a sibling card the family differs
    std::string gameForBinding = game;
    if (!isGen5(game) && c == Case::Valid && familyOf(game) != "DP") return db;   // only DP has a fixture
    PokeVault::Legacy::BindingRecord binding;
    binding.profileIdentity = kProfile;
    binding.gameIdentity = gameForBinding;
    binding.sourcePath = path;
    binding.sourceType = "Manual";
    binding.expectedRawFamily = familyOf(game);
    if (isGen5(game) && c == Case::Valid) {
        const auto probe = G5::inspectSourceFile(path, "Manual", game);
        check(probe.ready(), game + ": the valid synthetic save validates (" + probe.diagnostic + ")");
        check(bindings.assignFileAndSave(probe.sourceIdentity, binding), game + ": valid binding stored");
        return db;
    }
    check(bindings.assignFileAndSave("host-test:" + game + ":" + caseName(c), binding),
          game + ": binding stored (" + caseName(c) + ")");
    return db;
}

UI::SaveSelectScreen::NavigationState focusOn(const std::string& game, Kind kind) {
    UI::SaveSelectScreen::NavigationState nav;
    nav.profileIdentity = kProfile;
    nav.gameId = game;
    nav.sourceKind = kind;
    return nav;
}

void dsCard(const Paths& p, const std::string& game, Case c, int& serial) {
    const bool gen5 = isGen5(game);
    if (!gen5 && c == Case::Valid && familyOf(game) != "DP") return;
    // Diamond and Pearl saves are byte-format identical (no version byte): a Diamond save IS a valid
    // Pearl save, so "sibling" is only checked across families (a Diamond save on a Platinum /
    // HeartGold / SoulSilver card), where the format can tell.
    if (!gen5 && c == Case::Sibling && familyOf(game) == "DP") return;
    const Kind kind = gen5 ? Kind::Gen5AssignedFile : Kind::Gen4AssignedFile;
    const std::string what = game + " / " + caseName(c);
    const bool valid = c == Case::Valid;
    for (const char* entry : {"A", "+", "Items", "Quick Games A"}) {
        const std::string db = bindingsFor(p, game, c, ++serial);
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), what + ": bindings load");
        PokeVault::Legacy::FRLGDiscoveryResult legacy;
        const auto nav = focusOn(game, kind);
        UI::SaveSelectScreen screen(legacy, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        check(screen.navigationState().gameId == game, what + ": the " + game + " card is focused");
        const std::string label = what + " / " + entry;
        if (std::string(entry) == "A") d.press(HidNpadButton_A);
        else if (std::string(entry) == "+") d.press(HidNpadButton_Plus);
        else if (std::string(entry) == "Items") {
            // Down focuses the dock (Games logo), Right x2 -> Items.
            d.press(HidNpadButton_Down);
            d.press(HidNpadButton_Right);
            d.press(HidNpadButton_Right);
            d.press(HidNpadButton_A);
        } else {
            d.press(HidNpadButton_Y);
            for (int i = 0; i < 40; ++i) d.frame();
            d.press(HidNpadButton_A);
        }
        d.dump(label);
        const bool workspaceMenu = d.drawn("GAME WORKSPACE");
        const bool opened = screen.hasSelectedTitle();
        if (valid) {
            if (std::string(entry) == "+") check(workspaceMenu, label + ": the Game Workspace menu opens");
            else check(opened && screen.getSelectedGameId() == game, label + ": the workspace opens");
            continue;
        }
        check(!opened, label + ": NO workspace opens (editor / Party / Boxes / Bag / Trainer)");
        check(!workspaceMenu, label + ": the Game Workspace menu (Party / Boxes / Editor) does not open");
        // The owner: "it should have the file menu pop up to assign a save". No save exists for this
        // exact game in any known emulator folder on the host, so the save-file chooser opens directly,
        // not the intermediate setup menu.
        check(d.drawn("ASSIGN SAVE FILE"), label + ": the save-file chooser opens");
        check(!d.drawn("Browse Saves / Open Shared Editor") && !d.drawn("SOURCE SETUP"),
              label + ": not the intermediate setup menu (R02 \"it shouldnt even go to that menu\")");
        // B closes the chooser back to Product Home; still nothing opened.
        d.press(HidNpadButton_B);
        check(!screen.hasSelectedTitle() && !d.drawn("ASSIGN SAVE FILE"), label + ": B returns Home, nothing opened");
    }
}

void retroArchAndForwarder(const Paths& p) {
    // Gen I-III battery files that failed strict parsing: no card, so nothing to open.
    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    for (const char* game : {"red_gb", "yellow_gb", "gold_gbc", "crystal_gbc", "emerald_gba", "firered_gba"}) {
        PokeVault::Legacy::FRLGSource source;
        source.path = source.normalizedPath = p.junk;
        source.sourceIdentity = std::string("host-test:") + game;
        source.gameId = game;
        source.status = PokeVault::Legacy::LegacySourceStatus::InvalidSave;
        legacy.sources.push_back(std::move(source));
    }
    // The FireRed HOME forwarder (Switch) with no linked GBA save.
    HostUi::consoleSaveCount = 0;
    auto& e = HostUi::consoleSaveEntries[HostUi::consoleSaveCount];
    e = {};
    e.application_id = 0x0100554023408000ULL;
    e.save_data_type = FsSaveDataType_Account;
    e.uid[0] = kOwner[0];
    e.uid[1] = kOwner[1];
    HostUi::consoleSaveSpace[HostUi::consoleSaveCount++] = FsSaveDataSpaceId_User;
    PokeVault::Legacy::LegacySourceBindings bindings(p.root + "/bindings-legacy");
    check(bindings.load(), "legacy bindings database");
    for (const char* game : {"red_gb", "yellow_gb", "gold_gbc", "crystal_gbc", "emerald_gba", "firered_gba"}) {
        const auto nav = focusOn(game, Kind::RetroArchFRLG);
        UI::SaveSelectScreen screen(legacy, bindings, &nav);
        // Round 15 (owner decision 1, updated deliberately: "All generations should have the same
        // Save Instances experience ... A missing save should open the save chooser, never an empty
        // editable workspace"): every Gen I-III game now has a card even when its only battery file
        // is invalid; that card has no save, and A opens the save chooser, never a workspace.
        Driver d{screen, {}, {}};
        d.frame();
        check(screen.navigationState().gameId == game,
              std::string(game) + ": the card exists although its only battery file is invalid");
        d.press(HidNpadButton_A);
        check(!screen.hasSelectedTitle() && !d.drawn("GAME WORKSPACE") && d.drawn("ASSIGN SAVE FILE"),
              std::string(game) + ": an invalid battery save opens nothing; A opens the save chooser");
    }
    const auto nav = focusOn("firered_switch", Kind::SwitchTitle);
    UI::SaveSelectScreen screen(legacy, bindings, &nav);
    Driver d{screen, {}, {}};
    d.frame();
    if (screen.navigationState().gameId == "firered_switch") {
        d.press(HidNpadButton_Plus);
        d.dump("FireRed forwarder +");
        check(!d.drawn("GAME WORKSPACE"), "FireRed HOME forwarder with no linked GBA save: no Game Workspace menu");
        d.press(HidNpadButton_B);
        d.press(HidNpadButton_A);
        check(!screen.hasSelectedTitle(), "FireRed HOME forwarder with no linked GBA save: A opens nothing");
    } else {
        check(false, "the FireRed HOME forwarder card is present on the scripted console");
    }
    HostUi::consoleSaveCount = 0;
}

void sharedRule() {
    using S = Gate::Source;
    using A = Gate::Assignment;
    using V = Gate::Validation;
    check(Gate::decide({S::ConsoleManaged, A::None, V::NotChecked, "Sword"}).open(),
          "console-managed Switch savedata opens (the console owns it; the open still parses it)");
    for (const auto source : {S::DiscoveredFile, S::AssignedFile}) {
        for (const auto a : {A::None, A::Missing, A::Unreadable, A::Ambiguous, A::Ready})
            for (const auto v : {V::NotChecked, V::Valid, V::Invalid}) {
                const auto d = Gate::decide({source, a, v, "White"});
                const bool expected = a == A::Ready && v == V::Valid;
                check(d.open() == expected, "open only for an assigned save that passed validation");
                check(d.open() == d.message.empty(), "a refusal always says why");
                if (!expected)
                    check(d.message.find("Pokémon White") != std::string::npos &&
                          d.message.find("Choose a save file") != std::string::npos,
                          "plain sentence: " + d.message);
            }
    }
    check(Gate::decide({S::AssignedFile, A::None, V::NotChecked, "White"}).message ==
              "No save assigned for Pokémon White. Choose a save file.", "the no-save sentence");
    for (const char* name : {"POKEMON W.sav", "white.SRM", "b2.dsv"})
        check(Gate::saveFileNameOffered(name), std::string(name) + " is offered");
    for (const char* name : {"white.dss", "white.state", "white.nds", "sav", "readme.txt"})
        check(!Gate::saveFileNameOffered(name), std::string(name) + " is not offered");
}
} // namespace

int main() {
    sharedRule();
    char pattern[] = "/tmp/pokebank-savegate-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    Paths p;
    p.root = temp;
    p.junk = p.root + "/junk.sav";
    writeFile(p.junk, std::vector<uint8_t>(0x80000, 0x5A));
    const char* names[4] = {"white_nds", "black_nds", "white2_nds", "black2_nds"};
    for (int i = 0; i < 4; ++i) {
        p.gen5[i] = p.root + "/" + names[i] + ".sav";
        writeFile(p.gen5[i], gen5Save(names[i]));
    }
    p.diamond = p.root + "/diamond.sav";
    writeFile(p.diamond, diamondSave());
    std::vector<std::vector<uint8_t>> before;
    for (const auto& f : {p.junk, p.gen5[0], p.gen5[1], p.gen5[2], p.gen5[3], p.diamond}) before.push_back(diskBytes(f));

    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
    int serial = 0;
    for (const char* game : {"white_nds", "white2_nds", "black_nds", "black2_nds",
                             "diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds"})
        for (const auto c : {Case::None, Case::Sibling, Case::Junk, Case::Valid})
            dsCard(p, game, c, serial);
    retroArchAndForwarder(p);
    HostUi::consoleSaves = {};

    size_t i = 0;
    for (const auto& f : {p.junk, p.gen5[0], p.gen5[1], p.gen5[2], p.gen5[3], p.diamond})
        check(diskBytes(f) == before[i++], f + ": the source file is byte-identical (nothing written)");
    const std::string cleanup = "rm -rf '" + p.root + "'";
    if (std::system(cleanup.c_str()) != 0) std::cerr << "cleanup failed: " << p.root << '\n';

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "H01/R02 save gate: Gen IV/V cards x {none, sibling, junk, valid} x {A, +, Items, Quick "
                 "Games}, Gen I-III cards, FireRed forwarder through the real SaveSelectScreen ("
              << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
