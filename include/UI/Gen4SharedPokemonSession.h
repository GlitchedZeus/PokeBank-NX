#pragma once

#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "UI/CreateRequiredFields.h"
#include "UI/HistoryConsistencyFacts.h"
#include "UI/PokemonEditorExitGuard.h"
#include "UI/SpeciesChangeHistoryPolicy.h"
#include "UI/SpeciesChangeMoves.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::Gen4SharedEditor {

namespace Gen4 = PokeVault::Integration::Gen4;

enum class Mode : uint8_t { None, View, Edit, Create };

// The draft's stored Met Location and origin. HG/SS keep it only in the extended field 0x46 (the
// D/P field holds the Faraway Place 3002 sentinel even when nothing is chosen, so 0x46 == 0 is
// "not chosen"); D/P/Pt read as the shared view does (Pokemon4Mutable::setMetLocation).
struct MetLocation4 { uint8_t origin = 0; uint16_t location = 0; };
inline std::optional<MetLocation4> metLocationOf(const Pokemon::Pokemon4Mutable& mon, Enums::GameVersion group) {
    const auto bytes = mon.encryptedBytes();
    const Pokemon::Pokemon4ReadOnly record(bytes, group);
    if (!record.valid() || record.empty()) return std::nullopt;
    const uint8_t origin = record.originVersion();
    if (origin == 7 || origin == 8) return MetLocation4{origin, record.metLocationExtended()};
    const Pokemon::Pokemon4ReadOnlyView view(record);
    return MetLocation4{origin, view.metLocation()};
}

struct Session {
    Mode mode = Mode::None;
    bool confirmExit = false;
    std::vector<std::byte> baselineEncrypted;
    std::array<uint16_t, 4> baselineMoves{};  // Edit: the moves the record had when the session began
    uint16_t baselineSpecies = 0;              // Edit: the species the record had when the session began
    std::optional<Pokemon::Pokemon4Mutable> working;
    // Round 11 follow-up (owner clarification 1): a Create draft's Met Location counts only once the
    // user picked it from the exact game's own list (noteMetLocationChosen). The flag, never the
    // value: 0 and the special / "----" entries are real stored values in Gen IV.
    bool metLocationChosen = false;
    bool pokerusCleared = false;               // the last Create species change cleared Pokerus

