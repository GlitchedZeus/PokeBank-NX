// Round 15 (owner decisions, Nov 2, verbatim):
//  "Gen I–III manual save browser: YES. All generations should have the same Save Instances
//   experience: automatic discovery, manual assignment and safe file browsing. A missing save should
//   open the save chooser, never an empty editable workspace. This must be implemented in the shared
//   provider system."
//  "Fresh emulator saves: A file without an initialized game save or valid trainer information should
//   not be treated as an editable save merely because the emulator created a file. However, don't
//   reject valid initialized saves just because one optional trainer field is empty. Validate against
//   each game's native format and initialization requirements. Creating a completely new save must be
//   a separate, explicitly supported operation."
//
// Through the REAL SaveSelectScreen (Product Home) and the shared provider code on the host harness,
// for every generation (Red, Crystal, FireRed, Diamond, White) x {no save, uninitialized
// emulator-created file, junk, initialized save with an empty optional field (TID 0), valid save}:
//   - Gen I-III have a game card with no save; A / + / Items open the save chooser (never a workspace);
//   - the SD save-file browser assigns a Gen I-III file; strict per-game parsing + the shared
//     initialization rule decide; a valid save opens, and stays assigned after a restart;
//   - Gen IV / V: an uninitialized file never opens (A, +) and says why; TID 0 + a name opens;
//   - savestates and .dsv are not offered for GB / GBA; source files stay byte-identical.
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
#include "Source/SaveInitialization.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveOpenGate.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"
#include "Utils/Gen4TextCodec.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace fsx = std::filesystem;
namespace G5x = PokeVault::Integration::Gen5;
namespace C5x = PokeVault::Integration::Gen5::Crypto;
namespace L5x = PokeVault::Integration::Gen5::LayoutInfo;
namespace G3D = PokeVault::Integration::Gen3::Detail;
namespace Init = PokeVault::Source::SaveInitialization;
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
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// Trainer state of a synthetic save: Valid (name + TID), OptionalEmpty (name, TID 0 / SID 0),
// Uninitialized (no name, TID 0, SID 0, 0:00:00 -- the state before the player names a character).
enum class T { Valid, OptionalEmpty, Uninitialized };

// ---- Gen I (Red), same construction as tests/test_gen1_edit_draft.cpp ----
uint8_t gen1Sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return static_cast<uint8_t>((~s) & 255);
}
std::vector<uint8_t> gen1Save(T t) {
    const auto bank = [](size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; };
    std::vector<uint8_t> v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;   // "RED"
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    v[0x30C0 + 1] = 0xFF;
    if (t != T::Valid) { v[0x2605] = 0; v[0x2606] = 0; }
    if (t == T::Uninitialized) std::fill_n(v.begin() + 0x2598, 11, 0x50);
    v[0x3523] = gen1Sum(v, 0x2598, 0xF8B);
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        v[b + 0x1A4C] = gen1Sum(v, b, 0x1A4C);
        for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = gen1Sum(v, b + i * 0x462, 0x462);
    }
    return v;
}

// ---- Gen II (Crystal) from tests/fixtures/gsc_pokemon_fixture.h ----
std::vector<uint8_t> gen2Save(T t) {
    auto d = fixture(C, false);
    if (t != T::Valid) { d[0x2009] = 0; d[0x200A] = 0; }
    if (t == T::Uninitialized) std::fill_n(d.begin() + 0x200B, 11, 0x50);
    checksum(d, C);
    return d;
}

