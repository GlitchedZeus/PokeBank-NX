// B01-B06 / B04: the ONE shared Bag Add flow (UI/SharedBagAddFlow.h) for every Gen I-V game.
//
// Each generation drives the shared decisions against its REAL staged writer, exactly as its
// surface does: X opens the picker, B cancels with no mutation, A on a new item stages it at
// x1 with no keypad, A on a held item focuses the existing stack (never a duplicate), A on the
// row afterwards edits the quantity only when ItemQuantityPolicy says it is editable, a full
// pocket refuses with a precise message, Discard restores the source, and an export reopens
// through the strict parser while the source bytes stay byte-identical.
// Synthetic fixtures only (same builders as the per-generation staged inventory tests).
#include "UI/SharedBagAddFlow.h"

#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen5/Gen5BagScreen.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "UI/Gen4BagQuantity.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace QP = PokeVault::Inventory::QuantityPolicy;
namespace BagAdd = PokeBank::UIModel::BagAdd;
using PokeVault::Inventory::ClassicPocket;

static int failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            ++failures;                                                                   \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                                 \
    } while (0)

namespace {

// What one generation's surface exposes to the shared flow, over its real staged writer.
struct Backend {
    std::function<uint16_t(uint16_t)> held;                         // staged quantity of id in pocket
    std::function<bool(uint16_t, uint16_t, std::string&)> stage;    // stage id at quantity (0 = remove)
    std::function<std::vector<uint8_t>()> bytes;                    // current staged image
    std::function<BagAdd::PocketState(uint16_t)> state = {};        // optional: slots known to the surface
};

struct Case {
    const char* what;
    QP::Pocket pocket;
    uint16_t id;
    bool unique;
    uint16_t maximum;  // expected effective maximum (1 for unique)
};

BagAdd::PocketState pocketState(const Backend& b, uint16_t id) {
    if (b.state) return b.state(id);
    BagAdd::PocketState s{};
    s.heldInPocket = b.held(id);
    return s;
}

// The exact surface sequence for one item, against the real writer.
void addScenario(QP::Game game, const Case& c, const Backend& b, uint16_t writerCap = 0) {
    const auto rule = QP::quantityRule(game, c.pocket, c.id);
    CHECK(rule.valid);
    CHECK(rule.unique == c.unique);
    CHECK(BagAdd::effectiveMaximum(rule, writerCap) == c.maximum);
    CHECK(b.held(c.id) == 0);

    // 2/3/9: X opens the picker; B cancels with no mutation.
    BagAdd::Flow flow;
    const auto before = b.bytes();
    CHECK(!flow.openPicker(false));  // an empty picker never opens
    CHECK(flow.openPicker(true) && flow.pickerOpen());
    CHECK(flow.cancel() && !flow.pickerOpen());
    CHECK(!flow.cancel());           // consumed once
    CHECK(b.bytes() == before);
    // A outside the picker means nothing.
    CHECK(flow.pick(rule, pocketState(b, c.id)).outcome == BagAdd::PickOutcome::NotAllowed);

    // 4/5: A on a new item stages it at x1 at once; no keypad, picker closed.
    CHECK(flow.openPicker(true));
    const auto pick = flow.pick(rule, pocketState(b, c.id));
    CHECK(pick.outcome == BagAdd::PickOutcome::StageNew);
    CHECK(pick.quantity == 1);
    CHECK(!flow.pickerOpen());
    std::string error;
    CHECK(b.stage(c.id, pick.quantity, error));
    CHECK(b.held(c.id) == 1);

    // 6/7: A on the row: unique items get no editor and the writer agrees; stackable items
    // edit up to the native maximum and never beyond.
    const auto row = BagAdd::decideRow(rule, b.held(c.id), writerCap);
    if (c.unique) {
        CHECK(row.outcome == BagAdd::RowOutcome::FixedQuantity);
        CHECK(row.maximum == 1);
        const auto fixed = b.bytes();
        CHECK(!b.stage(c.id, 2, error));
        CHECK(b.bytes() == fixed);
    } else {
        CHECK(row.outcome == BagAdd::RowOutcome::EditQuantity);
        CHECK(row.minimum == 1 && row.maximum == c.maximum && row.current == 1);
        CHECK(b.stage(c.id, row.maximum, error));
        CHECK(b.held(c.id) == row.maximum);
        const auto capped = b.bytes();
        CHECK(!b.stage(c.id, static_cast<uint16_t>(row.maximum + 1), error));
        CHECK(b.bytes() == capped);
    }

    // 8: adding it again reuses the existing stack: focus only, nothing staged.
    const auto staged = b.bytes();
    CHECK(flow.openPicker(true));
    const auto again = flow.pick(rule, pocketState(b, c.id));
    CHECK(again.outcome == BagAdd::PickOutcome::FocusExisting);
    CHECK(!flow.pickerOpen());
    CHECK(b.bytes() == staged);
    CHECK(!BagAdd::decidePick(rule, pocketState(b, c.id)).message.empty());
}

void rejectsInvalid(QP::Game game, QP::Pocket pocket) {
    BagAdd::Flow flow;
    CHECK(flow.openPicker(true));
    const auto zero = flow.pick(QP::quantityRule(game, pocket, 0), {});
    CHECK(zero.outcome == BagAdd::PickOutcome::NotAllowed && zero.quantity == 0);
    CHECK(flow.pickerOpen());  // the player may still choose another row
    CHECK(BagAdd::decideRow(QP::quantityRule(game, pocket, 0xFFFF), 1).outcome ==
          BagAdd::RowOutcome::NotAllowed);
}

// ---------------------------------------------------------------- Gen I (R/B/Y) ----
namespace gen1 {
using namespace PokeVault::Integration::Gen1;

struct Layout {
    std::size_t checksum, mainLength, bag, pc, party, currentBox, currentBoxIndex, starter, friendship;
};
constexpr Layout INTL{0x3523, 0x0F8B, 0x25C9, 0x27E6, 0x2F2C, 0x30C0, 0x284C, 0x29C3, 0x271C};

uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value = 0xFF;
    for (uint8_t byte : bytes) value = static_cast<uint8_t>(value - byte);
    return value;
}
void writeBlock(std::vector<uint8_t>& raw, std::size_t offset, std::size_t capacity,
                const std::vector<InventoryEntry>& entries) {
    std::fill(raw.begin() + static_cast<std::ptrdiff_t>(offset),
              raw.begin() + static_cast<std::ptrdiff_t>(offset + 1 + capacity * 2 + 1), 0);
    raw[offset] = static_cast<uint8_t>(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        raw[offset + 1 + i * 2] = entries[i].itemId;
        raw[offset + 2 + i * 2] = entries[i].quantity;
    }
    raw[offset + 1 + entries.size() * 2] = 0xFF;
}
std::vector<uint8_t> fixture(SourceGame game, const std::vector<InventoryEntry>& bag) {
    const Layout& l = INTL;
    std::vector<uint8_t> raw(kRawSaveSize, 0);
    raw[l.party] = 0; raw[l.party + 1] = 0xFF;
    raw[l.currentBox] = 0; raw[l.currentBox + 1] = 0xFF;
    raw[l.currentBoxIndex] = 2;
    raw[l.starter] = game == SourceGame::Yellow ? 0x54 : 0x99;
    raw[l.friendship] = game == SourceGame::Yellow ? 90 : 0;
    writeBlock(raw, l.bag, 20, bag);
    writeBlock(raw, l.pc, 50, {});
    raw[l.checksum] = diff8(std::span<const uint8_t>(raw).subspan(0x2598, l.mainLength));
    return raw;
}
uint16_t heldIn(const StagedInventoryEditor& editor, ClassicPocket pocket, uint16_t id) {
    std::string error;
    for (const auto& e : editor.entries(pocket, error))
        if (e.itemId == id) return e.quantity;
    return 0;
}
Backend backend(StagedInventoryEditor& editor, ClassicPocket pocket) {
    Backend b;
    b.held = [&editor, pocket](uint16_t id) { return heldIn(editor, pocket, id); };
    b.stage = [&editor, pocket](uint16_t id, uint16_t q, std::string& e) {
        return editor.stageSetQuantity(pocket, id, q, e);
    };
    b.bytes = [&editor] { return std::vector<uint8_t>(editor.stagedBytes().begin(), editor.stagedBytes().end()); };
    return b;
}

void run(SourceGame source, QP::Game game) {
    const auto raw = fixture(source, {});
    const auto pristine = raw;
    auto parsed = parse(raw, source);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string error;
    auto editor = StagedInventoryEditor::create(*parsed.save, error);
    CHECK(editor != nullptr);
    if (!editor) return;
    const std::array<std::pair<Case, ClassicPocket>, 8> cases{{
        {{"Potion", QP::Pocket::Medicine, 20, false, 99}, ClassicPocket::Medicines},
        {{"Poke Ball", QP::Pocket::Balls, 4, false, 99}, ClassicPocket::Balls},
        {{"Bicycle", QP::Pocket::KeyItems, 6, true, 1}, ClassicPocket::KeyItems},
        {{"Old Rod", QP::Pocket::KeyItems, 76, true, 1}, ClassicPocket::KeyItems},
        {{"HM01", QP::Pocket::TMHM, 196, true, 1}, ClassicPocket::TMHM},
        {{"TM01", QP::Pocket::TMHM, 201, false, 99}, ClassicPocket::TMHM},
        {{"PC Potion", QP::Pocket::PCItems, 20, false, 99}, ClassicPocket::PCItems},
        {{"PC Super Rod", QP::Pocket::PCItems, 78, true, 1}, ClassicPocket::PCItems},
    }};
    for (const auto& [c, pocket] : cases) addScenario(game, c, backend(*editor, pocket));
    rejectsInvalid(game, QP::Pocket::Items);

    // Export + reopen through the strict parser; the source stays byte-identical.
    const auto out = editor->finalize(error);
    CHECK(!out.empty());
    auto reopened = parse(out, source);
    CHECK(static_cast<bool>(reopened));
    if (reopened) {
        auto again = StagedInventoryEditor::create(*reopened.save, error);
        CHECK(again && heldIn(*again, ClassicPocket::KeyItems, 6) == 1);
        CHECK(again && heldIn(*again, ClassicPocket::Medicines, 20) == 99);
        CHECK(again && heldIn(*again, ClassicPocket::TMHM, 196) == 1);
        CHECK(again && heldIn(*again, ClassicPocket::PCItems, 78) == 1);
    }
    CHECK(raw == pristine);
    CHECK(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(), pristine.begin()));
    // Discard restores every staged byte.
    editor->discard();
    CHECK(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), pristine.begin()));

    // Bag full: the 20-slot Bag refuses a new stack with the exact pocket named; nothing changes.
    std::vector<InventoryEntry> full;
    for (uint8_t id : {1, 2, 3, 5, 6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 29, 30, 31, 32, 33}) full.push_back({id, 1});
    const auto fullRaw = fixture(source, full);
    auto fullParsed = parse(fullRaw, source);
    CHECK(static_cast<bool>(fullParsed));
    if (!fullParsed) return;
    auto fullEditor = StagedInventoryEditor::create(*fullParsed.save, error);
    CHECK(fullEditor != nullptr);
    if (!fullEditor) return;
    const auto rule = QP::quantityRule(game, QP::Pocket::Medicine, 20);
    const auto decision = BagAdd::decidePick(rule, {heldIn(*fullEditor, ClassicPocket::Medicines, 20), 0, 0});
    CHECK(decision.outcome == BagAdd::PickOutcome::StageNew);  // capacity is the writer's to judge
    const auto beforeFull = std::vector<uint8_t>(fullEditor->stagedBytes().begin(), fullEditor->stagedBytes().end());
    CHECK(!fullEditor->stageSetQuantity(ClassicPocket::Medicines, 20, decision.quantity, error));
    CHECK(error.find("No free slots") != std::string::npos);
    CHECK(std::equal(beforeFull.begin(), beforeFull.end(), fullEditor->stagedBytes().begin()));
    // A held item in a full Bag is still reachable: it focuses the existing stack.
    CHECK(BagAdd::decidePick(QP::quantityRule(game, QP::Pocket::Balls, 1),
                             {heldIn(*fullEditor, ClassicPocket::Balls, 1), 0, 0}).outcome ==
          BagAdd::PickOutcome::FocusExisting);
}
} // namespace gen1

