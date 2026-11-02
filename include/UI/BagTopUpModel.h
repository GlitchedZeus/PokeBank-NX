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
//   Round 12 (owner H14: "it only selects like 20 items ... we need the all items in that slot"):
//   the planner already fills every free native slot; the preview now opens with one line per native
//   list that names its slot count and the exact numbers ("Items (20 slots): 20 free, 118 more did
//   not fit", "Balls (16 slots): 16 free, all 11 missing added"), and a partial result names the full list and its native size,
//   so a list the game itself limits (Ruby / Sapphire Items: 20) is never mistaken for a cap of ours.
//   Round 12 (owner H12): TMs and HMs are planned; only Key Items are excluded (SETUP says so).
//   Round 14 (owner H07 "again only adds some of the items"; shared-UX audit P0-9: a partial fill
//   with no list of what was skipped is a silent drop): before A, PREVIEW opens with one emphasised
//   line per native list in plain words -- "Items: 20 native slots, 0 used. Only 20 fit; 118
//   missing items won't fit" -- its totals count "won't fit" apart from other skips, and every item
//   that is not added is listed with its reason (no room in <list> (N native slots) / added to PC
//   Items instead, only one allowed, Key Item, not valid for this game). After A, RESULT shows the
//   same list as what WAS staged and what was NOT (A / B close it). The pocket on screen is planned
//   first, so a shared list (Gen I's 20-slot Bag) fills from the category being looked at.

#include "Inventory/BulkItemPlan.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace PokeBank::UIModel::BagTopUp {

namespace BI = PokeVault::Inventory::BulkItems;

enum class Step : uint8_t { Closed, Setup, Preview, Result };
enum class Row : int { Scope = 0, Mode, Amount, Preview, Cancel };
enum class Scope : uint8_t { ThisPocket, AllBag, PcItems, BagAndPc };
inline constexpr int kRows = 5;
inline constexpr uint16_t kDefaultQuantity = 99;
inline constexpr uint16_t kMaxQuantity = 999;   // highest native per-slot cap in Gen I-V (PKHeX bags)
// SETUP's caption after the game line: what is excluded and that native slot counts bound the fill.
inline constexpr const char* kSetupCaption = "Fills native free slots; Key Items are never changed";

enum class Action : uint8_t { None, Close, PromptAmount, BuildPreview, Confirm };

struct Keys {
    bool up = false, down = false, left = false, right = false;
    bool a = false, b = false, pageUp = false, pageDown = false;
};

enum class LineKind : uint8_t { Heading, TopUp, Add, Skip, Note };
struct Line {
    LineKind kind = LineKind::Note;
    std::string text;
    bool emphasis = false;   // a native list's capacity line (drawn in the accent colour)
};