// ---- Gen III (FireRed): the pinned deterministic fixture (tests/test_pksm_gen3_adapter.cpp
// --write-fixture, SHA-256 c0aec024...), trainer section edited and sector checksums refreshed. ----
void w16(std::span<uint8_t> b, size_t o, uint16_t v) { b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8); }
void w32(std::span<uint8_t> b, size_t o, uint32_t v) { w16(b, o, static_cast<uint16_t>(v)); w16(b, o + 2, static_cast<uint16_t>(v >> 16)); }
std::vector<uint8_t> gen3Save(T t) {
    const char* env = std::getenv("POKEBANK_QA_GEN3_FIXTURE");
    auto save = diskBytes(env ? env : "build-host/frlg_fixture.sav");
    if (save.size() != G3D::kSaveSize) return save;
    for (size_t slot = 0; slot < 2; ++slot) {
        for (size_t physical = 0; physical < G3D::kSectorCount; ++physical) {
            const size_t offset = G3D::kSlotBases[slot] + physical * G3D::kSectorSize;
            const uint16_t id = static_cast<uint16_t>(save[offset + 0xFF4] | (save[offset + 0xFF5] << 8));
            if (id != 0) continue;   // logical sector 0: trainer info (name 0x00, TID 0x0A, time 0x0E)
            auto sector = std::span<uint8_t>(save).subspan(offset, G3D::kSectorSize);
            if (t != T::Valid) w32(sector, 0x0A, 0);
            if (t == T::Uninitialized) {
                std::fill_n(sector.begin(), 8, 0xFF);
                std::fill_n(sector.begin() + 0x0E, 5, 0);
            }
            w16(sector, 0xFF6, G3D::sectorChecksum(
                std::span<const uint8_t>(save).subspan(offset, G3D::kChunkLengths[0])));
        }
    }
    return save;
}

// ---- Gen V (White), same construction as tests/test_gen5_source_e2e.cpp writeCopy ----
void writeCopy5(std::vector<uint8_t>& sav, size_t base, T t) {
    const auto family = G5x::SaveFamily::BlackWhite;
    const size_t player = base + 0x19400;
    C5x::write16(sav, player + 0x04, t == T::Uninitialized ? 0xFFFF : 'P');
    C5x::write16(sav, player + 0x06, t == T::Uninitialized ? 0 : 'X');
    C5x::write16(sav, player + 0x08, 0xFFFF);
    C5x::write16(sav, player + 0x14, t == T::Valid ? 12345 : 0);
    C5x::write16(sav, player + 0x16, t == T::Valid ? 54321 : 0);
    sav[player + 0x1F] = 20;                       // White
    C5x::write16(sav, player + 0x24, t == T::Uninitialized ? 0 : 5);
    const size_t misc = base + G5x::trainerBlock5(family, G5x::TrainerLayout5::MiscBlockIndex).offset;
    C5x::write16(sav, misc, 1000);
    std::vector<uint8_t> pk(C5x::PartySize, 0);
    C5x::write16(pk, 0, 0x1357);
    C5x::write16(pk, 8, 25);
    C5x::write16(pk, 12, 12345);
    C5x::write16(pk, 14, 54321);
    pk[0x8C] = 20;
    const auto party = C5x::encryptCandidate(pk);
    sav[base + L5x::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L5x::PartyOffset + 8));
    auto copy = std::span<uint8_t>(sav).subspan(base, L5x::BlackWhiteCopySize);
    for (const auto& b : std::span<const L5x::SaveBlock>(L5x::BlackWhite)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C5x::write16(copy, b.checksumOffset, crc);
        C5x::write16(copy, b.mirrorOffset, crc);
    }
}
std::vector<uint8_t> gen5Save(T t) {
    std::vector<uint8_t> raw(L5x::FullSaveSize, 0);
    writeCopy5(raw, 0, t);
    writeCopy5(raw, L5x::BlackWhiteCopySize, t);
    return raw;
}