// ---------------------------------------------------------------- Gen II (G/S/C) ---
namespace gen2 {
using namespace PokeVault::Integration::Gen2;

struct L {
    VersionFamily family;
    std::size_t money, idx, names, party, current, end, sum, sum2, gender;
    std::size_t tm, items, key, balls, pc;
};
constexpr std::size_t N = static_cast<std::size_t>(-1);
constexpr L GS{VersionFamily::GoldSilver, 0x23DB, 0x2724, 0x2727, 0x288A, 0x2D6C, 0x2D68, 0x2D69, 0x7E6D, N,
               0x23E6, 0x241F, 0x2449, 0x2464, 0x247E};
constexpr L C{VersionFamily::Crystal, 0x23DC, 0x2700, 0x2703, 0x2865, 0x2D10, 0x2B82, 0x2D0D, 0x1F0D, 0x3E3D,
              0x23E7, 0x2420, 0x244A, 0x2465, 0x247F};

std::size_t listLen(std::size_t cap, std::size_t body, std::size_t str) { return 1 + (cap + 1) + cap * body + 2 * cap * str; }
std::size_t stride() { return listLen(20, 32, 11) + 2; }
std::size_t boxStart(std::size_t b) { return b < 7 ? 0x4000 + b * stride() : 0x6000 + (b - 7) * stride(); }
void emptyList(std::vector<uint8_t>& d, std::size_t ofs) { d[ofs] = 0; d[ofs + 1] = 0xFF; }
void name(std::vector<uint8_t>& d, std::size_t ofs, std::size_t len, char c) {
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(ofs), len, 0x50);
    d[ofs] = static_cast<uint8_t>(0x80 + c - 'A');
}
void pokemonList(std::vector<uint8_t>& d, std::size_t ofs, std::size_t cap, std::size_t bodySize, std::size_t str, bool party) {
    constexpr uint8_t species = 25;
    d[ofs] = 1; d[ofs + 1] = species; d[ofs + 2] = 0xFF;
    const std::size_t body = ofs + 1 + (cap + 1);
    d[body] = species; d[body + 1] = 1; d[body + 2] = 33; d[body + 3] = 45;
    d[body + 6] = 0x12; d[body + 7] = 0x34; d[body + 9] = 0x10;
    for (int i = 0; i < 5; i++) { d[body + 11 + 2 * i] = 0; d[body + 12 + 2 * i] = static_cast<uint8_t>(i + 1); }
    d[body + 21] = 0x7A; d[body + 22] = 0xAA; d[body + 23] = 10; d[body + 24] = 20;
    d[body + 27] = 123; d[body + 28] = 0x21; d[body + 31] = 15;
    if (party) {
        auto be = [&](std::size_t o, uint16_t v) { d[body + o] = static_cast<uint8_t>(v >> 8); d[body + o + 1] = static_cast<uint8_t>(v); };
        d[body + 32] = 0x08; be(34, 35); be(36, 40); be(38, 30); be(40, 20); be(42, 25); be(44, 26); be(46, 27);
    }
    const std::size_t ot = body + cap * bodySize;
    const std::size_t nick = ot + cap * str;
    name(d, ot, str, 'O'); name(d, nick, str, 'M');
}
std::vector<uint8_t> fixture(const L& l) {
    std::vector<uint8_t> d(0x8000);
    d[0x2009] = 0x12; d[0x200A] = 0x34; name(d, 0x200B, 11, 'A');
    d[l.money] = 0x01; d[l.money + 1] = 0xE2; d[l.money + 2] = 0x40;
    d[l.idx] = 0; if (l.gender != N) d[l.gender] = 1;
    for (std::size_t b = 0; b < 14; b++) { emptyList(d, boxStart(b)); name(d, l.names + b * 9, 9, static_cast<char>('A' + b)); }
    pokemonList(d, boxStart(0), 20, 32, 11, false);
    emptyList(d, l.current); pokemonList(d, l.party, 6, 48, 11, true);
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(l.tm), 57, 0);
    d[l.items] = 1; d[l.items + 1] = kPotionItemId; d[l.items + 2] = 2; d[l.items + 3] = 0xFF;  // Potion x2 held
    d[l.key] = 0; d[l.key + 1] = 0xFF;
    d[l.balls] = 0; d[l.balls + 1] = 0xFF;
    d[l.pc] = 0; d[l.pc + 1] = 0xFF;
    uint16_t s = 0;
    for (std::size_t i = 0x2009; i <= l.end; i++) s = static_cast<uint16_t>(s + d[i]);
    d[l.sum] = static_cast<uint8_t>(s); d[l.sum + 1] = static_cast<uint8_t>(s >> 8);
    return d;
}
Backend backend(StagedEditor& editor, InventoryPocket pocket) {
    Backend b;
    b.held = [&editor, pocket](uint16_t id) { return static_cast<uint16_t>(editor.stagedInventoryQuantity(pocket, static_cast<uint8_t>(id))); };
    b.stage = [&editor, pocket](uint16_t id, uint16_t q, std::string& e) {
        if (q > 255) { e = "outside the stored byte"; return false; }
        return editor.stageInventoryQuantity(pocket, static_cast<uint8_t>(id), static_cast<uint8_t>(q), e);
    };
    b.bytes = [&editor] { return std::vector<uint8_t>(editor.stagedBytes().begin(), editor.stagedBytes().end()); };
    return b;
}

