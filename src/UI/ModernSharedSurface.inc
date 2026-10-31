// The ONE shared application surface for every Generation VI-IX title served by a
// Modern::NativeSession adapter (X/Y/ORAS today through Gen6NativeSession, SM/USUM through
// Gen7NativeSession, later families as their adapters register). It is the Gen V adapter of the
// shared Trainer/Party/Boxes/Items/Trainer screen (Gen5SharedReadOnlySurface.inc) rewritten
// function-for-function against the NativeSession contract: loaded-game hub, Party, Boxes, Pokemon
// Actions, the ONE shared View/Edit workspace (SharedPokemonEditorSurface.inc), Keep/Discard draft,
// Clone/Move/Remove, Items (Bag), Trainer page with the Pokedex summary, Origin & Legality, Review,
// Discard All, Export Copy and the staged-exit confirmation, with controller and touch parity.
//
// No title-family type appears here: everything is asked of the session (view, fieldState,
// stageValue/stageText, the draft bracket, clone/move/remove support, bag, trainer facts, pokedex,
// reviewRows, exportSupport/exportWorkingCopy) and of the capability registry row it carries. The
// original save is immutable: Export produces a separate verified copy only; nothing here opens the
// source for writing, and no legality analyzer runs (Legality is always "Incomplete").
//
// Follow-ups (not in this surface): the shared Gen I-V carry overlay for Move (Move uses the Clone
// destination keypads here), a per-species Pokedex list (the adapter exposes counts only), and a
// Create draft (Create stays Blocked with the registry's reason until an adapter can build one).
#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Gen5/Gen5BoxTouchNavigation.h"
#include "Integration/Gen5/Gen5HubRouting.h"
#include "Integration/Modern/ModernScreenState.h"
#include "Integration/Modern/ModernSources.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "Names/ItemNames.h"
#include "Names/LocationDisplayNames.h"
#include "Names/MachineDisplay.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/PokemonTypes.h"
#include "UI/ClassicStagedExit.h"
#include "UI/ClassicTrainerPage.h"
#include "UI/ClassicTrainerSession.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/InventoryUIContract.h"
#include "UI/NarrowPokemonActionMenu.h"
#include "UI/ReviewEntry.h"
#include "UI/SharedBagAddFlow.h"
#include "UI/SharedBagAddPicker.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/SharedReviewPresentation.h"
#include "UI/SharedTrainerPanel.h"
#include "UI/SharedSpeciesPicker.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"
#include "Utils/SHA256.h"
#include "SharedPokemonEditorSurface.inc"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace UI::ModernSharedSurface {

// Host-UI tests redirect Export Copy into a temporary root; production leaves it empty and exports
// into Modern::exportsRootFor(gameId) (the app-owned exports/<gen> folder).
std::string exportRootOverrideForHostTests;

namespace Modern = PokeVault::Integration::Modern;
namespace Capability = PokeVault::Integration::Capability;
using ScreenState = Modern::ModernScreenState;
using Surface = ScreenState::Surface;
using Modern::SlotRef;
using Modern::Region;
using MField = Modern::Field;
using MFieldState = Modern::FieldState;
namespace EditorSurface = UI::SharedPokemonEditorSurface;
namespace Presentation = PokeBank::UIModel::SharedEditorPresentation;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;
using SharedAction = PokeBank::UIModel::SharedPokemonEditor::Action;

// Screen-local touch ids: bases that cannot collide with Gen I-V's (Gen V uses 77000 / 77400 /
// 81000 / 81800 / 82000). Every tap is release-confirmed by touchedButtonId(), never touch-down.
constexpr int kActionTouchBase = 79000;
constexpr int kPartyTouchBase = 79400;
constexpr int kBagAddTouchBase = 81500;
constexpr int kTrainerRowTouchBase = 82000;
constexpr int kDexRowTouchBase = 83000;

bool isModernSource(const TrainerViewScreen& screen) noexcept {
    // The exact assigned id decides (never the Trainer's claimed group alone), then the group the
    // shared Modern bridge presents; nothing else may reach the inherited mutators.
    if (!Modern::isModernAssignedId(screen.sourceGameId)) return false;
    const auto family = screen.trainer.getGameGroup();
    return family == Enums::GameVersion::XY || family == Enums::GameVersion::ORAS ||
           family == Enums::GameVersion::SM || family == Enums::GameVersion::USUM;
}

namespace {

PokeVault::Legacy::ModernReadOnlyTrainer& nativeTrainer(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::ModernReadOnlyTrainer&>(screen.trainer);
}
Modern::NativeSession& sessionOf(TrainerViewScreen& screen) { return nativeTrainer(screen).session(); }
const Modern::NativeSession& sessionOf(const TrainerViewScreen& screen) {
    return static_cast<const PokeVault::Legacy::ModernReadOnlyTrainer&>(screen.trainer).session();
}
// "Generation VI" from the registry row of the exact game (ModernSources::generationLabelFor).
std::string generationLabel(const TrainerViewScreen& screen) {
    const auto label = Modern::generationLabelFor(screen.sourceGameId);
    return label.empty() ? std::string(sessionOf(screen).familyLabel()) : std::string(label);
}
std::string gameTitle(const TrainerViewScreen& screen) {
    return std::string(sessionOf(screen).capabilities().title);
}
std::string recordFormat(const TrainerViewScreen& screen) {
    return std::string(sessionOf(screen).capabilities().recordFormat);
}
std::string sha256Hex(std::span<const uint8_t> bytes) {
    Utils::SHA256 sha;
    sha.update(bytes.data(), bytes.size());
    uint8_t digest[Utils::PKSE_SHA256_HASH_SIZE];
    sha.finalize(digest);
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(sizeof digest * 2);
    for (const uint8_t b : digest) {
        out.push_back(kDigits[b >> 4]);
        out.push_back(kDigits[b & 0xF]);
    }
    return out;
}

// ---- per-screen session state (never carried to another opened save: instanceSerial) ----
enum class BagSurface : uint8_t { Browse, AddPicker, ConfirmRemove };
struct ModernEditorState : EditorSurface::EditorState {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;
    ScreenState screen;                 // the control model (ModernScreenState.h)
    bool workspaceOpen = false;
    // Items (Bag)
    BagSurface bagSurface = BagSurface::Browse;
    size_t pocket = 0, row = 0;
    bool bagEntered = false;
    std::vector<uint16_t> addChoices;
    size_t addIndex = 0;
    SharedScrollbar::State bagAddScrollbar;
    // Trainer page
    bool trainerOpen = false;
    size_t trainerRow = 0;
    bool dexOpen = false;
};
ModernEditorState& stateFor(const TrainerViewScreen& screen) {
    static ModernEditorState state;
    if (state.owner != &screen || state.ownerSerial != screen.instanceSerial) {
        state = {};
        state.owner = &screen;
        state.ownerSerial = screen.instanceSerial;
    }
    return state;
}
bool refreshPresentation(TrainerViewScreen& screen, std::string& why) {
    return nativeTrainer(screen).refreshPresentation(why);
}
SlotRef selectedNativeSlot(const TrainerViewScreen& screen) {
    return screen.selectedMode == TrainerViewScreen::ViewMode::Party
        ? SlotRef::party(static_cast<size_t>(std::max(0, screen.selectedPartyIndex)))
        : SlotRef::boxed(static_cast<size_t>(std::max(0, screen.selectedBoxIndex)),
                         static_cast<size_t>(std::max(0, screen.selectedItemIndex)));
}
void beginActions(TrainerViewScreen& screen) {
    auto& st = stateFor(screen);
    std::string error;
    const auto slot = selectedNativeSlot(screen);
    if (!st.screen.openActions(sessionOf(screen), slot, error)) {
        screen.postStatus(error, 300);
    } else if (const auto why = ScreenState::createRefusal(sessionOf(screen), slot); !why.empty()) {
        // An empty slot: say at once why there is no Add row (the sheet footer repeats it).
        screen.postStatus("Add Pokemon unavailable: " + why, 360);
    }
    screen.controllerNavigation.reset();
}

// ---- text helpers ----
std::string shortValue(std::string value, size_t limit = 18) {
    if (value.size() <= limit) return value;
    size_t cut = limit - 3;
    while (cut > 0 && (static_cast<unsigned char>(value[cut]) & 0xC0) == 0x80) --cut;
    return value.substr(0, cut) + "...";
}
std::string dateText(const Modern::Date& d) {
    if (d.month == 0 && d.day == 0) return "Not recorded";
    return std::to_string(d.year) + "-" + (d.month < 10 ? "0" : "") + std::to_string(d.month) + "-" +
           (d.day < 10 ? "0" : "") + std::to_string(d.day);
}
std::string metLocationText(const Modern::RecordView& p) {
    // Player-facing name from the shared location tables; an id the tables do not know reads
    // "Unknown location" (never a guessed name).
    return Names::getMetLocationDisplayName(p.originGame, p.metLocation);
}
std::string pokerusText(uint8_t raw) { return Presentation::pokerusLabel(Presentation::pokerusState(raw)); }
std::array<uint8_t, 2> classicTypes(uint16_t species, uint8_t form, Enums::GameVersion group) {
    const auto types = Pokemon::getPokemonTypes(species, form, group);
    if (types.type1 == 0xFF) return {0xFF, 0xFF};
    return {SharedSpeciesPicker::canonicalToClassicType(types.type1),
            types.type2 == 0xFF ? uint8_t{0xFF} : SharedSpeciesPicker::canonicalToClassicType(types.type2)};
}
std::string bagItemLabel(const TrainerViewScreen& screen, uint16_t id) {
    return Names::machineDisplayLabel(screen.trainer.getGameGroup(), id, Names::getItemName(id));
}

// ---- the editor policy: the Modern adapter of the ONE shared Pokemon editor workspace ----
// View and Edit run through SharedPokemonEditorSurface.inc, the same code path, pickers, focus
// model and controls as Gen IV/V. This policy supplies only the rows, the adapter's values, the
// per-field states (session.fieldState) and the staging calls; every change goes through the
// adapter's draft bracket.
enum DetailRow : int {
    Species = 0, Nickname, Level, Experience, OriginalTrainer, TrainerId, SecretId, HeldItem, Friendship,
    Language, Origin, Ball, MetLevel, MetLocation, Pokerus, Form, OriginalTrainerGender, FatefulEncounter,
    RowCount
};
constexpr std::array<const char*, RowCount> kDetailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID", "Held Item", "Friendship",
    "Language", "Origin", "Ball", "Met Level", "Met Location", "Pokerus", "Form", "OT Gender", "Fateful"}};
constexpr std::array<MField, RowCount> kDetailFields{{
    MField::Species, MField::Nickname, MField::Level, MField::Experience, MField::OriginalTrainer,
    MField::TrainerId, MField::SecretId, MField::HeldItem, MField::Friendship, MField::Language,
    MField::OriginGame, MField::Ball, MField::MetLevel, MField::MetLocation, MField::Pokerus, MField::Form,
    MField::OriginalTrainerGender, MField::FatefulEncounter}};

