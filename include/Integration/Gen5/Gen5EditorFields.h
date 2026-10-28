#ifndef POKEBANK_GEN5_EDITOR_FIELDS_H
#define POKEBANK_GEN5_EDITOR_FIELDS_H

#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5MoveCompatibility.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5Progression.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Generation V field model of the ONE shared Pokemon editor. The shared
// workspace (DETAILS | STATS | MOVES, its pickers and move editor) is drawn and
// navigated by src/UI/SharedPokemonEditorSurface.inc; this header supplies only
// Gen V's rows, native choice lists, capabilities and staged mutations, so it is
// host-testable without any UI. Every value goes through the guarded native PK5
// draft (Gen5SharedPokemonSession / StagedPokemon5Record); nothing is written
// to a save, and no choice list asserts encounter or learnset legality.
namespace PokeVault::Integration::Gen5::EditorFields {

using Field = StagedPokemon5Record::Field;
using Draft = Gen5SharedPokemonSession;

// DETAILS rows: the Gen IV rows in the same order, then OT Gender and Fateful.
enum Row : int {
    Species = 0, Nickname, Level, Experience, OriginalTrainer, TrainerId, SecretId,
    HeldItem, Friendship, Language, Origin, Ball, MetLevel, MetLocation, Pokerus, Form,
    OriginalTrainerGender, FatefulEncounter, RowCount
};
inline constexpr std::array<const char*, RowCount> kDetailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID",
    "Held Item", "Friendship", "Language", "Origin", "Ball",
    "Met Level", "Met Location", "Pokerus", "Form", "OT Gender", "Fateful"
}};

// STATS PID-linked rows (shared workspace rows 6..10).
enum PidRow : int { NatureRow = 6, GenderRow = 7, ShinyRow = 8, AbilityRow = 9, PidValueRow = 10 };

[[nodiscard]] inline std::optional<NativePersonalSelection5> personalFor(
    const Pokemon5ReadOnly& p, bool b2w2) {
    const auto personal = personal5(p.species(), p.form(), b2w2);
    if (!personal) return std::nullopt;
    return personal;
}
[[nodiscard]] inline bool earlierOrigin(const Pokemon5ReadOnly& p) noexcept {
    return earlierGenerationOrigin5(p.originVersion());
}
[[nodiscard]] inline uint8_t level(const Pokemon5ReadOnly& p, bool b2w2) noexcept {
    if (p.partyRecord()) return p.partyLevel();
    const auto personal = personal5(p.species(), p.form(), b2w2);
    return personal ? levelForExperience5(personal.info->growthRate, p.experience()) : 1;
}

// ---- Native choice lists (pinned sources in the headers they come from) ----

// PKHeX 6501f0ab ItemStorage5.GetAllHeld(): General + Medicine + Berry,
// without the Unreleased list. "None" (0) first.
[[nodiscard]] inline std::vector<uint16_t> heldItemChoices() {
    std::vector<uint16_t> ids{0};
    const auto add = [&](const auto& list) {
        for (const uint16_t id : list)
            if (!detail5::contains(BagCatalog5::kUnreleased, id) &&
                std::find(ids.begin(), ids.end(), id) == ids.end())
                ids.push_back(id);
    };
    add(BagCatalog5::kGeneral);
    add(BagCatalog5::kMedicine);
    add(BagCatalog5::kBerry);
    return ids;
}
// PKHeX 6501f0ab Game/Enums/Ball.cs: Master (1) .. Dream (25) exist in Gen V.
[[nodiscard]] inline std::vector<uint16_t> ballChoices() {
    std::vector<uint16_t> balls;
    for (uint16_t ball = 1; ball <= 25; ++ball) balls.push_back(ball);
    return balls;
}
[[nodiscard]] inline std::vector<uint16_t> metLocationChoices() {
    return {NativeMetLocations5.begin(), NativeMetLocations5.end()};
}
// Species-specific level/egg/tutor/TM and pre-evolution moves for the actual
// Black/White/B2W2 game, plus valid earlier-generation transfers. Do not
// silently erase unusual source event moves or certify them as clean.
[[nodiscard]] inline std::vector<uint16_t> moveChoices(
    std::string_view exactGameId, uint16_t species, uint8_t form,
    const std::array<uint16_t,4>& existingSourceMoves) {
    return Gen5MoveCompatibility::selectableMoves(
        exactGameId,species,form,existingSourceMoves);
}
// National Dex 1..649: every species that exists in BW/B2W2, including the
// Gen I-IV species that transfer in. No later-generation species.
inline constexpr uint16_t kMaxSpecies = 649;

