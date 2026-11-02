// Round 13 owner decision 4 ("Yes, implement Gen V touchscreen drag multi-select with controller parity
// and regression tests"), through the REAL TrainerViewScreen::update / draw (tests/host-ui harness).
//
// Every case runs on TWO identical synthetic saves: one driven only by the controller (Y hold + D-pad,
// Y release, A, B), the other only by the touchscreen (press-and-drag, lift, tap, tap outside). Each
// step must leave both screens in the same state:
//   * rectangle selection: a drag from one cell to another highlights the same cells as Y hold + D-pad;
//   * group pickup: lifting the finger picks up the same group as releasing Y, silently;
//   * one tap on a box cell during a carry is the controller's "cursor there + A" (owner Nov 2): a full
//     or occupied destination gives the same refusal text, stages nothing and keeps the carry; a valid
//     one stages exactly the bytes A stages, silently (a group and a single Pokemon);
//   * cancel: B during the drag, or a tap outside every touch target during the carry, cancels exactly
//     like B, silently;
//   * NOT a cancel or a place: the box arrows (they change box like L / R, Gen I / II packed carries
//     included), the summary panel, a footer tap, a drag outside the grid or across cells, - Help over
//     a carry, a tap under Help and the tap that closes Help;
//   * a single tap with nothing carried keeps its meaning (Gen V: focus, then Actions; Gen I-IV: Actions);
//   * a modal (Help, the Actions menu) owns the touch layer: a drag over the box cells behind it selects
//     and picks up nothing.
// Gen V is the generation the owner asked for. Gen I-IV run the SAME shared overlay (ClassicPackedMove)
// and the same cases, so the generations cannot drift apart. Every save is synthetic, built in code (QA
// builders, the gsc fixture, HostUiFixtures): no real save, ROM or Pokemon file is read.
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5PokemonFile.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "PokemonFile/NativePokemonFile.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace G3 = PokeVault::Integration::Gen3;
namespace G5 = PokeVault::Integration::Gen5;
using View = UI::TrainerViewScreen::ViewMode;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