// ---- Gen IV (Diamond), same construction as tests/test_home_card_gen4_snapshot.cpp ----
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
std::vector<uint8_t> gen4Save(T t) {
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
        if (t == T::Uninitialized) le16(save, tr, 0xFFFF);
        else putBytes(save, tr, Utils::encodeGen4Field(u"ASH", 8, 7, language));
        if (t == T::Valid) { le16(save, tr + 0x10, 12345); le16(save, tr + 0x12, 54321); }
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
    bool tapText(const std::string& text) {
        for (const auto& t : HostUi::visibleText) {
            if (t.text != text) continue;
            const auto x = static_cast<u32>(t.x + 6), y = static_cast<u32>(t.y + 6);
            HostUi::touch = {true, x, y};
            frame();
            HostUi::touch = {true, x, y};
            frame();
            HostUi::touch = {false, x, y};
            frame();
            frame();
            return true;
        }
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

UI::SaveSelectScreen::NavigationState focusOn(const std::string& game, Kind kind) {
    UI::SaveSelectScreen::NavigationState nav;
    nav.profileIdentity = kProfile;
    nav.gameId = game;
    nav.sourceKind = kind;
    return nav;
}

struct Files {
    std::string root;      // absolute temp dir (Gen IV / V)
    std::string sdDir;     // "sdmc:/aaa-pbx-r15-<pid>" relative to the harness cwd (Gen I-III browser)
    std::string sdLeaf;
};

// The shared rule and the per-generation inspection (no UI).
void sharedRule(const Files& f) {
    check(!Init::initialized({0, 0, 0, true}), "no name, IDs 0, timer 0: not initialized");
    check(Init::initialized({3, 0, 0, true}), "a name alone: initialized (TID/SID/timer optional)");
    check(Init::initialized({0, 1, 0, true}), "a TID alone: initialized");
    check(Init::initialized({0, 0, 0, false}), "play time alone: initialized");
    check(Gate::decide({Gate::Source::AssignedFile, Gate::Assignment::Ready, Gate::Validation::Uninitialized, "White"})
              .message.find("not an initialized save") != std::string::npos, "the gate says why");
    for (const char* name : {"red.sav", "RED.SRM"})
        check(Gate::saveFileNameOffered(name, 1) && Gate::saveFileNameOffered(name, 3), std::string(name) + " offered for GB/GBA");
    for (const char* name : {"red.dsv", "red.state", "red.st0", "red.ss1", "red.dss", "red.gb"})
        check(!Gate::saveFileNameOffered(name, 1) && !Gate::saveFileNameOffered(name, 3),
              std::string(name) + " is not offered for GB/GBA (savestates / containers never)");
    check(Gate::saveFileNameOffered("w.dsv", 5) && !Gate::saveFileNameOffered("w.dss", 5), "DS: .dsv yes, .dss no");

    struct Row { const char* game; std::vector<uint8_t> (*make)(T); };
    const Row rows[] = {{"red_gb", gen1Save}, {"crystal_gbc", gen2Save}, {"firered_gba", gen3Save}};
    for (const auto& row : rows) {
        for (const T t : {T::Valid, T::OptionalEmpty, T::Uninitialized}) {
            const std::string path = f.root + "/" + row.game + std::to_string(static_cast<int>(t)) + ".sav";
            writeFile(path, row.make(t));
            const auto before = diskBytes(path);
            const auto s = PokeVault::Legacy::inspectLegacySaveFile(path, row.game);
            const std::string what = std::string(row.game) + (t == T::Valid ? " valid" : t == T::OptionalEmpty ? " TID 0" : " uninitialized");
            if (t == T::Uninitialized) {
                check(!s.ready() && s.detail == Init::kNotInitializedReason, what + ": refused as not initialized (" + s.detail + ")");
            } else {
                check(s.ready() && s.gameId == row.game, what + ": opens (" + s.detail + ")");
            }
            check(diskBytes(path) == before, what + ": source byte-identical");
        }
        const std::string junk = f.root + "/" + row.game + "-junk.sav";
        writeFile(junk, std::vector<uint8_t>(std::string(row.game) == "firered_gba" ? 0x20000 : 0x8000, 0xFF));
        check(!PokeVault::Legacy::inspectLegacySaveFile(junk, row.game).ready(), std::string(row.game) + ": an all-0xFF emulator file is refused");
        const std::string zero = f.root + "/" + row.game + "-zero.sav";
        writeFile(zero, std::vector<uint8_t>(std::string(row.game) == "firered_gba" ? 0x20000 : 0x8000, 0x00));
        check(!PokeVault::Legacy::inspectLegacySaveFile(zero, row.game).ready(), std::string(row.game) + ": an all-0x00 emulator file is refused");
    }
    // A Crystal save is never accepted for a Gold card (exact game).
    const std::string crystal = f.root + "/crystal_gbc0.sav";
    check(!PokeVault::Legacy::inspectLegacySaveFile(crystal, "gold_gbc").ready(), "a Crystal save is not a Gold save");
}

// Gen I-III through Product Home: card without a save -> chooser; manual browser assignment.
void classicCards(const Files& f) {
    struct Row { const char* game; const char* title; std::vector<uint8_t> (*make)(T); };
    const Row rows[] = {{"red_gb", "Red", gen1Save}, {"crystal_gbc", "Crystal", gen2Save},
                        {"firered_gba", "FireRed", gen3Save}};
    int serial = 0;
    for (const auto& row : rows) {
        const std::string game = row.game;
        // 1. No save: the card exists; A / + / Items open the save chooser, never a workspace.
        for (const char* entry : {"A", "+", "Items"}) {
            PokeVault::Legacy::LegacySourceBindings bindings(f.root + "/b-none-" + std::to_string(++serial));
            check(bindings.load(), "bindings");
            PokeVault::Legacy::FRLGDiscoveryResult legacy;
            const auto nav = focusOn(game, Kind::RetroArchFRLG);
            UI::SaveSelectScreen screen(legacy, bindings, &nav);
            Driver d{screen, {}, {}};
            d.frame();
            const std::string what = game + " / no save / " + entry;
            check(screen.navigationState().gameId == game, what + ": the card exists without a save");
            if (std::string(entry) == "A") d.press(HidNpadButton_A);
            else if (std::string(entry) == "+") d.press(HidNpadButton_Plus);
            else { d.press(HidNpadButton_Down); d.press(HidNpadButton_Right); d.press(HidNpadButton_Right); d.press(HidNpadButton_A); }
            d.dump(what);
            check(!screen.hasSelectedTitle() && !d.drawn("GAME WORKSPACE"), what + ": no workspace opens");
            check(d.drawn("ASSIGN SAVE FILE") && d.drawn(std::string("Choose the save file for Pokémon ") + row.title),
                  what + ": the save-file chooser opens");
        }
        // 2. Manual assignment through the browser: uninitialized, junk, TID 0, valid.
        for (const T t : {T::Uninitialized, T::OptionalEmpty, T::Valid}) {
            const std::string leaf = game + "-" + std::to_string(static_cast<int>(t)) + ".sav";
            const std::string path = f.sdDir + "/" + leaf;
            writeFile(path, row.make(t));
            const std::string junkLeaf = game + "-junk.srm";
            writeFile(f.sdDir + "/" + junkLeaf, std::vector<uint8_t>(64, 0x5A));
            const auto before = diskBytes(path);
            const std::string db = f.root + "/b-manual-" + std::to_string(++serial);
            bool opened = false;
            {
                PokeVault::Legacy::LegacySourceBindings bindings(db);
                check(bindings.load(), "bindings");
                PokeVault::Legacy::FRLGDiscoveryResult legacy;
                const auto nav = focusOn(game, Kind::RetroArchFRLG);
                UI::SaveSelectScreen screen(legacy, bindings, &nav);
                Driver d{screen, {}, {}};
                d.frame();
                d.press(HidNpadButton_A);
                const std::string what = game + " / manual " + leaf;
                check(d.tapText("[Folder]  " + f.sdLeaf), what + ": the test folder is listed in the browser");
                d.dump(what + " folder");
                check(!d.drawn("x.state"), what + ": savestates are not listed");
                // Junk first: refused, browser stays, nothing opened.
                check(d.tapText(junkLeaf), what + ": the junk file is listed");
                check(!screen.hasSelectedTitle() && d.drawn("ASSIGN SAVE FILE"), what + ": junk is refused, chooser stays");
                check(d.tapText(leaf), what + ": the save file is listed");
                d.dump(what + " after choose");
                opened = screen.hasSelectedTitle();
                if (t == T::Uninitialized) {
                    check(!opened && d.drawn("not an initialized save"), what + ": refused as not initialized");
                } else {
                    check(opened && screen.getSelectedGameId() == game &&
                              screen.getSelectedSourceKind() == Kind::RetroArchFRLG,
                          what + ": the valid save opens through the shared gate");
                }
            }
            if (t != T::Uninitialized) {
                // 3. After a restart the manual file is still this card's save (strictly re-inspected).
                PokeVault::Legacy::LegacySourceBindings bindings(db);
                check(bindings.load(), "bindings reload");
                PokeVault::Legacy::FRLGDiscoveryResult legacy;
                const auto nav = focusOn(game, Kind::RetroArchFRLG);
                UI::SaveSelectScreen screen(legacy, bindings, &nav);
                Driver d{screen, {}, {}};
                d.frame();
                d.press(HidNpadButton_A);
                check(screen.hasSelectedTitle() && screen.getSelectedGameId() == game,
                      game + " / " + leaf + ": the remembered manual save opens after a restart");
            }
            check(diskBytes(path) == before, game + " / " + leaf + ": the source save is byte-identical");
            // Keep the browser folder short (it shows 7 rows): one save + the junk per round.
            std::error_code ignored;
            fsx::remove(path, ignored);
            fsx::remove(f.sdDir + "/" + junkLeaf, ignored);
        }
    }
}

// Gen IV / V: uninitialized never opens; TID/SID 0 with a name opens.
void dsCards(const Files& f) {
    struct Row { const char* game; const char* family; Kind kind; std::vector<uint8_t> (*make)(T); };
    const Row rows[] = {{"white_nds", "BW", Kind::Gen5AssignedFile, gen5Save},
                        {"diamond_nds", "DP", Kind::Gen4AssignedFile, gen4Save}};
    int serial = 0;
    for (const auto& row : rows) {
        for (const T t : {T::Uninitialized, T::OptionalEmpty, T::Valid}) {
            const std::string path = f.root + "/" + row.game + std::to_string(static_cast<int>(t)) + ".sav";
            writeFile(path, row.make(t));
            const auto before = diskBytes(path);
            for (const char* entry : {"A", "+"}) {
                const std::string db = f.root + "/b-ds-" + std::to_string(++serial);
                PokeVault::Legacy::LegacySourceBindings bindings(db);
                check(bindings.load(), "bindings");
                PokeVault::Legacy::BindingRecord binding;
                binding.profileIdentity = kProfile;
                binding.gameIdentity = row.game;
                binding.sourcePath = path;
                binding.sourceType = "Manual";
                binding.expectedRawFamily = row.family;
                std::string identity = "host-r15:" + std::string(row.game) + std::to_string(serial);
                if (row.kind == Kind::Gen5AssignedFile) {
                    // Gen V binds the exact inspected source identity (Gen5AssignedSource openAssignedSource).
                    const auto probe = G5x::inspectSourceFile(path, "Manual", row.game);
                    if (t == T::Uninitialized) {
                        // Gen V strict validation itself refuses a copy with no trainer data.
                        check(!probe.ready() && probe.diagnostic.find("no trainer data") != std::string::npos,
                              std::string(row.game) + ": strict validation refuses the blank copy (" + probe.diagnostic + ")");
                    } else {
                        check(probe.ready(), std::string(row.game) + ": the synthetic file is valid (" + probe.diagnostic + ")");
                        identity = probe.sourceIdentity;
                    }
                }
                check(bindings.assignFileAndSave(identity, binding), "binding stored");
                PokeVault::Legacy::FRLGDiscoveryResult legacy;
                const auto nav = focusOn(row.game, row.kind);
                UI::SaveSelectScreen screen(legacy, bindings, &nav);
                Driver d{screen, {}, {}};
                d.frame();
                const std::string what = std::string(row.game) + (t == T::Valid ? " valid" : t == T::OptionalEmpty ? " TID/SID 0" : " uninitialized") + " / " + entry;
                d.press(std::string(entry) == "A" ? HidNpadButton_A : HidNpadButton_Plus);
                d.dump(what);
                const bool workspace = screen.hasSelectedTitle() || d.drawn("GAME WORKSPACE");
                if (t == T::Uninitialized) {
                    check(!workspace, what + ": an uninitialized file never opens a workspace");
                    check(d.drawn("not an initialized save"), what + ": the chooser says it is not an initialized save");
                } else {
                    check(workspace, what + ": an initialized save opens");
                }
            }
            check(diskBytes(path) == before, std::string(row.game) + ": the source save is byte-identical");
        }
    }
}
} // namespace

int main() {
    char pattern[] = "/tmp/pokebank-r15-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    Files f;
    f.root = temp;
    f.sdLeaf = "aaa-pbx-r15-" + std::to_string(::getpid());
    f.sdDir = "sdmc:/" + f.sdLeaf;
    fsx::create_directories(f.sdDir);
    writeFile(f.sdDir + "/x.state", std::vector<uint8_t>(16, 1));

    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];

    sharedRule(f);
    classicCards(f);
    dsCards(f);

    HostUi::consoleSaves = {};
    std::error_code ignored;
    fsx::remove_all(f.sdDir, ignored);
    fsx::remove_all(f.root, ignored);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Round 15 Save Instances (Gen I-V chooser, manual assignment, initialized-save rule) through "
                 "the real SaveSelectScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
