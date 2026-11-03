// Round 15 P0 (owner hardware results on d160aa42, issue #313, Nov 2). Verbatim owner fixes:
//  "1. One profile-scoped SourceAssignment service and UI for Assign/Change/Forget for every
//   generation; Forget must not modify the source file. 2. Hard exact-game save gate on all editor
//   entry routes (A, +, Items dock, Quick Games, touch, overview, direct editor) and source
//   revalidation whenever active game/profile changes. 3. A real end-to-end integration test
//   reproducing Black 2 Items shortcut misrouting and cross-profile assignment leakage."
// Failures it reproduces: SAF-02 (White 2 opens a workspace with no intended source), SAF-01/03/04/05
// (no Forget for Gen I-III), SAF-09 (Profile B inherits Profile A's RetroArch / DS saves), HOME-07
// (Black 2 Items dock opened an all-DS chooser; assignment looked lost; "belongs to someone else").
//
// Through the REAL SaveSelectScreen (Product Home) with TWO scripted Switch accounts and real
// discovery roots (sdmc:/retroarch/cores/savefiles, sdmc:/switch/drastic/backup) inside a private
// temporary working directory, for Red (I), Crystal (II), FireRed (III), Diamond (IV), Black 2 and
// White (V) x {fresh, assign, change, forget, switch profile, stale (missing / other-game bytes)}
// x {A, +, Items dock, Quick Games, touch card, touch Items}:
//   - nothing opens a workspace without THIS profile's assignment for THIS exact game;
//   - the chooser that opens instead is this game's own chooser (never another game's saves);
//   - Profile B never sees or uses Profile A's assignment; switching back keeps A's assignment;
//   - Forget removes only the app's record (source file byte-identical) and the card returns to
//     the chooser.
// Evidence class: SYNTHETIC host files.
#include "HostUi.h"

#include "fixtures/gsc_pokemon_fixture.h"

#include "Utils/NXTypes.h"
#include "Utils/CRC16.h"
#include "Encryption/Encryption4.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"
#include "Utils/Gen4TextCodec.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace fsx = std::filesystem;
namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;
namespace L5 = PokeVault::Integration::Gen5::LayoutInfo;
namespace G3D = PokeVault::Integration::Gen3::Detail;
using Kind = UI::SaveSelectScreen::SelectedSourceKind;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