// One screen's input loop: controller and/or finger, then update -> draw (as on the device).
struct Driver {
    explicit Driver(UI::TrainerViewScreen& s) : screen(s) {}
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    std::vector<HostUi::Outline> outlines;   // the last frame's
    std::vector<std::string> texts;
    std::vector<std::string> rows;           // the narrow Actions sheet's rows (x = 448)
    void frame(u64 down = 0, u64 held = 0) {
        PadState pad{down, held | down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
        outlines = HostUi::outlines;
        texts = HostUi::texts;
        rows.clear();
        for (const auto& t : HostUi::visibleText)
            if (t.x == 448 && t.style == 1) rows.push_back(t.text);
    }
    void press(u64 button) {
        frame(button);
        frame();
    }
    bool drawn(const std::string& text) const { return std::find(texts.begin(), texts.end(), text) != texts.end(); }
    bool quiet() const { return screen.storageStatusFrames == 0 || screen.storageStatus.empty(); }
    const UI::TrainerViewScreen::TouchButton* cell(int slot) const {
        for (const auto& b : screen.touchButtons)
            if (b.id == slot) return &b;
        return nullptr;
    }
    // The finger: down on a cell, slide (several frames, past the tap slop) to another, lift.
    bool fingerDown(int slot) {
        const auto* b = cell(slot);
        if (!b) return false;
        fx = b->x + b->w / 2;
        fy = b->y + b->h / 2;
        HostUi::touch = {true, static_cast<u32>(fx), static_cast<u32>(fy)};
        frame();
        return true;
    }
    bool slideTo(int slot) {
        const auto* b = cell(slot);
        if (!b) return false;
        const int tx = b->x + b->w / 2, ty = b->y + b->h / 2;
        const int sx = fx, sy = fy;
        for (int k = 1; k <= 4; ++k) {
            fx = sx + (tx - sx) * k / 4;
            fy = sy + (ty - sy) * k / 4;
            HostUi::touch = {true, static_cast<u32>(fx), static_cast<u32>(fy)};
            frame();
        }
        return true;
    }
    void lift() {
        HostUi::touch = {false, static_cast<u32>(fx), static_cast<u32>(fy)};
        frame();
    }
    bool tap(int slot) {
        if (!fingerDown(slot)) return false;
        lift();
        frame();
        return true;
    }
    bool tapId(int id) {
        const auto* b = cell(id);
        if (!b) return false;
        tapAt(b->x + b->w / 2, b->y + b->h / 2);
        return true;
    }
    void tapAt(int x, int y) {
        fx = x;
        fy = y;
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        lift();
        frame();
    }
    // A point above the footer that lies on no published touch target (outside the box grid).
    std::optional<std::pair<int, int>> outsidePoint() const {
        for (int y = 80; y < 640; y += 8)
            for (int x = 8; x < 1272; x += 8) {
                bool hit = false;
                for (const auto& b : screen.touchButtons)
                    hit = hit || (x >= b.x - 4 && x < b.x + b.w + 4 && y >= b.y - 4 && y < b.y + b.h + 4);
                if (!hit) return std::make_pair(x, y);
            }
        return std::nullopt;
    }
    void hover(int box, int slot) {
        screen.selectedBoxIndex = box;
        screen.selectedItemIndex = slot;
        frame();
    }
    int fx = 0, fy = 0;
};

// The cell outlines a selection rectangle draws (inset 2 px, radius 10, 3 px), sorted for comparison.
std::vector<HostUi::Outline> selectionOutlines(const Driver& d) {
    std::vector<HostUi::Outline> out;
    for (const auto& o : d.outlines) {
        if (o.radius != 10 || o.thickness != 3) continue;
        for (const auto& b : d.screen.touchButtons)
            if (b.id >= 0 && b.id < 30 && o.x == b.x + 2 && o.y == b.y + 2 && o.w == b.w - 4 && o.h == b.h - 4) {
                out.push_back(o);
                break;
            }
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
    return out;
}
std::vector<HostUi::Outline> cellsAsOutlines(const Driver& d, std::initializer_list<int> slots) {
    std::vector<HostUi::Outline> out;
    for (const int s : slots)
        if (const auto* b = d.cell(s)) out.push_back({b->x + 2, b->y + 2, b->w - 4, b->h - 4, 10, 3, 0});
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
    return out;
}
bool sameCells(std::vector<HostUi::Outline> a, std::vector<HostUi::Outline> b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (a[i].x != b[i].x || a[i].y != b[i].y || a[i].w != b[i].w || a[i].h != b[i].h) return false;
    return true;
}
std::string carriedCount(const Driver& d) {
    for (const auto& t : d.texts)
        if (t.size() > 8 && t.compare(t.size() - 8, 8, " Pokemon") == 0 && std::isdigit(static_cast<unsigned char>(t[0])))
            return t;
    return "";
}

// ---- Synthetic saves: Box 1 holds the group, Box 2 is full, Box 3 is empty. ----
struct Fixture {
    int generation = 0;
    std::string game;
    UI::TrainerViewScreen* screen = nullptr;
    int capacity = 30;
    std::function<std::vector<uint8_t>()> snapshot;   // the staged bytes (+ pending count)
    std::function<bool(int, int)> occupied;
};
std::deque<std::vector<uint8_t>> heldBytes;
std::vector<std::unique_ptr<Trainer::Trainer>> heldTrainers;
std::vector<std::unique_ptr<UI::TrainerViewScreen>> heldScreens;

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
Trainer::Trainer* openClassic(const qasynth::Bytes& bytes, const std::string& game, PokeVault::Safety::SourceKind kind,
                              const std::string& provider, UI::TrainerViewScreen*& screen) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    char pattern[] = "/tmp/pokebank-touch-multiselect-save-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, game + ": temp dir");
    if (!dir) return nullptr;
    const std::string path = std::string(dir) + "/" + game + ".sav";
    writeFile(path, bytes);
    std::string error;
    S::File file;
    check(S::read(path, dir, {}, file, error), game + ": read synthetic save: " + error);
    auto trainer = PokeVault::Legacy::openBackupTrainer(file, game, error);
    check(trainer != nullptr, game + ": strict parse + trainer: " + error);
    fs::remove_all(dir);
    if (!trainer) return nullptr;
    heldScreens.push_back(std::make_unique<UI::TrainerViewScreen>(*trainer, "Pokemon " + game, path, 0, AccountUid{},
                                                                  kind, game, provider));
    screen = heldScreens.back().get();
    heldTrainers.push_back(std::move(trainer));
    return heldTrainers.back().get();
}
template <class Editor>
std::vector<uint8_t> classicSnapshot(const Editor* e) {
    const auto bytes = e->stagedBytes();
    std::vector<uint8_t> out(bytes.begin(), bytes.end());
    out.push_back(static_cast<uint8_t>(e->pendingChanges().size()));
    return out;
}

std::optional<Fixture> gen1() {
    using Kind = PokeVault::Safety::SourceKind;
    Fixture f;
    f.generation = 1;
    f.game = "red_gb";
    auto* rby = dynamic_cast<PokeVault::Legacy::RBYReadOnlyTrainer*>(
        openClassic(qasynth::gen1::save(), "red_gb", Kind::RetroArchLegacy, "RetroArch", f.screen));
    if (!rby || !rby->stagedPokemon()) return std::nullopt;
    auto* e = rby->stagedPokemon();
    std::string error;
    PokeVault::Integration::Gen1::BoxPokemonCreate mon;
    mon.species = 1;
    bool ok = true;
    for (size_t slot = 0; slot < 3; ++slot) ok = ok && e->stageAdd(0, slot, mon, error);
    for (size_t slot = 0; slot < 20; ++slot) ok = ok && e->stageAdd(1, slot, mon, error);
    check(ok && rby->refreshBoxesFromStagedPokemon(error), "Gen 1 fixture: " + error);
    f.capacity = 20;
    f.snapshot = [e] { return classicSnapshot(e); };
    f.occupied = [e](int box, int slot) {
        std::string ignored;
        return e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored).has_value();
    };
    return f;
}

