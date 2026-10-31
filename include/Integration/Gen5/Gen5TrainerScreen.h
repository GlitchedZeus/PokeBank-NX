#ifndef POKEBANK_GEN5_TRAINER_SCREEN_H
#define POKEBANK_GEN5_TRAINER_SCREEN_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5TrainerName.h"
#include "Integration/Gen5/Gen5Pokedex.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Session-local model for the shared Trainer panel's Gen V field list.
// Editable rows (Name, Money, Play Time, Gender) stage through Gen5StagedTrainer;
// every other row is read-only and says so. Controller and touch share the
// same entry points: Up/Down or tap focuses, A or a second tap edits.
class Gen5TrainerScreen {
public:
    using Workspace = Gen5StagedPokemonWorkspace;
    enum class Row : uint8_t { Name, Money, PlayTime, TrainerId, SecretId, Gender, Badges, Language, Pokedex };
    static constexpr std::array<Row, 9> kRows{{Row::Name, Row::Money, Row::PlayTime, Row::TrainerId,
                                               Row::SecretId, Row::Gender, Row::Badges, Row::Language,
                                               Row::Pokedex}};
    struct FooterEntry { const char* glyph; const char* label; };

    [[nodiscard]] bool open() const noexcept { return open_; }
    [[nodiscard]] size_t row() const noexcept { return row_; }
    [[nodiscard]] Row selectedRow() const noexcept { return kRows[row_]; }
    [[nodiscard]] static constexpr bool editable(Row row) noexcept {
        return row == Row::Name || row == Row::Money || row == Row::PlayTime || row == Row::Gender;
    }
    [[nodiscard]] static constexpr const char* label(Row row) noexcept {
        switch (row) {
            case Row::Name: return "Name";
            case Row::Money: return "Money";
            case Row::PlayTime: return "Play Time";
            case Row::TrainerId: return "Trainer ID";
            case Row::SecretId: return "Secret ID";
            case Row::Gender: return "Gender";
            case Row::Badges: return "Badges";
            case Row::Language: return "Language";
            case Row::Pokedex: return "Pokedex";
        }
        return "";
    }
    [[nodiscard]] static bool available(const Workspace& ws) { return ws.trainerPresentation().has_value(); }
    [[nodiscard]] static std::string value(const Workspace& ws, Row row) {
        if (row == Row::Pokedex) {
            const auto dex = decodePokedex5(ws.sourceSave());
            return dex ? "Seen " + std::to_string(dex->seen) + " / Caught " + std::to_string(dex->caught)
                       : "Unsupported";
        }
        const auto t = ws.trainerPresentation();
        if (!t) return "Unavailable";
        if (newGame(*t)) {
            switch (row) {
                case Row::Name: return "- (no trainer yet)";
                case Row::TrainerId:
                case Row::SecretId:
                case Row::Gender: return "-";
                default: break;
            }
        }
        switch (row) {
            case Row::Name: return displayTrainerName(t->rawName).value_or("(unsupported glyphs)");
            case Row::Money: return std::to_string(t->money);
            case Row::PlayTime: return playTime5(t->hours, t->minutes, t->seconds);
            case Row::TrainerId: return std::to_string(t->tid);
            case Row::SecretId: return std::to_string(t->sid);
            case Row::Gender: return t->gender == 0 ? "Male" : t->gender == 1 ? "Female" : "Unknown";
            case Row::Badges: return std::to_string(t->badgeCount()) + " / 8";
            case Row::Language: return std::to_string(t->language);
            case Row::Pokedex: break;
        }
        return {};
    }