// Forms that the native draft accepts for this Pokemon (probed on a copy).
[[nodiscard]] inline std::vector<uint16_t> formChoices(const Draft& draft, bool b2w2) {
    std::vector<uint16_t> forms;
    const auto current = draft.current();
    if (!current) return forms;
    const auto personal = personal5(current->species(), 0, b2w2);
    if (!personal) return forms;
    for (uint16_t form = 0; form < personal.forms; ++form) {
        auto probe = draft;
        std::string error;
        if (form == current->form() || probe.stage(Field::Form, 0, form, error))
            forms.push_back(form);
    }
    return forms;
}
[[nodiscard]] inline bool hasAlternateForm(const Draft& draft, bool b2w2) {
    return formChoices(draft, b2w2).size() > 1;
}
// Named ability choices: slot 1, slot 2 when it is a different ability, and the
// hidden ability when it exists. No duplicate ID-only entries.
[[nodiscard]] inline std::vector<uint16_t> abilityChoices(const Pokemon5ReadOnly& p, bool b2w2) {
    std::vector<uint16_t> slots;
    const auto personal = personal5(p.species(), p.form(), b2w2);
    if (!personal) return slots;
    const auto& info = *personal.info;
    if (info.ability1) slots.push_back(0);
    if (info.ability2 && info.ability2 != info.ability1) slots.push_back(1);
    if (info.abilityHidden) slots.push_back(2);
    return slots;
}
[[nodiscard]] inline uint16_t abilitySlot(const Pokemon5ReadOnly& p) noexcept {
    return p.hiddenAbility() ? 2 : static_cast<uint16_t>(abilityBit5(p.pid()));
}
// Choosing `slot` would leave the stored ability exactly as it is (same ability id, same hidden flag).
[[nodiscard]] inline bool abilityChoiceKeepsStored(const Pokemon5ReadOnly& p, bool b2w2, uint16_t slot) {
    const auto personal = personal5(p.species(), p.form(), b2w2);
    return personal && slot <= 2 && abilityForSlot5(*personal.info, static_cast<uint8_t>(slot)) == p.ability() &&
           (slot == 2) == p.hiddenAbility();
}
// The picker opens on the slot that holds the ability the record actually stores (the row shows it), so
// accepting the preselection changes nothing even when the stored ability disagrees with the PID
// (owner Oct 28 PID-route review: a stored slot-2 ability was rewritten to the PID's slot on A).
// Otherwise the PID slot; slot 1 for a single-ability species even when PID bit 16 is 1.
[[nodiscard]] inline uint16_t abilityChoiceFor(const Pokemon5ReadOnly& p, bool b2w2) {
    const auto choices = abilityChoices(p, b2w2);
    const uint16_t slot = abilitySlot(p);
    if (std::find(choices.begin(), choices.end(), slot) != choices.end() && abilityChoiceKeepsStored(p, b2w2, slot))
        return slot;
    for (const uint16_t c : choices)
        if (abilityChoiceKeepsStored(p, b2w2, c)) return c;
    return std::find(choices.begin(), choices.end(), slot) != choices.end() ? slot : 0;
}

// ---- Capabilities ----