void run(const L& layout, SourceGame source, QP::Game game) {
    const auto raw = fixture(layout);
    const auto pristine = raw;
    auto parsed = parse(raw, source);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    CHECK(editor != nullptr);
    if (!editor) return;

    // The held Potion: A in the picker focuses the existing stack, nothing is staged.
    const auto potion = QP::quantityRule(game, QP::Pocket::Items, kPotionItemId);
    const auto before = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    BagAdd::Flow flow;
    CHECK(flow.openPicker(true));
    CHECK(flow.pick(potion, {editor->stagedInventoryQuantity(InventoryPocket::Items, kPotionItemId), 1, 20}).outcome ==
          BagAdd::PickOutcome::FocusExisting);
    CHECK(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));

    const std::array<std::pair<Case, InventoryPocket>, 8> cases{{
        {{"Antidote", QP::Pocket::Items, 9, false, 99}, InventoryPocket::Items},
        {{"Master Ball", QP::Pocket::Balls, 1, false, 99}, InventoryPocket::Balls},
        {{"Bicycle", QP::Pocket::KeyItems, 7, true, 1}, InventoryPocket::KeyItems},
        {{"Old Rod", QP::Pocket::KeyItems, 58, true, 1}, InventoryPocket::KeyItems},
        {{"TM01", QP::Pocket::TMHM, 191, false, 99}, InventoryPocket::TMHM},
        {{"HM01", QP::Pocket::TMHM, 243, true, 1}, InventoryPocket::TMHM},
        {{"PC Potion", QP::Pocket::PCItems, kPotionItemId, false, 99}, InventoryPocket::PCItems},
        {{"PC Bicycle", QP::Pocket::PCItems, 7, true, 1}, InventoryPocket::PCItems},
    }};
    for (const auto& [c, pocket] : cases) addScenario(game, c, backend(*editor, pocket));
    rejectsInvalid(game, QP::Pocket::Balls);

    const auto out = editor->finalizedBytes(error);
    CHECK(!out.empty());
    auto reopened = parse(out, source);
    CHECK(static_cast<bool>(reopened));
    if (reopened) {
        auto again = StagedEditor::create(*reopened.save, error);
        CHECK(again && again->stagedInventoryQuantity(InventoryPocket::KeyItems, 7) == 1);
        CHECK(again && again->stagedInventoryQuantity(InventoryPocket::TMHM, 191) == 99);
        CHECK(again && again->stagedInventoryQuantity(InventoryPocket::TMHM, 243) == 1);
        CHECK(again && again->stagedInventoryQuantity(InventoryPocket::Items, kPotionItemId) == 2);
    }
    CHECK(raw == pristine);
    CHECK(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(), pristine.begin()));
    editor->discard();
    CHECK(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), pristine.begin()));

    // Pocket full: twenty Items stacks, the 21st new item is refused naming the pocket.
    const auto items = PokeVault::Inventory::addableItems(
        static_cast<PokeVault::Inventory::ClassicGame>(static_cast<uint8_t>(game)), ClassicPocket::Items);
    std::size_t staged = 1;  // the held Potion
    std::size_t next = 0;
    for (; staged < 20 && next < items.size(); ++next) {
        if (items[next] == kPotionItemId) continue;
        CHECK(editor->stageInventoryQuantity(InventoryPocket::Items, static_cast<uint8_t>(items[next]), 1, error));
        ++staged;
    }
    while (next < items.size() && items[next] == kPotionItemId) ++next;
    CHECK(next < items.size());
    if (next >= items.size()) return;
    const auto rule = QP::quantityRule(game, QP::Pocket::Items, items[next]);
    CHECK(BagAdd::decidePick(rule, {0, 0, 0}).outcome == BagAdd::PickOutcome::StageNew);
    const auto beforeFull = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    CHECK(!editor->stageInventoryQuantity(InventoryPocket::Items, static_cast<uint8_t>(items[next]), 1, error));
    CHECK(error.find("No free slots") != std::string::npos);
    CHECK(std::equal(beforeFull.begin(), beforeFull.end(), editor->stagedBytes().begin()));
}
} // namespace gen2