struct ModernEditor {
    using Record = Modern::RecordView;
    using PickerTarget = EditorSurface::PickerTarget;
    // The shared shell's row geometry (18 DETAILS rows, six IV/EV rows, five PID-linked rows) is
    // the Gen V layout; SharedPokemonEditorContract::layoutFor has no Gen VI+ row yet, so the
    // layout generation is Gen5 (see the proposed contract patch in the lane report). Review's
    // button rule and every visible label use the real generation.
    static constexpr auto generation = PokeBank::UIModel::SharedPokemonEditor::Generation::Gen5;
    static constexpr int kGeneration = 6;   // ReviewEntry footer rule (+ Review in Edit)
    static constexpr int kDetailRows = RowCount;
    // Upper bound of the species chooser grid (PKHeX 6501f0ab Legal.MaxSpeciesID_9 = 1025); the
    // picker itself lists only what the adapter allows, and no current adapter edits Species.
    static constexpr int kMaxSpecies = 1025;
    static constexpr bool kCompactForm = true;
    static constexpr const char* kDetailReadOnly = "This field is read-only for this record; no verified writer";
    static constexpr const char* kValueReadOnly = "This value is read-only for this record";
    static constexpr const char* kHeldItemTitle = "Held Item";
    static constexpr const char* kMovePickerTitle = "Move";
    static constexpr const char* kMovePickerSubtitle = "Stored moves • learnability not checked";
    static constexpr const char* kMoveEditorSubtitle = "Moves are read-only in this build";
    static constexpr const char* kDataTitle = "Origin & Legality";

    static ModernEditorState& st(const TrainerViewScreen& screen) { return stateFor(screen); }
    static ScreenState& backend(const TrainerViewScreen& screen) { return stateFor(screen).screen; }
    static Modern::NativeSession& session(const TrainerViewScreen& screen) {
        return sessionOf(const_cast<TrainerViewScreen&>(screen));
    }
    static Region region(const TrainerViewScreen& screen) { return backend(screen).target().region; }
    static MFieldState fieldState(const TrainerViewScreen& screen, MField field) {
        return session(screen).fieldState(field, region(screen));
    }
    static bool fieldEditable(const TrainerViewScreen& screen, MField field) {
        return fieldState(screen, field) == MFieldState::Editable;
    }
    static std::optional<Record> current(const TrainerViewScreen& screen) {
        const auto v = session(screen).view(backend(screen).target());
        if (!v || !v->valid || v->empty) return std::nullopt;
        return v;
    }

