// The one Master Vault browser (Withdraw from Vault). See UI/VaultDialog.h.
#include "UI/VaultDialog.h"

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedPopupDismiss.h"
#include "UI/SharedScrollbar.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"
#include "Vault/VaultSearch.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace UI::VaultDialog {
namespace {

namespace Geo = PokeBank::UIModel::SharedMenuGeometry;
namespace V = PokeBank::Vault;
namespace Search = PokeBank::Vault::Search;

constexpr int kRowTouchBase = 86400;   // + visible row (clear of the Party sheet 89000 and file dialog 88000)

struct DialogState {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;
    bool active = false;
    int generation = 0;
    PokemonFileDialog::StageFn stage;
    std::string query;
    std::vector<Search::Hit> hits;
    int cursor = 0;
    int first = 0;
    std::string message;
    SharedScrollbar::State scrollbar;
};

DialogState& stateFor(const TrainerViewScreen& screen) {
    static DialogState state;
    if (state.owner != &screen || state.ownerSerial != screen.instanceSerial) {
        state = {};
        state.owner = &screen;
        state.ownerSerial = screen.instanceSerial;
    }
    return state;
}

Geo::Layout layoutFor(const DialogState& s) {
    return Geo::layout(Geo::MenuType::FileBrowser, static_cast<int>(s.hits.size()));
}

void refilter(DialogState& s) {
    std::string error;
    const V::Store* store = sharedStore(error);
    s.hits = store ? Search::search(store->entries(), s.query, s.generation) : std::vector<Search::Hit>{};
    s.cursor = std::clamp(s.cursor, 0, std::max(0, static_cast<int>(s.hits.size()) - 1));
    s.first = 0;
    s.scrollbar.reset();
    if (!store) s.message = error;
}

void finish(TrainerViewScreen& screen, DialogState& s) {
    s.active = false;
    s.stage = nullptr;
    s.scrollbar.reset();
    screen.controllerNavigation.reset();
}

void withdrawFocused(TrainerViewScreen& screen, DialogState& s) {
    std::string error;
    V::Store* store = sharedStore(error);
    if (!store || s.cursor < 0 || s.cursor >= static_cast<int>(s.hits.size())) {
        s.message = error.empty() ? "Nothing to withdraw" : error;
        screen.postStatus(s.message, 360);
        return;
    }
    const auto& entry = store->entries()[s.hits[static_cast<std::size_t>(s.cursor)].entryIndex];
    const auto decoded = store->withdrawCopy(entry.id, s.generation, error);
    if (!decoded) {
        s.message = error;
        screen.postStatus(error, 420);
        return;
    }
    std::string status;
    error.clear();
    if (!s.stage || !s.stage(*decoded, status, error)) {
        // The generation's own refusal (slot occupied, box full, wrong game...). Nothing was staged.
        s.message = error.empty() ? "The Pokemon could not be withdrawn; nothing was staged" : error;
        screen.postStatus(s.message, 420);
        return;
    }
    finish(screen, s);
    screen.postStatus(status + " (a copy; it stays in the Vault)", 600);
}

void search(TrainerViewScreen& screen, DialogState& s) {
    // Blocking keyboard from update(); the list re-filters under it as the query is typed.
    const std::string before = s.query;
    const auto result = Utils::promptText("Search the Vault", "Species, nickname, OT, game or ID", s.query, 24,
                                          [&s](const std::string& text) {
                                              s.query = text;
                                              refilter(s);
                                          });
    s.query = result.accepted ? result.text : before;   // cancel keeps the previous search
    refilter(s);
    screen.controllerNavigation.reset();
}

// One text run with its matched spans highlighted (Search::spansFor), left to right from x.
int drawHighlighted(PKSEFramebuffer& fb, int x, int y, int h, const std::string& text,
                    const std::vector<Search::Span>& spans, Color color, TextStyle style) {
    std::size_t at = 0;
    const auto run = [&](std::size_t from, std::size_t to, bool match) {
        if (to <= from) return;
        const std::string part = text.substr(from, to - from);
        int w = 0, th = 0;
        fb.measureText(part, w, th, style);
        if (match) {
            Color mark = Colors::Accent;
            mark.a = 110;
            fb.drawFilledRoundedRect(x - 2, y + (h - th) / 2 - 2, w + 4, th + 4, 4, mark);
        }
        fb.drawText(x, y + (h - th) / 2, part, match ? Colors::TextPrimary : color, style);
        x += w;
    };
    for (const auto& span : spans) {
        const std::size_t start = std::min(span.start, text.size());
        const std::size_t end = std::min(span.start + span.length, text.size());
        run(at, start, false);
        run(start, end, true);
        at = end;
    }
    run(at, text.size(), false);
    return x;
}

} // namespace