    [[nodiscard]] bool openEditor(const Workspace& ws, std::string& error) {
        if (!available(ws)) {
            error = "Gen V trainer data failed native validation; view only";
            return false;
        }
        open_ = true;
        row_ = 0;
        error.clear();
        return true;
    }
    void close() noexcept { open_ = false; row_ = 0; dex_.close(); }
    // Shared Trainer panel entry from the hub. Unlike openEditor, the last
    // focused row is kept (focus memory); leave() returns to the hub without
    // forgetting it.
    [[nodiscard]] bool enter(const Workspace& ws, std::string& error) {
        if (!available(ws)) {
            error = "Gen V trainer data failed native validation; view only";
            return false;
        }
        open_ = true;
        if (row_ >= kRows.size()) row_ = 0;
        error.clear();
        return true;
    }
    void leave() noexcept { open_ = false; dex_.close(); }
    // First visible row so the focused row is always on screen.
    [[nodiscard]] size_t firstVisible(size_t visibleRows) const noexcept {
        if (visibleRows == 0 || kRows.size() <= visibleRows) return 0;
        const size_t maxFirst = kRows.size() - visibleRows;
        const size_t centred = row_ >= visibleRows / 2 ? row_ - visibleRows / 2 : 0;
        return centred < maxFirst ? centred : maxFirst;
    }
    // Read-only Pokedex list (no Pokedex editor exists for Gen V).
    [[nodiscard]] Gen5PokedexView& dex() noexcept { return dex_; }
    [[nodiscard]] const Gen5PokedexView& dex() const noexcept { return dex_; }
    [[nodiscard]] bool openPokedex(const Workspace& ws, std::string& error) {
        if (!open_ || selectedRow() != Row::Pokedex || !decodePokedex5(ws.sourceSave())) {
            error = "Gen V Pokedex view requires the Pokedex row and a recognized dex block";
            return false;
        }
        dex_.openAt(1);
        error.clear();
        return true;
    }
    // Gen I-IV Trainer convention: focus moves only between rows that can be
    // acted on (the staged rows and the Pokedex list); read-only rows stay
    // visible and labelled but are never focused. Up/Down wrap.
    [[nodiscard]] static constexpr bool focusable(Row row) noexcept {
        return editable(row) || row == Row::Pokedex;
    }
    void moveRow(int delta) noexcept {
        if (!open_ || delta == 0) return;
        constexpr int count = static_cast<int>(kRows.size());
        const int step = delta < 0 ? -1 : 1;
        for (int moved = 0; moved < (delta < 0 ? -delta : delta); ++moved) {
            int next = static_cast<int>(row_);
            for (int i = 0; i < count; ++i) {
                next = ((next + step) % count + count) % count;
                if (focusable(kRows[static_cast<size_t>(next)])) break;
            }
            row_ = static_cast<size_t>(next);
        }
    }
    // A completed tap on an actionable row selects it and acts like A at once
    // (the Gen I-IV Trainer rows); a tap on a read-only row does nothing.
    [[nodiscard]] bool tapRow(size_t row) noexcept {
        if (!open_ || row >= kRows.size() || !focusable(kRows[row])) return false;
        row_ = row;
        return true;
    }
    // A on the focused row: true when the row may be edited by the caller's
    // keypad/keyboard prompt; false (with a reason) for read-only rows.
    [[nodiscard]] bool beginEdit(const Workspace& ws, std::string& error) const {
        if (!open_ || !available(ws)) {
            error = "Gen V trainer editor is not open";
            return false;
        }
        if (const auto t = ws.trainerPresentation(); t && newGame(*t)) {
            error = "New-game save: the game has not written a trainer yet; nothing to edit";
            return false;
        }
        if (!editable(selectedRow())) {
            error = std::string(label(selectedRow())) +
                    " is read-only for Gen V (ownership/story data is never edited)";
            return false;
        }
        if (selectedRow() == Row::Gender) {
            const auto t = ws.trainerPresentation();
            const auto* staged = ws.stagedTrainerIfOpened();
            const uint8_t stored = staged ? staged->baseline().gender : (t ? t->gender : 0xFF);
            if (stored > 1) {
                error = "Gen V trainer gender has an unrecognized stored value; read-only";
                return false;
            }
        }
        error.clear();
        return true;
    }
    [[nodiscard]] std::vector<FooterEntry> footer() const {
        if (!open_) return {{"A", "Open"}, {"B", "Back"}};
        if (dex_.open())
            return {{"D-pad/Stick", "Browse"}, {"L/R", "Prev/Next Registered"}, {"B", "Back"}};
        if (selectedRow() == Row::Pokedex)
            return {{"D-pad/Stick", "Choose"}, {"A", "Open Pokedex"}, {"Y", "Review"}, {"B", "Back"}};
        if (editable(selectedRow()))
            return {{"D-pad/Stick", "Choose"}, {"A", "Edit"}, {"Y", "Review"}, {"B", "Back"}};
        return {{"D-pad/Stick", "Choose"}, {"Y", "Review"}, {"B", "Back"}};
    }
    // R2-BUG-001 parity (Gen I-IV Trainer footer, Lane C cccec27b): "A: Edit" appears only
    // when the FOCUSED row can really be edited in this session. A new-game save has no
    // trainer to edit yet (beginEdit refuses), so it never advertises A there. The page
    // always draws this footer itself; no read-only override replaces it.
    [[nodiscard]] std::vector<FooterEntry> footer(const Workspace& ws) const {
        auto entries = footer();
        const auto t = ws.trainerPresentation();
        if (open_ && !dex_.open() && editable(selectedRow()) && (!t || newGame(*t)))
            std::erase_if(entries, [](const FooterEntry& e) { return std::string_view(e.glyph) == "A"; });
        return entries;
    }