[[nodiscard]] inline bool detailEditable(const Draft& draft, int row, bool b2w2) {
    if (!draft.editable()) return false;
    const auto p = draft.current();
    if (!p) return false;
    switch (row) {
        case Species: case Nickname: case Level: case Experience: case OriginalTrainer:
        case TrainerId: case SecretId: case HeldItem: case Friendship: case MetLevel:
        case Pokerus: case OriginalTrainerGender: case FatefulEncounter: case Language:
            return true;
        case Ball: case MetLocation:
            // Same policy as Gen IV: encounter data of a transferred Pokemon stays read-only.
            return gen5Origin5(p->originVersion());
        case Form:
            return hasAlternateForm(draft, b2w2);
        default:
            return false; // Origin remains a historical identity, never silently rewritten.
    }
}
// Rows that stay locked for this record in every mode (View, Edit, Create): shown in the gold
// ReadOnly field state (UI/SharedFieldState.h) instead of a "(read-only)" label suffix.
[[nodiscard]] inline bool detailLocked(const Pokemon5ReadOnly& p, int row, bool singleForm) noexcept {
    switch (row) {
        case Origin: return true;
        case Ball: case MetLocation: return !gen5Origin5(p.originVersion());
        case Form: return singleForm;
        default: return false;
    }
}
[[nodiscard]] inline bool pidRowEditable(const Draft& draft, int row, bool b2w2) {
    if (!draft.editable()) return false;
    const auto p = draft.current();
    if (!p || earlierOrigin(*p)) return false;
    const auto personal = personal5(p->species(), p->form(), b2w2);
    if (!personal) return false;
    switch (row) {
        case NatureRow: case ShinyRow: return true;
        case GenderRow: return variableGenderRatio5(personal.info->genderRatio);
        case AbilityRow: return abilityChoices(*p, b2w2).size() > 1;
        default: return false; // the PID itself is never typed in
    }
}
// Why a PID-linked row cannot be edited, for the explanatory status line.
[[nodiscard]] inline const char* pidRowRestriction(const Draft& draft, int row, bool b2w2) {
    const auto p = draft.current();
    if (!p) return "No Generation V draft is open";
    if (row == PidValueRow)
        return "PID is read-only; Gender, Shiny and Ability change it through native rules";
    if (earlierOrigin(*p))
        return "From Gen III/IV: its original PID fixes nature, gender, ability and shiny";
    if (row == GenderRow) return "Gender is fixed for this Generation V species";
    if (row == AbilityRow && abilityChoices(*p, b2w2).size() <= 1)
        return "This Generation V species has a single ability";
    return "This Generation V value is read-only";
}

// ---- Staged mutations (all through the guarded native draft) ----

// TID / SID: the raw ID changes, then the PID keeps the previous shiny state
// (Gen V origins only; an earlier origin is refused if shininess would change).
[[nodiscard]] inline bool stageTrainerId(Draft& draft, bool secret, uint16_t value,
                                         std::string& error) {
    const auto before = draft.current();
    if (!before) { error = "No Generation V draft is open"; return false; }
    auto probe = draft;
    if (!probe.stage(secret ? Field::SecretId : Field::TrainerId, 0, value, error)) return false;
    const auto after = probe.current();
    if (after->shiny() != before->shiny()) {
        if (earlierOrigin(*before)) {
            error = "That ID would change this Gen III/IV-origin Pokemon's shininess";
            return false;
        }
        const uint32_t pid = keepShinyForIds5(before->pid(), before->tid(), before->sid(),
                                              after->tid(), after->sid());
        if (!probe.stage(Field::Personality, 0, pid, error)) return false;
    }
    draft = std::move(probe);
    return true;
}
[[nodiscard]] inline bool toggleGender(Draft& draft, std::string& error) {
    const auto p = draft.current();
    if (!p || p->gender() > 1) { error = "Gender is fixed for this Generation V species"; return false; }
    return draft.stage(Field::Gender, 0, p->gender() == 0 ? 1u : 0u, error);
}
[[nodiscard]] inline bool toggleShiny(Draft& draft, std::string& error) {
    const auto p = draft.current();
    if (!p) { error = "No Generation V draft is open"; return false; }
    return draft.stage(Field::Shiny, 0, p->shiny() ? 0u : 1u, error);
}
[[nodiscard]] inline bool toggleFlag(Draft& draft, Field field, std::string& error) {
    const auto p = draft.current();
    if (!p) { error = "No Generation V draft is open"; return false; }
    const bool current = field == Field::FatefulEncounter ? p->fatefulEncounter()
                                                          : p->originalTrainerGender() != 0;
    return draft.stage(field, 0, current ? 0u : 1u, error);
}

[[nodiscard]] inline const char* pokerusLabel(uint32_t mode) noexcept {
    return mode == 0 ? "None" : mode == 1 ? "Cured" : "Infected";
}
[[nodiscard]] constexpr const char* natureName(uint32_t nature) noexcept {
    constexpr std::array<const char*, 25> names{{
        "Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax",
        "Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash",
        "Calm","Gentle","Sassy","Careful","Quirky"}};
    return nature < names.size() ? names[nature] : "";
}

} // namespace PokeVault::Integration::Gen5::EditorFields
#endif