constexpr u64 kUserA[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
constexpr u64 kUserB[2] = {0x1111222233334444ULL, 0x5555666677778888ULL};
const std::string kProfileA = "0123456789abcdef0fedcba987654321";
const std::string kProfileB = "11112222333344445555666677778888";

std::vector<uint8_t> diskBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// ---- synthetic saves (same constructions as tests/test_ui_save_instances_gen15_host.cpp) ----
// variant 0 / 1 are two DIFFERENT valid initialized saves of the same game.
uint8_t gen1Sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return static_cast<uint8_t>((~s) & 255);
}
std::vector<uint8_t> gen1Save(int variant) {
    const auto bank = [](size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; };
    std::vector<uint8_t> v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;   // "RED"
    v[0x2605] = 0x12; v[0x2606] = static_cast<uint8_t>(0x34 + variant);
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    v[0x30C0 + 1] = 0xFF;
    v[0x3523] = gen1Sum(v, 0x2598, 0xF8B);
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        v[b + 0x1A4C] = gen1Sum(v, b, 0x1A4C);
        for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = gen1Sum(v, b + i * 0x462, 0x462);
    }
    return v;
}
std::vector<uint8_t> gen2Save(int variant) {
    auto d = fixture(C, false);
    d[0x200A] = static_cast<uint8_t>(d[0x200A] + variant);
    checksum(d, C);
    return d;
}
void w16(std::span<uint8_t> b, size_t o, uint16_t v) { b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8); }
void w32(std::span<uint8_t> b, size_t o, uint32_t v) { w16(b, o, static_cast<uint16_t>(v)); w16(b, o + 2, static_cast<uint16_t>(v >> 16)); }
std::vector<uint8_t> gGen3Fixture;
std::vector<uint8_t> gen3Save(int variant) {
    auto save = gGen3Fixture;
    if (save.size() != G3D::kSaveSize || variant == 0) return save;
    for (size_t slot = 0; slot < 2; ++slot) {
        for (size_t physical = 0; physical < G3D::kSectorCount; ++physical) {
            const size_t offset = G3D::kSlotBases[slot] + physical * G3D::kSectorSize;
            const uint16_t id = static_cast<uint16_t>(save[offset + 0xFF4] | (save[offset + 0xFF5] << 8));
            if (id != 0) continue;
            auto sector = std::span<uint8_t>(save).subspan(offset, G3D::kSectorSize);
            w32(sector, 0x0A, 0x00010002u + static_cast<uint32_t>(variant));
            w16(sector, 0xFF6, G3D::sectorChecksum(
                std::span<const uint8_t>(save).subspan(offset, G3D::kChunkLengths[0])));
        }
    }
    return save;
}
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
std::vector<uint8_t> gen5Save(const std::string& game, int variant) {
    const bool bw = game == "black_nds" || game == "white_nds";
    const auto family = bw ? G5::SaveFamily::BlackWhite : G5::SaveFamily::Black2White2;
    const uint8_t version = game == "white_nds" ? 20 : game == "black_nds" ? 21 : game == "white2_nds" ? 22 : 23;
    std::vector<uint8_t> raw(L5::FullSaveSize, 0);
    const size_t copySize = bw ? L5::BlackWhiteCopySize : L5::Black2White2CopySize;
    for (size_t base : {size_t(0), copySize}) {
        const size_t player = base + 0x19400;
        C5::write16(raw, player + 0x04, 'P');
        C5::write16(raw, player + 0x06, 'X');
        C5::write16(raw, player + 0x08, 0xFFFF);
        C5::write16(raw, player + 0x14, static_cast<uint16_t>(12345 + variant));
        C5::write16(raw, player + 0x16, 54321);
        raw[player + 0x1F] = version;
        C5::write16(raw, player + 0x24, 5);
        std::vector<uint8_t> pk(C5::PartySize, 0);
        C5::write16(pk, 0, 0x1357);
        C5::write16(pk, 8, 25);
        C5::write16(pk, 12, 12345);
        C5::write16(pk, 14, 54321);
        pk[0x8C] = 20;
        const auto party = C5::encryptCandidate(pk);
        raw[base + L5::PartyOffset + 4] = 1;
        std::copy(party.begin(), party.end(), raw.begin() + static_cast<std::ptrdiff_t>(base + L5::PartyOffset + 8));
        stampCopy5(raw, base, family);
    }
    return raw;
}
void le16(std::vector<uint8_t>& b, size_t o, uint16_t v) { b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8); }
void le32(std::vector<uint8_t>& b, size_t o, uint32_t v) { for (int i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>(v >> (8 * i)); }
void putBytes(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i) dst[off + i] = static_cast<uint8_t>(src[i]);
}
void footer4(std::vector<uint8_t>& save, size_t off, size_t len, uint32_t major, uint32_t minor) {
    const size_t end = off + len;
    le32(save, end - 0x14, major); le32(save, end - 0x10, minor);
    le32(save, end - 0x0C, static_cast<uint32_t>(len)); le32(save, end - 0x08, 0x20060623);
    le16(save, end - 2, Utils::crc16ccitt(save.data() + off, len - 0x14));
}
std::vector<uint8_t> gen4Save(int variant) {
    constexpr size_t GENERAL = 0xC100, STORAGE_START = 0xC100, STORAGE = 0x121E0;
    const uint8_t language = static_cast<uint8_t>(Enums::LanguageID::English);
    std::vector<uint8_t> save(0x80000, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    for (int p = 0; p < 2; ++p) {
        const size_t base = static_cast<size_t>(p) * 0x40000;
        std::fill(save.begin() + base, save.begin() + base + GENERAL, 0);
        const size_t st = base + STORAGE_START;
        std::fill(save.begin() + st, save.begin() + st + STORAGE, 0);
        const size_t tr = base + 0x64;
        putBytes(save, tr, Utils::encodeGen4Field(u"ASH", 8, 7, language));
        le16(save, tr + 0x10, static_cast<uint16_t>(12345 + variant)); le16(save, tr + 0x12, 54321);
        save[tr + 0x19] = language;
        for (size_t box = 0; box < 18; ++box)
            for (size_t slot = 0; slot < 30; ++slot)
                putBytes(save, st + 4 + box * 0xFF0 + slot * Encryption::SIZE_STORED4, blank);
        for (size_t box = 0; box < 18; ++box)
            putBytes(save, st + 4 + 18 * 0xFF0 + box * 40, Utils::encodeGen4Field(u"BOX", 20, 8, language));
        footer4(save, base, GENERAL, p == 0 ? 20 : 10, 1);
        footer4(save, st, STORAGE, p == 0 ? 30 : 15, 2);
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
    bool tapAt(int x, int y) {
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {false, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        frame();
        return true;
    }
    bool tapText(const std::string& text, bool contains = false) {
        for (const auto& t : HostUi::visibleText) {
            if (contains ? t.text.find(text) == std::string::npos : t.text != text) continue;
            return tapAt(t.x + 6, t.y + 6);
        }
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

struct Game {
    std::string id, title;
    int generation;
    Kind kind;
    std::string file1, file2;     // two different valid saves of THIS game (discovery roots)
    std::string other;            // a valid save of ANOTHER game (same emulator folder)
};

std::vector<std::string> gAllFiles;
std::map<std::string, std::vector<uint8_t>> gOriginal;

PokeVault::Legacy::FRLGDiscoveryResult classicCatalog(const std::vector<Game>& games) {
    PokeVault::Legacy::FRLGDiscoveryResult catalog;
    for (const auto& g : games) {
        if (g.generation > 3) continue;
        for (const auto& path : {g.file1, g.file2}) {
            auto source = PokeVault::Legacy::inspectLegacySaveFile(path, g.id, "RetroArch");
            // A deliberately moved / replaced file (stale phases) is simply not discovered.
            if (fsx::exists(path) && diskBytes(path) == gOriginal[path])
                check(source.ready(), g.id + ": synthetic classic save " + path + " validates (" + source.detail + ")");
            if (source.ready()) catalog.sources.push_back(std::move(source));
        }
    }
    return catalog;
}

UI::SaveSelectScreen::NavigationState focusOn(const Game& g, const std::string& profile) {
    UI::SaveSelectScreen::NavigationState nav;
    nav.profileIdentity = profile;
    nav.gameId = g.id;
    nav.sourceKind = g.kind;
    return nav;
}

// The chooser for THIS game is on screen (Save Instances / setup / SD browser naming the game).
bool chooserFor(const Driver& d, const Game& g) {
    const bool named = d.drawn("Pokémon " + g.title);
    const bool chooser = d.drawn("Save Instances") || d.drawn("ASSIGN SAVE FILE") ||
                         d.drawn("SOURCE SETUP") || d.drawn("GEN V SAVE SOURCES");
    return named && chooser;
}

// Opens the given route. Returns the route label.
void route(Driver& d, const std::string& r, const std::string& title = {}) {
    if (r == "A") d.press(HidNpadButton_A);
    else if (r == "+") d.press(HidNpadButton_Plus);
    else if (r == "Items") {
        d.press(HidNpadButton_Down);
        d.press(HidNpadButton_Right);
        d.press(HidNpadButton_Right);
        d.press(HidNpadButton_A);
    } else if (r == "Quick Games") {
        d.press(HidNpadButton_Y);
        for (int i = 0; i < 40; ++i) d.frame();
        d.press(HidNpadButton_A);
    } else if (r == "touch Items") {
        if (!d.tapText("Items")) check(false, "the Items dock label is on screen to tap");
    } else if (r == "touch card") {
        // The whole selected-game panel is one Open touch target; tap its game title.
        if (!d.tapText("Pokemon " + title)) check(false, "the game card is on screen to tap");
    }
}
const char* kRoutes[] = {"A", "+", "Items", "Quick Games", "touch Items", "touch card"};

bool opened(const Driver& d, const std::string& r) {
    return d.screen.hasSelectedTitle() || (r == "+" && d.drawn("GAME WORKSPACE"));
}

void switchProfile(Driver& d, int index) {
    const bool onB = d.screen.navigationState().profileIdentity == kProfileB;
    // Tap the profile header (works wherever focus is: card, dock, feature cards).
    if (!d.tapText(onB ? "QA Second" : "QA Owner")) check(false, "the profile header is on screen to tap");
    if (!d.drawn("SWITCH PROFILE")) check(false, "the profile picker opens");
    d.dump("profile picker");
    if (onB != (index == 1)) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
}

std::string assignedPath(const std::string& db, const std::string& profile, const Game& g) {
    PokeVault::Legacy::LegacySourceBindings bindings(db);
    if (!bindings.load()) return "(load failed)";
    if (g.generation >= 4) {
        const auto r = bindings.resolveFileForGame(profile, g.id);
        return r.status == PokeVault::Legacy::AssignedFileStatus::Unassigned ? std::string() : r.binding.sourcePath;
    }
    for (const auto& path : {g.file1, g.file2}) {
        auto s = PokeVault::Legacy::inspectLegacySaveFile(path, g.id, "RetroArch");
        PokeVault::Source::SaveInstance instance;
        instance.sourceIdentity = s.sourceIdentity;
        instance.gameId = g.id;
        if (bindings.isPreferredGameSource(instance, profile, g.id)) return path;
    }
    const auto r = bindings.resolveFileForGame(profile, g.id);
    return r.status == PokeVault::Legacy::AssignedFileStatus::Unassigned ? std::string() : r.binding.sourcePath;
}

// Which file did the editor get? Gen I-III: the catalog entry; Gen IV/V: the profile's binding.
std::string openedPath(const UI::SaveSelectScreen& screen, const PokeVault::Legacy::FRLGDiscoveryResult& catalog,
                       const PokeVault::Legacy::LegacySourceBindings& bindings, const std::string& profile,
                       const Game& g) {
    if (g.generation <= 3) {
        const size_t i = screen.getSelectedLegacySourceIndex();
        return i < catalog.sources.size() ? catalog.sources[i].path : std::string();
    }
    return bindings.resolveFileForGame(profile, g.id).binding.sourcePath;
}

void routesRefused(const Game& g, const std::string& db, const std::string& profile, const std::string& phase) {
    for (const char* r : kRoutes) {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), phase + ": bindings load");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, profile);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        const std::string what = g.id + " / " + phase + " / " + r;
        check(screen.navigationState().gameId == g.id && screen.navigationState().profileIdentity == profile,
              what + ": the card of this game and profile is focused");
        route(d, r, g.title);
        d.dump(what);
        check(!opened(d, r), what + ": NO workspace / editor opens");
        check(chooserFor(d, g), what + ": this game's own save chooser opens instead");
    }
}

void routesOpen(const Game& g, const std::string& db, const std::string& profile, const std::string& expected,
                const std::string& phase) {
    for (const char* r : kRoutes) {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), phase + ": bindings load");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, profile);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        const std::string what = g.id + " / " + phase + " / " + r;
        route(d, r, g.title);
        d.dump(what);
        check(opened(d, r), what + ": the assigned save opens");
        if (screen.hasSelectedTitle()) {
            check(screen.getSelectedGameId() == g.id, what + ": exactly this game opens");
            check(openedPath(screen, catalog, bindings, profile, g) == expected,
                  what + ": exactly this profile's assigned file opens (" + expected + ")");
        }
    }
}

