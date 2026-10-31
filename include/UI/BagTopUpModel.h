#pragma once
// Top Up / Add Missing (owner Oct 28, Feature B): the Gen I-V Bag flow as a pure model (no libnx, no
// framebuffer), shared by the Gen I-III classic inventory overlay, the Gen IV Bag and the Gen V Bag.
//
//   ZR on the Bag (or the Gen I-III Inventory Options row) opens SETUP:
//       Scope   This pocket / All Bag pockets / PC Items / Bag + PC Items   (A, Left / Right change it)
//       Mode    Top up held stacks / Add missing items / Both
//       Amount  target per item (A opens the numpad; the planner caps each item natively)
//       Preview changes
//       Cancel
//   Round 11 (owner H04 / H34: "the bag categories arnt filled", "the pc items doesnt get filled"):
//   Scope is the FIRST row, so the cursor starts on it and its choices are visible ("< ... >").
//   Until the player picks a scope it follows the tab: All Bag pockets on a Bag tab, PC Items on the
//   PC Items tab; a picked scope is kept for the session. PC Items and Bag + PC Items are offered
//   only where the save has PC item storage (Gen I-III); Gen IV / V never list them. PC Items stays
//   out of "All Bag pockets": the PC is storage, so it is filled only when a PC scope says so.
//   "Preview changes" builds the exact plan (Inventory/BulkItemPlan.h) and opens PREVIEW: every
//   change ("Potion 3 → 99", "+ Ultra Ball ×99") and every skip with its reason, scrollable, with
//   totals. A stages it all through Inventory/BulkItemApply.h (all or nothing, read back); B goes
//   back to SETUP with nothing staged. Nothing here writes: the surface owns the staged editor.

#include "Inventory/BulkItemPlan.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace PokeBank::UIModel::BagTopUp {

namespace BI = PokeVault::Inventory::BulkItems;

enum class Step : uint8_t { Closed, Setup, Preview };
enum class Row : int { Scope = 0, Mode, Amount, Preview, Cancel };
enum class Scope : uint8_t { ThisPocket, AllBag, PcItems, BagAndPc };
inline constexpr int kRows = 5;
inline constexpr uint16_t kDefaultQuantity = 99;
inline constexpr uint16_t kMaxQuantity = 999;   // highest native per-slot cap in Gen I-V (PKHeX bags)

enum class Action : uint8_t { None, Close, PromptAmount, BuildPreview, Confirm };

struct Keys {
    bool up = false, down = false, left = false, right = false;
    bool a = false, b = false, pageUp = false, pageDown = false;
};

enum class LineKind : uint8_t { Heading, TopUp, Add, Skip, Note };
struct Line {
    LineKind kind = LineKind::Note;
    std::string text;
};

struct State {
    Step step = Step::Closed;
    int row = 0;
    BI::Mode mode = BI::Mode::Both;
    Scope scope = Scope::ThisPocket;   // open() sets the tab's default (defaultScope)
    bool scopeChosen = false;     // the player picked a scope this session (else it follows the tab)
    bool pcAvailable = false;     // the save has PC item storage (Gen I-III)
    uint16_t quantity = kDefaultQuantity;
    BI::Plan plan;
    std::vector<Line> lines;
    std::string totals;
    int scroll = 0;
    [[nodiscard]] bool active() const noexcept { return step != Step::Closed; }
};

[[nodiscard]] inline bool pcScope(Scope scope) noexcept { return scope == Scope::PcItems || scope == Scope::BagAndPc; }

// The scope SETUP starts with until the player picks one: the PC on the PC Items tab, else the Bag.
[[nodiscard]] inline Scope defaultScope(BI::Pocket current, bool pcAvailable) noexcept {
    return pcAvailable && current == BI::Pocket::PCItems ? Scope::PcItems : Scope::AllBag;
}

// The choices Left / Right cycle through for this save, in order.
[[nodiscard]] inline std::vector<Scope> scopes(const State& s) {
    std::vector<Scope> out{Scope::ThisPocket, Scope::AllBag};
    if (s.pcAvailable) { out.push_back(Scope::PcItems); out.push_back(Scope::BagAndPc); }
    return out;
}

// `current` is the pocket on screen; `pcAvailable` whether the save has PC item storage.
inline void open(State& s, BI::Pocket current = BI::Pocket::Items, bool pcAvailable = false) {
    const auto mode = s.mode;
    const auto scope = s.scope;
    const bool chosen = s.scopeChosen;
    const uint16_t quantity = s.quantity;
    s = {};
    s.mode = mode;          // the last choices are kept for the next run in this session
    s.pcAvailable = pcAvailable;
    s.scopeChosen = chosen && (pcAvailable || !pcScope(scope));
    s.scope = s.scopeChosen ? scope : defaultScope(current, pcAvailable);
    s.quantity = quantity == 0 ? kDefaultQuantity : quantity;
    s.step = Step::Setup;
}
inline void close(State& s) {
    s.step = Step::Closed;
    s.plan = {};
    s.lines.clear();
    s.totals.clear();
    s.scroll = 0;
}