// ---------------------------------------------------------------- Gen III (R/S/E/FR/LG)
namespace gen3 {
using namespace PokeVault::Integration::Gen3;
enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
// Same synthetic layout as tests/test_gen3_staged_inventory.cpp: one held Potion (13) per family,
// and a held bike (Mach Bike 259 in R/S/E, Coin Case 260 in FR/LG) in Key Items.
LogicalSectors makeLogical(Family family) {
    LogicalSectors logical{};
    const auto put = [&](std::size_t offset, uint16_t item, uint16_t quantity, uint16_t key16) {
        write16(logical[1], offset, item);
        write16(logical[1], offset + 2, static_cast<uint16_t>(quantity ^ key16));
    };
    if (family == Family::FRLG) {
        write32(logical[0], 0xAC, 1);
        const uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xF20, key);
        put(0x0310, 13, 3, static_cast<uint16_t>(key));
        put(0x03B8, 260, 1, static_cast<uint16_t>(key));
    } else if (family == Family::Emerald) {
        const uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        put(0x0560, 13, 3, static_cast<uint16_t>(key));
        put(0x05D8, 259, 1, static_cast<uint16_t>(key));
    } else {
        write32(logical[0], 0xAC, 0);
        put(0x0560, 13, 3, 0);
        put(0x05B0, 259, 1, 0);
    }
    return logical;
}
void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family) {
    auto logical = makeLogical(family);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}