    static bool hasWorking(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).workspaceOpen() && current(screen).has_value();
    }
    static bool editable(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        const auto surface = backend(screen).surface();
        return (surface == Surface::Edit || surface == Surface::ConfirmDraft) && session(screen).draftActive();
    }
    static bool passive(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).surface() == Surface::View;
    }
    static bool creating(const TrainerViewScreen&, const EditorSurface::EditorState&) { return false; }
    static bool confirmExit(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).surface() == Surface::ConfirmDraft;
    }
    static std::optional<Record> preview(const TrainerViewScreen& screen, const EditorSurface::EditorState& state) {
        if (!hasWorking(screen, state)) return std::nullopt;
        return current(screen);
    }
    static void previewFailed(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        screen.postStatus(generationLabel(screen) + " editor draft failed the strict " + recordFormat(screen) + " preview", 360);
        std::string error;
        (void)backend(screen).discardDraft(session(screen), [&](std::string& why) { return refreshPresentation(screen, why); }, error);
    }
    static void sessionBack(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        (void)backend(screen).back(session(screen));
        screen.controllerNavigation.reset();
    }
    static void continueEditing(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        backend(screen).continueEditing();
    }
    static void discard(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        std::string error;
        if (!backend(screen).discardDraft(session(screen), [&](std::string& why) { return refreshPresentation(screen, why); }, error))
            screen.postStatus(error, 320);
        else
            screen.postStatus("Local " + generationLabel(screen) + " draft discarded; staged edits preserved", 220);
    }
    static void keep(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        std::string error;
        if (!backend(screen).keep(session(screen), [&](std::string& why) { return refreshPresentation(screen, why); }, error)) {
            screen.postStatus(error.empty() ? generationLabel(screen) + " Keep was rejected" : error, 360);
            return;   // the dirty draft, cursor and prior staged changes are intact
        }
        screen.postStatus(recordFormat(screen) + " draft kept in app memory only; the source save is unchanged", 300);
        screen.controllerNavigation.reset();
    }
    static void openReview(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        if (!backend(screen).openReviewFrom(session(screen)))
            screen.postStatus("No staged " + generationLabel(screen) + " changes to review", 220);
        screen.controllerNavigation.reset();
    }

    // Stage one adapter field on the draft slot; the adapter's refusal reason is shown verbatim.
    static bool stage(TrainerViewScreen& screen, MField field, size_t index, uint32_t value) {
        std::string error;
        if (session(screen).stageValue(backend(screen).target(), field, value, index, error)) return true;
        screen.postStatus(error.empty() ? generationLabel(screen) + " draft refused that value" : error, 320);
        return false;
    }
    static bool stageText(TrainerViewScreen& screen, MField field, const std::string& text) {
        std::string error;
        if (session(screen).stageText(backend(screen).target(), field, text, error)) return true;
        screen.postStatus(error.empty() ? generationLabel(screen) + " draft refused that text" : error, 320);
        return false;
    }
    static void readOnly(TrainerViewScreen& screen, const char* label) {
        screen.postStatus(std::string(label) + " is read-only for " + generationLabel(screen) + " records; no verified writer", 300);
    }
    static uint32_t maximumOr(const TrainerViewScreen& screen, MField field, uint32_t fallback) {
        const auto maximum = session(screen).fieldMaximum(field);
        return maximum ? maximum : fallback;
    }

    static const char* detailLabel(int row) { return kDetailLabels[static_cast<size_t>(row)]; }
    static bool detailEditable(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int row, const Record&) {
        return fieldEditable(screen, kDetailFields[static_cast<size_t>(row)]);
    }
    static bool detailLocked(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int row, const Record&) {
        return !fieldEditable(screen, kDetailFields[static_cast<size_t>(row)]);
    }
    static std::string detailValue(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                   const Record& p, int row) {
        if (fieldState(screen, kDetailFields[static_cast<size_t>(row)]) == MFieldState::Hidden) return "(not stored)";
        switch (row) {
            case Species: return std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species);
            case Nickname: return p.nickname.empty() ? std::string("(none)") : p.nickname;
            case OriginalTrainer: return p.otName.empty() ? std::string("(none)") : p.otName;
            case Level: return std::to_string(p.level);
            case Experience: return std::to_string(p.experience);
            case TrainerId: return std::to_string(p.tid);
            case SecretId: return std::to_string(p.sid);
            case HeldItem: return p.heldItem == 0 ? "(none)" : Names::getItemName(p.heldItem);
            case Friendship: return std::to_string(p.friendship);
            case Language: return Enums::getLanguageName(p.language);
            case Origin: return Enums::getOriginGameName(p.originGame);
            case Ball: return Enums::getBallName(p.ball);
            case MetLevel: return std::to_string(p.metLevel);
            case MetLocation: return metLocationText(p);
            case Pokerus: return pokerusText(p.pokerus);
            case Form: return p.form == 0 ? "Default" : "Form " + std::to_string(p.form);
            case OriginalTrainerGender: return p.otGender == 0 ? "Male" : "Female";
            case FatefulEncounter: return p.fatefulEncounter ? "Yes" : "No";
            default: return {};
        }
    }
    static bool editDetail(TrainerViewScreen& screen, EditorSurface::EditorState& state, int row, const Record& p) {
        const MField field = kDetailFields[static_cast<size_t>(row)];
        if (!fieldEditable(screen, field)) {
            readOnly(screen, detailLabel(row));
            return true;
        }
        auto& s = session(screen);
        switch (row) {
            case Species: {
                const auto maximum = s.fieldMaximum(MField::Species);
                if (maximum == 0) { readOnly(screen, "Species"); return true; }
                std::vector<uint16_t> v;
                for (uint32_t id = 1; id <= maximum && id <= static_cast<uint32_t>(kMaxSpecies); ++id)
                    v.push_back(static_cast<uint16_t>(id));
                EditorSurface::openPicker(screen, state, PickerTarget::Species, std::move(v), p.species);
                state.speciesPreviewShiny = p.shiny;
                return true;
            }
            case Nickname: case OriginalTrainer: {
                const bool nickname = row == Nickname;
                const auto r = Utils::promptText(nickname ? "Nickname" : "Original Trainer",
                    "1-12 ASCII characters", nickname ? p.nickname : p.otName, 12);
                if (r.accepted) (void)stageText(screen, field, r.text);
                return true;
            }
            case Level: {
                const auto r = Utils::promptNumber("Level", p.level, 1, static_cast<int>(maximumOr(screen, MField::Level, 100)));
                if (r.accepted) (void)stage(screen, MField::Level, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case Experience: {
                const auto maximum = s.fieldMaximum(MField::Experience);
                if (maximum == 0) { readOnly(screen, "Experience"); return true; }
                const auto r = Utils::promptNumber("Experience", static_cast<int>(std::min<uint32_t>(p.experience, maximum)),
                                                   0, static_cast<int>(maximum));
                if (r.accepted) (void)stage(screen, MField::Experience, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case TrainerId: case SecretId: {
                const bool secret = row == SecretId;
                const auto r = Utils::promptNumber(secret ? "Secret ID" : "Trainer ID", secret ? p.sid : p.tid, 0,
                                                   static_cast<int>(maximumOr(screen, field, 0xFFFF)));
                if (r.accepted) (void)stage(screen, field, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case HeldItem:
                EditorSurface::openPicker(screen, state, PickerTarget::HeldItem, s.heldItemChoices(), p.heldItem);
                return true;
            case Friendship: {
                const auto r = Utils::promptNumber("Friendship", p.friendship, 0, static_cast<int>(maximumOr(screen, MField::Friendship, 255)));
                if (r.accepted) (void)stage(screen, MField::Friendship, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case Language: {
                const auto maximum = s.fieldMaximum(MField::Language);
                if (maximum == 0) { readOnly(screen, "Language"); return true; }
                std::vector<uint16_t> v;
                for (uint32_t id = 1; id <= maximum; ++id) v.push_back(static_cast<uint16_t>(id));
                EditorSurface::openPicker(screen, state, PickerTarget::Language, std::move(v), p.language);
                return true;
            }
            case Ball:
                EditorSurface::openPicker(screen, state, PickerTarget::Ball, s.ballChoices(), p.ball);
                return true;
            case MetLevel: {
                const auto r = Utils::promptNumber("Met Level", p.metLevel, 0, static_cast<int>(maximumOr(screen, MField::MetLevel, 100)));
                if (r.accepted) (void)stage(screen, MField::MetLevel, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case Pokerus: {
                // Inline toggle None -> Infected -> Cured -> None over the native byte; cycling back to
                // the draft's original state restores that exact byte (SharedEditorPresentation).
                const auto& baseline = backend(screen).draftBaseline();
                const uint8_t next = Presentation::nextPokerusByte(p.pokerus, baseline ? baseline->pokerus : p.pokerus);
                (void)stage(screen, MField::Pokerus, 0, next);
                return true;
            }
            case OriginalTrainerGender:
                (void)stage(screen, MField::OriginalTrainerGender, 0, p.otGender == 0 ? 1u : 0u);
                return true;
            case FatefulEncounter:
                (void)stage(screen, MField::FatefulEncounter, 0, p.fatefulEncounter ? 0u : 1u);
                return true;
            default:
                // Origin, Met Location and Form have no verified choice list in this build.
                screen.postStatus(std::string(detailLabel(row)) + " has no verified " + generationLabel(screen) +
                                  " choice list in this build", 300);
                return true;
        }
    }
    static bool passiveActivate(TrainerViewScreen&, EditorSurface::EditorState&, int) { return false; }

    static std::array<uint8_t, 6> ivs(const Record& p) { return p.ivs; }
    static std::array<uint8_t, 6> evs(const Record& p) { return p.evs; }
    static std::array<uint16_t, 6> stats(const TrainerViewScreen&, const EditorSurface::EditorState&, const Record& p) {
        return p.stats;
    }
    static void setIv(TrainerViewScreen& screen, EditorSurface::EditorState&, size_t stat, uint8_t value) {
        (void)stage(screen, MField::IV, stat, value);
    }
    static void setEv(TrainerViewScreen& screen, EditorSurface::EditorState&, size_t stat, uint8_t value) {
        (void)stage(screen, MField::EV, stat, value);
    }
    static bool pidRowEditable(const TrainerViewScreen& screen, const EditorSurface::EditorState& state, int row, const Record&) {
        return row == Workspace::kNatureRow && editable(screen, state) && fieldEditable(screen, MField::Nature);
    }
    static bool pidRowLocked(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int row, const Record&) {
        return !(row == Workspace::kNatureRow && fieldEditable(screen, MField::Nature));
    }
    static uint8_t gender(const Record& p) { return p.gender; }
    static std::string pidRowValue(const TrainerViewScreen&, const Record& p, int row) {
        switch (row) {
            case Workspace::kNatureRow: return Trainer::getNatureName(p.nature);
            case Workspace::kGenderRow: return EditorSurface::genderText(p.gender);
            case Workspace::kShinyRow: return p.shiny ? "Yes" : "No";
            case Workspace::kAbilityRow:
                return std::string(Trainer::getAbilityName(p.ability)) + " (slot " + std::to_string(p.abilityNumber) + ")";
            default: return std::to_string(p.pid);
        }
    }
    static bool editPidRow(TrainerViewScreen& screen, EditorSurface::EditorState& state, int row, const Record& p) {
        if (row == Workspace::kNatureRow) {
            if (!fieldEditable(screen, MField::Nature)) { readOnly(screen, "Nature"); return true; }
            std::vector<uint16_t> natures;
            for (uint32_t i = 0; i <= maximumOr(screen, MField::Nature, 24); ++i) natures.push_back(static_cast<uint16_t>(i));
            EditorSurface::openPicker(screen, state, PickerTarget::Nature, std::move(natures), p.nature);
            return true;
        }
        if (row == Workspace::kGenderRow || row == Workspace::kShinyRow || row == Workspace::kAbilityRow) {
            screen.postStatus(std::string(row == Workspace::kGenderRow ? "Gender" : row == Workspace::kShinyRow ? "Shiny" : "Ability") +
                              " is PID-derived; no verified " + generationLabel(screen) + " writer", 300);
            return true;
        }
        return false;
    }

    static std::array<uint16_t, 4> moves(const Record& p) { return p.moves; }
    static std::array<uint8_t, 4> pp(const Record& p) { return p.pp; }
    static std::array<uint8_t, 4> ppUps(const Record& p) { return p.ppUps; }
    static uint16_t workingMove(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = current(screen);
        return p && slot >= 0 && slot < 4 ? p->moves[static_cast<size_t>(slot)] : 0;
    }
    static uint8_t workingPP(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = current(screen);
        return p && slot >= 0 && slot < 4 ? p->pp[static_cast<size_t>(slot)] : 0;
    }
    static uint8_t workingPPUps(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = current(screen);
        return p && slot >= 0 && slot < 4 ? p->ppUps[static_cast<size_t>(slot)] : 0;
    }
    static void setMove(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint16_t move) {
        (void)stage(screen, MField::Move, static_cast<size_t>(slot), move);
    }
    static void setPP(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint8_t value) {
        (void)stage(screen, MField::MovePP, static_cast<size_t>(slot), value);
    }
    static void setPPUps(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint8_t value) {
        (void)stage(screen, MField::MovePPUps, static_cast<size_t>(slot), value);
    }
    // The adapter's PP table, or the stored PP itself when it has none (0 = unknown): the panel
    // then reads "n/n" and never invents a maximum.
    static int maxPP(const TrainerViewScreen& screen, uint16_t move, uint8_t ppUps) {
        const int known = session(screen).maxPP(move, ppUps);
        if (known > 0) return known;
        const auto p = current(screen);
        if (!p) return 0;
        for (size_t i = 0; i < 4; ++i)
            if (p->moves[i] == move) return p->pp[i];
        return 0;
    }
    static std::vector<uint16_t> moveChoices(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        const auto p = current(screen);
        std::vector<uint16_t> choices{0};
        if (p)
            for (const auto move : p->moves)
                if (move && std::find(choices.begin(), choices.end(), move) == choices.end()) choices.push_back(move);
        return choices;
    }
    static void moveStatus(TrainerViewScreen&, const EditorSurface::EditorState&, const Record&, uint16_t move,
                           const char*& status, Color& color) {
        // No learnset is pinned for these titles: the stored move is shown, never judged.
        status = move == 0 ? "-" : Presentation::legalityLabel(Presentation::LegalityState::NotChecked);
        color = Colors::TextDim;
    }
    static Enums::GameVersion moveGame(const TrainerViewScreen& screen) { return screen.trainer.getGameGroup(); }
    static std::string pickerLabel(TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                   PickerTarget target, uint16_t value) {
        switch (target) {
            case PickerTarget::Species: return EditorSurface::dexLabel(value);
            case PickerTarget::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
            case PickerTarget::HeldItem: return value == 0 ? std::string("(none)") : bagItemLabel(screen, value);
            case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
            case PickerTarget::Pokerus: return pokerusText(static_cast<uint8_t>(value));
            case PickerTarget::Form: return value == 0 ? "Default" : "Form " + std::to_string(value);
            case PickerTarget::Move: return MoveUI::numberedName(value);
            case PickerTarget::Nature: return Trainer::getNatureName(static_cast<uint8_t>(value));
            default: return std::to_string(value);
        }
    }
    static bool applyPicker(TrainerViewScreen& screen, EditorSurface::EditorState& state, PickerTarget target, uint16_t value) {
        switch (target) {
            case PickerTarget::Species: return stage(screen, MField::Species, 0, value);
            case PickerTarget::HeldItem: return stage(screen, MField::HeldItem, 0, value);
            case PickerTarget::Language: return stage(screen, MField::Language, 0, value);
            case PickerTarget::Ball: return stage(screen, MField::Ball, 0, value);
            case PickerTarget::Nature: return stage(screen, MField::Nature, 0, value);
            case PickerTarget::Move: return stage(screen, MField::Move, static_cast<size_t>(state.moveEditorSlot), value);
            default: return false;
        }
    }
    static std::array<uint8_t, 2> speciesPreviewTypes(const TrainerViewScreen& screen, uint16_t species) {
        return classicTypes(species, 0, screen.trainer.getGameGroup());
    }
    static uint8_t nature(const Record& p) { return p.nature; }
    static std::string speciesLabel(uint16_t value) { return EditorSurface::dexLabel(value); }
    static std::string heldItemArtKey(TrainerViewScreen&, uint16_t value) { return Names::getItemName(value); }

    static EditorSurface::ChromeInfo chrome(const TrainerViewScreen& screen, const EditorSurface::EditorState&, const Record& p) {
        EditorSurface::ChromeInfo info;
        info.species = p.species;
        info.nickname = p.nickname;
        info.level = p.level;
        info.gender = p.gender;
        info.shiny = p.shiny;
        info.mode = backend(screen).surface() == Surface::View ? EditorSurface::Context::SurfaceMode::ViewReadOnly
                                                               : EditorSurface::Context::SurfaceMode::EditStaged;
        info.context = ScreenState::slotLabel(backend(screen).target());
        return info;
    }
    static std::array<uint8_t, 2> portraitTypes(const TrainerViewScreen&, const Record& p) {
        if (p.types[0] == 0xFF) return {0xFF, 0xFF};
        return {SharedSpeciesPicker::canonicalToClassicType(p.types[0]),
                p.types[1] == 0xFF ? uint8_t{0xFF} : SharedSpeciesPicker::canonicalToClassicType(p.types[1])};
    }
    // Short rows only, so nothing runs into the stats graph beside them. Legality is always the
    // shared "Incomplete": no legality analyzer exists for these titles and none is ever run.
    static std::vector<EditorSurface::DataRow> dataRows(TrainerViewScreen& screen, const EditorSurface::EditorState&, const Record& p) {
        std::vector<EditorSurface::DataRow> rows{
            {"Game", gameTitle(screen)},
            {"Origin", shortValue(Enums::getOriginGameName(p.originGame))},
            {"Language", Enums::getLanguageName(p.language)},
            {"Ball", shortValue(Enums::getBallName(p.ball))},
            {"Met", shortValue(metLocationText(p))},
            {"Met Lv. / date", std::to_string(p.metLevel) + " / " + dateText(p.metDate)},
            {"Friendship", std::to_string(p.friendship)},
            {"Pokérus", pokerusText(p.pokerus)},
            {"Fateful", p.fatefulEncounter ? "Yes" : "No"},
            {"Egg status", p.egg ? "Unhatched" : "Not egg"},
            {"OT gender", p.otGender == 0 ? "Male" : "Female"},
        };
        for (const auto& fact : p.nativeFacts) rows.push_back({shortValue(fact.first, 16), shortValue(fact.second)});
        rows.push_back({"Legality", Presentation::legalityLabel(Presentation::LegalityState::Incomplete)});
        return rows;
    }
};

bool handleSharedWorkspace(TrainerViewScreen& screen, u64 keys, const TouchInput& touch) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    if (!st.workspaceOpen) {
        EditorSurface::resetWorkspaceFocus(st);
        st.workspaceOpen = true;
    }
    const bool editable = ModernEditor::editable(screen, st);
    if (editable && !EditorSurface::editorModalOpen(st) && st.screen.surface() == Surface::Edit) {
        // One Review key per screen (ReviewEntry.h): + in Edit. The shared shell asks ReviewEntry
        // with kGeneration; that rule stops at Gen V, so the Modern surface routes + itself.
        if (keys & HidNpadButton_Plus) {
            ModernEditor::openReview(screen, st);
            return true;
        }
        // Moves the adapter marks read-only never open the contextual move editor: A or a completed
        // tap on a move row explains instead (owner Oct 25 L06: no dead taps).
        if (s.fieldState(MField::Move, st.screen.target().region) != MFieldState::Editable) {
            const int tap = screen.touchedButtonId(touch);
            const bool tappedMove = tap >= Workspace::kMoveTouchBase && tap < Workspace::kMoveTouchBase + Workspace::kMoveRows;
            if (tappedMove) st.focus = {PokeBank::UIModel::SharedPokemonEditor::Panel::Moves,
                                        static_cast<uint8_t>(tap - Workspace::kMoveTouchBase), 0};
            if (tappedMove || ((keys & HidNpadButton_A) && st.focus.panel == PokeBank::UIModel::SharedPokemonEditor::Panel::Moves)) {
                screen.postStatus("Moves are read-only for " + generationLabel(screen) + " records; no verified learnset or PP writer", 300);
                return true;
            }
        }
    }
    EditorSurface::handleWorkspace<ModernEditor>(screen, st, keys, touch);
    const bool reviewOverWorkspace = st.screen.surface() == Surface::Review && st.screen.reviewReturn() == Surface::Edit;
    if (!st.screen.workspaceOpen() && !reviewOverWorkspace) {
        EditorSurface::resetWorkspaceFocus(st);
        st.workspaceOpen = false;
    }
    return true;
}
void drawSharedWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    if (!st.workspaceOpen) {
        EditorSurface::resetWorkspaceFocus(st);
        st.workspaceOpen = true;
    }
    EditorSurface::drawWorkspace<ModernEditor>(screen, fb, st);
}

// ---- Items (Bag): the adapter's pockets, staged through the adapter only ----
int bagItemsPerPage() noexcept { return std::max(1, (CONTENT_PANEL_HEIGHT - 106) / 52); }
std::vector<Modern::BagPocketView> bagPockets(const TrainerViewScreen& screen) {
    return sessionOf(screen).bagAvailable() ? sessionOf(screen).bag() : std::vector<Modern::BagPocketView>{};
}
void clampBagCursor(ModernEditorState& st, const std::vector<Modern::BagPocketView>& pockets) {
    if (pockets.empty()) { st.pocket = 0; st.row = 0; return; }
    if (st.pocket >= pockets.size()) st.pocket = 0;
    const size_t count = pockets[st.pocket].items.size();
    if (count == 0) st.row = 0;
    else if (st.row >= count) st.row = count - 1;
}
void syncBagCursor(TrainerViewScreen& screen) {
    auto& st = stateFor(screen);
    const auto pockets = bagPockets(screen);
    if (!st.bagEntered) {
        st.pocket = static_cast<size_t>(std::max(0, screen.selectedCategory));
        st.row = static_cast<size_t>(std::max(0, screen.selectedItemIndex));
        st.bagEntered = true;
    }
    clampBagCursor(st, pockets);
    screen.selectedCategory = static_cast<int>(st.pocket);
    screen.selectedItemIndex = static_cast<int>(st.row);
    screen.currentPage = static_cast<int>(st.row) / bagItemsPerPage();
}
// The shared Gen I-V quantity rule for one native stack: the adapter's pocket cap, editable
// pockets only (SharedBagAddFlow decides what A, X and Y mean from it).
PokeVault::Inventory::QuantityPolicy::ItemQuantityRule bagRule(const Modern::BagPocketView& pocket, uint16_t id) noexcept {
    PokeVault::Inventory::QuantityPolicy::ItemQuantityRule rule;
    rule.valid = id != 0 && pocket.editable && pocket.maxQuantity > 0;
    rule.editable = rule.valid && pocket.maxQuantity > 1;
    rule.minimum = rule.valid ? 1 : 0;
    rule.maximum = pocket.maxQuantity;
    rule.kind = rule.valid ? PokeVault::Inventory::QuantityPolicy::ItemKind::Regular
                           : PokeVault::Inventory::QuantityPolicy::ItemKind::Invalid;
    rule.reason = pocket.editable ? "adapter pocket cap" : "read-only pocket";
    return rule;
}
std::vector<ControllerHint> bagFooter(const TrainerViewScreen& screen, const ModernEditorState& st,
                                      const std::vector<Modern::BagPocketView>& pockets) {
    if (st.bagSurface == BagSurface::AddPicker) return {{"D-pad/Stick", "Choose"}, {"L/R", "Page"}, {"A", "Add"}, {"B", "Cancel"}};
    if (st.bagSurface == BagSurface::ConfirmRemove) return {{"A", "Confirm Remove"}, {"B", "Cancel"}};
    std::vector<ControllerHint> out{{"L/R", "Pocket"}};
    const bool editable = st.pocket < pockets.size() && pockets[st.pocket].editable;
    const bool hasRow = st.pocket < pockets.size() && !pockets[st.pocket].items.empty();
    if (editable && hasRow) out.push_back({"A", "Quantity"});
    if (editable && !sessionOf(screen).bagAddChoices(st.pocket).empty()) out.push_back({"X", "Add Item"});
    if (editable && hasRow) out.push_back({"Y", "Remove"});
    if (sessionOf(screen).hasChanges()) out.push_back({"+", "Review"});
    out.push_back({"B", "Back"});
    return out;
}
bool handleBag(TrainerViewScreen& screen, u64 keys, int tap, const TouchInput& touch) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    syncBagCursor(screen);
    auto pockets = bagPockets(screen);
    const auto refresh = [&](std::string& why) { return refreshPresentation(screen, why); };
    const auto label = generationLabel(screen);
    std::string error;
    const auto focusItem = [&](uint16_t id) {
        pockets = bagPockets(screen);
        if (st.pocket >= pockets.size()) return;
        for (size_t i = 0; i < pockets[st.pocket].items.size(); ++i)
            if (pockets[st.pocket].items[i].itemId == id) { st.row = i; break; }
    };
    switch (st.bagSurface) {
        case BagSurface::AddPicker: {
            const int count = static_cast<int>(st.addChoices.size());
            namespace AddGrid = UI::SharedItemGridPicker;
            const auto grid = UI::SharedBagAddPicker::pickerGrid(count, [&](int index) {
                return bagItemLabel(screen, st.addChoices[static_cast<size_t>(index)]);
            });
            const int selected = static_cast<int>(st.addIndex);
            const auto bar = SharedScrollbar::handleTouch(st.bagAddScrollbar, SharedScrollbar::sample(touch),
                AddGrid::scrollModel(grid, AddGrid::firstRowFor(grid, selected)));
            if (bar.changed && count > 0)
                st.addIndex = static_cast<size_t>(std::clamp(AddGrid::indexForFirstRow(grid, selected, bar.scroll), 0, count - 1));
            if (!bar.consumed && tap >= kBagAddTouchBase && tap < kBagAddTouchBase + count) {
                st.addIndex = static_cast<size_t>(tap - kBagAddTouchBase);
                keys |= HidNpadButton_A;
            }
            if (count > 0) {
                const int to = AddGrid::navigate(grid, static_cast<int>(st.addIndex), AddGrid::Keys{
                    (keys & HidNpadButton_Up) != 0, (keys & HidNpadButton_Down) != 0,
                    (keys & HidNpadButton_Left) != 0, (keys & HidNpadButton_Right) != 0,
                    (keys & HidNpadButton_L) != 0, (keys & HidNpadButton_R) != 0,
                    (keys & HidNpadButton_ZL) != 0, (keys & HidNpadButton_ZR) != 0});
                st.addIndex = static_cast<size_t>(std::clamp(to, 0, count - 1));
            }
            if (keys & HidNpadButton_B) {
                st.bagSurface = BagSurface::Browse;
                st.addChoices.clear();
            } else if ((keys & HidNpadButton_A) && count > 0 && st.pocket < pockets.size()) {
                // Shared Gen I-V Add (UI/SharedBagAddFlow.h): stage x1 with no keypad, or focus the
                // existing stack; A on the row edits the quantity afterwards.
                const uint16_t id = st.addChoices[st.addIndex];
                const auto& pocket = pockets[st.pocket];
                PokeBank::UIModel::BagAdd::PocketState state;
                for (const auto& item : pocket.items) if (item.itemId == id) state.heldInPocket = item.count;
                state.usedSlots = pocket.items.size();
                const auto decision = PokeBank::UIModel::BagAdd::decidePick(bagRule(pocket, id), state, pocket.name);
                using Outcome = PokeBank::UIModel::BagAdd::PickOutcome;
                if (decision.outcome == Outcome::StageNew) {
                    if (!s.bagAdd(st.pocket, id, decision.quantity, error)) {
                        screen.postStatus(error.empty() ? label + " bag refused that item" : error, 320);
                    } else if (!refresh(error)) {
                        s.discardBag();
                        screen.postStatus(label + " bag presentation rejected; nothing staged: " + error, 360);
                        st.bagSurface = BagSurface::Browse;
                    } else {
                        st.bagSurface = BagSurface::Browse;
                        focusItem(id);
                        screen.postStatus(decision.message + "; the source save is unchanged", 300);
                    }
                } else if (decision.outcome == Outcome::FocusExisting) {
                    st.bagSurface = BagSurface::Browse;
                    focusItem(id);
                    screen.postStatus(decision.message, 260);
                } else {
                    screen.postStatus(decision.message, 320);
                    if (decision.outcome == Outcome::PocketFull) st.bagSurface = BagSurface::Browse;
                }
                screen.controllerNavigation.reset();
            }
            syncBagCursor(screen);
            return true;
        }
        case BagSurface::ConfirmRemove:
            // Y opened the explicit Bag Remove question; A confirms it, B always cancels (the
            // owner-directed Round 4 exception for Bag Remove).
            if (keys & HidNpadButton_B) {
                st.bagSurface = BagSurface::Browse;
            } else if (keys & HidNpadButton_A) {
                st.bagSurface = BagSurface::Browse;
                if (!s.bagRemove(st.pocket, st.row, error)) {
                    screen.postStatus(error.empty() ? label + " bag refused the removal" : error, 320);
                } else if (!refresh(error)) {
                    s.discardBag();
                    screen.postStatus(label + " bag presentation rejected; nothing staged: " + error, 360);
                } else {
                    screen.postStatus(label + " item removed from the staged bag ONLY; the source save is unchanged", 300);
                }
                screen.controllerNavigation.reset();
            }
            syncBagCursor(screen);
            return true;
        case BagSurface::Browse:
            break;
    }
    // Pocket arrows and visible-row ids published by the last frame.
    using CategoryTouch = PokeBank::UIModel::InventoryCategoryTouch;
    const size_t rows = st.pocket < pockets.size() ? pockets[st.pocket].items.size() : 0;
    if (tap == CategoryTouch::PreviousId) keys |= HidNpadButton_L;
    else if (tap == CategoryTouch::NextId) keys |= HidNpadButton_R;
    else if (tap >= 0 && static_cast<size_t>(tap) < rows) {
        if (static_cast<size_t>(tap) == st.row) keys |= HidNpadButton_A;
        else st.row = static_cast<size_t>(tap);
    }
    if (keys & HidNpadButton_B) {
        if (screen.itemsShortcutActive) {
            // Opened from Product Home Items: B leaves the session through the same staged-changes
            // confirmation as the Browse exit.
            if (st.screen.requestExit(s)) screen.goBack = true;
        } else {
            st.bagEntered = false;
            screen.detailViewActive = false;   // back to the hub with Items focused
        }
        screen.controllerNavigation.reset();
        return true;
    }
    if (!pockets.empty()) {
        if (keys & HidNpadButton_L) { st.pocket = (st.pocket + pockets.size() - 1) % pockets.size(); st.row = 0; }
        if (keys & HidNpadButton_R) { st.pocket = (st.pocket + 1) % pockets.size(); st.row = 0; }
    }
    const size_t count = st.pocket < pockets.size() ? pockets[st.pocket].items.size() : 0;
    const auto perPage = static_cast<size_t>(bagItemsPerPage());
    if (count > 0) {
        if (touch.justReleased() && touch.dragged()) {
            const int dy = touch.deltaY();
            const bool startedOnItem = std::any_of(screen.touchButtons.begin(), screen.touchButtons.end(), [&](const auto& hit) {
                return hit.id >= 0 && hit.id < 1000 && touch.startX() >= hit.x && touch.startX() < hit.x + hit.w &&
                       touch.startY() >= hit.y && touch.startY() < hit.y + hit.h;
            });
            if (startedOnItem && std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())) {
                const int steps = std::max(1, std::abs(dy) / 52);
                const int next = std::clamp(static_cast<int>(st.row) + (dy < 0 ? steps : -steps), 0, static_cast<int>(count) - 1);
                st.row = static_cast<size_t>(next);
            }
        }
        if (keys & HidNpadButton_Up) st.row = (st.row + count - 1) % count;
        if (keys & HidNpadButton_Down) st.row = (st.row + 1) % count;
        if (keys & HidNpadButton_Left) st.row = st.row >= perPage ? st.row - perPage : 0;
        if (keys & HidNpadButton_Right) st.row = std::min(count - 1, st.row + perPage);
    }
    const bool editable = st.pocket < pockets.size() && pockets[st.pocket].editable;
    const std::string pocketReason = st.pocket < pockets.size() ? pockets[st.pocket].reason : std::string("Bag unavailable");
    if (keys & HidNpadButton_Plus) {
        if (!st.screen.openReviewFromBrowse(s)) screen.postStatus("No staged " + label + " changes to review", 220);
        screen.controllerNavigation.reset();
    } else if (keys & HidNpadButton_X) {
        if (!editable) screen.postStatus(pocketReason, 300);
        else if (auto choices = s.bagAddChoices(st.pocket); choices.empty()) screen.postStatus("No addable items for this pocket", 300);
        else {
            st.addChoices = std::move(choices);
            st.addIndex = 0;
            st.bagAddScrollbar = {};
            st.bagSurface = BagSurface::AddPicker;
        }
        screen.controllerNavigation.reset();
    } else if (keys & HidNpadButton_Y) {
        if (!editable) screen.postStatus(pocketReason, 300);
        else if (count == 0) screen.postStatus("This pocket has no item to remove", 260);
        else st.bagSurface = BagSurface::ConfirmRemove;
        screen.controllerNavigation.reset();
    } else if (keys & HidNpadButton_A) {
        if (!editable) screen.postStatus(pocketReason, 300);
        else if (count == 0) screen.postStatus("This pocket is empty; X adds an item", 260);
        else {
            const auto& pocket = pockets[st.pocket];
            const auto& item = pocket.items[st.row];
            const auto row = PokeBank::UIModel::BagAdd::decideRow(bagRule(pocket, item.itemId), item.count, pocket.maxQuantity);
            if (row.outcome != PokeBank::UIModel::BagAdd::RowOutcome::EditQuantity) {
                screen.postStatus(row.message, 260);
            } else {
                const auto r = Utils::promptNumber("Edit " + bagItemLabel(screen, item.itemId), static_cast<int>(row.current),
                                                   static_cast<int>(row.minimum), static_cast<int>(row.maximum));
                screen.controllerNavigation.reset();
                if (r.accepted && r.value != static_cast<int>(item.count)) {   // Cancel cannot mutate the stage.
                    if (!s.bagSetQuantity(st.pocket, st.row, static_cast<uint16_t>(r.value), error)) {
                        screen.postStatus(error.empty() ? label + " bag refused that quantity" : error, 320);
                    } else if (!refresh(error)) {
                        s.discardBag();
                        screen.postStatus(label + " bag presentation rejected; nothing staged: " + error, 360);
                    } else {
                        screen.postStatus(label + " item quantity staged; the source save is unchanged", 280);
                    }
                }
            }
        }
    }
    syncBagCursor(screen);
    return true;
}
void drawBagPanel(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    const auto pockets = bagPockets(screen);
    const int x = LEFT_PANEL_X, y = CONTENT_PANEL_Y;
    const int width = fb.getWidth() - x, height = CONTENT_PANEL_HEIGHT;
    fb.drawFilledRoundedRect(x, y, width, height, 16, Colors::Panel);
    fb.drawRoundedRect(x, y, width, height, 16, Colors::Border, 1);
    constexpr int hH = 46;
    fb.drawFilledRoundedRect(x, y, width, hH, 16, Colors::AccentDim);
    fb.drawFilledRect(x, y + hH - 16, width, 16, Colors::AccentDim);
    const std::string pocketName = st.pocket < pockets.size() ? pockets[st.pocket].name : std::string("Bag");
    fb.drawText(x + 22, y + (hH - fb.lineHeight(TextStyle::Heading)) / 2, "Items - " + pocketName, Colors::Text, TextStyle::Heading);
    screen.touchButtons.clear();
    using CategoryTouch = PokeBank::UIModel::InventoryCategoryTouch;
    const int arrowY = CategoryTouch::topY(y, hH);
    const int prevX = CategoryTouch::previousX(x, width), nextX = CategoryTouch::nextX(x, width);
    fb.drawFilledRoundedRect(prevX, arrowY, CategoryTouch::Width, CategoryTouch::Height, 9, Colors::PanelAlt);
    fb.drawFilledRoundedRect(nextX, arrowY, CategoryTouch::Width, CategoryTouch::Height, 9, Colors::PanelAlt);
    fb.drawText(prevX + 14, arrowY + 3, "<", Colors::Text, TextStyle::Body);
    fb.drawText(nextX + 14, arrowY + 3, ">", Colors::Text, TextStyle::Body);
    screen.touchButtons.push_back({CategoryTouch::PreviousId, prevX, arrowY, CategoryTouch::Width, CategoryTouch::Height});
    screen.touchButtons.push_back({CategoryTouch::NextId, nextX, arrowY, CategoryTouch::Width, CategoryTouch::Height});
    if (pockets.empty() || st.pocket >= pockets.size()) {
        fb.drawText(x + 24, y + hH + 30, "Bag unavailable", Colors::Text, TextStyle::Body);
        fb.drawText(x + 24, y + hH + 58, "Native " + generationLabel(screen) + " bag data failed validation.", Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 24, y + hH + 82, "Trainer, Party and Boxes remain available; no item is guessed.", Colors::TextDim, TextStyle::Caption);
        return;
    }
    const auto& pocket = pockets[st.pocket];
    const int total = static_cast<int>(pocket.items.size());
    constexpr int rowPitch = 52, tileH = 46;
    const int itemsPerPage = bagItemsPerPage();
    const int totalPages = std::max(1, (total + itemsPerPage - 1) / itemsPerPage);
    const int tileW = std::min(width - 60, 860);
    const int tileX = x + (width - tileW) / 2;
    const int page = static_cast<int>(st.row) / itemsPerPage;
    std::string countText = std::to_string(total) + " items";
    if (totalPages > 1) countText += "      Page " + std::to_string(page + 1) + " / " + std::to_string(totalPages);
    fb.drawText(tileX, y + hH + 10, countText, Colors::TextDim, TextStyle::Caption);
    if (total == 0) fb.drawText(x + 24, y + hH + 40, "No items in this pocket", Colors::TextDim);
    const int startIdx = page * itemsPerPage;
    const int endIdx = std::min(startIdx + itemsPerPage, total);
    int ry = y + hH + 40;
    for (int i = startIdx; i < endIdx; ++i) {
        const auto& item = pocket.items[static_cast<size_t>(i)];
        const bool selected = static_cast<size_t>(i) == st.row;
        fb.drawSoftShadow(tileX, ry, tileW, tileH, tileH / 2);
        fb.drawFilledRoundedRect(tileX, ry, tileW, tileH, 12, Colors::PanelAlt);
        if (selected) fb.drawRoundedRect(tileX, ry, tileW, tileH, 12, Colors::FocusBorder, 2);
        const std::string name = bagItemLabel(screen, item.itemId);
        ItemPickerArtwork::drawNativeItem(fb, tileX + 22 + 38, ry + (tileH - 38) / 2, 38, screen.trainer.getGameGroup(), item.itemId, name);
        fb.drawText(tileX + 70, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2, name, Colors::Text, TextStyle::Body);
        const std::string cnt = "\xC3\x97" + std::to_string(item.count);
        int cw = 0, ch = 0;
        fb.measureText(cnt, cw, ch, TextStyle::Body);
        fb.drawText(tileX + tileW - 24 - cw, ry + (tileH - ch) / 2, cnt, selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
        screen.touchButtons.push_back({i, tileX, ry, tileW, tileH});
        ry += rowPitch;
    }
    // The pocket's own state line: a read-only pocket says why; an editable one says where edits go.
    const std::string note = pocket.editable
        ? "Bag edits are staged in app memory; the source " + generationLabel(screen) + " save is unchanged"
        : pocket.reason;
    fb.drawText(x + 24, y + height - 30, SharedMovesPanel::fit(fb, note, width - 48, TextStyle::Caption), Colors::Accent, TextStyle::Caption);
}
void drawBag(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    syncBagCursor(screen);
    const auto pockets = bagPockets(screen);
    screen.drawGSCOverlay(fb);   // the hub chrome; the pocket panel below is the adapter's
    drawBagPanel(screen, fb);
    const auto hints = bagFooter(screen, st, pockets);
    if (st.bagSurface == BagSurface::Browse) {
        drawNavBar(fb, hints);
        return;
    }
    const auto game = screen.trainer.getGameGroup();
    if (st.bagSurface == BagSurface::AddPicker) {
        const int count = static_cast<int>(st.addChoices.size());
        const auto& pocket = pockets[std::min(st.pocket, pockets.empty() ? size_t{0} : pockets.size() - 1)];
        UI::SharedBagAddPicker::drawPicker(fb, screen.touchButtons, kBagAddTouchBase, game, "Add Item",
            pocket.name + "  |  " + generationLabel(screen) + " native items only", count, static_cast<int>(st.addIndex),
            [&](int index) { return bagItemLabel(screen, st.addChoices[static_cast<size_t>(index)]); },
            [&](int index) { return st.addChoices[static_cast<size_t>(index)]; },
            [&](int index) {
                const auto id = st.addChoices[static_cast<size_t>(index)];
                for (const auto& stack : pocket.items) if (stack.itemId == id) return stack.count;
                return uint16_t{0};
            },
            [&](int index) { return Names::getMachineDescriptor(game, st.addChoices[static_cast<size_t>(index)]).moveId; },
            st.bagAddScrollbar);
        drawNavBar(fb, hints);
        return;
    }
    const bool hasRow = st.pocket < pockets.size() && st.row < pockets[st.pocket].items.size();
    const auto item = hasRow ? pockets[st.pocket].items[st.row] : Trainer::InventoryItem{};
    UI::SharedBagAddPicker::drawRemoveConfirm(fb, screen.touchButtons, game, "Remove " + generationLabel(screen) + " Item?",
        item.itemId, hasRow ? bagItemLabel(screen, item.itemId) : std::string(), item.count,
        "Staged bag only; Discard All in Review restores it.");
    // The modal owns its A/B choices; they are not repeated in a footer.
}

// ---- Trainer page: every adapter fact read-only, plus the Pokedex summary ----
struct TrainerRows {
    std::vector<Modern::TrainerFact> facts;   // shown rows (trailing facts dropped only if they would not fit)
    bool dexRow = true;
    [[nodiscard]] size_t count() const noexcept { return facts.size() + (dexRow ? 1 : 0); }
    [[nodiscard]] bool isDex(size_t row) const noexcept { return dexRow && row == facts.size(); }
};
TrainerRows trainerRows(const TrainerViewScreen& screen, PokeBank::UIModel::ClassicTrainerPage::Page& page) {
    namespace TPage = PokeBank::UIModel::ClassicTrainerPage;
    TrainerRows rows;
    rows.facts = sessionOf(screen).trainerFacts();
    const int x = LEFT_PANEL_X, y = CONTENT_PANEL_Y;
    for (;;) {
        std::vector<TPage::Row> model;
        for (size_t i = 0; i < rows.facts.size(); ++i) model.push_back({TPage::Field::Name, false, -1});
        model.push_back({TPage::Field::Pokedex, false, -1});
        page = TPage::layout(model, x, y, 1280 - x, CONTENT_PANEL_HEIGHT, false);
        if (page.fits || rows.facts.empty()) break;
        rows.facts.pop_back();   // never overlap the Review slot; the remaining facts stay exact
    }
    return rows;
}
std::vector<ControllerHint> trainerFooter(const ModernEditorState& st, const TrainerRows& rows) {
    if (st.dexOpen) return {{"B", "Back"}};
    if (rows.isDex(st.trainerRow)) return {{"D-pad/Stick", "Choose"}, {"A", "Open Pokedex"}, {"Y", "Review"}, {"B", "Back"}};
    return {{"D-pad/Stick", "Choose"}, {"Y", "Review"}, {"B", "Back"}};
}
bool handleTrainerPage(TrainerViewScreen& screen, u64 keys, int tap) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    PokeBank::UIModel::ClassicTrainerPage::Page page;
    const auto rows = trainerRows(screen, page);
    if (st.dexOpen) {
        // Read-only Pokedex summary: browsing never stages or writes anything.
        if (keys & HidNpadButton_B) {
            st.dexOpen = false;
            screen.controllerNavigation.reset();
        }
        return true;
    }
    if (rows.count() > 0 && st.trainerRow >= rows.count()) st.trainerRow = rows.count() - 1;
    if (tap >= kTrainerRowTouchBase && tap < kTrainerRowTouchBase + static_cast<int>(rows.count())) {
        const auto tapped = static_cast<size_t>(tap - kTrainerRowTouchBase);
        if (tapped == st.trainerRow) keys |= HidNpadButton_A;
        st.trainerRow = tapped;
    }
    if (keys & HidNpadButton_B) {
        st.trainerOpen = false;   // the focused row is remembered for the next visit
        screen.detailViewActive = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (rows.count() > 0) {
        if (keys & HidNpadButton_Up) st.trainerRow = (st.trainerRow + rows.count() - 1) % rows.count();
        if (keys & HidNpadButton_Down) st.trainerRow = (st.trainerRow + 1) % rows.count();
    }
    // Y (or the Review Changes button): the Trainer page's one Review key (ReviewEntry.h).
    if ((keys & HidNpadButton_Y) || tap == PokeBank::UIModel::ClassicTrainerSession::kReviewTouchId) {
        if (!st.screen.openReviewFromBrowse(s)) screen.postStatus("No staged " + generationLabel(screen) + " changes to review", 220);
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(keys & HidNpadButton_A)) return true;
    if (rows.isDex(st.trainerRow)) {
        if (!s.pokedex().available()) screen.postStatus(generationLabel(screen) + " Pokedex counts are unavailable for this save", 280);
        else st.dexOpen = true;
    } else {
        const auto& status = s.capabilities().status(Capability::Workflow::Trainer);
        screen.postStatus(status.reason.empty() ? "Trainer fields are read-only for " + generationLabel(screen)
                                                : std::string(status.reason), 300);
    }
    screen.controllerNavigation.reset();
    return true;
}
void drawTrainerPage(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    PokeBank::UIModel::ClassicTrainerPage::Page page;
    const auto rows = trainerRows(screen, page);
    const std::string label = generationLabel(screen);
    if (st.dexOpen) {
        screen.drawGSCOverlay(fb);
        fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 150));
        SharedEditorChrome::beginModal(screen.touchButtons);
        constexpr int w = 760, h = 260;
        const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - kNavBarH - h) / 2;
        fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
        fb.drawRoundedRect(x, y, w, h, 18, Colors::Border, 1);
        const auto dex = s.pokedex();
        fb.drawText(x + 24, y + 18, "Pokedex - " + label + " (read-only)", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 24, y + 62, "Seen " + std::to_string(dex.seen) + " / Caught " + std::to_string(dex.caught) + " of " +
                    std::to_string(dex.total) + "  |  native flags, never edited", Colors::Accent, TextStyle::Caption);
        const auto& status = s.capabilities().status(Capability::Workflow::Pokedex);
        fb.drawText(x + 24, y + 100, SharedMovesPanel::fit(fb, std::string(status.reason), w - 48, TextStyle::Caption),
                    Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 24, y + 128, "Per-species entries are not exposed by this adapter yet (follow-up).",
                    Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 24, y + h - 40, "Counts only; nothing here is a legality or completion claim.", Colors::TextDim, TextStyle::Caption);
        drawNavBar(fb, trainerFooter(st, rows));
        return;
    }
    screen.drawGSCOverlay(fb);
    namespace TPanel = UI::SharedTrainerPanel;
    const auto& trainerStatus = s.capabilities().status(Capability::Workflow::Trainer);
    TPanel::drawPanel(fb, page.panel.x, page.panel.y, page.panel.w, page.panel.h,
        trainerStatus.reason.empty() ? label + " trainer facts, read-only" : std::string(trainerStatus.reason));
    {
        const int cardX = page.rows.empty() ? page.panel.x : page.rows.front().card.x;
        TPanel::drawDivider(fb, cardX, page.dividerY, PokeBank::UIModel::ClassicTrainerPage::kCardW);
    }
    for (size_t i = 0; i < page.rows.size() && i < rows.count(); ++i) {
        const auto& c = page.rows[i].card;
        const std::string rowLabel = rows.isDex(i) ? std::string("Pokédex") : rows.facts[i].label;
        const std::string value = rows.isDex(i)
            ? (s.pokedex().available() ? "Seen " + std::to_string(s.pokedex().seen) + " / Caught " + std::to_string(s.pokedex().caught) +
                                             " of " + std::to_string(s.pokedex().total)
                                       : std::string("Unavailable"))
            : rows.facts[i].value;
        TPanel::drawInfoRow(fb, c.x, c.y, c.w, c.h, rowLabel, value);
        screen.touchButtons.push_back({kTrainerRowTouchBase + static_cast<int>(i), c.x, c.y, c.w, c.h});
        if (st.trainerOpen && i == st.trainerRow) fb.drawRoundedRect(c.x, c.y, c.w, c.h, 10, Colors::Accent, 2);
    }
    drawGlyphButton(fb, page.review.x, page.review.y, page.review.w, page.review.h, "Y", "Review Changes");
    screen.touchButtons.push_back({PokeBank::UIModel::ClassicTrainerSession::kReviewTouchId,
                                   page.review.x, page.review.y, page.review.w, page.review.h});
    drawNavBar(fb, trainerFooter(st, rows));
}

// ---- Clone / Move destination: the SAME box / slot keypads as the Gen IV/V Clone action ----
void stageCloneOrMoveByKeypad(TrainerViewScreen& screen) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    const bool move = st.screen.moveDestination();
    const auto first = st.screen.cloneDestination().value_or(SlotRef::boxed(0, 0));
    const std::string label = generationLabel(screen);
    const auto box = Utils::promptNumber(move ? "Move to " + label + " Box" : "Clone to " + label + " Box",
                                         static_cast<int>(first.box) + 1, 1, static_cast<int>(s.boxCount()));
    if (!box.accepted) { st.screen.cancelBoxAction(); return; }
    const auto slot = Utils::promptNumber(move ? "Move to Slot" : "Clone to Slot",
                                          static_cast<int>(first.slot) + 1, 1, static_cast<int>(s.slotsPerBox()));
    if (!slot.accepted) { st.screen.cancelBoxAction(); return; }
    const auto destination = SlotRef::boxed(static_cast<size_t>(box.value - 1), static_cast<size_t>(slot.value - 1));
    std::string error;
    if (!st.screen.setCloneDestination(s, destination, error) ||
        !st.screen.confirmClone(s, [&](std::string& why) { return refreshPresentation(screen, why); }, error)) {
        st.screen.cancelBoxAction();
        screen.postStatus(error.empty() ? "Box " + std::to_string(box.value) + " / Slot " + std::to_string(slot.value) +
                                              " is not an empty verified " + label + " box slot"
                                        : error, 320);
        return;
    }
    screen.postStatus(move ? label + " Pokemon moved in the staged copy; the source save is unchanged"
                           : label + " Pokemon cloned to the empty staged box slot; the source save is unchanged", 260);
}

// ---- Export Copy: a separate verified copy; the source file is re-read and must be unchanged ----
void exportWorkingCopy(TrainerViewScreen& screen) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    const std::string label = generationLabel(screen);
    std::vector<uint8_t> observed;
    std::string diagnostic;
    if (!Modern::readSourceFileReadOnly(screen.backupDir, observed, diagnostic)) {
        screen.postStatus(label + " export blocked: " + diagnostic, 400);
        return;
    }
    if (sha256Hex(observed) != sha256Hex(s.sourceBytes())) {
        screen.postStatus(label + " export blocked: the source file changed since it was opened; reopen it", 480);
        return;
    }
    Modern::ExportRequest request;
    request.rootDirectory = exportRootOverrideForHostTests.empty() ? Modern::exportsRootFor(screen.sourceGameId)
                                                                   : exportRootOverrideForHostTests;
    request.sourcePath = screen.backupDir;
    request.timestamp = Utils::getTimestamp();
#ifdef POKEVAULT_GIT_COMMIT
    request.applicationSha = POKEVAULT_GIT_COMMIT;
#endif
    request.profileIdentity = PokeBank::Paths::accountUidHex(screen.userUid);
    // AccountUid{0,0} is the existing offline "Pokemon Saves" profile: use that explicit identity.
    if (request.profileIdentity.empty()) request.profileIdentity = std::string(32, '0');
    if (request.rootDirectory.empty()) {
        screen.postStatus(label + " export blocked: " + ScreenState::exportRefusal(s), 480);
        return;
    }
    const auto result = s.exportWorkingCopy(request);
    if (!result.success) {
        screen.postStatus(label + " export blocked: " + result.error, 480);
        return;
    }
    st.screen.finishExport();
    screen.postStatus("Verified app-owned " + label + " copy: " + result.directory + " (original save unchanged)", 480);
    screen.controllerNavigation.reset();
}

// ---- drawing helpers ----
void dimShared(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 140));
}
const char* narrowActionLabel(SharedAction action) noexcept {
    switch (action) {
        case SharedAction::View: return "View";
        case SharedAction::Edit: return "Edit";
        case SharedAction::Move: return "Move";
        case SharedAction::Clone: return "Clone";
        case SharedAction::Remove: return "Release";
        case SharedAction::LegalityProvenance: return PokeBank::UIModel::NarrowPokemonMenu::kOriginLegalityLabel;
        case SharedAction::Add: return "Add Pokemon";
        case SharedAction::Review: return "Review Pending Changes";
        case SharedAction::Close: return "Cancel";
        default: return "";
    }
}
void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    screen.drawGSCOverlay(fb);
    const auto choices = st.screen.availableActions(s);
    PokeBank::UIModel::NarrowPokemonMenu::Model model{};
    for (size_t i = 0; i < choices.count; ++i) model.append(narrowActionLabel(choices[i]), static_cast<int>(i));
    const auto view = s.view(st.screen.target());
    const std::string title = view && view->valid && !view->empty ? std::string(Names::getSpeciesName(view->species)) : "Empty Slot";
    drawNarrowPokemonActions(fb, screen.touchButtons, model, st.screen.actionRow(), title,
                             screen.legacyProviderLabel() + " " + std::string(Modern::platformLabelFor(screen.sourceGameId)) +
                                 " SOURCE / READ ONLY",
                             kActionTouchBase);
    // An empty slot shows the registry / adapter reason for the missing Add row under the sheet.
    if (const auto why = ScreenState::createRefusal(s, st.screen.target()); !why.empty()) {
        const auto g = PokeBank::UIModel::NarrowPokemonMenu::geometry(model.count, fb.getWidth(), fb.getHeight(), kNavBarH);
        fb.drawText(40, g.y + g.height + 10, SharedMovesPanel::fit(fb, "Add Pokemon unavailable: " + why, fb.getWidth() - 80, TextStyle::Caption),
                    Colors::Warning, TextStyle::Caption);
    }
    Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}