std::optional<Fixture> gen2() {
    using Kind = PokeVault::Safety::SourceKind;
    Fixture f;
    f.generation = 2;
    f.game = "gold_gbc";
    auto* gsc = dynamic_cast<PokeVault::Legacy::GSCReadOnlyTrainer*>(
        openClassic(fixture(GS), "gold_gbc", Kind::RetroArchLegacy, "RetroArch", f.screen));
    if (!gsc || !gsc->stagedEditor()) return std::nullopt;
    auto* e = gsc->stagedEditor();
    std::string error;
    PokeVault::Integration::Gen2::BoxPokemonCreate create;
    create.species = 152;
    create.level = 10;
    create.nickname = "LEAF";
    create.otName = "GOLD";
    std::size_t added = 0;
    const int capacity = static_cast<int>(e->metadata().boxCapacity);
    const auto count = [&](size_t box) {
        int n = 0;
        std::string ignored;
        while (n < capacity && e->boxedPokemon(box, static_cast<size_t>(n), ignored)) ++n;
        return n;
    };
    bool ok = true;
    while (ok && count(0) < 3) ok = e->stageAddBoxPokemon(0, create, added, error);
    while (ok && count(1) < capacity) ok = e->stageAddBoxPokemon(1, create, added, error);
    check(ok && gsc->refreshStagedBoxPresentation(error), "Gen 2 fixture: " + error);
    f.capacity = capacity;
    f.snapshot = [e] { return classicSnapshot(e); };
    f.occupied = [e](int box, int slot) {
        std::string ignored;
        return e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored).has_value();
    };
    return f;
}

std::optional<Fixture> gen3() {
    Fixture f;
    f.generation = 3;
    f.game = "firered_gba";
    heldBytes.push_back(HostUiFixtures::gen3Fixture(0));
    auto parsed = G3::parse(heldBytes.back(), G3::SourceGame::FireRedGBA);
    check(static_cast<bool>(parsed), "Gen III fixture parses: " + parsed.detail);
    if (!parsed) return std::nullopt;
    std::string error;
    auto trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    if (!trainer || !trainer->stagedPokemon()) return std::nullopt;
    auto* raw = trainer.get();
    heldScreens.push_back(std::make_unique<UI::TrainerViewScreen>(*trainer, "Pokemon FireRed", "/host/firered.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA"));
    f.screen = heldScreens.back().get();
    heldTrainers.push_back(std::move(trainer));
    auto* e = raw->stagedPokemon();
    const auto pk3 = e->exportPartyPokemonFile(0, error);
    const auto decoded = pk3 ? PokeBank::PokemonFile::decode(*pk3, "pika.pk3", error) : std::nullopt;
    bool ok = decoded.has_value();
    for (const int slot : {0, 1, 7}) ok = ok && e->stageImportBoxPokemon(0, slot, *decoded, error);
    for (int slot = 0; slot < 30; ++slot) ok = ok && e->stageImportBoxPokemon(1, slot, *decoded, error);
    check(ok && raw->refreshStagedPokemonPresentation(error), "Gen 3 fixture: " + error);
    f.snapshot = [e] { return classicSnapshot(e); };
    f.occupied = [e](int box, int slot) {
        std::string ignored;
        return e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), ignored).has_value();
    };
    return f;
}

std::optional<Fixture> gen4() {
    using Kind = PokeVault::Safety::SourceKind;
    Fixture f;
    f.generation = 4;
    f.game = "platinum_nds";
    auto* g4 = dynamic_cast<PokeVault::Legacy::Gen4ReadOnlyTrainer*>(
        openClassic(qasynth::gen4::save(qasynth::gen4::Kind::Platinum), "platinum_nds", Kind::ExternalLegacy, "DraStic",
                    f.screen));
    if (!g4 || !g4->stagedPokemon()) return std::nullopt;
    auto* e = g4->stagedPokemon();
    std::string error;
    bool ok = true;
    const auto put = [&](size_t box, size_t slot) {
        if (!ok || e->boxedPokemon(box, slot)) return;
        auto draft = e->createBoxDraft(box, slot, 393, &error);
        ok = draft && e->stageCreateBoxPokemon(box, slot, *draft, &error);
    };
    for (const size_t slot : {0u, 1u, 7u}) put(0, slot);
    for (size_t slot = 0; slot < 30; ++slot) put(1, slot);
    check(ok && g4->refreshStagedPokemonPresentation(error), "Gen 4 fixture: " + error);
    f.snapshot = [e] { return e->stagedBytes(); };
    f.occupied = [e](int box, int slot) {
        return e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot)).has_value();
    };
    return f;
}