std::vector<uint8_t> fixture(Family family) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family);
    writeSlot(save, 1, 9, 5, family);
    return save;
}
Backend backend(StagedInventoryEditor& editor, InventoryPouch pouch) {
    Backend b;
    b.held = [&editor, pouch](uint16_t id) { return editor.stagedInventoryQuantity(pouch, id); };
    b.stage = [&editor, pouch](uint16_t id, uint16_t q, std::string& e) { return editor.stageInventoryQuantity(pouch, id, q, e); };
    b.bytes = [&editor] { return editor.stagedBytes(); };
    return b;
}

void run(SourceGame source, Family family, QP::Game game) {
    const bool frlg = family == Family::FRLG;
    const uint16_t bagCap = frlg ? 999 : 99;
    const auto raw = fixture(family);
    const auto pristine = raw;
    std::string error;
    auto editor = StagedInventoryEditor::create(raw, source, error);
    CHECK(editor != nullptr);
    if (!editor) return;

    // Held Potion and held bike: A focuses the existing stack, never a duplicate entry.
    const uint16_t heldKey = frlg ? 260 : 259;
    for (const auto& [pocket, pouch, id] : {std::tuple{QP::Pocket::Items, InventoryPouch::Items, uint16_t{13}},
                                           std::tuple{QP::Pocket::KeyItems, InventoryPouch::KeyItems, heldKey}}) {
        const auto rule = QP::quantityRule(game, pocket, id);
        CHECK(BagAdd::decidePick(rule, {editor->stagedInventoryQuantity(pouch, id), 1, 20}).outcome ==
              BagAdd::PickOutcome::FocusExisting);
    }
    std::vector<std::pair<Case, InventoryPouch>> cases{
        {{"Antidote", QP::Pocket::Items, 14, false, bagCap}, InventoryPouch::Items},
        {{"Great Ball", QP::Pocket::Balls, 3, false, bagCap}, InventoryPouch::PokeBalls},
        {{"Old Rod", QP::Pocket::KeyItems, 262, true, 1}, InventoryPouch::KeyItems},
        {{"TM01", QP::Pocket::TMHM, 289, false, bagCap}, InventoryPouch::TMCase},
        {{"HM01", QP::Pocket::TMHM, 339, true, 1}, InventoryPouch::TMCase},
        {{"Cheri Berry", QP::Pocket::Berries, 133, false, 999}, InventoryPouch::BerryPouch},
        {{"PC Potion", QP::Pocket::PCItems, 13, false, 999}, InventoryPouch::PCItems},
    };
    if (frlg) cases.push_back({{"Bicycle", QP::Pocket::KeyItems, 360, true, 1}, InventoryPouch::KeyItems});
    else {
        // Acro Bike while the Mach Bike is held: story-exclusive, reported but never enforced.
        CHECK(QP::exclusiveWith(game, 272).size() == 2);
        cases.push_back({{"Acro Bike", QP::Pocket::KeyItems, 272, true, 1}, InventoryPouch::KeyItems});
    }
    if (source == SourceGame::RubyGBA || source == SourceGame::SapphireGBA)
        cases.push_back({{"PC Super Rod", QP::Pocket::PCItems, 264, true, 1}, InventoryPouch::PCItems});
    for (const auto& [c, pouch] : cases) addScenario(game, c, backend(*editor, pouch));
    rejectsInvalid(game, QP::Pocket::Berries);

    const auto out = editor->finalizedBytes(error);
    CHECK(!out.empty());
    auto again = StagedInventoryEditor::create(out, source, error);
    CHECK(again != nullptr);
    if (again) {
        CHECK(again->stagedInventoryQuantity(InventoryPouch::KeyItems, 262) == 1);
        CHECK(again->stagedInventoryQuantity(InventoryPouch::TMCase, 339) == 1);
        CHECK(again->stagedInventoryQuantity(InventoryPouch::TMCase, 289) == bagCap);
        CHECK(again->stagedInventoryQuantity(InventoryPouch::BerryPouch, 133) == 999);
        CHECK(again->stagedInventoryQuantity(InventoryPouch::Items, 13) == 3);
    }
    CHECK(raw == pristine && editor->originalBytes() == pristine);
    editor->discard();
    CHECK(editor->stagedBytes() == pristine);

    // Pocket full (R/S Items: 20 slots): the next new item is refused, naming the full pocket.
    if (family == Family::RS) {
        const auto items = PokeVault::Inventory::addableItems(PokeVault::Inventory::ClassicGame::Ruby, ClassicPocket::Items);
        std::size_t used = 1;  // the held Potion
        std::size_t next = 0;
        for (; used < 20; ++next) {
            if (items[next] == 13) continue;
            CHECK(editor->stageInventoryQuantity(InventoryPouch::Items, items[next], 1, error));
            ++used;
        }
        const auto beforeFull = editor->stagedBytes();
        CHECK(!editor->stageInventoryQuantity(InventoryPouch::Items, items[next], 1, error));
        CHECK(error.find("No free slots") != std::string::npos);
        CHECK(editor->stagedBytes() == beforeFull);
    }
}
} // namespace gen3