// Assign through the chooser the gate opened (A, then A on the focused / tapped row).
bool assignThroughChooser(Driver& d, const Game& g, const std::string& leaf) {
    d.press(HidNpadButton_A);
    d.dump(g.id + " assign: chooser");
    if (!chooserFor(d, g)) return false;
    if (d.drawn("GEN V SAVE SOURCES")) {   // Gen V setup menu: Browse Saves / Open Shared Editor
        d.press(HidNpadButton_Down);
        d.press(HidNpadButton_A);
    } else if (d.drawn("SOURCE SETUP")) {
        d.press(HidNpadButton_A);           // Refresh Known Emulator Saves
    }
    d.dump(g.id + " assign: list");
    if (!d.tapText(leaf, true)) return false;
    if (!d.screen.hasSelectedTitle()) d.press(HidNpadButton_A);
    return d.screen.hasSelectedTitle();
}

// "+" -> Source / Game File -> Forget (shared Minus) -> Y confirm.
bool forgetThroughUi(Driver& d) {
    d.press(HidNpadButton_Plus);
    if (!d.drawn("GAME WORKSPACE")) return false;
    d.press(HidNpadButton_Up);
    d.press(HidNpadButton_Right);
    d.press(HidNpadButton_A);
    d.dump("forget: source overlay");
    d.press(HidNpadButton_Minus);
    d.dump("forget: confirm");
    if (!d.drawn("FORGET")) return false;
    d.press(HidNpadButton_Y);
    d.dump("forget: done");
    return true;
}