[[nodiscard]] inline const char* modeLabel(BI::Mode m) noexcept {
    switch (m) {
        case BI::Mode::TopUpExisting: return "Top up held stacks";
        case BI::Mode::AddMissing: return "Add missing items";
        case BI::Mode::Both: return "Top up + add missing";
    }
    return "";
}
[[nodiscard]] inline BI::Mode nextMode(BI::Mode m, int delta) noexcept {
    const int i = (static_cast<int>(m) + (delta < 0 ? 2 : 1)) % 3;
    return static_cast<BI::Mode>(i);
}

[[nodiscard]] inline Scope nextScope(const State& s, int delta) {
    const auto list = scopes(s);
    const auto at = std::find(list.begin(), list.end(), s.scope);
    const int n = static_cast<int>(list.size());
    const int i = at == list.end() ? 0 : static_cast<int>(at - list.begin());
    return list[static_cast<std::size_t>((i + (delta < 0 ? n - 1 : 1)) % n)];
}

// What a scope covers, in words (SETUP's Scope row and the PREVIEW caption).
[[nodiscard]] inline std::string scopeLabel(Scope scope, const std::string& pocketName) {
    switch (scope) {
        case Scope::ThisPocket: return "This pocket (" + pocketName + ")";
        case Scope::AllBag: return "All Bag pockets";
        case Scope::PcItems: return "PC Items";
        case Scope::BagAndPc: return "Bag + PC Items";
    }
    return {};
}

[[nodiscard]] inline const char* rowLabel(Row r) noexcept {
    switch (r) {
        case Row::Mode: return "Mode";
        case Row::Scope: return "Scope";
        case Row::Amount: return "Amount";
        case Row::Preview: return "Preview changes";
        case Row::Cancel: return "Cancel";
    }
    return "";
}
[[nodiscard]] inline std::string rowDetail(const State& s, Row r, const std::string& pocketName) {
    switch (r) {
        case Row::Mode: return modeLabel(s.mode);
        // "< ... >": the row says it has more choices (Left / Right), not only the one shown.
        case Row::Scope: return "< " + scopeLabel(s.scope, pocketName) + " >";
        case Row::Amount: return std::to_string(s.quantity) + " per item (capped natively)";
        default: return {};
    }
}

// SETUP: Up / Down choose a row, Left / Right (or A) change Mode / Scope, A on Amount asks for the
// number, A on Preview builds the plan, A on Cancel or B closes.
[[nodiscard]] inline Action setupInput(State& s, const Keys& k, int tappedRow = -1) {
    if (s.step != Step::Setup) return Action::None;
    bool a = k.a;
    if (tappedRow >= 0 && tappedRow < kRows) { s.row = tappedRow; a = true; }
    if (k.b) { close(s); return Action::Close; }
    if (k.up) s.row = (s.row + kRows - 1) % kRows;
    if (k.down) s.row = (s.row + 1) % kRows;
    const auto row = static_cast<Row>(s.row);
    const int delta = k.left ? -1 : (k.right || a) ? 1 : 0;
    if (delta != 0 && row == Row::Mode) { s.mode = nextMode(s.mode, delta); return Action::None; }
    if (delta != 0 && row == Row::Scope) {
        s.scope = nextScope(s, delta);
        s.scopeChosen = true;
        return Action::None;
    }
    if (!a) return Action::None;
    switch (row) {
        case Row::Amount: return Action::PromptAmount;
        case Row::Preview: return Action::BuildPreview;
        case Row::Cancel: close(s); return Action::Close;
        default: return Action::None;
    }
}

inline void setAmount(State& s, int value) {
    s.quantity = static_cast<uint16_t>(std::clamp(value, 1, static_cast<int>(kMaxQuantity)));
}

// The request the SETUP choices describe. `current` is the pocket on screen; `state` every pocket
// the save has (Bag and, in Gen I-III, PC Items). The request never comes out empty (the planner
// would read an empty list as "every pocket"): a scope the save cannot serve falls back to `current`.
[[nodiscard]] inline BI::Request request(const State& s, BI::Pocket current, std::span<const BI::PocketState> state) {
    BI::Request r;
    r.mode = s.mode;
    r.quantity = s.quantity;
    switch (s.scope) {
        case Scope::ThisPocket: r.pockets = {current}; break;
        case Scope::AllBag: r.pockets = BI::bagPockets(state); break;
        case Scope::PcItems: r.pockets = BI::pcPockets(state); break;
        case Scope::BagAndPc:
            r.pockets = BI::bagPockets(state);
            for (const auto p : BI::pcPockets(state)) r.pockets.push_back(p);
            break;
    }
    if (r.pockets.empty()) r.pockets = {current};
    return r;
}