std::vector<uint8_t> pk5() {
    namespace C = PokeVault::Integration::Gen5::Crypto;
    std::vector<uint8_t> b(C::StoredSize, 0);
    const auto put32 = [&](size_t o, uint32_t v) {
        C::write16(b, o, static_cast<uint16_t>(v));
        C::write16(b, o + 2, static_cast<uint16_t>(v >> 16));
    };
    put32(0, 0x2468ACE1u);
    C::write16(b, 0x08, 25);
    C::write16(b, 0x0C, 12345);
    C::write16(b, 0x0E, 54321);
    put32(0x10, 1000);
    b[0x14] = 70;
    b[0x15] = 9;
    b[0x17] = 2;
    b[0x18] = 4;
    C::write16(b, 0x28, 33);
    b[0x30] = 35;
    put32(0x38, 0x0012345u);
    b[0x41] = 3;
    const char16_t name[] = u"SPARK";
    for (size_t i = 0; i < 5; ++i) C::write16(b, 0x48 + i * 2, name[i]);
    C::write16(b, 0x48 + 10, 0xFFFF);
    b[0x5F] = 20;
    const char16_t ot[] = u"ASH";
    for (size_t i = 0; i < 3; ++i) C::write16(b, 0x68 + i * 2, ot[i]);
    C::write16(b, 0x68 + 6, 0xFFFF);
    b[0x7B] = 11; b[0x7C] = 3; b[0x7D] = 9;
    C::write16(b, 0x80, 4);
    b[0x83] = 4;
    b[0x84] = 5;
    C::write16(b, 0x06, C::checksum(b));
    return b;
}