struct State {
    Step step = Step::Closed;
    int row = 0;
    BI::Mode mode = BI::Mode::Both;
    Scope scope = Scope::ThisPocket;   // open() sets the tab's default (defaultScope)
    bool scopeChosen = false;     // the player picked a scope this session (else it follows the tab)
    bool pcAvailable = false;     // the save has PC item storage (Gen I-III)
    // > 0: every Bag category shares ONE native list of this many slots (Gen I: 20). Round 11 QA:
    // "All Bag pockets" is then one shared Bag, so SETUP / PREVIEW say so instead of implying
    // per-pocket room.
    std::size_t sharedBagSlots = 0;
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
inline void open(State& s, BI::Pocket current = BI::Pocket::Items, bool pcAvailable = false,
                 std::size_t sharedBagSlots = 0) {
    const auto mode = s.mode;
    const auto scope = s.scope;
    const bool chosen = s.scopeChosen;
    const uint16_t quantity = s.quantity;
    s = {};
    s.mode = mode;          // the last choices are kept for the next run in this session
    s.pcAvailable = pcAvailable;
    s.sharedBagSlots = sharedBagSlots;
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
// `sharedBagSlots` > 0: one shared Bag list (Gen I), named with its native size.
[[nodiscard]] inline std::string scopeLabel(Scope scope, const std::string& pocketName,
                                            std::size_t sharedBagSlots = 0) {
    switch (scope) {
        case Scope::ThisPocket: return "This pocket (" + pocketName + ")";
        case Scope::AllBag:
            return sharedBagSlots > 0 ? "Whole Bag (" + std::to_string(sharedBagSlots) + " slots shared)"
                                      : "All Bag pockets";
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
        case Row::Scope: return "< " + scopeLabel(s.scope, pocketName, s.sharedBagSlots) + " >";
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
    // Round 14 (H07 / H08): the pocket on screen is planned first (a shared list fills from it).
    const auto bag = [&] {
        auto pockets = BI::bagPockets(state);
        const auto at = std::find(pockets.begin(), pockets.end(), current);
        if (at != pockets.end()) std::rotate(pockets.begin(), at, at + 1);
        return pockets;
    };
    switch (s.scope) {
        case Scope::ThisPocket: r.pockets = {current}; break;
        case Scope::AllBag: r.pockets = bag(); break;
        case Scope::PcItems: r.pockets = BI::pcPockets(state); break;
        case Scope::BagAndPc:
            r.pockets = bag();
            for (const auto p : BI::pcPockets(state)) r.pockets.push_back(p);
            break;
    }
    if (r.pockets.empty()) r.pockets = {current};
    return r;
}

// Round 14 (H07): one native list's room in plain words (PREVIEW's first lines).
template <class PocketName>
[[nodiscard]] inline std::string capacityText(const BI::ListRoom& r, PocketName&& pocketName) {
    const std::string name = r.sharedList
        ? "Bag (all categories share " + std::to_string(r.slots) + " native slots)"
        : std::string(pocketName(r.pocket)) + ": " + std::to_string(r.slots) + " native slots";
    const std::size_t used = r.slots > r.freeBefore ? r.slots - r.freeBefore : 0;
    std::string text = name + (r.sharedList ? ": " : ", ") + std::to_string(used) + " used";
    const auto items = [](std::size_t n) { return std::to_string(n) + (n == 1 ? " missing item" : " missing items"); };
    if (r.eligible == 0) return text + ". Nothing missing to add";
    if (r.notFitted == 0) return text + ". All " + items(r.planned) + " fit";
    return text + ". " + (r.planned == 0 ? std::string("None fit") : "Only " + std::to_string(r.planned) + " fit") +
           "; " + items(r.notFitted) + " won't fit";
}

// PREVIEW lines: per pocket its top-ups, adds and skips, in plan order.
template <class ItemName, class PocketName>
[[nodiscard]] inline std::vector<Line> previewLines(const BI::Plan& plan, const std::vector<BI::Pocket>& pockets,
                                                    ItemName&& itemName, PocketName&& pocketName) {
    std::vector<Line> out;
    // Round 14 (H07): one line per native list, full lists first, in plain words: its native slot
    // count, how many are used, how many new items fit and how many missing items will NOT fit
    // ("Items: 20 native slots, 0 used. Only 20 fit; 118 missing items won't fit"). The "won't
    // fit" item lines below are the game's own limit, not a cap of ours (Gen I: one 20-slot Bag).
    for (const bool fullLists : {true, false})
        for (const auto& r : plan.rooms)
            if ((r.notFitted > 0) == fullLists) out.push_back({LineKind::Note, capacityText(r, pocketName), true});
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

// Round 14 (H07): "won't fit" (the game's slot limit) is counted apart from other skips.
[[nodiscard]] inline std::string totals(const BI::Plan& plan) {
    const auto noRoom = static_cast<std::size_t>(std::count_if(plan.skipped.begin(), plan.skipped.end(),
        [](const BI::Skip& k) { return k.reason == BI::SkipReason::PocketFull; }));
    const std::string head = std::to_string(plan.topUps.size()) + " topped up, " + std::to_string(plan.adds.size()) + " added, ";
    if (noRoom == 0) return head + std::to_string(plan.skipped.size()) + " skipped";
    return head + std::to_string(noRoom) + " won't fit (native slots), " +
           std::to_string(plan.skipped.size() - noRoom) + " skipped";
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

// Round 14 (P0-9): after A stages the plan, RESULT lists what WAS staged ("+" / arrow lines) and
// what was NOT (each with its reason) -- the preview's lines, headed by the applied status. A or B
// closes it; Up / Down / L / R scroll. Nothing is staged here.
// `applied` is the plan A staged; the caption is its totals ("Staged: 0 topped up, 20 added, ...").
inline void showResult(State& s, std::vector<Line> lines, const BI::Plan& applied) {
    const std::string status = "Staged: " + totals(applied);
    s.plan = {};
    s.lines = std::move(lines);
    s.lines.insert(s.lines.begin(), Line{LineKind::Note, "Staged: every + and \xE2\x86\x92 line. Not added: every line with a reason."});
    s.totals = status;
    s.scroll = 0;
    s.step = Step::Result;
}
[[nodiscard]] inline Action resultInput(State& s, const Keys& k, int visible) {
    if (s.step != Step::Result) return Action::None;
    if (k.a || k.b) { close(s); return Action::Close; }
    const int page = std::max(1, visible - 1);
    if (k.up) s.scroll -= 1;
    if (k.down) s.scroll += 1;
    if (k.pageUp) s.scroll -= page;
    if (k.pageDown) s.scroll += page;
    s.scroll = std::clamp(s.scroll, 0, maxScroll(s, visible));
    return Action::None;
}

// The status a surface shows after A in PREVIEW.
[[nodiscard]] inline std::string appliedStatus(std::size_t toppedUp, std::size_t added) {
    return "Staged: " + std::to_string(toppedUp) + " topped up, " + std::to_string(added) +
           " added; source save unchanged (Review lists them)";
}

// Items the plan could not fit because a native list was full (never overflowed).
[[nodiscard]] inline std::size_t notFitted(const BI::Plan& plan) noexcept {
    std::size_t n = 0;
    for (const auto& f : plan.full) n += f.notFitted;
    return n;
}

// Round 11 QA: a partial result says so. When a native list ran out of slots the status names how
// many items did not fit and why (Gen I: the 20 shared Bag slots), so a partial Add Missing is
// never reported as complete.
[[nodiscard]] inline std::string appliedStatus(std::size_t toppedUp, std::size_t added, const BI::Plan& plan) {
    const std::size_t missed = notFitted(plan);
    if (missed == 0) return appliedStatus(toppedUp, added);
    bool shared = false;
    std::size_t sharedSlots = 0;
    for (const auto& f : plan.full)
        if (f.sharedList) { shared = true; sharedSlots = f.slots; }
    std::string why = shared ? "the Bag is full (" + std::to_string(sharedSlots) + " slots shared by every category)"
                             : "a pocket is full";
    return "Partly staged: " + std::to_string(toppedUp) + " topped up, " + std::to_string(added) + " added; " +
           std::to_string(missed) + " did not fit because " + why +
           ". Nothing overflowed; source save unchanged (Review lists them)";
}

// Round 12 (H14): the same status, naming each full native list and its size ("Items (20 native
// slots)"), so the player sees the game's own limit, not a vague "a pocket is full".
template <class PocketName>
[[nodiscard]] inline std::string appliedStatus(std::size_t toppedUp, std::size_t added, const BI::Plan& plan,
                                               PocketName&& pocketName) {
    const std::size_t missed = notFitted(plan);
    // No full list, or only Gen I's one shared Bag: the Round 11 wording already names it exactly.
    if (missed == 0 || std::all_of(plan.full.begin(), plan.full.end(), [](const BI::FullList& f) { return f.sharedList; }))
        return appliedStatus(toppedUp, added, plan);
    std::string lists;
    for (std::size_t i = 0; i < plan.full.size(); ++i) {
        const auto& f = plan.full[i];
        if (i > 0) lists += i + 1 == plan.full.size() ? " and " : ", ";
        lists += f.sharedList ? "the Bag (" + std::to_string(f.slots) + " slots shared by every category)"
                              : std::string(pocketName(f.pocket)) + " (" + std::to_string(f.slots) + " native slots)";
    }
    const bool one = plan.full.size() == 1;
    return "Partly staged: " + std::to_string(toppedUp) + " topped up, " + std::to_string(added) + " added; " +
           std::to_string(missed) + " did not fit because " + lists + (one ? " is full" : " are full") +
           ". Nothing overflowed; source save unchanged (Review lists them)";
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