// PREVIEW lines: per pocket its top-ups, adds and skips, in plan order.
template <class ItemName, class PocketName>
[[nodiscard]] inline std::vector<Line> previewLines(const BI::Plan& plan, const std::vector<BI::Pocket>& pockets,
                                                    ItemName&& itemName, PocketName&& pocketName) {
    std::vector<Line> out;
    // A list that ran out of slots says so first, with its native size: the "pocket full" lines
    // below are the game's own limit, not a failure (Gen I: one 20-slot Bag for every category).
    for (const auto& f : plan.full) {
        const std::string where = f.sharedList ? std::string("Bag (every category shares ") + std::to_string(f.slots) + " slots)"
                                               : std::string(pocketName(f.pocket)) + " (" + std::to_string(f.slots) + " slots)";
        out.push_back({LineKind::Note, where + ": " + std::to_string(f.freeBefore) + " free, " +
                                           std::to_string(f.notFitted) + " more did not fit"});
    }
    for (const auto pocket : pockets) {
        const auto first = out.size();
        for (const auto& t : plan.topUps)
            if (t.pocket == pocket)
                out.push_back({LineKind::TopUp, itemName(pocket, t.itemId) + " " + std::to_string(t.from) +
                                                    " \xE2\x86\x92 " + std::to_string(t.to)});
        for (const auto& a : plan.adds)
            if (a.pocket == pocket)
                out.push_back({LineKind::Add, "+ " + itemName(pocket, a.itemId) + " \xC3\x97" + std::to_string(a.quantity)});
        for (const auto& k : plan.skipped)
            if (k.pocket == pocket)
                out.push_back({LineKind::Skip, itemName(pocket, k.itemId) + ": " + BI::skipText(k)});
        if (out.size() != first && pockets.size() > 1)
            out.insert(out.begin() + static_cast<std::ptrdiff_t>(first), Line{LineKind::Heading, std::string(pocketName(pocket))});
    }
    if (plan.empty()) out.insert(out.begin(), Line{LineKind::Note, "Nothing to change: every stack is already at the amount or excluded."});
    return out;
}

[[nodiscard]] inline std::string totals(const BI::Plan& plan) {
    return std::to_string(plan.topUps.size()) + " topped up, " + std::to_string(plan.adds.size()) +
           " added, " + std::to_string(plan.skipped.size()) + " skipped";
}

template <class ItemName, class PocketName>
inline void showPreview(State& s, BI::Plan plan, const std::vector<BI::Pocket>& pockets,
                        ItemName&& itemName, PocketName&& pocketName) {
    s.lines = previewLines(plan, pockets, itemName, pocketName);
    s.totals = totals(plan);
    s.plan = std::move(plan);
    s.scroll = 0;
    s.step = Step::Preview;
}

[[nodiscard]] inline int maxScroll(const State& s, int visible) noexcept {
    return std::max(0, static_cast<int>(s.lines.size()) - std::max(1, visible));
}

// PREVIEW: Up / Down scroll a line, L / R a page; A confirms (only when there is something to
// change); B returns to SETUP with nothing staged.
[[nodiscard]] inline Action previewInput(State& s, const Keys& k, int visible) {
    if (s.step != Step::Preview) return Action::None;
    if (k.b) {
        s.step = Step::Setup;
        s.plan = {};
        s.lines.clear();
        s.totals.clear();
        s.scroll = 0;
        return Action::None;
    }
    const int page = std::max(1, visible - 1);
    if (k.up) s.scroll -= 1;
    if (k.down) s.scroll += 1;
    if (k.pageUp) s.scroll -= page;
    if (k.pageDown) s.scroll += page;
    s.scroll = std::clamp(s.scroll, 0, maxScroll(s, visible));
    if (k.a && !s.plan.empty()) return Action::Confirm;
    return Action::None;
}

// The status a surface shows after A in PREVIEW.
[[nodiscard]] inline std::string appliedStatus(std::size_t toppedUp, std::size_t added) {
    return "Staged: " + std::to_string(toppedUp) + " topped up, " + std::to_string(added) +
           " added; source save unchanged (Review lists them)";
}

// Same plan twice (the Bag cannot change under the modal, but a confirm re-plans to be sure).
[[nodiscard]] inline bool samePlan(const BI::Plan& a, const BI::Plan& b) noexcept {
    const auto sameTop = [](const BI::TopUp& x, const BI::TopUp& y) {
        return x.pocket == y.pocket && x.slot == y.slot && x.itemId == y.itemId && x.from == y.from && x.to == y.to;
    };
    const auto sameAdd = [](const BI::Add& x, const BI::Add& y) {
        return x.pocket == y.pocket && x.itemId == y.itemId && x.quantity == y.quantity;
    };
    return std::equal(a.topUps.begin(), a.topUps.end(), b.topUps.begin(), b.topUps.end(), sameTop) &&
           std::equal(a.adds.begin(), a.adds.end(), b.adds.begin(), b.adds.end(), sameAdd);
}

} // namespace PokeBank::UIModel::BagTopUp