std::optional<Fixture> gen5() {
    Fixture f;
    f.generation = 5;
    f.game = "black_nds";
    heldBytes.push_back(HostUiFixtures::gen5Fixture());
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(heldBytes.back(), "black_nds", &error, G5::SaveCopySelection::Primary);
    check(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return std::nullopt;
    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    if (!trainer) return std::nullopt;
    auto* raw = trainer.get();
    heldScreens.push_back(std::make_unique<UI::TrainerViewScreen>(*trainer, "Pokemon Black", "/host/black.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic"));
    f.screen = heldScreens.back().get();
    heldTrainers.push_back(std::move(trainer));
    const auto decoded = PokeBank::PokemonFile::decode(pk5(), "seed.pk5", error);
    bool ok = decoded.has_value();
    for (const size_t slot : {0u, 1u, 7u}) ok = ok && G5::importPokemonFile5(raw->stagedPokemon(), *decoded, 0, slot, &error);
    for (size_t slot = 0; slot < 30; ++slot) ok = ok && G5::importPokemonFile5(raw->stagedPokemon(), *decoded, 1, slot, &error);
    check(ok && raw->refreshStagedPokemonPresentation(error), "Gen 5 fixture: " + error);
    f.snapshot = [raw] {
        const auto& ws = raw->stagedPokemon();
        std::vector<uint8_t> snap{static_cast<uint8_t>(ws.changedRecordCount()),
                                  static_cast<uint8_t>(ws.pendingReview().size())};
        for (size_t box = 0; box < 4; ++box)
            for (size_t slot = 0; slot < 30; ++slot) {
                snap.push_back(static_cast<uint8_t>(slot));
                if (const auto p = ws.viewBox(box, slot); p && p->valid() && !p->empty())
                    snap.insert(snap.end(), p->decodedBytes().begin(), p->decodedBytes().end());
            }
        return snap;
    };
    f.occupied = [raw](int box, int slot) {
        const auto p = raw->stagedPokemon().viewBox(static_cast<size_t>(box), static_cast<size_t>(slot));
        return p && p->valid() && !p->empty();
    };
    return f;
}

// ---- The parity cases. ----
// The carry overlay keeps ONE interaction state for the screen on display (as on the device), so the two
// screens never interleave frames: the controller script runs to the end on save A, then the touch script
// on save B, each recording what the player sees at the same checkpoints; the checkpoints must match.
struct Obs {
    std::string label;
    std::vector<HostUi::Outline> selection;   // the highlighted rectangle's cells
    std::vector<HostUi::Outline> rectangle;   // where Box 1 / Slots 1-2 / 7-8 are drawn on this frame
    bool carrying = false;                    // the carry footer ("Place Group" / "Place")
    std::string count;                        // the "N Pokemon" badge of a carried group
    std::string status;                       // the toast text (empty: none)
    bool quiet = true;
    std::vector<uint8_t> snapshot;            // the staged image
    int box = 0, slot = 0;                    // the cursor
    std::vector<std::string> rows;            // an open Actions menu
    bool help = false;                        // Help is open
};
Obs observe(const Driver& d, const Fixture& f, const std::string& label) {
    Obs o;
    o.label = label;
    o.selection = selectionOutlines(d);
    o.rectangle = cellsAsOutlines(d, {0, 1, 6, 7});
    o.carrying = d.drawn("Place Group") || d.drawn("Place");
    o.count = carriedCount(d);
    o.status = d.screen.storageStatusFrames > 0 ? d.screen.storageStatus : std::string();
    o.quiet = d.quiet();
    o.snapshot = f.snapshot();
    o.box = d.screen.selectedBoxIndex;
    o.slot = d.screen.selectedItemIndex;
    o.rows = d.rows;
    o.help = d.screen.helpOverlayActive;
    return o;
}

void enterBoxes(Driver& d) {
    d.screen.goBack = false;
    d.screen.helpOverlayActive = false;
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Boxes;
    d.hover(0, 0);
    d.frame();
}
// Controller: Y held on Box 1 / Slot 1, Right and Down while held (the 2 x 2 rectangle Slots 1-2 / 7-8).
void padSelect(Driver& d) {
    d.hover(0, 0);
    d.frame(HidNpadButton_Y, HidNpadButton_Y);
    for (int i = 0; i < 9; ++i) d.frame(0, HidNpadButton_Y);
    for (const u64 key : {HidNpadButton_Right, HidNpadButton_Down}) {
        d.frame(key, HidNpadButton_Y);
        d.frame(0, HidNpadButton_Y);
    }
}
// Touch: the cursor starts elsewhere (the anchor is where the finger lands); press Slot 1, drag to Slot 8.
bool fingerSelect(Driver& d) {
    d.hover(0, 4);
    return d.fingerDown(0) && d.slideTo(7);
}
// Changing box during a carry: the controller's L / R, the finger's box arrows (1000 / 1001). A tap on a
// box arrow is another control: it must change the box, never place or cancel.
void toBox(Driver& d, bool touch, int box) {
    for (int guard = 0; guard < 30 && d.screen.selectedBoxIndex != box; ++guard) {
        const bool next = d.screen.selectedBoxIndex < box;
        if (touch) check(d.tapId(next ? 1001 : 1000), "the box arrows publish touch targets");
        else d.press(next ? HidNpadButton_R : HidNpadButton_L);
    }
}
// Owner Nov 2: "tapping a valid destination places" - ONE tap on a cell is the controller's cursor + A.
void placeAt(Driver& d, bool touch, int box, int slot) {
    toBox(d, touch, box);
    if (touch) {
        check(d.tap(slot), "tap a box cell");
    } else {
        d.hover(box, slot);
        d.press(HidNpadButton_A);
    }
}
int firstOccupied(const Fixture& f, int box) {
    for (int s = 0; s < f.capacity; ++s)
        if (f.occupied(box, s)) return s;
    return -1;
}

std::vector<Obs> script(Driver& d, const Fixture& f, bool touch) {
    std::vector<Obs> out;
    const std::string g = "Gen " + std::to_string(f.generation) + (touch ? " touch: " : " controller: ");
    enterBoxes(d);
    // 1. Rectangle selection.
    if (touch) check(fingerSelect(d), g + "the box cells publish touch targets");
    else padSelect(d);
    out.push_back(observe(d, f, "selecting"));
    // 2. B during the selection.
    if (touch) {
        d.frame(HidNpadButton_B);
        d.lift();
    } else {
        d.frame(HidNpadButton_B, HidNpadButton_Y);
    }
    d.frame();
    out.push_back(observe(d, f, "selection cancelled"));
    // 3. Group pickup: lift the finger / release Y.
    if (touch) {
        check(fingerSelect(d), g + "drag again");
        d.lift();
    } else {
        padSelect(d);
        d.frame();   // release Y
    }
    d.frame();
    out.push_back(observe(d, f, "picked up"));
    // 4. Box 2 by its arrow / R (never a place or a cancel), then the full destination refused like A.
    toBox(d, touch, 1);
    out.push_back(observe(d, f, "box arrow"));
    placeAt(d, touch, 1, 8);
    out.push_back(observe(d, f, "full destination"));
    // 5. Taps and drags that are NOT a cancel or a place: the summary panel, a footer tap, a drag outside
    // the grid, a drag across cells, and the tap that closes Help.
    if (touch) {
        if (d.cell(2000)) check(d.tapId(2000), g + "tap the summary panel");
        d.tapAt(1272, 708);   // the footer band
        if (const auto outside = d.outsidePoint()) {   // a drag that starts and ends outside the grid
            d.fx = outside->first;
            d.fy = outside->second;
            HostUi::touch = {true, static_cast<u32>(d.fx), static_cast<u32>(d.fy)};
            d.frame();
            for (int k = 1; k <= 4; ++k) {
                HostUi::touch = {true, static_cast<u32>(d.fx), static_cast<u32>(d.fy + 8 * k)};
                d.frame();
            }
            HostUi::touch = {false, static_cast<u32>(d.fx), static_cast<u32>(d.fy + 32)};
            d.frame();
            d.frame();
        }
        check(d.fingerDown(10) && d.slideTo(17), g + "drag across cells");
        d.lift();
        d.frame();
    }
    out.push_back(observe(d, f, "controls, drags"));
    d.press(HidNpadButton_Minus);
    out.push_back(observe(d, f, "help over the carry"));
    if (touch) {
        // A tap where a box cell is drawn while Help is up: Help owns it (7D may close Help with it); it
        // never places or cancels the carry underneath. Then the tap outside Help that closes it.
        // (Gen I / II keep - inside a packed carry, so Help only opens over a Gen III-V carry.)
        if (const auto* c = d.cell(3); c && d.screen.helpOverlayActive) d.tapAt(c->x + c->w / 2, c->y + c->h / 2);
        out.push_back(observe(d, f, "tap under help"));
        if (d.screen.helpOverlayActive) d.tapAt(8, 120);
        if (f.generation >= 3)
            check(!d.screen.helpOverlayActive, g + "a tap outside Help closed it (the modal-close tap under test)");
        if (d.screen.helpOverlayActive) d.press(HidNpadButton_Minus);
    } else {
        out.push_back(observe(d, f, "tap under help"));
        d.press(HidNpadButton_Minus);
    }
    d.frame();
    out.push_back(observe(d, f, "help closed"));
    // 6. Cancel the carry: B, or a tap outside every touch target.
    if (touch) {
        const auto outside = d.outsidePoint();
        check(outside.has_value(), g + "the screen has an area outside the grid");
        if (outside) d.tapAt(outside->first, outside->second);
    } else {
        d.press(HidNpadButton_B);
    }
    out.push_back(observe(d, f, "carry cancelled"));
    // 7. Group placement on Box 3 / Slot 9 with one tap (controller: cursor + A).
    if (touch) {
        check(fingerSelect(d), g + "drag a third time");
        d.lift();
    } else {
        padSelect(d);
        d.frame();
    }
    d.frame();
    placeAt(d, touch, 2, 8);
    out.push_back(observe(d, f, "placed"));
    // 8. One Pokemon: a one-cell drag / Y hold picks it up; an occupied (or full) slot is refused like A;
    // one tap on an empty slot places it.
    const int single = firstOccupied(f, 2);
    check(single >= 0, g + "the placed group is in Box 3");
    d.hover(2, single);
    if (touch) {
        const auto* c = d.cell(single);
        check(c != nullptr, g + "the cell publishes a target");
        if (c) {
            d.fx = c->x + 8;
            d.fy = c->y + 8;
            HostUi::touch = {true, static_cast<u32>(d.fx), static_cast<u32>(d.fy)};
            d.frame();
            HostUi::touch = {true, static_cast<u32>(c->x + c->w - 8), static_cast<u32>(c->y + c->h - 8)};
            d.frame();
            d.fx = c->x + c->w - 8;
            d.fy = c->y + c->h - 8;
            d.lift();
        }
    } else {
        d.frame(HidNpadButton_Y, HidNpadButton_Y);
        for (int i = 0; i < 9; ++i) d.frame(0, HidNpadButton_Y);
        d.frame();
    }
    d.frame();
    out.push_back(observe(d, f, "single picked up"));
    if (f.generation != 4) {   // Gen IV swaps a single Pokemon losslessly onto an occupied slot
        placeAt(d, touch, 1, 5);
        out.push_back(observe(d, f, "single occupied"));
    }
    placeAt(d, touch, 3, 3);
    out.push_back(observe(d, f, "single placed"));
    // 7. A single tap keeps today's meaning. Gen V: a tap focuses a cell and a tap on the focused cell is
    // A (decodeGen5BoxTouch); Gen I-IV: a tap on a cell opens its Actions at once (the same sheet as A).
    d.hover(1, 3);
    if (touch) {
        check(d.tap(4), "Gen " + std::to_string(f.generation) + ": tap Box 2 / Slot 5");
        if (d.rows.empty()) {
            out.push_back(observe(d, f, "single tap focuses"));
            check(d.tap(4), "Gen " + std::to_string(f.generation) + ": tap it again");
        }
    } else {
        d.hover(1, 4);
        if (f.generation == 5) out.push_back(observe(d, f, "single tap focuses"));
        d.press(HidNpadButton_A);
    }
    out.push_back(observe(d, f, "actions"));
    d.press(HidNpadButton_B);
    out.push_back(observe(d, f, "actions closed"));
    return out;
}

// Touch only: a modal owns the touch layer. A drag over the box cells behind Help or Actions does nothing.
void modalNoOp(Driver& d, const Fixture& f) {
    const std::string g = "Gen " + std::to_string(f.generation) + " " + f.game + " modal: ";
    enterBoxes(d);
    d.hover(1, 4);
    const auto settled = f.snapshot();
    const auto dragAcross = [&](int x, int y) {
        d.fx = x;
        d.fy = y;
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        d.frame();
        for (int k = 1; k <= 4; ++k) {
            HostUi::touch = {true, static_cast<u32>(x + 30 * k), static_cast<u32>(y + 30 * k)};
            d.frame();
            check(selectionOutlines(d).empty(), g + "no rectangle is drawn behind the modal");
        }
        HostUi::touch = {false, static_cast<u32>(x + 120), static_cast<u32>(y + 120)};
        d.frame();
        d.frame();
    };
    // Where the grid's first cell is drawn (published by the grid before the modal opens).
    const auto* first = d.cell(0);
    check(first != nullptr, g + "the grid publishes its cells");
    const int cx = first ? first->x + first->w / 2 : 200, cy = first ? first->y + first->h / 2 : 200;
    d.press(HidNpadButton_Minus);
    check(d.screen.helpOverlayActive, g + "- opens Help");
    dragAcross(cx, cy);
    check(d.screen.helpOverlayActive && !d.drawn("Place Group") && !d.drawn("Place"),
          g + "a drag behind Help picks up nothing");
    d.press(HidNpadButton_B);
    check(!d.screen.helpOverlayActive && !d.drawn("Place Group") && !d.drawn("Place") && d.screen.selectedItemIndex == 4 &&
              d.screen.selectedBoxIndex == 1 && f.snapshot() == settled,
          g + "after Help closes nothing was selected, picked up or staged");
    d.press(HidNpadButton_A);
    check(!d.rows.empty(), g + "A opens Actions");
    dragAcross(cx, cy);
    check(!d.drawn("Place Group") && !d.drawn("Place") && f.snapshot() == settled,
          g + "a drag behind the Actions menu picks up nothing");
    d.press(HidNpadButton_B);
    check(d.rows.empty() && !d.drawn("Place Group") && !d.drawn("Place") && f.snapshot() == settled,
          g + "closing Actions leaves nothing carried or staged");
    // No stale gesture is left behind: the grid owns the next drag again.
    d.hover(1, 4);
    check(d.fingerDown(0) && d.slideTo(1), g + "drag after the modal closed");
    check(sameCells(selectionOutlines(d), cellsAsOutlines(d, {0, 1})), g + "the grid owns the next drag again");
    d.frame(HidNpadButton_B);
    d.lift();
    d.frame();
    check(!d.drawn("Place Group") && f.snapshot() == settled, g + "B cancelled that drag");
}

void runParity(const Fixture& a, const Fixture& b) {
    const std::string g = "Gen " + std::to_string(a.generation) + " " + a.game + " touch == controller: ";
    const auto before = a.snapshot();
    check(before == b.snapshot(), g + "the two synthetic saves start identical");
    check(!a.occupied(2, 0) && !a.occupied(2, 8) && a.occupied(1, a.capacity - 1), g + "Box 3 is empty and Box 2 is full");
    Driver pad(*a.screen);
    const auto controller = script(pad, a, false);
    Driver finger(*b.screen);
    const auto touch = script(finger, b, true);
    const auto at = [](const std::vector<Obs>& v, const std::string& label) -> const Obs& {
        for (const auto& o : v)
            if (o.label == label) return o;
        static const Obs none;
        return none;
    };
    // Absolute expectations (the controller is the reference behaviour, the touch must equal it).
    for (const auto* run : {&controller, &touch}) {
        const std::string who = run == &controller ? "controller: " : "touch: ";
        check(at(*run, "selecting").rectangle.size() == 4 &&
                  sameCells(at(*run, "selecting").selection, at(*run, "selecting").rectangle),
              g + who + "the rectangle is Box 1 / Slots 1-2 / 7-8 (" + std::to_string(at(*run, "selecting").selection.size()) +
                  " cells)");
        check(!at(*run, "selecting").carrying && at(*run, "selecting").snapshot == before, g + who + "selecting stages nothing");
        check(at(*run, "selection cancelled").selection.empty() && !at(*run, "selection cancelled").carrying &&
                  at(*run, "selection cancelled").status.empty() && at(*run, "selection cancelled").snapshot == before,
              g + who + "B cancels the selection silently");
        check(at(*run, "picked up").carrying && !at(*run, "picked up").count.empty() && at(*run, "picked up").status.empty(),
              g + who + "the group is picked up silently (" + at(*run, "picked up").count + ", \"" +
                  at(*run, "picked up").status + "\")");
        check(at(*run, "full destination").carrying && !at(*run, "full destination").status.empty() &&
                  at(*run, "full destination").snapshot == at(*run, "picked up").snapshot,
              g + who + "a full destination is refused, nothing staged, the carry stays (\"" +
                  at(*run, "full destination").status + "\")");
        check(!at(*run, "carry cancelled").carrying && at(*run, "carry cancelled").status.empty() &&
                  at(*run, "carry cancelled").snapshot == before,
              g + who + "cancelling the carry restores the exact staged state silently (\"" +
                  at(*run, "carry cancelled").status + "\")");
        check(!at(*run, "placed").carrying && at(*run, "placed").status.empty() && at(*run, "placed").snapshot != before &&
                  at(*run, "placed").rows.empty(),
              g + who + "the group is placed silently, no Actions menu (\"" + at(*run, "placed").status + "\")");
        if (a.generation == 5)
            check(at(*run, "single tap focuses").label == "single tap focuses" && at(*run, "single tap focuses").rows.empty() &&
                      !at(*run, "single tap focuses").carrying && at(*run, "single tap focuses").slot == 4,
                  g + who + "Gen V: the first tap / D-pad only focuses Box 2 / Slot 5");
        else
            check(at(*run, "single tap focuses").label.empty(), g + who + "Gen I-IV: one tap opens Actions, as before");
        check(!at(*run, "actions").carrying && at(*run, "actions").slot == 4 && at(*run, "actions").box == 1,
              g + who + "the tap neither selects nor picks up anything");
        check(!at(*run, "actions").rows.empty(), g + who + "a tap on the focused cell / A opens Actions");
        check(at(*run, "actions closed").rows.empty(), g + who + "B closes Actions");
    }
    for (const auto* run : {&controller, &touch}) {
        const std::string who = run == &controller ? "controller: " : "touch: ";
        const auto& picked = at(*run, "picked up");
        for (const std::string label : {"box arrow", "controls, drags", "help over the carry", "tap under help",
                                         "help closed"}) {
            const bool help = label == "help over the carry" || label == "tap under help";   // Help hides the footer
            check((at(*run, label).carrying || help) && (at(*run, label).count == picked.count || help) &&
                      at(*run, label).snapshot == picked.snapshot && at(*run, label).box == 1 &&
                      (label == "box arrow" || at(*run, label).slot == at(*run, "full destination").slot),
                  g + who + label + ": the carry stays, nothing placed or cancelled, Box 2 shown (" +
                      std::to_string(at(*run, label).box) + ")");
        }
        if (a.generation >= 3)
            check(at(*run, "help over the carry").help && !at(*run, "help closed").help,
                  g + who + "- opens Help over the carry and it closes again");
        check(at(*run, "single picked up").carrying && at(*run, "single picked up").count.empty(),
              g + who + "one Pokemon is picked up");
        if (a.generation != 4)
            check(at(*run, "single occupied").carrying && !at(*run, "single occupied").status.empty() &&
                  at(*run, "single occupied").snapshot == at(*run, "single picked up").snapshot,
              g + who + "an occupied / full slot is refused, the carry stays (\"" + at(*run, "single occupied").status + "\")");
        check(!at(*run, "single placed").carrying && at(*run, "single placed").status.empty() &&
                  at(*run, "single placed").snapshot != at(*run, "placed").snapshot && at(*run, "single placed").box == 3,
              g + who + "one tap / cursor + A places it in Box 4, silently (\"" + at(*run, "single placed").status + "\")");
    }
    // Parity: every checkpoint the player sees is the same.
    for (const auto& c : controller) {
        const auto& t = at(touch, c.label);
        check(t.label == c.label, g + c.label + ": checkpoint recorded");
        check(sameCells(t.selection, c.selection), g + c.label + ": same highlighted cells");
        check(t.carrying == c.carrying && t.count == c.count,
              g + c.label + ": same carry (" + c.count + " / " + t.count + ")");
        check(t.status == c.status, g + c.label + ": same message (\"" + c.status + "\" / \"" + t.status + "\")");
        check(t.snapshot == c.snapshot, g + c.label + ": same staged bytes");
        check(t.box == c.box && t.slot == c.slot,
              g + c.label + ": same cursor (" + std::to_string(c.box) + "/" + std::to_string(c.slot) + " vs " +
                  std::to_string(t.box) + "/" + std::to_string(t.slot) + ")");
        check(t.rows == c.rows, g + c.label + ": same Actions menu");
    }
    modalNoOp(finger, b);
    std::cout << g << "checked\n";
}

}  // namespace

int main() {
    char pattern[] = "/tmp/pokebank-touch-multiselect-XXXXXX";
    const char* made = ::mkdtemp(pattern);
    if (!made) {
        std::cerr << "mkdtemp failed\n";
        return EXIT_FAILURE;
    }
    const std::string root = made;
    const fs::path previous = fs::current_path();
    fs::current_path(root);
    fs::create_directory("sdmc:");

    // Gen V first: the generation the owner decision names.
    for (const auto& make : {gen5, gen1, gen2, gen3, gen4}) {
        auto a = make();
        auto b = make();
        check(a.has_value() && b.has_value(), "fixture pair opens");
        if (a && b) runParity(*a, *b);
    }

    fs::current_path(previous);
    fs::remove_all(root);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Touch drag multi-select == controller multi-select, Gen I-V (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