// ---------------------------------------------------------------- Gen IV (D/P/Pt/HG/SS)
namespace gen4 {
namespace G4 = PokeVault::Integration::Gen4;
// The Gen IV surface asks with its native pocket index and clamps to gen4BagMaxQuantity; the
// staged-writer round trip is covered by tests/test_gen4_readonly_foundation.cpp.
void run(QP::Game game, G4::Layout layout) {
    const auto pocketOf = [](G4::BagPocket p) { return static_cast<std::size_t>(p); };
    struct G4Case { const char* what; G4::BagPocket pocket; uint16_t id; bool unique; uint16_t maximum; };
    const std::array<G4Case, 8> cases{{
        {"Potion", G4::BagPocket::Medicine, 17, false, 999},
        {"Poke Ball", G4::BagPocket::Balls, 4, false, 999},
        {"Cheri Berry", G4::BagPocket::Berries, 149, false, 999},
        {"Bicycle", G4::BagPocket::KeyItems, 450, true, 1},
        {"Old Rod", G4::BagPocket::KeyItems, 445, true, 1},
        {"Super Rod", G4::BagPocket::KeyItems, 447, true, 1},
        {"TM01", G4::BagPocket::Machines, 328, false, 99},
        {"HM01", G4::BagPocket::Machines, 420, true, 1},
    }};
    for (const auto& c : cases) {
        const std::size_t pocket = pocketOf(c.pocket);
        CHECK(G4::gen4BagItemAllowed(layout, pocket, c.id));
        const auto rule = PokeBank::UIModel::Gen4BagQuantity::policyRule(game, pocket, c.id);
        const uint16_t cap = G4::gen4BagMaxQuantity(pocket, c.id);
        CHECK(rule.valid && rule.unique == c.unique);
        CHECK(BagAdd::effectiveMaximum(rule, cap) == c.maximum);
        const std::size_t slots = G4::bagLayout(layout)[pocket].slots;
        BagAdd::Flow flow;
        CHECK(flow.openPicker(true) && flow.cancel());
        CHECK(flow.openPicker(true));
        const auto pick = flow.pick(rule, {0, 1, slots}, G4::bagPocketName(pocket));
        CHECK(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1);
        const auto row = BagAdd::decideRow(rule, 1, cap);
        CHECK(row.outcome == (c.unique ? BagAdd::RowOutcome::FixedQuantity : BagAdd::RowOutcome::EditQuantity));
        CHECK(row.maximum == c.maximum);
        CHECK(BagAdd::decideRow(rule, static_cast<uint16_t>(c.maximum + 1), cap).outcome ==
              BagAdd::RowOutcome::NotAllowed);
        CHECK(BagAdd::decidePick(rule, {1, 1, slots}).outcome == BagAdd::PickOutcome::FocusExisting);
        // A full pouch refuses a new item with the native pouch name and exact slot count.
        const auto full = BagAdd::decidePick(rule, {0, slots, slots}, G4::bagPocketName(pocket));
        CHECK(full.outcome == BagAdd::PickOutcome::PocketFull);
        CHECK(full.message.find(std::string(G4::bagPocketName(pocket)) + " is full (" +
                                std::to_string(slots) + " of " + std::to_string(slots) + " slots)") == 0);
    }
    rejectsInvalid(game, QP::Pocket::Medicine);
    CHECK(!PokeBank::UIModel::Gen4BagQuantity::policyRule(game, G4::BagPocketCount, 17).valid);
}
} // namespace gen4