    template <class Refresh>
    [[nodiscard]] bool applyName(Workspace& ws, std::string_view name, Refresh&& refresh, std::string& error) {
        return apply(ws, Row::Name, std::forward<Refresh>(refresh), error,
                     [&](Gen5StagedTrainer& t, std::string* why) { return t.setName(name, why); });
    }
    template <class Refresh>
    [[nodiscard]] bool applyMoney(Workspace& ws, uint32_t money, Refresh&& refresh, std::string& error) {
        return apply(ws, Row::Money, std::forward<Refresh>(refresh), error,
                     [&](Gen5StagedTrainer& t, std::string* why) { return t.setMoney(money, why); });
    }
    template <class Refresh>
    [[nodiscard]] bool applyPlayTime(Workspace& ws, uint32_t h, uint32_t m, uint32_t s, Refresh&& refresh,
                                     std::string& error) {
        return apply(ws, Row::PlayTime, std::forward<Refresh>(refresh), error,
                     [&](Gen5StagedTrainer& t, std::string* why) { return t.setPlayTime(h, m, s, why); });
    }

    // Gender is a two-value inline toggle (Gen III / IV ClassicTrainerSession: A flips Male / Female).
    template <class Refresh>
    [[nodiscard]] bool applyGender(Workspace& ws, uint8_t gender, Refresh&& refresh, std::string& error) {
        return apply(ws, Row::Gender, std::forward<Refresh>(refresh), error,
                     [&](Gen5StagedTrainer& t, std::string* why) { return t.setGender(gender, why); });
    }

    [[nodiscard]] static bool newGame(const Trainer5View& t) noexcept {
        return newGameNoTrainer5(t.rawName.size(), t.tid, t.sid, t.hours, t.minutes, t.seconds);
    }

private:
    template <class Refresh, class Op>
    [[nodiscard]] bool apply(Workspace& ws, Row expected, Refresh&& refresh, std::string& error, Op&& op) {
        if (!open_ || selectedRow() != expected) {
            error = "Gen V trainer selection changed; edit cancelled";
            return false;
        }
        Workspace snapshot = ws;
        auto* trainer = ws.stagedTrainer(&error);
        if (!trainer) return false;
        if (!op(*trainer, &error)) {
            ws = std::move(snapshot);
            return false;
        }
        if (!refresh(error)) {
            ws = std::move(snapshot);
            std::string restored;
            (void)refresh(restored);
            if (error.empty()) error = "Gen V trainer presentation refresh failed; edit rolled back";
            return false;
        }
        error.clear();
        return true;
    }

    bool open_ = false;
    size_t row_ = 0;
    Gen5PokedexView dex_;
};

} // namespace PokeVault::Integration::Gen5
#endif