V::Store* sharedStore(std::string& error) {
    static std::unique_ptr<V::Store> store;
    if (store && store->isOpen()) return store.get();
    store = std::make_unique<V::Store>();
    V::OpenReport report;
    if (!store->open(PokeBank::Paths::vaultRoot(), &report, error)) {
        store.reset();
        return nullptr;
    }
    return store.get();
}

void openWithdraw(TrainerViewScreen& screen, int generation, PokemonFileDialog::StageFn stage) {
    auto& s = stateFor(screen);
    s = {};
    s.owner = &screen;
    s.ownerSerial = screen.instanceSerial;
    s.generation = generation;
    s.stage = std::move(stage);
    s.active = true;
    refilter(s);
    screen.controllerNavigation.reset();
}

bool active(const TrainerViewScreen& screen) noexcept { return stateFor(screen).active; }

void close(TrainerViewScreen& screen) noexcept {
    auto& s = stateFor(screen);
    s.active = false;
    s.stage = nullptr;
}

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                 const TouchInput& touch) {
    auto& s = stateFor(screen);
    if (!s.active) return false;
    const auto layout = layoutFor(s);
    const int count = static_cast<int>(s.hits.size());

    const auto bar = SharedScrollbar::handleTouch(s.scrollbar, SharedScrollbar::sample(touch),
        SharedScrollbar::rows(count, layout.visibleRows, s.first));
    if (bar.changed) s.first = bar.scroll;
    if (bar.consumed) return true;
    if (SharedPopupDismiss::tappedOutside(touch)) {
        finish(screen, s);
        return true;
    }
    const int tapped = screen.touchedButtonId(touch);
    if (tapped >= kRowTouchBase && tapped < kRowTouchBase + layout.visibleRows) {
        const int row = s.first + (tapped - kRowTouchBase);
        if (row >= 0 && row < count) {
            s.cursor = row;
            down |= HidNpadButton_A;
        }
    }
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY, HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    const int page = std::max(1, layout.visibleRows);
    if (down & (HidNpadButton_Minus | HidNpadButton_B)) {   // non-destructive: nothing staged
        finish(screen, s);
        return true;
    }
    if (nav & HidNpadButton_Up) s.cursor = count ? (s.cursor + count - 1) % count : 0;
    if (nav & HidNpadButton_Down) s.cursor = count ? (s.cursor + 1) % count : 0;
    if (down & (HidNpadButton_L | HidNpadButton_Left)) s.cursor = std::max(0, s.cursor - page);
    if (down & (HidNpadButton_R | HidNpadButton_Right)) s.cursor = std::min(std::max(0, count - 1), s.cursor + page);
    if (down & HidNpadButton_Y) {
        search(screen, s);
        return true;
    }
    if (down & HidNpadButton_A) {
        withdrawFocused(screen, s);
        if (!s.active) return true;
    }
    if (s.cursor < s.first) s.first = s.cursor;
    if (s.cursor >= s.first + layout.visibleRows) s.first = s.cursor - layout.visibleRows + 1;
    s.first = std::clamp(s.first, 0, std::max(0, count - layout.visibleRows));
    return true;
}

void drawDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& s = stateFor(screen);
    if (!s.active) return;
    SharedEditorChrome::beginModal(screen.touchButtons);
    const auto l = layoutFor(s);
    const auto& frame = l.frame;
    drawModalSurface(fb, frame.x, frame.y, frame.w, frame.h, 16);
    SharedPopupDismiss::publish(frame.x, frame.y, frame.w, frame.h);

    fb.drawText(frame.x + 24, frame.y + 10, "Withdraw from Vault: Generation " + std::to_string(s.generation),
                Colors::TextPrimary, TextStyle::Heading);
    const std::string searchLine = s.query.empty()
        ? "Copies go into this save's staged changes; the Vault keeps its record"
        : "Search: \"" + s.query + "\"  (" + std::to_string(s.hits.size()) + " found)";
    fb.drawText(frame.x + 24, frame.y + 48, searchLine, Colors::Accent, TextStyle::Caption);
    if (!s.message.empty())
        fb.drawText(frame.x + 24, frame.y + 70, s.message, Colors::Warning, TextStyle::Caption);

    std::string error;
    const V::Store* store = sharedStore(error);
    const int count = static_cast<int>(s.hits.size());
    if (count == 0) {
        fb.drawText(l.list.x + 12, l.list.y + 16,
                    s.query.empty() ? "No Generation " + std::to_string(s.generation) + " Pokemon in the Vault yet."
                                    : "No Vault Pokemon match this search.",
                    Colors::TextSecondary, TextStyle::Body);
    }
    for (int i = 0; store && i < l.visibleRows; ++i) {
        const int index = s.first + i;
        if (index < 0 || index >= count) break;
        const auto& hit = s.hits[static_cast<std::size_t>(index)];
        if (hit.entryIndex >= store->entries().size()) break;
        const auto& e = store->entries()[hit.entryIndex];
        const auto cell = Geo::cellRect(l, i);
        SharedBoxedRows::State state;
        state.focused = index == s.cursor;
        SharedBoxedRows::drawBox(fb, cell, state);
        SharedBoxedRows::publish(screen.touchButtons, kRowTouchBase + i, cell, state);
        const Color text = SharedBoxedRows::labelColor(state);
        int x = cell.x + SharedBoxedRows::kLabelInset;
        const int half = cell.h / 2;
        const std::string species = e.summary.speciesName.empty() ? "Pokemon" : e.summary.speciesName;
        x = drawHighlighted(fb, x, cell.y + 2, half, species, Search::spansFor(hit, Search::Field::Species), text,
                            TextStyle::Body);
        if (!e.summary.nickname.empty()) {
            x += 14;
            (void)drawHighlighted(fb, x, cell.y + 2, half, e.summary.nickname,
                                  Search::spansFor(hit, Search::Field::Nickname), text, TextStyle::Body);
        }
        // Second line: OT, source game, level, and the ID when the search matched it.
        int dx = cell.x + SharedBoxedRows::kLabelInset;
        const int dy = cell.y + half - 2;
        if (!e.summary.otName.empty()) {
            fb.drawText(dx, dy + 4, "OT ", Colors::TextSecondary, TextStyle::Caption);
            int w = 0, h = 0;
            fb.measureText("OT ", w, h, TextStyle::Caption);
            dx = drawHighlighted(fb, dx + w, dy, half, e.summary.otName, Search::spansFor(hit, Search::Field::OtName),
                                 Colors::TextSecondary, TextStyle::Caption) + 14;
        }
        if (!e.provenance.sourceGame.empty())
            dx = drawHighlighted(fb, dx, dy, half, e.provenance.sourceGame,
                                 Search::spansFor(hit, Search::Field::SourceGame), Colors::TextSecondary,
                                 TextStyle::Caption) + 14;
        if (e.summary.level)
            fb.drawText(dx, dy + 4, "Lv " + std::to_string(e.summary.level), Colors::TextSecondary, TextStyle::Caption);
        const auto idSpans = Search::spansFor(hit, Search::Field::VaultId);
        if (!idSpans.empty()) {
            const std::string id = e.id.text();
            int w = 0, h = 0;
            fb.measureText(id, w, h, TextStyle::Caption);
            (void)drawHighlighted(fb, cell.right() - SharedBoxedRows::kDetailInset - w, cell.y, cell.h, id, idSpans,
                                  Colors::TextSecondary, TextStyle::Caption);
        }
    }
    SharedScrollbar::drawAndPublish(fb, s.scrollbar, frame.right() - 14, l.list.y, SharedScrollbar::kTrackWidth,
                                    l.list.h, SharedScrollbar::rows(count, l.visibleRows, s.first));
    drawNavBar(fb, {{"A", "Withdraw Copy"}, {"Y", "Search"}, {"L/R", "Page"}, {"B", "Close"}});
}

} // namespace UI::VaultDialog