std::string leafOf(const std::string& path) {
    return path.substr(path.find_last_of('/') + 1);
}

void lifecycle(const Game& g, const std::string& root) {
    const std::string db = root + "/bindings-" + g.id;
    // 1. Fresh database: no profile has an assignment. Every route refuses for A and B (SAF-02).
    routesRefused(g, db, kProfileA, "fresh, profile A");
    routesRefused(g, db, kProfileB, "fresh, profile B");

    // 2. Profile A assigns file1 through the chooser.
    {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        check(assignThroughChooser(d, g, leafOf(g.file1)), g.id + ": profile A assigns " + g.file1 + " in the chooser");
        check(screen.hasSelectedTitle() && screen.getSelectedGameId() == g.id &&
                  openedPath(screen, catalog, bindings, kProfileA, g) == g.file1,
              g.id + ": the chosen save opens for profile A");
    }
    check(assignedPath(db, kProfileA, g) == g.file1, g.id + ": profile A's record names file1");
    check(assignedPath(db, kProfileB, g).empty(), g.id + ": profile B has no record");
    routesOpen(g, db, kProfileA, g.file1, "assigned, profile A");

    // 3. SAF-09: switch to profile B on the LIVE screen; B never sees / uses A's assignment.
    for (const char* r : kRoutes) {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        switchProfile(d, 1);
        const std::string what = g.id + " / switched to profile B / " + r;
        check(screen.navigationState().profileIdentity == kProfileB, what + ": profile B is active");
        if (screen.navigationState().gameId != g.id) {
            // Focus the same game on B (L/R walks the cards).
            for (int i = 0; i < 40 && screen.navigationState().gameId != g.id; ++i) d.press(HidNpadButton_R);
        }
        check(screen.navigationState().gameId == g.id, what + ": the same game is focused on profile B");
        check(!d.drawn("PX") || g.generation < 5, what + ": profile A's Gen V trainer is not shown on B's card");
        route(d, r, g.title);
        d.dump(what);
        check(!opened(d, r), what + ": profile A's assignment is NOT usable on profile B");
        check(chooserFor(d, g), what + ": B gets this game's chooser");
        check(!d.drawn("belongs to another profile") && !d.drawn("already linked to another profile"),
              what + ": no cross-profile ownership message on B's normal route");
        // B back to A on the same screen: A's assignment is intact (HOME-07 "assignment lost").
        d.press(HidNpadButton_B);
        if (!screen.hasSelectedTitle()) {
            switchProfile(d, 0);
            check(screen.navigationState().profileIdentity == kProfileA, what + ": back on profile A");
            for (int i = 0; i < 40 && screen.navigationState().gameId != g.id; ++i) d.press(HidNpadButton_R);
            route(d, r, g.title);
            check(opened(d, r), what + ": after switching back, profile A's assignment still opens");
        }
    }
    check(assignedPath(db, kProfileA, g) == g.file1, g.id + ": profile A's record survived profile switching");

    // 4. Stale: the bound file is missing, then replaced by ANOTHER game's bytes.
    const auto original = diskBytes(g.file1);
    fsx::rename(g.file1, g.file1 + ".moved");
    routesRefused(g, db, kProfileA, "assigned file missing");
    fsx::rename(g.file1 + ".moved", g.file1);
    writeFile(g.file1, diskBytes(g.other));
    routesRefused(g, db, kProfileA, "assigned file now holds another game's save");
    writeFile(g.file1, original);

    // 5. Change: A re-chooses file2 from Source / Game File.
    {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        d.press(HidNpadButton_Plus);
        d.press(HidNpadButton_Up);
        d.press(HidNpadButton_Right);
        d.press(HidNpadButton_A);
        d.dump(g.id + " change: source overlay");
        if (d.drawn("Browse Saves / Open Shared Editor")) { d.press(HidNpadButton_Down); d.press(HidNpadButton_A); }
        else if (d.drawn("Refresh Known Emulator Saves")) d.press(HidNpadButton_A);
        d.dump(g.id + " change: list");
        check(d.tapText(leafOf(g.file2), true), g.id + ": file2 is offered in this game's Save Instances");
        if (!screen.hasSelectedTitle()) d.press(HidNpadButton_A);
        check(screen.hasSelectedTitle() && openedPath(screen, catalog, bindings, kProfileA, g) == g.file2,
              g.id + ": Change opens file2");
    }
    check(assignedPath(db, kProfileA, g) == g.file2, g.id + ": profile A's record now names file2");
    routesOpen(g, db, kProfileA, g.file2, "changed, profile A");

    // 6. Forget: only the app's record for (A, this game) goes; files untouched; chooser again.
    {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        auto catalog = classicCatalog({g});
        const auto nav = focusOn(g, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        check(forgetThroughUi(d), g.id + ": Forget is offered for this generation's save (Source / Game File, - then Y)");
        check(!screen.hasSelectedTitle(), g.id + ": Forget opens nothing");
    }
    check(assignedPath(db, kProfileA, g).empty(), g.id + ": Forget removed profile A's record");
    routesRefused(g, db, kProfileA, "forgotten, profile A");
    for (const auto& f : {g.file1, g.file2, g.other})
        check(diskBytes(f) == gOriginal[f], g.id + ": " + f + " is byte-identical after assign/change/forget");
}

// HOME-07 exact repro: Black 2 assigned on profile A; the HOME dock Items shortcut must open
// Black 2's bag from A's file; on profile B it must open Black 2's OWN chooser (never the White 2
// save, never an all-DS file list), with no "belongs to someone else"; A keeps its assignment.
void black2ItemsRepro(const Game& b2, const Game& w2, const std::string& root) {
    const std::string db = root + "/bindings-home07";
    {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        PokeVault::Legacy::FRLGDiscoveryResult catalog;
        const auto nav = focusOn(b2, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        route(d, "Items");
        d.dump("HOME-07 unassigned Items");
        check(!screen.hasSelectedTitle(), "HOME-07: unassigned Black 2 Items opens nothing");
        check(chooserFor(d, b2) && d.drawn(leafOf(b2.file1)) && !d.drawn(leafOf(w2.file1)) &&
                  !d.drawn(leafOf(w2.file2)),
              "HOME-07: Items opens Black 2's Save Instances listing only Black 2 saves (no White 2 file)");
        check(!d.drawn("ASSIGN SAVE FILE"), "HOME-07: Items does not drop into the all-DS SD file list");
        check(d.tapText(leafOf(b2.file1), true), "HOME-07: the Black 2 save row is tappable");
        if (!screen.hasSelectedTitle()) d.press(HidNpadButton_A);
        check(screen.hasSelectedTitle() && screen.getSelectedGameId() == "black2_nds" &&
                  screen.getOpenIntent() == UI::SaveSelectScreen::OpenIntent::Items,
              "HOME-07: choosing it opens Black 2 straight into Items");
    }
    for (int round = 0; round < 2; ++round) {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        PokeVault::Legacy::FRLGDiscoveryResult catalog;
        const auto nav = focusOn(b2, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        route(d, round == 0 ? "Items" : "touch Items");
        check(screen.hasSelectedTitle() && screen.getSelectedGameId() == "black2_nds" &&
                  screen.getOpenIntent() == UI::SaveSelectScreen::OpenIntent::Items &&
                  bindings.resolveFileForGame(kProfileA, "black2_nds").binding.sourcePath == b2.file1,
              std::string("HOME-07: assigned Black 2 ") + (round ? "touch " : "") + "Items opens A's Black 2 save");
    }
    {
        PokeVault::Legacy::LegacySourceBindings bindings(db);
        check(bindings.load(), "bindings");
        PokeVault::Legacy::FRLGDiscoveryResult catalog;
        const auto nav = focusOn(b2, kProfileA);
        UI::SaveSelectScreen screen(catalog, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        switchProfile(d, 1);
        check(screen.navigationState().profileIdentity == kProfileB, "HOME-07: profile B is active");
        for (int i = 0; i < 40 && screen.navigationState().gameId != "black2_nds"; ++i) d.press(HidNpadButton_R);
        route(d, "Items");
        d.dump("HOME-07 profile B Items");
        check(!screen.hasSelectedTitle(), "HOME-07: profile B's Items does not open A's Black 2 save");
        check(chooserFor(d, b2) && !d.drawn(leafOf(w2.file1)),
              "HOME-07: profile B gets Black 2's own chooser (no White 2 save)");
        check(!d.drawn("ASSIGN SAVE FILE"), "HOME-07: profile B is not dropped into the all-DS SD file list");
        check(!d.drawn("belongs to another profile") && !d.drawn("already linked to another profile"),
              "HOME-07: no 'belongs to someone else' error on profile B's Items");
        d.press(HidNpadButton_B);
        switchProfile(d, 0);
        for (int i = 0; i < 40 && screen.navigationState().gameId != "black2_nds"; ++i) d.press(HidNpadButton_R);
        route(d, "Items");
        check(screen.hasSelectedTitle() && screen.getSelectedGameId() == "black2_nds",
              "HOME-07: back on profile A, Black 2 Items still opens (assignment not lost)");
    }
    check(diskBytes(b2.file1) == gOriginal[b2.file1], "HOME-07: the Black 2 save is byte-identical");
}
} // namespace

int main() {
    {
        const char* env = std::getenv("POKEBANK_QA_GEN3_FIXTURE");
        gGen3Fixture = diskBytes(env ? env : "build-host/frlg_fixture.sav");
        if (gGen3Fixture.size() != G3D::kSaveSize) {
            std::cerr << "missing Gen III fixture (build-host/frlg_fixture.sav)\n";
            return EXIT_FAILURE;
        }
    }
    char pattern[] = "/tmp/pokebank-sourceassign-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    const std::string root = temp;
    // Private working directory: the app's sdmc:/ discovery roots resolve inside it on the host.
    if (::chdir(root.c_str()) != 0) { std::cerr << "chdir failed\n"; return EXIT_FAILURE; }
    const std::string ra = "sdmc:/retroarch/cores/savefiles";
    const std::string ds = "sdmc:/switch/drastic/backup";
    fsx::create_directories(ra);
    fsx::create_directories(ds);
    const auto put = [&](const std::string& path, const std::vector<uint8_t>& bytes) {
        writeFile(path, bytes);
        gAllFiles.push_back(path);
        gOriginal[path] = bytes;
    };
    put(ra + "/red-a.sav", gen1Save(0));
    put(ra + "/red-b.sav", gen1Save(1));
    put(ra + "/crystal-a.sav", gen2Save(0));
    put(ra + "/crystal-b.sav", gen2Save(1));
    put(ra + "/firered-a.sav", gen3Save(0));
    put(ra + "/firered-b.sav", gen3Save(1));
    put(ds + "/diamond-a.sav", gen4Save(0));
    put(ds + "/diamond-b.sav", gen4Save(1));
    for (const char* game : {"black2_nds", "white2_nds", "black_nds", "white_nds"}) {
        put(ds + "/" + game + "-a.sav", gen5Save(game, 0));
        put(ds + "/" + game + "-b.sav", gen5Save(game, 1));
    }

    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kUserA[0];
    HostUi::consoleSaves.user[1] = kUserA[1];
    HostUi::consoleSaves.user2[0] = kUserB[0];
    HostUi::consoleSaves.user2[1] = kUserB[1];
    HostUi::consoleSaves.userCount = 2;

    const std::vector<Game> games = {
        {"red_gb", "Red", 1, Kind::RetroArchFRLG, ra + "/red-a.sav", ra + "/red-b.sav", ra + "/crystal-a.sav"},
        {"crystal_gbc", "Crystal", 2, Kind::RetroArchFRLG, ra + "/crystal-a.sav", ra + "/crystal-b.sav", ra + "/red-a.sav"},
        {"firered_gba", "FireRed", 3, Kind::RetroArchFRLG, ra + "/firered-a.sav", ra + "/firered-b.sav", ra + "/crystal-a.sav"},
        {"diamond_nds", "Diamond", 4, Kind::Gen4AssignedFile, ds + "/diamond-a.sav", ds + "/diamond-b.sav", ds + "/black2_nds-a.sav"},
        {"black2_nds", "Black 2", 5, Kind::Gen5AssignedFile, ds + "/black2_nds-a.sav", ds + "/black2_nds-b.sav", ds + "/white2_nds-a.sav"},
        {"white_nds", "White", 5, Kind::Gen5AssignedFile, ds + "/white_nds-a.sav", ds + "/white_nds-b.sav", ds + "/black_nds-a.sav"},
    };
    const std::string only = std::getenv("SOURCEASSIGN_ONLY") ? std::getenv("SOURCEASSIGN_ONLY") : "";
    if (only.empty() || only == "home07") black2ItemsRepro(games[4],
        {"white2_nds", "White 2", 5, Kind::Gen5AssignedFile, ds + "/white2_nds-a.sav", ds + "/white2_nds-b.sav", ""}, root);
    for (const auto& g : games)
        if (only.empty() || only == g.id) lifecycle(g, root);

    for (const auto& f : gAllFiles)
        check(diskBytes(f) == gOriginal[f], f + ": the source file is byte-identical at the end");

    HostUi::consoleSaves = {};
    std::error_code ignored;
    if (::chdir("/") != 0) std::cerr << "chdir / failed\n";
    fsx::remove_all(root, ignored);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Round 15 P0 source assignment (two profiles x Gen I-V x assign/change/forget/switch/stale x "
                 "A/+/Items/Quick Games/touch, HOME-07 Black 2 Items repro) through the real SaveSelectScreen ("
              << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