// ---------------------------------------------------------------- Gen V (B/W/B2/W2) ---
namespace gen5 {
namespace G = PokeVault::Integration::Gen5;
namespace Cr = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
using Screen = G::Gen5BagScreen;
using Surface = Screen::Surface;
struct Stack { std::size_t pocket; uint16_t id; uint16_t count; };

std::span<const L::SaveBlock> blocksFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                               : std::span<const L::SaveBlock>(L::Black2White2);
}
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version, const std::vector<Stack>& stacks) {
    const std::size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    sav[0x19400 + 0x1F] = version;
    sav[L::PartyOffset + 4] = 1;
    std::vector<uint8_t> party(Cr::PartySize, 0);
    party[0] = 0x23; party[1] = 0x01;
    Cr::write16(party, 8, 25);
    party[0x8C] = 30;
    const auto encrypted = Cr::encryptCandidate(party);
    std::copy(encrypted.begin(), encrypted.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    std::vector<std::size_t> next(G::BagPocket5Count, 0);
    const std::size_t items = G::itemsBlockOffset(family);
    for (const auto& s : stacks) {
        const auto spec = G::bagPocketLayout5(family, s.pocket);
        const std::size_t at = items + spec.offset + next[s.pocket]++ * 4;
        Cr::write16(sav, at, s.id);
        Cr::write16(sav, at + 2, s.count);
    }
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    for (const auto& b : blocksFor(family)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        Cr::write16(copy, b.checksumOffset, crc);
        Cr::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}
uint16_t heldIn(const G::Gen5StagedPokemonWorkspace& ws, std::size_t pocket, uint16_t id) {
    const auto bag = ws.bagPresentation();
    if (!bag) return 0;
    for (const auto& s : (*bag)[pocket]) if (s.itemId == id) return s.count;
    return 0;
}

void run(G::SaveFamily family, std::string_view gameId, uint8_t version, QP::Game game) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const uint16_t key = b2w2 ? 627 : 437;
    const auto input = fixture(family, version, {{0, 4, 30}, {1, key, 1}, {2, 328, 1}, {3, 17, 12}});
    const auto pristine = input;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, gameId, &error, G::SaveCopySelection::Primary);
    CHECK(static_cast<bool>(parsed));
    if (!parsed) return;
    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, std::string(gameId), error);
    CHECK(trainer != nullptr);
    if (!trainer) return;
    auto& ws = trainer->stagedPokemon();
    auto& bag = trainer->bagScreen();
    const auto refresh = [&](std::string& why) { return trainer->refreshStagedPokemonPresentation(why); };
    const auto pocketRule = [&](std::size_t pocket, uint16_t id) {
        static constexpr std::array<QP::Pocket, 5> kPockets{{QP::Pocket::Items, QP::Pocket::KeyItems,
            QP::Pocket::TMHM, QP::Pocket::Medicine, QP::Pocket::Berries}};
        return QP::quantityRule(game, kPockets[pocket], id);
    };

    // Key Items, bikes, rods, TMs and HMs: unique by policy, and the native pockets are view-only.
    for (const auto& [pocket, id] : {std::pair<std::size_t, uint16_t>{1, 450}, {1, 447}, {2, 328}, {2, 618}, {2, 420}}) {
        const auto rule = pocketRule(pocket, id);
        CHECK(rule.valid && rule.unique && rule.maximum == 1);
        CHECK(BagAdd::decideRow(rule, 1, G::bagPocketLayout5(family, pocket).maxCount).outcome ==
              BagAdd::RowOutcome::FixedQuantity);
    }
    while (bag.pocket() != 1) bag.changePocket(1);
    CHECK(!bag.openAdd(ws, error) && bag.surface() == Surface::Browse);
    while (bag.pocket() != 3) bag.changePocket(1);

    // B cancels with no mutation.
    CHECK(bag.openAdd(ws, error) && bag.surface() == Surface::AddPicker);
    bag.cancel();
    CHECK(bag.surface() == Surface::Browse && !ws.hasChanges());

    // A on a new item (Antidote): staged at x1, no keypad.
    CHECK(bag.openAdd(ws, error));
    while (bag.chosenAddItem() != 18) bag.moveAddCursor(1);
    const auto pick = bag.pickAdd(ws, pocketRule(3, 18), refresh, error);
    CHECK(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1 && error.empty());
    CHECK(bag.surface() == Surface::Browse && heldIn(ws, 3, 18) == 1);
    // The row edits up to 999 and never beyond.
    const auto request = bag.beginQuantity(ws, error);
    CHECK(request.has_value() && request && request->itemId == 18);
    if (request) {
        const auto row = BagAdd::decideRow(pocketRule(3, 18), request->current, request->maximum);
        CHECK(row.outcome == BagAdd::RowOutcome::EditQuantity && row.maximum == 999);
        CHECK(!bag.applyQuantity(ws, *request, 1000, refresh, error));
        CHECK(bag.applyQuantity(ws, *request, row.maximum, refresh, error));
        CHECK(heldIn(ws, 3, 18) == 999);
    }
    // A on the held Potion: focuses the existing stack, nothing staged, no duplicate.
    const auto stagedBefore = ws.bagReviewChanges().size();
    CHECK(bag.openAdd(ws, error));
    while (bag.chosenAddItem() != 17) bag.moveAddCursor(1);
    const auto held = bag.pickAdd(ws, pocketRule(3, 17), refresh, error);
    CHECK(held.outcome == BagAdd::PickOutcome::FocusExisting && error.empty());
    CHECK(bag.surface() == Surface::Browse && heldIn(ws, 3, 17) == 12);
    CHECK(ws.bagReviewChanges().size() == stagedBefore);
    const auto row = bag.selected(ws);
    CHECK(row && row->itemId == 17);

    // Export + reopen; the source stays byte-identical.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    CHECK(out && out->bagChanged);
    if (out) {
        const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, gameId, &error, G::SaveCopySelection::Primary);
        CHECK(static_cast<bool>(reopened));
        if (reopened) {
            const auto exported = G::decodeBag5(*reopened);
            CHECK(exported && (*exported)[3].size() == 2 && (*exported)[3][1].itemId == 18 &&
                  (*exported)[3][1].count == 999);
        }
    }
    CHECK(input == pristine);
    // Discard restores the source bag.
    ws.discardAll();
    std::string why;
    CHECK(trainer->refreshStagedPokemonPresentation(why));
    CHECK(!ws.hasChanges() && heldIn(ws, 3, 18) == 0);
}
} // namespace gen5

} // namespace