    bool begin(const Pokemon::Pokemon4ReadOnly& record,
               Enums::GameVersion group, Mode next, std::string& error) {
        error.clear();
        if (next != Mode::View && next != Mode::Edit) {
            error = "Generation IV existing-record session supports View/Edit only";
            return false;
        }
        auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
            record.originalEncryptedBytes(), group, &error);
        if (!editable) return false;
        baselineEncrypted.assign(
            record.originalEncryptedBytes().begin(),
            record.originalEncryptedBytes().end());
        baselineMoves = editable->moves();
        baselineSpecies = editable->species();
        working = std::move(*editable);
        mode = next;
        confirmExit = false;
        metLocationChosen = false;
        pokerusCleared = false;
        return true;
    }

    bool beginCreate(Pokemon::Pokemon4Mutable draft, std::string& error) {
        error.clear();
        if (!draft.valid() || draft.isParty()) {
            error = "Generation IV Create requires a valid stored PK4 draft";
            return false;
        }
        baselineEncrypted.clear(); // Empty native destination is the Create baseline.
        baselineMoves = {};
        baselineSpecies = 0;
        working = std::move(draft);
        mode = Mode::Create;
        confirmExit = false;
        metLocationChosen = false;
        pokerusCleared = false;
        return true;
    }

    bool editable() const noexcept {
        return (mode == Mode::Edit || mode == Mode::Create) && working.has_value();
    }
    bool creating() const noexcept { return mode == Mode::Create && working.has_value(); }

    bool dirty() const {
        if (creating()) return true; // An uncommitted Create draft always needs an explicit decision.
        return mode == Mode::Edit && working &&
               working->encryptedBytes() != baselineEncrypted;
    }

    bool cycleGender() noexcept {
        if (!editable()) return false;
        const uint8_t current = working->gender();
        if (current > 1) return false;
        return working->setGender(current == 0 ? 1 : 0);
    }

    bool cycleShiny() noexcept {
        return editable() && working->setShiny(!working->shiny());
    }

    bool cycleAbility() noexcept {
        if (!editable()) return false;
        const uint8_t current = working->abilitySlot();
        return working->setAbilitySlot(current == 0 ? 1 : 0);
    }

    // The Species picker's one apply (Round 11; was inline in applyGen4Picker). setSpecies keeps
    // the PID-linked nature / gender / ability and the shiny preview (owner PASS), then:
    //  - moves (SpeciesChangeMoves.h, H28): each current move the new species can use in this exact
    //    game (Gen4MoveCompatibility: native or Transfer) keeps its PP and PP Ups; the rest clear;
    //    nothing is added. The same species changes no move.
    //  - history (SpeciesChangeHistoryPolicy.h, H25): a Create draft's Pokerus resets and a chosen
    //    Met Location that is neither a wild encounter of the new species or its ancestry
    //    (EncounterContext45 kWild45) nor a noted special location goes back to "not chosen"
    //    (0, Met Level = level). An existing Pokemon keeps its Met Location and Pokerus.
    // Any refused write rolls the whole draft back to its bytes before the call.
    bool changeSpecies(uint16_t value, bool shinyWanted, std::string_view exactGameId,
                       Enums::GameVersion group, bool& encounterReset) {
        encounterReset = false;
        pokerusCleared = false;
        if (!editable()) return false;
        const auto before = working->encryptedBytes();
        const bool chosenBefore = metLocationChosen;
        const uint16_t previousSpecies = working->species();
        bool ok = working->setSpecies(value);
        if (ok && working->shiny() != shinyWanted) ok = working->setShiny(shinyWanted);
        if (ok && value != previousSpecies) {
            namespace Compat = PokeVault::Integration::Gen4MoveCompatibility;
            const uint8_t form = working->form();
            ok = PokeBank::UIModel::SpeciesMoveDefaults::setMoves(*working,
                PokeBank::UIModel::SpeciesMoveDefaults::keepLearnable(working->moves(), working->pp(), working->ppUps(),
                    [&](uint16_t move) {
                        const auto a = Compat::classify(exactGameId, value, form, move, false);
                        return a == Compat::Availability::Direct || a == Compat::Availability::Transfer;
                    }));
            namespace History = PokeBank::UIModel::SpeciesChangeHistoryPolicy;
            const auto met = ok ? metLocationOf(*working, group) : std::nullopt;
            ok = ok && met.has_value();
            const auto decision = !ok ? History::Decision{} : History::reconcile(
                creating() ? History::Mode::Create : History::Mode::Edit, metLocationChosen, [&] {
                    const PokeVault::Integration::EncounterContext45::Catalogue context(
                        met->origin, value, met->location, {}, {});
                    return context.wildRow(met->location) != nullptr ||
                           !::Names::specialMetLocation(met->origin, met->location).empty();
                });
            const bool hadPokerus = ok && working->pokerus() != 0;
            if (ok && decision.resetPokerus) ok = working->setPokerusMode(Pokemon::PokerusMode::None);
            if (ok && decision.resetMetLocation)
                ok = working->setMetLocation(0) && working->setMetLevel(working->level());
            encounterReset = ok && decision.resetMetLocation;
            pokerusCleared = ok && decision.resetPokerus && hadPokerus;
            if (encounterReset) metLocationChosen = false;
        }
        if (!ok) {
            auto restored = Pokemon::Pokemon4Mutable::fromEncrypted(before, group);
            if (restored) working = std::move(*restored);
            encounterReset = false;
            pokerusCleared = false;
            metLocationChosen = chosenBefore;
        }
        return ok;
    }

    // The Met Location picker applied a location the user chose (Create only).
    void noteMetLocationChosen() noexcept { if (creating()) metLocationChosen = true; }

    // The status a Create species change posts: what it reset ("" when nothing was).
    std::string speciesChangeResetStatus(bool encounterReset) const {
        return SpeciesChangeHistoryPolicy::resetStatus(pokerusCleared, encounterReset);
    }

    // Round 11 (owner clarification 2): an existing Pokemon whose species was changed in this
    // session keeps its history; a combination that no longer fits is flagged, never cleared.
    std::vector<std::string> historyFlags(Enums::GameVersion group) const {
        if (mode != Mode::Edit || !working || working->species() == baselineSpecies) return {};
        const auto met = metLocationOf(*working, group);
        if (!met) return {};
        return HistoryConsistency::gen45Flags(working->species(), met->origin, met->location,
                                              working->metLevel(), working->level());
    }

    // The shared Round 11 rule (UI/CreateRequiredFields.h): a Create draft needs a Met Location the
    // user explicitly chose from the exact game's list (the chosen flag, never "value != 0") and at
    // least one move; an Edit is refused only when this session took the last move away.
    std::string missingRequiredFields(Enums::GameVersion) const {
        namespace Required = PokeBank::UIModel::CreateRequiredFields;
        if (!working) return {};
        if (creating())
            return Required::message(Required::forCreate(4, metLocationChosen, working->moves()));
        if (mode == Mode::Edit) return Required::message(Required::forEdit(baselineMoves, working->moves()));
        return {};
    }

    bool keep(Gen4::Gen4StagedPokemonEditor& editor,
              std::size_t box, std::size_t slot, std::string& error) {
        if (!editable()) {
            error = "Generation IV Keep is only available for an Edit draft";
            return false;
        }
        if (auto missing = missingRequiredFields(editor.sourceGroup()); !missing.empty()) {
            error = std::move(missing);
            return false;
        }
        if (!editor.commitBoxPokemon(box, slot, *working, &error)) return false;
        close();
        return true;
    }

    bool keepParty(Gen4::Gen4StagedPokemonEditor& editor,
                   std::size_t slot, std::string& error) {
        if (!editable()) {
            error = "Generation IV Keep is only available for an Edit draft";
            return false;
        }
        if (auto missing = missingRequiredFields(editor.sourceGroup()); !missing.empty()) {
            error = std::move(missing);
            return false;
        }
        if (!editor.commitPartyPokemon(slot, *working, &error)) return false;
        close();
        return true;
    }

    bool keepCreate(Gen4::Gen4StagedPokemonEditor& editor,
                    std::size_t box, std::size_t slot, std::string& error) {
        if (!creating()) {
            error = "Generation IV Add is only available for a Create draft";
            return false;
        }
        if (auto missing = missingRequiredFields(editor.sourceGroup()); !missing.empty()) {
            error = std::move(missing);
            return false;
        }
        if (!editor.stageCreateBoxPokemon(box, slot, *working, &error)) return false;
        close();
        return true;
    }

    bool back() noexcept {
        using Guard = PokeBank::UIModel::PokemonEditorExitGuard::SessionKind;
        const Guard kind = mode == Mode::Create ? Guard::Create
                         : mode == Mode::Edit ? Guard::Edit
                                              : Guard::View;
        if (PokeBank::UIModel::PokemonEditorExitGuard::requiresConfirmation(kind, dirty())) {
            confirmExit = true;
            return false;
        }
        close();
        return true;
    }

    void discardDraft() noexcept { close(); }
    void continueEditing() noexcept { confirmExit = false; }

    void close() noexcept {
        mode = Mode::None;
        confirmExit = false;
        baselineEncrypted.clear();
        baselineMoves = {};
        baselineSpecies = 0;
        metLocationChosen = false;
        pokerusCleared = false;
        working.reset();
    }
};

} // namespace PokeBank::UIModel::Gen4SharedEditor