PokeBank::UIModel::SharedReview::Rows sharedReviewRows(const TrainerViewScreen& screen) {
    PokeBank::UIModel::SharedReview::Rows rows;
    for (const auto& row : sessionOf(screen).reviewRows()) rows.emplace_back(row.label, row.value);
    return rows;
}
void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    const auto rows = sharedReviewRows(screen);
    SharedEditorChrome::drawReviewPanel(fb, screen.touchButtons, rows, st.screen.reviewPage(), s.hasChanges(), generationLabel(screen));
    if (s.hasChanges() && !ScreenState::exportAvailable(s)) {
        // The shared footer offers X; here it cannot publish, and the panel says why.
        constexpr int w = 1100, h = 584;
        const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - kNavBarH - h) / 2;
        fb.drawText(x + 24, y + h - 36, SharedMovesPanel::fit(fb, "Export Copy unavailable: " + ScreenState::exportRefusal(s), w - 48, TextStyle::Caption),
                    Colors::Warning, TextStyle::Caption);
    }
}
// Origin & Legality: the neutral Gen I-V panel over the adapter's stored facts. No legality
// checker exists for these titles, so Legality is "Incomplete" and the encounter check "Unknown".
void drawLegality(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    dimShared(screen, fb);
    const auto p = s.view(st.screen.target());
    const std::string format = recordFormat(screen);
    std::vector<std::pair<std::string, std::string>> rows{
        {"Source game", gameTitle(screen) + " (" + generationLabel(screen) + ")"},
        {"Native format", p && p->valid ? format + " checksum and structure valid" : format + " unavailable or invalid"},
        {"Encounter / evolution / transfers", "Full history not evaluated"},
        {"Rule coverage", "Structure and editor guardrails; full legality incomplete"}};
    if (p && p->valid && !p->empty) {
        rows.push_back({"Pokemon", std::string(Names::getSpeciesName(p->species)) + " #" + std::to_string(p->species)});
        rows.push_back({"Nickname", p->nickname.empty() ? "(none)" : p->nickname});
        rows.push_back({"Original Trainer", p->otName.empty() ? "(none)" : p->otName});
        rows.push_back({"Trainer / Secret ID", std::to_string(p->tid) + " / " + std::to_string(p->sid)});
        rows.push_back({"Origin game", Enums::getOriginGameName(p->originGame)});
        rows.push_back({"Met location", metLocationText(*p)});
        rows.push_back({"Met level", std::to_string(p->metLevel)});
        rows.push_back({"Met date (stored)", dateText(p->metDate)});
        rows.push_back({"Egg / fateful", std::string(p->egg ? "Unhatched" : "Not egg") + " / " + (p->fatefulEncounter ? "Fateful" : "Not fateful")});
        for (size_t i = 0; i < 4; ++i)
            rows.push_back({"Move " + std::to_string(i + 1), p->moves[i] ? Names::getMoveName(p->moves[i]) : "Empty"});
        for (const auto& fact : p->nativeFacts) rows.push_back({fact.first, fact.second});
    }
    SharedEditorChrome::drawOriginLegalityPanel(fb, screen.touchButtons, rows, Presentation::LegalityState::Incomplete);
}
void drawDecision(TrainerViewScreen& screen, PKSEFramebuffer& fb, const std::string& title, const std::string& message,
                  const std::vector<ControllerHint>& keys) {
    dimShared(screen, fb);
    SharedEditorChrome::beginModal(screen.touchButtons);   // the decision owns the frame's touch
    const int w = 870, h = 250, x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Border, 1);
    fb.drawText(x + 26, y + 24, title, Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 93, SharedMovesPanel::fit(fb, message, w - 52, TextStyle::Body), Colors::TextSecondary, TextStyle::Body);
    fb.drawText(x + 26, y + 142, "The source " + generationLabel(screen) + " save is never modified.", Colors::Accent, TextStyle::Caption);
    std::vector<SharedBoxedRows::GlyphChoice> choices;
    for (const auto& key : keys) choices.push_back({key.button, key.label});
    SharedBoxedRows::drawGlyphChoices(fb, x + 26, y + h - SharedBoxedRows::kChoiceHeight - 18, w - 52, choices);
    if (!keys.empty()) drawNavBar(fb, keys);
}

} // namespace

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held, int stickX, int stickY, const TouchInput& touch) {
    if (!isModernSource(screen)) return false;
    // Refuse all inherited controls on a mismatched Trainer type / family.
    auto& trainer = nativeTrainer(screen);
    if (trainer.sourceGameId() != screen.sourceGameId) {
        screen.postStatus("Shared Modern source: exact title identity mismatch", 180);
        if (down & HidNpadButton_B) screen.goBack = true;
        return true;
    }
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    auto& state = st.screen;
    const std::string label = generationLabel(screen);
    if (screen.storageStatusFrames > 0) --screen.storageStatusFrames;
    // Consume only the hitboxes this surface published last frame; never forward a tap to the
    // inherited bank / source / create handlers.
    const int tap = screen.touchedButtonId(touch);
    if (state.surface() == Surface::Browse && (screen.helpOverlayActive || (down & HidNpadButton_Minus))) return false;
    if (state.surface() == Surface::Browse && !screen.detailViewActive) {
        // The loaded-game hub (same five tiles as Gen I-V). The base hub keeps cursor, focus and
        // help; only activation, Exit and Review are intercepted so no inherited Bank / source
        // writer is reachable.
        namespace G = PokeVault::Integration::Gen5;
        const auto decision = G::routeHub5(screen.homeMenuIndex, down & HidNpadButton_A, down & HidNpadButton_B,
                                           down & HidNpadButton_Plus, tap);
        switch (decision.command) {
            case G::HubCommand5::PassToBase: return false;
            case G::HubCommand5::Exit:
                if (state.requestExit(s)) screen.goBack = true;
                return true;
            case G::HubCommand5::Review:
                if (!state.openReviewFromBrowse(s)) screen.postStatus("No staged " + label + " changes to review", 220);
                return true;
            case G::HubCommand5::StorageRefused:
                screen.homeMenuIndex = decision.tile;
                screen.selectedMode = TrainerViewScreen::ViewMode::Storage;
                screen.postStatus("Legacy Storage (Bank) has no " + recordFormat(screen) +
                                  " conversion in this build. Use Pokemon or Party; the source save is unchanged.", 360);
                return true;
            case G::HubCommand5::Enter: break;
        }
        screen.homeMenuIndex = decision.tile;
        screen.details.active = false;
        screen.controllerNavigation.reset();
        switch (static_cast<G::HubTile5>(decision.tile)) {
            case G::HubTile5::Pokemon: {
                const int boxes = static_cast<int>(screen.trainer.getBoxCount());
                const int slots = static_cast<int>(screen.trainer.getSlotsPerBox());
                screen.selectedMode = TrainerViewScreen::ViewMode::Boxes;
                screen.selectedBoxIndex = std::clamp(screen.selectedBoxIndex, 0, std::max(0, boxes - 1));
                screen.selectedItemIndex = std::clamp(screen.selectedItemIndex, 0, std::max(0, slots - 1));
                break;
            }
            case G::HubTile5::Party: {
                const int count = static_cast<int>(screen.trainer.getPartySize());
                screen.selectedMode = TrainerViewScreen::ViewMode::Party;
                screen.selectedPartyIndex = std::clamp(screen.selectedPartyIndex, 0, std::max(0, count - 1));
                break;
            }
            case G::HubTile5::Items:
                screen.selectedMode = TrainerViewScreen::ViewMode::Items;
                st.bagSurface = BagSurface::Browse;
                st.bagEntered = false;
                screen.selectedCategory = 0;
                screen.selectedItemIndex = 0;
                syncBagCursor(screen);
                break;
            case G::HubTile5::Trainer:
                screen.selectedMode = TrainerViewScreen::ViewMode::Trainer;
                st.trainerOpen = true;
                st.dexOpen = false;
                break;
            case G::HubTile5::Storage: return true;
        }
        screen.detailViewActive = true;
        return true;
    }
    // View / Edit: the ONE shared editor workspace (Gen IV/V code path).
    if (state.workspaceOpen()) {
        screen.details.active = false;
        const u64 workspaceKeys = screen.controllerNavigation.apply(down, held, stickX, stickY,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
        return handleSharedWorkspace(screen, workspaceKeys, touch);
    }
    if (state.surface() == Surface::Actions) {
        const auto actions = state.availableActions(s);
        if (tap >= kActionTouchBase && tap < kActionTouchBase + static_cast<int>(actions.count)) {
            state.moveAction(tap - kActionTouchBase - state.actionRow(), s);
            down |= HidNpadButton_A;
        }
    } else if (state.surface() == Surface::Browse && screen.selectedMode == TrainerViewScreen::ViewMode::Party &&
               tap >= kPartyTouchBase && tap < kPartyTouchBase + static_cast<int>(screen.trainer.getPartySize())) {
        screen.selectedPartyIndex = tap - kPartyTouchBase;
        down |= HidNpadButton_A;
    } else if (state.surface() == Surface::Browse && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes) {
        // Only the existing box panel's native hit ids (generation-neutral decoder); inherited
        // storage, header-rename and source-write hits stay inert.
        const auto boxTouch = PokeVault::Integration::Gen5::decodeGen5BoxTouch(
            tap, screen.selectedItemIndex, static_cast<int>(screen.trainer.getSlotsPerBox()));
        using TouchKind = PokeVault::Integration::Gen5::BoxTouchKind5;
        switch (boxTouch.kind) {
            case TouchKind::PreviousBox: down |= HidNpadButton_L; break;
            case TouchKind::NextBox: down |= HidNpadButton_R; break;
            case TouchKind::FocusSlot: screen.selectedItemIndex = boxTouch.slot; break;
            case TouchKind::OpenSlot: screen.selectedItemIndex = boxTouch.slot; down |= HidNpadButton_A; break;
            case TouchKind::OpenSummary: down |= HidNpadButton_A; break;
            case TouchKind::Ignore: break;
        }
    }
    const u64 keys = screen.controllerNavigation.apply(down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (state.surface() == Surface::Browse && screen.selectedMode == TrainerViewScreen::ViewMode::Trainer && st.trainerOpen) {
        screen.details.active = false;
        return handleTrainerPage(screen, keys, tap);
    }
    if (state.surface() == Surface::Browse && screen.selectedMode == TrainerViewScreen::ViewMode::Items) {
        screen.details.active = false;
        return handleBag(screen, keys, tap, touch);
    }
    const auto refresh = [&](std::string& why) { return refreshPresentation(screen, why); };
    switch (state.surface()) {
        case Surface::Actions:
            if (keys & HidNpadButton_Up) state.moveAction(-1, s);
            if (keys & HidNpadButton_Down) state.moveAction(1, s);
            if (keys & HidNpadButton_B) (void)state.back(s);
            else if (keys & HidNpadButton_A) {
                std::string error;
                if (!state.activateSelected(s, error)) screen.postStatus(error, 300);
                else if (state.surface() == Surface::CloneDestination) stageCloneOrMoveByKeypad(screen);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::ConfirmRemoveBox:
            if (keys & HidNpadButton_B) state.cancelBoxAction();
            else if (keys & HidNpadButton_Y) {
                std::string error;
                if (!state.confirmRemoveBox(s, refresh, error)) screen.postStatus(error, 320);
                else screen.postStatus(label + " Release staged; the source save is unchanged", 300);
            }
            return true;
        case Surface::Review:
            if (keys & HidNpadButton_B) (void)state.back(s);
            else if (keys & HidNpadButton_X) {
                if (!state.requestExport(s))
                    screen.postStatus(s.hasChanges() ? "Export Copy unavailable: " + ScreenState::exportRefusal(s)
                                                     : label + " needs verified staged changes to export", 360);
            } else if (keys & HidNpadButton_Y) state.requestDiscardAll();
            else if (keys & (HidNpadButton_Left | HidNpadButton_Up | HidNpadButton_L)) state.moveReviewPage(-1, sharedReviewRows(screen).size());
            else if (keys & (HidNpadButton_Right | HidNpadButton_Down | HidNpadButton_R)) state.moveReviewPage(1, sharedReviewRows(screen).size());
            return true;
        case Surface::ConfirmExport:
            if (keys & HidNpadButton_B) (void)state.back(s);
            else if (keys & HidNpadButton_Y) exportWorkingCopy(screen);
            return true;
        case Surface::ConfirmDiscardAll:
            if (keys & HidNpadButton_B) (void)state.back(s);
            else if (keys & HidNpadButton_Y) {
                std::string error;
                if (!state.confirmDiscardAll(s, refresh, error))
                    screen.postStatus(error.empty() ? "Discard rejected; staged " + recordFormat(screen) + " changes preserved" : error, 360);
                else screen.postStatus("All app-memory " + recordFormat(screen) + " changes discarded; the source save is unchanged", 260);
            }
            return true;
        case Surface::ConfirmExit: {
            namespace Exit = PokeBank::UIModel::ClassicStagedExit;
            const auto action = Exit::choose(keys & HidNpadButton_A, keys & HidNpadButton_B, keys & HidNpadButton_X, keys & HidNpadButton_Y, tap);
            if (action == Exit::Action::ContinueEditing) (void)state.back(s);
            else if (action == Exit::Action::Review) state.reviewFromExit(s);
            else if (action == Exit::Action::ExportCopy) {
                state.reviewFromExit(s);
                if (!state.requestExport(s)) screen.postStatus("Export Copy unavailable: " + ScreenState::exportRefusal(s), 360);
            } else if (action == Exit::Action::DiscardExit && state.confirmDiscardAndExit(s)) {
                std::string restored;
                (void)refresh(restored);
                screen.goBack = true;
            }
            return true;
        }
        case Surface::CloneDestination:
            state.cancelBoxAction();   // only transient while the keypads are open
            return true;
        case Surface::Legality:
            if (keys & HidNpadButton_B) (void)state.back(s);
            return true;
        case Surface::View:
        case Surface::Edit:
        case Surface::ConfirmDraft:
            return true;   // owned by the shared editor workspace above
        case Surface::Browse: break;
    }

    // The same Party / Boxes renderer inside an entered hub panel; every inherited storage / bank /
    // source mutation route stays barred.
    screen.details.active = false;
    if (keys & HidNpadButton_B) {
        if (PokeVault::Integration::Gen5::panelBack5(screen.itemsShortcutActive) == PokeVault::Integration::Gen5::PanelBack5::ExitSession) {
            if (state.requestExit(s)) screen.goBack = true;
        } else screen.detailViewActive = false;   // back to the hub, tile still focused
        screen.controllerNavigation.reset();
        return true;
    }
    // One Review key per screen (ReviewEntry.h, the Gen V convention: Y on Party and Boxes where Y
    // has no other job, + on the hub). ReviewEntry::opensReview stops at Gen V; the same rule is
    // applied here for every Modern generation until the contract accepts 6+.
    const bool party = screen.selectedMode == TrainerViewScreen::ViewMode::Party;
    const bool boxes = screen.selectedMode == TrainerViewScreen::ViewMode::Boxes;
    const bool panel = screen.detailViewActive && (party || boxes);
    static_assert(PokeBank::UIModel::ReviewEntry::yOpensReview(PokeBank::UIModel::ReviewEntry::Screen::Party, 5) &&
                  PokeBank::UIModel::ReviewEntry::yOpensReview(PokeBank::UIModel::ReviewEntry::Screen::Boxes, 5),
                  "the Modern surface restates the Gen V Party / Boxes Review key");
    if ((!screen.detailViewActive && (keys & HidNpadButton_Plus)) || (panel && (keys & HidNpadButton_Y))) {
        if (!state.openReviewFromBrowse(s)) screen.postStatus("No staged " + label + " changes to review", 220);
        return true;
    }
    if (!party && !boxes) return true;
    if (party) {
        const int count = static_cast<int>(screen.trainer.getPartySize());
        if (count > 0) {
            if (keys & (HidNpadButton_Down | HidNpadButton_Right)) screen.selectedPartyIndex = (screen.selectedPartyIndex + 1) % count;
            if (keys & (HidNpadButton_Up | HidNpadButton_Left)) screen.selectedPartyIndex = (screen.selectedPartyIndex + count - 1) % count;
            if (keys & HidNpadButton_A) beginActions(screen);
        }
        return true;
    }
    constexpr int columns = 6;
    const int capacity = static_cast<int>(screen.trainer.getSlotsPerBox());
    const int count = static_cast<int>(screen.trainer.getBoxCount());
    if (capacity <= 0 || count <= 0) return true;
    screen.selectedBoxIndex = std::clamp(screen.selectedBoxIndex, 0, count - 1);
    screen.selectedItemIndex = std::clamp(screen.selectedItemIndex, 0, capacity - 1);
    if (keys & HidNpadButton_L) screen.selectedBoxIndex = (screen.selectedBoxIndex + count - 1) % count;
    if (keys & HidNpadButton_R) screen.selectedBoxIndex = (screen.selectedBoxIndex + 1) % count;
    if (keys & HidNpadButton_ZL) screen.selectedBoxIndex = PokeVault::Integration::Gen5::advanceGen5BoxIndex(screen.selectedBoxIndex, count, -5);
    if (keys & HidNpadButton_ZR) screen.selectedBoxIndex = PokeVault::Integration::Gen5::advanceGen5BoxIndex(screen.selectedBoxIndex, count, 5);
    if (keys & HidNpadButton_Up) screen.selectedItemIndex = (screen.selectedItemIndex + capacity - columns) % capacity;
    if (keys & HidNpadButton_Down) screen.selectedItemIndex = (screen.selectedItemIndex + columns) % capacity;
    if (keys & HidNpadButton_Left) screen.selectedItemIndex = (screen.selectedItemIndex + capacity - 1) % capacity;
    if (keys & HidNpadButton_Right) screen.selectedItemIndex = (screen.selectedItemIndex + 1) % capacity;
    if (keys & HidNpadButton_A) beginActions(screen);
    return true;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isModernSource(screen)) return false;
    auto& st = stateFor(screen);
    auto& s = sessionOf(screen);
    const auto& state = st.screen;
    if (screen.helpOverlayActive) { screen.drawGSCOverlay(fb); return true; }
    // The inherited details modal would run an unsupported legality analyzer: never opened here.
    screen.details.active = false;
    const std::string label = generationLabel(screen);
    switch (state.surface()) {
        case Surface::Actions: drawActions(screen, fb); return true;
        case Surface::CloneDestination: dimShared(screen, fb); return true;   // transient while the keypads are open
        case Surface::Review: drawReview(screen, fb); return true;
        case Surface::Legality: drawLegality(screen, fb); return true;
        case Surface::Edit:
        case Surface::View:
        case Surface::ConfirmDraft:
            drawSharedWorkspace(screen, fb);   // the ONE shared editor workspace, including its unsaved-changes dialog
            return true;
        case Surface::ConfirmRemoveBox:
            // The Gen IV/V Release dialog: same title, box/slot line and Y confirm.
            drawDecision(screen, fb, "Release this Pokemon?",
                ScreenState::slotLabel(state.target()) + ": removed only from the app-owned staged copy.",
                {{"Y", "Confirm Release"}, {"B", "Cancel"}});
            return true;
        case Surface::ConfirmExport:
            drawDecision(screen, fb, "Export Verified " + label + " Working Copy?",
                "Creates an untouched backup + edited " + recordFormat(screen) + " save under " +
                    (exportRootOverrideForHostTests.empty() ? Modern::exportsRootFor(screen.sourceGameId) : exportRootOverrideForHostTests) + ".",
                {{"Y", "Export Copy"}, {"B", "Cancel"}});
            return true;
        case Surface::ConfirmDiscardAll:
            SharedEditorChrome::drawReviewDiscardConfirm(fb, screen.touchButtons);
            return true;
        case Surface::ConfirmExit:
            SharedEditorChrome::drawStagedExitPanel(fb, screen.touchButtons);
            return true;
        case Surface::Browse: break;
    }
    if (!screen.detailViewActive) {
        screen.drawGSCOverlay(fb);   // the same loaded-game hub as Gen I-V, drawn by the base renderer
        return true;
    }
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) { drawBag(screen, fb); return true; }
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Trainer) { drawTrainerPage(screen, fb); return true; }
    screen.drawGSCOverlay(fb);
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Party) {
        // The established two-column Party panel hit geometry, routed through this surface's
        // Actions instead of the base sheet.
        constexpr int x = LEFT_PANEL_X, y = CONTENT_PANEL_Y;
        constexpr int gutter = 16, slotGap = 12;
        const int width = fb.getWidth() - x;
        const int top = y + 58;
        const int colW = (width - 3 * gutter) / 2;
        const int slotH = (CONTENT_PANEL_HEIGHT - (top - y) - 2 * slotGap - gutter) / 3;
        const int size = static_cast<int>(screen.trainer.getPartySize());
        for (int i = 0; i < std::min(size, 6); ++i) {
            const int column = i >= 3 ? 1 : 0;
            const int row = i >= 3 ? i - 3 : i;
            screen.touchButtons.push_back({kPartyTouchBase + i, x + gutter + column * (colW + gutter), top + row * (slotH + slotGap), colW, slotH});
        }
    }
    (void)s;
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Boxes)
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Box"}, {"ZL/ZR", "Jump 5"}, {"A", "Actions"}, {"Y", "Review"}, {"B", "Back"}});
    else
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Actions"}, {"Y", "Review"}, {"B", "Back"}});
    return true;
}

} // namespace UI::ModernSharedSurface