int main() {
    namespace G1 = PokeVault::Integration::Gen1;
    gen1::run(G1::SourceGame::Red, QP::Game::Red);
    gen1::run(G1::SourceGame::Blue, QP::Game::Blue);
    gen1::run(G1::SourceGame::Yellow, QP::Game::Yellow);
    namespace G2 = PokeVault::Integration::Gen2;
    gen2::run(gen2::GS, G2::SourceGame::Gold, QP::Game::Gold);
    gen2::run(gen2::GS, G2::SourceGame::Silver, QP::Game::Silver);
    gen2::run(gen2::C, G2::SourceGame::Crystal, QP::Game::Crystal);
    using G3 = PokeVault::Integration::Gen3::SourceGame;
    gen3::run(G3::RubyGBA, gen3::Family::RS, QP::Game::Ruby);
    gen3::run(G3::SapphireGBA, gen3::Family::RS, QP::Game::Sapphire);
    gen3::run(G3::EmeraldGBA, gen3::Family::Emerald, QP::Game::Emerald);
    gen3::run(G3::FireRedGBA, gen3::Family::FRLG, QP::Game::FireRed);
    gen3::run(G3::LeafGreenGBA, gen3::Family::FRLG, QP::Game::LeafGreen);
    namespace G4 = PokeVault::Integration::Gen4;
    gen4::run(QP::Game::Diamond, G4::Layout::DiamondPearl);
    gen4::run(QP::Game::Pearl, G4::Layout::DiamondPearl);
    gen4::run(QP::Game::Platinum, G4::Layout::Platinum);
    gen4::run(QP::Game::HeartGold, G4::Layout::HeartGoldSoulSilver);
    gen4::run(QP::Game::SoulSilver, G4::Layout::HeartGoldSoulSilver);
    namespace G5 = PokeVault::Integration::Gen5;
    gen5::run(G5::SaveFamily::BlackWhite, "black_nds", 21, QP::Game::Black);
    gen5::run(G5::SaveFamily::BlackWhite, "white_nds", 20, QP::Game::White);
    gen5::run(G5::SaveFamily::Black2White2, "black2_nds", 23, QP::Game::Black2);
    gen5::run(G5::SaveFamily::Black2White2, "white2_nds", 22, QP::Game::White2);
    if (failures != 0) {
        std::fprintf(stderr, "test_bag_add_flow_gen15: %d failure(s)\n", failures);
        return 1;
    }
    std::puts("test_bag_add_flow_gen15: shared Gen I-V Bag Add flow PASS (20 games)");
    return 0;
}
