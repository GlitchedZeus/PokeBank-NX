// Generation V's guarded adapter for the EXISTING shared Trainer/Party/Boxes/
// Items renderer. View, verified PK5 staged Edit/Review, shared empty-box Add
// and native Bag edits are in app memory only. Gen V never calls the inherited
// source-save writer, Storage/Bank or generic uncontrolled mutation handlers.
// Touch targets are isolated to verified read-only and native-field staged
// actions; native bank, source-writing and unsupported mutation routes remain barred.
#include "Inventory/ItemQuantityPolicy.h"
#include "UI/SharedBagAddPicker.h"
#include "Names/LocationDisplayNames.h"
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Integration/Gen5/Gen5BoxTouchNavigation.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5HubRouting.h"
#include "Integration/Gen5/Gen5DisplayStats.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5PersonalTypes.h"
#include "Integration/Gen5/Gen5Progression.h"
#include "UI/SharedPokemonShell.h"
#include <ctime>
#include "Integration/Gen5/Gen5OriginSummary.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Utils/PokeBankPaths.h"
#include "Utils/FileUtilities.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Names/LocationNames.h"
#include "Names/ItemNames.h"
#include "Names/MachineDisplay.h"
#include "Names/FormNames.h"
#include "Names/MoveBattleStats5.h"
#include "UI/SpeciesChangeMoves.h"
#include "UI/ClassicMachineArtwork.h"
#include "UI/NarrowPokemonActionMenu.h"
#include "SharedPokemonEditorSurface.inc"
#include "UI/InventoryUIContract.h"
#include "UI/ItemPickerArtwork.h"
#include "Utils/Keyboard.h"
#include "UI/Dialogs/DialogFrame.h"
#include <cstdio>

namespace UI::Gen5SharedReadOnlySurface {
namespace {
bool isGen5Source(const TrainerViewScreen& screen) noexcept {
    // Deliberately check the exact source ID, not the Trainer's claimed game
    // group: even a broken adapter cannot fall back to generic mutators.
    return PokeVault::Integration::Gen5EditorProvider::isGen5NdsId(screen.sourceGameId);
}
using ScreenState=PokeVault::Integration::Gen5::Gen5SharedScreenState;
using Surface=ScreenState::Surface;
using Slot=ScreenState::Slot;
using Region=PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace::Region;
// Screen-local ids never overlap inherited Trainer/Storage touch actions.
// Every tap is release-confirmed by touchedButtonId(), not touch-down.
constexpr int kGen5ActionTouchBase=77000;
constexpr int kGen5PartyTouchBase=77400;

PokeVault::Legacy::Gen5ReadOnlyTrainer& nativeTrainer(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::Gen5ReadOnlyTrainer&>(screen.trainer);
}
const char* gen5GameName(std::string_view id) noexcept {
    return id=="black_nds"?"Black":id=="white_nds"?"White":
           id=="black2_nds"?"Black 2":id=="white2_nds"?"White 2":"Generation V";
}
Slot selectedNativeSlot(const TrainerViewScreen& screen) {
    return screen.selectedMode==TrainerViewScreen::ViewMode::Party
        ? Slot{Region::Party,0,static_cast<size_t>(std::max(0,screen.selectedPartyIndex))}
        : Slot{Region::Box,static_cast<size_t>(std::max(0,screen.selectedBoxIndex)),
               static_cast<size_t>(std::max(0,screen.selectedItemIndex))};
}
void beginActions(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    std::string error;
    if(!trainer.sharedScreen().openActions(trainer.stagedPokemon(),
                                          selectedNativeSlot(screen),error))
        screen.postStatus(error,300);
    screen.controllerNavigation.reset();
}
void keepDraft(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    std::string error;
    const bool kept=trainer.sharedScreen().keepWithPresentation(
        trainer.stagedPokemon(),
        [&](std::string& refreshError) {
            return trainer.refreshStagedPokemonPresentation(refreshError);
        },error);
    if(!kept) {
        screen.postStatus(error.empty()?"Gen V Keep was rejected":error,360);
        return; // Dirty draft, cursor and prior staged bytes are still intact.
    }
    screen.postStatus("PK5 draft kept in app memory only; emulator save unchanged",300);
    screen.controllerNavigation.reset();
}
// Shared Create (Gen I-IV parity): Add opens the SAME Edit draft and
// workspace as Edit, focused on Species. The PID comes from the system tick;
// OT, IDs, gender, language and game are copied from the validated save's own
// trainer; the met date is the console date. Nothing persists until Keep, and
// Discard restores the original native empty slot.
bool beginSharedCreate(TrainerViewScreen& screen,std::string& error) {
    namespace G=PokeVault::Integration::Gen5;
    auto& trainer=nativeTrainer(screen);
    auto& ws=trainer.stagedPokemon();
    auto& state=trainer.sharedScreen();
    const uint64_t tick=armGetSystemTick();
    const uint32_t pid=static_cast<uint32_t>(tick^(tick>>29)^(tick>>47));
    const std::time_t now=std::time(nullptr);
    std::tm local{};
    if(now==static_cast<std::time_t>(-1) || !localtime_r(&now,&local)) {
        error="Console date unavailable; Gen V Create needs a real met date";
        return false;
    }
    const ScreenState::CreateDate5 date{static_cast<uint32_t>(local.tm_year+1900),
                                        static_cast<uint32_t>(local.tm_mon+1),
                                        static_cast<uint32_t>(local.tm_mday)};
    const auto request=ScreenState::sharedCreateDefaults(
        ws,pid,Names::getSpeciesName(static_cast<uint16_t>(ScreenState::kSharedCreateSpecies)),
        date,error);
    if(!request)return false;
    return state.beginCreateDraft(ws,*request,[&](std::string& why) {
        return trainer.refreshStagedPokemonPresentation(why);
    },error);
}
bool discardEditDraft(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    auto& state=trainer.sharedScreen();
    if(!state.createPending()) {
        state.discardDraft();
        screen.postStatus("Local Gen V draft discarded; staged edits preserved",220);
        return true;
    }
    std::string error;
    if(!state.discardCreateDraft(trainer.stagedPokemon(),[&](std::string& why) {
        return trainer.refreshStagedPokemonPresentation(why);
    },error)) {
        screen.postStatus(error,320);
        return false;
    }
    screen.postStatus("New Pokemon discarded; the box slot is empty again",240);
    return true;
}
// ---- Generation V adapter of the ONE shared Pokemon editor workspace ----
// View, Edit and Create run through SharedPokemonEditorSurface.inc, the same
// code path, pickers, move editor, focus model and controls as Gen IV. This
// adapter supplies only Gen V's rows, native PK5 values, capabilities and
// validation (Gen5EditorFields.h); every change goes through the guarded
// native draft in Gen5SharedScreenState.
namespace Fields = PokeVault::Integration::Gen5::EditorFields;
namespace EditorSurface = UI::SharedPokemonEditorSurface;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;

// A PID-changing edit (Gender, Shiny, Ability slot, TID/SID keeping
// shininess, Species with a shiny choice) is never applied silently: it waits
// here until the player confirms a dialog that states the PID change.
struct PendingPidChange5 {
    bool active = false;
    std::string what;
    uint32_t before = 0, after = 0;
    std::optional<PokeVault::Integration::Gen5::Gen5SharedPokemonSession> probe;
    bool rebuildCreate = false;
    uint16_t species = 0;
    bool shiny = false;
};
struct Gen5EditorState : EditorSurface::EditorState {
    const TrainerViewScreen* owner = nullptr;
    bool workspaceOpen = false;
    PendingPidChange5 pid;
    SharedScrollbar::State bagAddScrollbar;   // R2-SCROLLBAR: draggable thumb on the Add Item list
};
constexpr int kGen5PidCancelTouch = 41700;
constexpr int kGen5PidConfirmTouch = 41701;
std::string pidHex5(uint32_t pid) {
    char text[16]{};
    std::snprintf(text, sizeof(text), "0x%08X", static_cast<unsigned>(pid));
    return text;
}
Gen5EditorState& editorStateFor(const TrainerViewScreen& screen) {
    static Gen5EditorState state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}
bool workspaceSurface(Surface surface) noexcept {
    return surface == Surface::View || surface == Surface::Edit || surface == Surface::ConfirmDraft;
}
// A location name made only of the full-width placeholder glyphs the Switch
// font cannot draw (e.g. Gen V location 0) is shown as text instead.
std::string metLocationText5(uint8_t origin, uint16_t location) {
    // Player-facing name only: no raw id; placeholder-glyph entries read "Unknown location".
    return Names::getMetLocationDisplayName(origin, location);
}
std::string shortValue5(std::string value, size_t limit = 18) {
    if (value.size() <= limit) return value;
    size_t cut = limit - 3;
    while (cut > 0 && (static_cast<unsigned char>(value[cut]) & 0xC0) == 0x80) --cut;
    return value.substr(0, cut) + "...";
}
std::string formLabel5(uint16_t species, uint16_t form) {
    const char* name = Names::getFormName(species, static_cast<uint8_t>(form));
    if (name && name[0]) return name;
    return form == 0 ? "Default" : "Form " + std::to_string(form);
}
std::array<uint8_t,2> classicTypes5(uint16_t species, uint8_t form, bool b2w2) {
    const auto types = PokeVault::Integration::Gen5::personalTypes5(species, form, b2w2);
    if (types[0] == 0xFF) return {0xFF, 0xFF};
    return {SharedSpeciesPicker::canonicalToClassicType(types[0]),
            SharedSpeciesPicker::canonicalToClassicType(types[1])};
}

struct Gen5Editor {
    using Record = PokeVault::Integration::Gen5::Pokemon5ReadOnly;
    using Field = Fields::Field;
    using PickerTarget = EditorSurface::PickerTarget;
    static constexpr auto generation = PokeBank::UIModel::SharedPokemonEditor::Generation::Gen5;
    static constexpr int kDetailRows = Fields::RowCount;
    static constexpr int kMaxSpecies = Fields::kMaxSpecies;
    static constexpr bool kCompactForm = true;
    static constexpr const char* kDetailReadOnly =
        "Language and Origin are read-only for a Generation V record in this editor";
    static constexpr const char* kValueReadOnly = "This Generation V value is read-only";
    static constexpr const char* kHeldItemTitle = "Held Item — Generation V";
    static constexpr const char* kMovePickerTitle = "Move — Generation V";
    static constexpr const char* kMovePickerSubtitle =
        "Valid BW/B2W2 moves • Pwr/Acc/PP • learnability not checked";
    static constexpr const char* kMoveEditorSubtitle =
        "Valid native BW/B2W2 move • species learnability not checked";
    static constexpr const char* kDataTitle = "Origin & Legality";

    static PokeVault::Integration::Gen5::Gen5SharedScreenState& backend(const TrainerViewScreen& screen) {
        return nativeTrainer(const_cast<TrainerViewScreen&>(screen)).sharedScreen();
    }
    static PokeVault::Integration::Gen5::Gen5SharedPokemonSession& draft(const TrainerViewScreen& screen) {
        return backend(screen).editDraft();
    }
    static bool b2w2(const TrainerViewScreen& screen) { return backend(screen).black2White2(); }

    static bool hasWorking(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return workspaceSurface(backend(screen).surface()) && draft(screen).current().has_value();
    }
    static bool editable(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        const auto surface = backend(screen).surface();
        return (surface == Surface::Edit || surface == Surface::ConfirmDraft) && draft(screen).editable();
    }
    static bool passive(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).surface() == Surface::View;
    }
    static bool creating(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).createPending();
    }
    static bool confirmExit(const TrainerViewScreen& screen, const EditorSurface::EditorState&) {
        return backend(screen).surface() == Surface::ConfirmDraft;
    }
    static std::optional<Record> preview(const TrainerViewScreen& screen,
                                         const EditorSurface::EditorState& state) {
        if (!hasWorking(screen, state)) return std::nullopt;
        return draft(screen).current();
    }
    static void previewFailed(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        screen.postStatus("Generation V editor draft failed strict PK5 preview", 360);
        if (!backend(screen).createPending()) backend(screen).discardDraft();
    }
    static void sessionBack(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        (void)backend(screen).back();
        screen.controllerNavigation.reset();
    }
    static void continueEditing(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        backend(screen).continueEditing();
    }
    static void discard(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        (void)discardEditDraft(screen);
    }
    static void keep(TrainerViewScreen& screen, EditorSurface::EditorState&) { keepDraft(screen); }

    // Stage one native field; report the native refusal reason when refused.
    static bool stage(TrainerViewScreen& screen, Field field, size_t stat, uint32_t value) {
        std::string error;
        if (draft(screen).stage(field, stat, value, error)) return true;
        screen.postStatus(error.empty() ? "Generation V draft refused that value" : error, 320);
        return false;
    }

    static Gen5EditorState& gen5(EditorSurface::EditorState& state) {
        return static_cast<Gen5EditorState&>(state);
    }
    // Strict re-verify of a PID-changing probe (PKHeX 6501f0ab rules, see
    // Gen5PidCoordination.h): a valid Gen V-origin PK5 whose stored gender and
    // normal ability still follow the new PID.
    static bool pidLinksHold(const Record& p, bool b2w2) {
        const auto personal = PokeVault::Integration::Gen5::personal5(p.species(), p.form(), b2w2);
        return p.valid() && !p.empty() && personal &&
               !PokeVault::Integration::Gen5::earlierGenerationOrigin5(p.originVersion()) &&
               PokeVault::Integration::Gen5::genderFromPid5(p.pid(), personal.info->genderRatio) == p.gender() &&
               (p.hiddenAbility() ||
                PokeVault::Integration::Gen5::abilityForSlot5(
                    *personal.info, PokeVault::Integration::Gen5::abilityBit5(p.pid())) == p.ability());
    }
    // Apply a probe draft at once when the PID is unchanged; otherwise hold it
    // for the explicit "Change PID?" confirmation.
    static bool offer(TrainerViewScreen& screen, EditorSurface::EditorState& state,
                      PokeVault::Integration::Gen5::Gen5SharedPokemonSession probe, std::string what) {
        const auto before = draft(screen).current();
        const auto after = probe.current();
        if (!before || !after) return false;
        if (before->pid() == after->pid()) {
            draft(screen) = std::move(probe);
            return true;
        }
        auto& pending = gen5(state).pid;
        pending = {};
        pending.active = true;
        pending.what = std::move(what);
        pending.before = before->pid();
        pending.after = after->pid();
        pending.probe = std::move(probe);
        screen.controllerNavigation.reset();
        return true;
    }

    static const char* detailLabel(int row) { return Fields::kDetailLabels[static_cast<size_t>(row)]; }
    static bool detailEditable(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                               int row, const Record&) {
        return Fields::detailEditable(draft(screen), row, b2w2(screen));
    }
    static bool detailLocked(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                             int row, const Record& p) {
        return Fields::detailLocked(p, row, singleForm(screen, p));
    }
    static std::string detailValue(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                   const Record& p, int row) {
        using namespace Fields;
        switch (row) {
            case Species: return std::to_string(p.species()) + " - " + Names::getSpeciesName(p.species());
            case Nickname: case OriginalTrainer: {
                const auto name = PokeVault::Integration::Gen5::asciiName5(p.decodedBytes(), row == Nickname);
                return name ? *name : std::string("(native glyphs)");
            }
            case Level: return std::to_string(Fields::level(p, b2w2(screen)));
            case Experience: return std::to_string(p.experience());
            case TrainerId: return std::to_string(p.tid());
            case SecretId: return std::to_string(p.sid());
            case HeldItem: return p.heldItem() == 0 ? "(none)" : Names::getItemName(p.heldItem());
            case Friendship: return std::to_string(p.friendship());
            case Language: return Enums::getLanguageName(p.language());
            case Origin: return Enums::getOriginGameName(p.originVersion());
            case Fields::Ball: return Enums::getBallName(p.ball());
            case MetLevel: return std::to_string(p.metLevel());
            case MetLocation: return metLocationText5(p.originVersion(), p.metLocation());
            case Pokerus: return Fields::pokerusLabel(p.pokerusMode());
            case Form:
                return singleForm(screen, p) ? std::string("Default (fixed)")
                                             : formLabel5(p.species(), p.form());
            case OriginalTrainerGender: return p.originalTrainerGender() == 0 ? "Male" : "Female";
            case FatefulEncounter: return p.fatefulEncounter() ? "Yes" : "No";
            default: return {};
        }
    }
    static bool editDetail(TrainerViewScreen& screen, EditorSurface::EditorState& state, int row,
                           const Record& p) {
        using namespace Fields;
        auto& d = draft(screen);
        std::string error;
        const auto refused = [&](const std::string& why) {
            screen.postStatus(why.empty() ? "Generation V draft refused that value" : why, 320);
        };
        switch (row) {
            case Species:
                EditorSurface::openPicker(screen, state, PickerTarget::Species,
                                          [] { std::vector<uint16_t> v;
                                               for (uint16_t s = 1; s <= Fields::kMaxSpecies; ++s) v.push_back(s);
                                               return v; }(), p.species());
                state.speciesPreviewShiny = p.shiny();
                return true;
            case Nickname: case OriginalTrainer: {
                const bool nickname = row == Nickname;
                const auto current = PokeVault::Integration::Gen5::asciiName5(p.decodedBytes(), nickname);
                const auto r = Utils::promptText(nickname ? "Nickname" : "Original Trainer",
                    nickname ? "1-10 ASCII characters" : "1-7 ASCII characters",
                    current ? *current : std::string{}, nickname ? 10 : 7);
                if (r.accepted && !d.stageName(nickname ? Field::NicknameText : Field::OriginalTrainerText,
                                               r.text, error))
                    refused(error);
                return true;
            }
            case Level: {
                const auto r = Utils::promptNumber("Level", Fields::level(p, b2w2(screen)), 1, 100);
                if (r.accepted) stage(screen, Field::Level, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case Experience: {
                const auto personal = PokeVault::Integration::Gen5::personal5(p.species(), p.form(), b2w2(screen));
                if (!personal) {
                    screen.postStatus("Missing BW/B2W2 experience growth data", 300);
                    return true;
                }
                const uint32_t maximum = PokeVault::Integration::Gen5::experienceForLevel5(
                    personal.info->growthRate, 100);
                const auto r = Utils::promptNumber("Experience",
                    static_cast<int>(std::min<uint32_t>(p.experience(), maximum)), 0,
                    static_cast<int>(maximum));
                if (r.accepted) stage(screen, Field::Experience, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case TrainerId: case SecretId: {
                const bool secret = row == SecretId;
                const auto r = Utils::promptNumber(secret ? "Secret ID" : "Trainer ID",
                    secret ? p.sid() : p.tid(), 0, 65535);
                if (!r.accepted) return true;
                auto probe = d;
                if (!Fields::stageTrainerId(probe, secret, static_cast<uint16_t>(r.value), error))
                    refused(error);
                else
                    offer(screen, state, std::move(probe),
                          std::string(secret ? "Secret ID " : "Trainer ID ") +
                              std::to_string(secret ? p.sid() : p.tid()) + " -> " +
                              std::to_string(r.value) + ", shininess kept");
                return true;
            }
            case HeldItem:
                EditorSurface::openPicker(screen, state, PickerTarget::HeldItem,
                                          Fields::heldItemChoices(), p.heldItem());
                return true;
            case Friendship: {
                const auto r = Utils::promptNumber("Friendship", p.friendship(), 0, 255);
                if (r.accepted) stage(screen, Field::Friendship, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case Fields::Ball:
                if (!PokeVault::Integration::Gen5::gen5Origin5(p.originVersion())) {
                    screen.postStatus("Ball editing is read-only for transferred non-Gen-V origins", 300);
                    return true;
                }
                EditorSurface::openPicker(screen, state, PickerTarget::Ball, Fields::ballChoices(), p.ball());
                return true;
            case MetLevel: {
                const auto r = Utils::promptNumber("Met Level", p.metLevel(), 0, 100);
                if (r.accepted) stage(screen, Field::MetLevel, 0, static_cast<uint32_t>(r.value));
                return true;
            }
            case MetLocation:
                if (!PokeVault::Integration::Gen5::gen5Origin5(p.originVersion())) {
                    screen.postStatus("Met Location editing is read-only for transferred non-Gen-V origins", 300);
                    return true;
                }
                EditorSurface::openPicker(screen, state, PickerTarget::MetLocation,
                                          Fields::metLocationChoices(), p.metLocation());
                return true;
            case Pokerus: {
                // Inline toggle: None -> Infected -> Cured -> None (Gen V modes 0 / 2 / 1).
                // The native record restores the source byte when the original mode returns.
                const uint32_t mode = p.pokerusMode();
                const uint32_t next = mode == 0 ? 2u : mode == 2 ? 1u : 0u;
                stage(screen, Field::Pokerus, 0, next);
                return true;
            }
            case Form: {
                auto forms = Fields::formChoices(d, b2w2(screen));
                if (forms.size() <= 1) {
                    screen.postStatus("This Pokémon has no alternate Form valid for this Generation V slot", 300);
                    return true;
                }
                EditorSurface::openPicker(screen, state, PickerTarget::Form, std::move(forms), p.form());
                return true;
            }
            case OriginalTrainerGender: case FatefulEncounter:
                if (!Fields::toggleFlag(d, row == FatefulEncounter ? Field::FatefulEncounter
                                                                    : Field::OriginalTrainerGender, error))
                    refused(error);
                return true;
            default:
                return false;
        }
    }
    static bool singleForm(const TrainerViewScreen& screen, const Record& p) {
        const auto personal = PokeVault::Integration::Gen5::personal5(p.species(), 0, b2w2(screen));
        return !personal || personal.forms <= 1;
    }
    static void passiveActivate(TrainerViewScreen& screen, EditorSurface::EditorState&, int row) {
        const auto p = draft(screen).current();
        if (row == Fields::Form && p && singleForm(screen, *p))
            screen.postStatus("This Gen V Pokémon has only its default form", 135);
    }

    static std::array<uint8_t,6> ivs(const Record& p) { return p.ivs(); }
    static std::array<uint8_t,6> evs(const Record& p) { return p.evs(); }
    // The same values the Box summary and View show (Gen5DisplayStats.h).
    static std::array<uint16_t,6> stats(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                        const Record& p) {
        return PokeVault::Integration::Gen5::displayStats5(p, b2w2(screen));
    }
    static void setIv(TrainerViewScreen& screen, EditorSurface::EditorState&, size_t stat, uint8_t value) {
        stage(screen, Field::IV, stat, value);
    }
    static void setEv(TrainerViewScreen& screen, EditorSurface::EditorState&, size_t stat, uint8_t value) {
        stage(screen, Field::EV, stat, value);
    }
    static bool pidRowEditable(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                               int row, const Record&) {
        return Fields::pidRowEditable(draft(screen), row, b2w2(screen));
    }
    static uint8_t gender(const Record& p) { return p.gender(); }
    static std::string pidRowValue(const TrainerViewScreen&, const Record& p, int row) {
        switch (row) {
            case Fields::NatureRow: return Fields::natureName(p.nature());
            case Fields::GenderRow: return EditorSurface::genderText(p.gender());
            case Fields::ShinyRow: return p.shiny() ? "Yes" : "No";
            case Fields::AbilityRow:
                return std::string(Trainer::getAbilityName(p.ability())) + (p.hiddenAbility() ? " (Hidden)" : "");
            default: return std::to_string(p.pid());
        }
    }
    static bool editPidRow(TrainerViewScreen& screen, EditorSurface::EditorState& state, int row,
                           const Record& p) {
        auto& d = draft(screen);
        if (!Fields::pidRowEditable(d, row, b2w2(screen))) {
            screen.postStatus(Fields::pidRowRestriction(d, row, b2w2(screen)), 300);
            return true;
        }
        std::string error;
        if (row == Fields::NatureRow) {
            std::vector<uint16_t> natures;
            for (uint16_t i = 0; i < 25; ++i) natures.push_back(i);
            EditorSurface::openPicker(screen, state, PickerTarget::Nature, std::move(natures), p.nature());
        } else if (row == Fields::GenderRow) {
            auto probe = d;
            if (!Fields::toggleGender(probe, error)) {
                screen.postStatus(error, 300);
            } else {
                std::string what = std::string("Gender ") + EditorSurface::genderText(p.gender()) +
                                   " -> " + EditorSurface::genderText(probe.current()->gender());
                offer(screen, state, std::move(probe), std::move(what));
            }
        } else if (row == Fields::ShinyRow) {
            auto probe = d;
            if (!Fields::toggleShiny(probe, error)) screen.postStatus(error, 300);
            else offer(screen, state, std::move(probe),
                       std::string("Shiny ") + (p.shiny() ? "Yes -> No" : "No -> Yes"));
        } else if (row == Fields::AbilityRow) {
            EditorSurface::openPicker(screen, state, PickerTarget::Ability,
                                      Fields::abilityChoices(p, b2w2(screen)),
                                      Fields::abilityChoiceFor(p, b2w2(screen)));
        } else {
            return false;
        }
        return true;
    }

    static std::array<uint16_t,4> moves(const Record& p) { return p.moves(); }
    static std::array<uint8_t,4> pp(const Record& p) { return p.pp(); }
    static std::array<uint8_t,4> ppUps(const Record& p) { return p.ppUps(); }
    static uint16_t workingMove(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = draft(screen).current();
        return p ? p->moves()[static_cast<size_t>(slot)] : 0;
    }
    static uint8_t workingPP(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = draft(screen).current();
        return p ? p->pp()[static_cast<size_t>(slot)] : 0;
    }
    static uint8_t workingPPUps(const TrainerViewScreen& screen, const EditorSurface::EditorState&, int slot) {
        const auto p = draft(screen).current();
        return p ? p->ppUps()[static_cast<size_t>(slot)] : 0;
    }
    static void setMove(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint16_t move) {
        stage(screen, Field::Move, static_cast<size_t>(slot), move);
    }
    static void setPP(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint8_t value) {
        stage(screen, Field::MovePP, static_cast<size_t>(slot), value);
    }
    static void setPPUps(TrainerViewScreen& screen, EditorSurface::EditorState&, int slot, uint8_t value) {
        stage(screen, Field::MovePPUps, static_cast<size_t>(slot), value);
    }
    static int maxPP(const TrainerViewScreen&, uint16_t move, uint8_t ppUps) {
        return PokeVault::Integration::Gen5::maxPP5(move, ppUps);
    }
    static std::vector<uint16_t> moveChoices(TrainerViewScreen&, EditorSurface::EditorState&) {
        return Fields::moveChoices();
    }
    static void moveStatus(TrainerViewScreen&, const EditorSurface::EditorState&, const Record&,
                           uint16_t move, const char*& status, Color& color) {
        status = move == 0 ? "-" : "Native";
        color = move == 0 ? Colors::TextDim : Colors::TextSecondary;
    }
    // Black/White or Black 2/White 2 for the shared Moves panel and chooser facts.
    static Enums::GameVersion moveGame(const TrainerViewScreen& screen) {
        return b2w2(screen) ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;
    }
    static std::string pickerLabel(TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                   PickerTarget target, uint16_t value) {
        const auto p = draft(screen).current();
        switch (target) {
            case PickerTarget::Species: return EditorSurface::dexLabel(value);
            case PickerTarget::HeldItem:
                return value == 0 ? "(none)" :
                    Names::machineDisplayLabel(screen.trainer.getGameGroup(), value, Names::getItemName(value));
            case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
            case PickerTarget::MetLocation: {
                std::string label = metLocationText5(p ? p->originVersion() : 20, value);
                if (p) label += "  |  Met Lv. " + std::to_string(p->metLevel());
                return label;
            }
            case PickerTarget::Pokerus: return Fields::pokerusLabel(value);
            case PickerTarget::Form: return formLabel5(p ? p->species() : 0, value);
            case PickerTarget::Move: return MoveUI::numberedName(value);
            case PickerTarget::Nature: return Trainer::getNatureName(static_cast<uint8_t>(value));
            case PickerTarget::Ability: {
                if (!p) return {};
                const auto personal = PokeVault::Integration::Gen5::personal5(p->species(), p->form(), b2w2(screen));
                const uint8_t ability = personal
                    ? PokeVault::Integration::Gen5::abilityForSlot5(*personal.info, value) : 0;
                // NEW-13: name and short effect; only the Hidden slot keeps a note.
                const std::string name = std::string(Trainer::getAbilityName(ability)) + (value == 2 ? " (Hidden)" : "");
                return UI::AbilityEffects::row(name, ability, 5);
            }
            default: return {};
        }
    }
    static bool applyPicker(TrainerViewScreen& screen, EditorSurface::EditorState& state,
                            PickerTarget target, uint16_t value) {
        auto& d = draft(screen);
        const auto p = d.current();
        if (!p) return false;
        switch (target) {
            case PickerTarget::Species: return applySpecies(screen, state, *p, value);
            case PickerTarget::HeldItem: return stage(screen, Field::HeldItem, 0, value);
            case PickerTarget::Ball: return stage(screen, Field::Ball, 0, value);
            case PickerTarget::MetLocation: return stage(screen, Field::MetLocation, 0, value);
            case PickerTarget::Pokerus: return stage(screen, Field::Pokerus, 0, value);
            case PickerTarget::Form: return stage(screen, Field::Form, 0, value);
            case PickerTarget::Move: return stage(screen, Field::Move, static_cast<size_t>(state.moveEditorSlot), value);
            case PickerTarget::Nature: return stage(screen, Field::Nature, 0, value);
            case PickerTarget::Ability: {
                auto probe = d;
                std::string why;
                if (!probe.stage(Field::AbilitySlot, 0, value, why)) {
                    screen.postStatus(why, 320);
                    return false;
                }
                return offer(screen, state, std::move(probe),
                             "Ability -> " + UI::AbilityEffects::nameOf(pickerLabel(screen, state, PickerTarget::Ability, value)));
            }
            default: return false;
        }
    }
    // Species + the Y shiny preview. A pending Create is rebuilt natively for
    // the new species; an existing Pokemon changes only when the native
    // identity can be preserved, otherwise the reason is shown.
    static bool applySpecies(TrainerViewScreen& screen, EditorSurface::EditorState& state,
                             const Record& p, uint16_t species) {
        auto& backendState = backend(screen);
        std::string error;
        if (backendState.createPending() && p.shiny() != state.speciesPreviewShiny) {
            // Rebuilding the new Pokemon with the other shiny state changes its PID.
            auto& pending = gen5(state).pid;
            pending = {};
            pending.active = true;
            pending.rebuildCreate = true;
            pending.species = species;
            pending.shiny = state.speciesPreviewShiny;
            pending.what = std::string(Names::getSpeciesName(species)) +
                           (state.speciesPreviewShiny ? ", Shiny" : ", not Shiny");
            pending.before = p.pid();
            pending.after = PokeVault::Integration::Gen5::withShiny5(
                p.pid(), p.tid(), p.sid(), state.speciesPreviewShiny);
            screen.controllerNavigation.reset();
            return true;
        }
        if (backendState.createPending()) {
            auto& trainer = nativeTrainer(screen);
            if (!backendState.recreateCreateSpecies(trainer.stagedPokemon(), species,
                    state.speciesPreviewShiny, Names::getSpeciesName(species),
                    Names::getSpeciesName(p.species()),
                    [&](std::string& why) { return trainer.refreshStagedPokemonPresentation(why); },
                    error)) {
                screen.postStatus(error, 360);
                return false;
            }
            return true;
        }
        const auto why = backendState.speciesRefusal(species);
        if (!why.empty()) {
            screen.postStatus(why, 420);
            return false;
        }
        auto probe = draft(screen);
        if (species != p.species() && !probe.stage(Field::Species, 0, species, error)) {
            screen.postStatus(error, 360);
            return false;
        }
        if (species != p.species()) {
            // R2-BUG-004: the new species' native default moveset at this level from the BW or
            // B2W2 table (PKHeX lvlmove_bw / _b2w2), base PP, no PP Ups; staged in Keep/Review's
            // replay order (Move, PP Ups, PP) so the replay reproduces these bytes exactly.
            if (!PokeBank::UIModel::SpeciesMoveDefaults::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                    PokeBank::UIModel::SpeciesMoveDefaults::forSpecies(PokeBank::UIModel::SpeciesMoveDefaults::gen5Table(b2w2(screen)), species, 0,
                        static_cast<uint8_t>(Fields::level(p, b2w2(screen))),
                        [](uint16_t move) { return PokeVault::Integration::Gen5::basePP5(move); }),
                    error)) {
                screen.postStatus("Could not reset moves for the new species: " + error, 360);
                return false;
            }
        }
        if (probe.current()->shiny() != state.speciesPreviewShiny &&
            !probe.stage(Field::Shiny, 0, state.speciesPreviewShiny ? 1u : 0u, error)) {
            screen.postStatus("Could not reconcile Species/Shiny while preserving PID-linked traits: " + error, 360);
            return false;
        }
        return offer(screen, state, std::move(probe),
                     std::string("Species ") + Names::getSpeciesName(species) +
                         (state.speciesPreviewShiny ? ", Shiny" : ", not Shiny"));
    }
    static std::array<uint8_t,2> speciesPreviewTypes(const TrainerViewScreen& screen, uint16_t species) {
        return classicTypes5(species, 0, b2w2(screen));
    }
    static std::string speciesLabel(uint16_t value) { return EditorSurface::dexLabel(value); }
    // R2-SEARCH: species search is the shared in-modal chooser (SharedSpeciesPicker.h), live
    // per keystroke; the former swkbd "jump to first match" hook is retired.
    static std::string heldItemArtKey(TrainerViewScreen& screen, uint16_t value) {
        const auto disc = UI::ClassicMachineArtwork::artKey(screen.trainer.getGameGroup(), value);
        return disc.empty() ? Names::getItemName(value) : disc;
    }

    static EditorSurface::ChromeInfo chrome(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                                            const Record& p) {
        const auto& backendState = backend(screen);
        EditorSurface::ChromeInfo info;
        info.species = p.species();
        const auto nickname = PokeVault::Integration::Gen5::asciiName5(p.decodedBytes(), true);
        info.nickname = nickname ? *nickname : std::string{};
        info.level = Fields::level(p, b2w2(screen));
        info.gender = p.gender();
        info.shiny = p.shiny();
        info.mode = backendState.createPending() ? EditorSurface::Context::SurfaceMode::CreateDraft
            : backendState.surface() == Surface::View ? EditorSurface::Context::SurfaceMode::ViewReadOnly
                                                      : EditorSurface::Context::SurfaceMode::EditStaged;
        const auto& target = backendState.target();
        info.context = target.region == Region::Party
            ? "Party / Slot " + std::to_string(target.slot + 1)
            : "Box " + std::to_string(target.box + 1) + " / Slot " + std::to_string(target.slot + 1);
        return info;
    }
    static std::array<uint8_t,2> portraitTypes(const TrainerViewScreen& screen, const Record& p) {
        return classicTypes5(p.species(), p.form(), b2w2(screen));
    }
    // Short rows only, so nothing runs into the stats graph beside them.
    static std::vector<EditorSurface::DataRow> dataRows(TrainerViewScreen& screen,
                                                        const EditorSurface::EditorState&, const Record& p) {
        const auto met = p.metDate();
        const auto date = [](auto d) {
            return std::to_string(d.year) + "-" + (d.month < 10 ? "0" : "") + std::to_string(d.month) +
                   "-" + (d.day < 10 ? "0" : "") + std::to_string(d.day);
        };
        std::vector<EditorSurface::DataRow> rows{
            {"Game", gen5GameName(screen.sourceGameId)},
            {"Origin", shortValue5(Enums::getOriginGameName(p.originVersion()))},
            {"Language", Enums::getLanguageName(p.language())},
            {"Ball", shortValue5(Enums::getBallName(p.ball()))},
            {"Met", shortValue5(metLocationText5(p.originVersion(), p.metLocation()))},
            {"Met Lv. / date", std::to_string(p.metLevel()) + " / " + date(met)},
            {"Friendship", std::to_string(p.friendship())},
            {"Pokérus", Fields::pokerusLabel(p.pokerusMode())},
            {"Fateful", p.fatefulEncounter() ? "Yes" : "No"},
            {"Egg status", p.egg() ? "Unhatched" : "Not egg"},
            {"OT gender", p.originalTrainerGender() == 0 ? "Male" : "Female"},
            // Only format validity is checked for Gen V; no legality conclusion is drawn.
            {"Legality", PokeBank::UIModel::SharedEditorPresentation::legalityLabel(
                PokeBank::UIModel::SharedEditorPresentation::LegalityState::NotChecked)},
        };
        return rows;
    }
};

// The explicit PID confirmation: A (or the Change PID button) applies the
// strictly re-verified probe; B (or Cancel) leaves the draft untouched.
bool handlePidConfirmation(TrainerViewScreen& screen, Gen5EditorState& state, u64 keys,
                           const TouchInput& touch) {
    auto& pending = state.pid;
    if (!pending.active) return false;
    const int tap = screen.touchedButtonId(touch);
    if (tap == kGen5PidConfirmTouch) keys |= HidNpadButton_A;
    if (tap == kGen5PidCancelTouch) keys |= HidNpadButton_B;
    if (keys & HidNpadButton_B) {
        pending = {};
        screen.postStatus("PID unchanged; the draft was not modified", 200);
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(keys & HidNpadButton_A)) return true;
    const bool b2w2 = nativeTrainer(screen).sharedScreen().black2White2();
    std::string error;
    bool applied = false;
    if (pending.rebuildCreate) {
        auto& trainer = nativeTrainer(screen);
        const auto current = trainer.sharedScreen().draft().current();
        applied = current && trainer.sharedScreen().recreateCreateSpecies(
            trainer.stagedPokemon(), pending.species, pending.shiny,
            Names::getSpeciesName(pending.species), Names::getSpeciesName(current->species()),
            [&](std::string& why) { return trainer.refreshStagedPokemonPresentation(why); }, error);
        const auto rebuilt = trainer.sharedScreen().draft().current();
        applied = applied && rebuilt && Gen5Editor::pidLinksHold(*rebuilt, b2w2);
    } else if (pending.probe) {
        const auto after = pending.probe->current();
        if (after && after->pid() == pending.after && Gen5Editor::pidLinksHold(*after, b2w2)) {
            Gen5Editor::draft(screen) = std::move(*pending.probe);
            applied = true;
        } else {
            error = "The new PID failed strict re-verification; nothing was changed";
        }
    }
    screen.postStatus(applied ? "PID changed in the draft: " + pidHex5(pending.before) + " -> " +
                                    pidHex5(pending.after)
                              : (error.empty() ? std::string("PID change refused") : error), 320);
    pending = {};
    screen.controllerNavigation.reset();
    return true;
}
void drawPidConfirmation(TrainerViewScreen& screen, PKSEFramebuffer& fb, const Gen5EditorState& state) {
    const auto& pending = state.pid;
    if (!pending.active) return;
    // The confirm owns the touch layer: covered glyphs and footer badges never inject a button.
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 150));
    constexpr int w = 700, h = 290;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Change PID?", Colors::Orange);
    fb.drawText(x + 28, cy, pending.what, Colors::Text);
    fb.drawText(x + 28, cy + 36, "This changes the Pokemon's PID (personality value):",
                Colors::TextSecondary, TextStyle::Caption);
    fb.drawText(x + 28, cy + 62, pidHex5(pending.before) + "  ->  " + pidHex5(pending.after),
                Colors::Text, TextStyle::Body);
    fb.drawText(x + 28, cy + 98, "Gender, ability and shininess stay linked to the new PID. "
                "Origin & Legality: not checked.", Colors::TextDim, TextStyle::Caption);
    const int bw = 190, bh = TouchTargetMin, by = y + h - bh - 18;
    drawGlyphButton(fb, x + 24, by, bw, bh, "B", "Cancel", Colors::PanelAlt);
    drawGlyphButton(fb, x + w - 24 - bw, by, bw, bh, "A", "Change PID", Colors::PanelAlt);
    screen.touchButtons.push_back({kGen5PidCancelTouch, x + 24, by, bw, bh});
    screen.touchButtons.push_back({kGen5PidConfirmTouch, x + w - 24 - bw, by, bw, bh});
    drawNavBar(fb, {{"A", "Change PID"}, {"B", "Cancel"}});
}

bool handleSharedWorkspace(TrainerViewScreen& screen, u64 keys, const TouchInput& touch) {
    auto& state = editorStateFor(screen);
    if (!state.workspaceOpen) {
        EditorSurface::resetWorkspaceFocus(state);
        state.workspaceOpen = true;
    }
    if (handlePidConfirmation(screen, state, keys, touch)) return true;
    EditorSurface::handleWorkspace<Gen5Editor>(screen, state, keys, touch);
    if (!workspaceSurface(nativeTrainer(screen).sharedScreen().surface())) {
        EditorSurface::resetWorkspaceFocus(state);
        state.workspaceOpen = false;
        state.pid = {};
    }
    return true;
}
void drawSharedWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = editorStateFor(screen);
    if (!state.workspaceOpen) {
        EditorSurface::resetWorkspaceFocus(state);
        state.workspaceOpen = true;
    }
    EditorSurface::drawWorkspace<Gen5Editor>(screen, fb, state);
    drawPidConfirmation(screen, fb, state);
}

// ---- Native Bag in the shared Items panel (same surface as Gen III/IV) ----
// Controller and touch reach the same Gen5BagScreen entry points. Every
// accepted change is staged in app memory through Gen5StagedBag only.
constexpr int kGen5BagAddTouchBase=81000;
int bagItemsPerPage() noexcept {
    return std::max(1,(CONTENT_PANEL_HEIGHT-106)/52); // mirrors Panels::drawItems
}
void syncBagCursor(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    auto& bag=trainer.bagScreen();
    bag.clamp(trainer.stagedPokemon());
    screen.selectedCategory=static_cast<int>(bag.pocket());
    screen.selectedItemIndex=static_cast<int>(bag.row());
    screen.currentPage=static_cast<int>(bag.row())/bagItemsPerPage();
}
std::string bagItemLabel(const TrainerViewScreen& screen,uint16_t id) {
    return Names::machineDisplayLabel(screen.trainer.getGameGroup(),id,Names::getItemName(id));
}
// The ONE per-item quantity query of the Gen V bag, backed by Thread I's
// Inventory/ItemQuantityPolicy.h. Never exceeds the native pocket cap the staged bag enforces
// (bagPocketLayout5 maxCount). Gen V keeps Poke Balls in Items; Key Items and TMs/HMs are unique.
// The exact-game policy rule of one Gen V bag item (Gen5Bag.h pocket order).
PokeVault::Inventory::QuantityPolicy::ItemQuantityRule gen5PolicyRule(
    const TrainerViewScreen& screen, size_t pocket, uint16_t id) noexcept {
    namespace QP = PokeVault::Inventory::QuantityPolicy;
    static constexpr std::array<QP::Pocket, 5> kPockets{{
        QP::Pocket::Items, QP::Pocket::KeyItems, QP::Pocket::TMHM, QP::Pocket::Medicine,
        QP::Pocket::Berries}};
    static_assert(kPockets.size() == PokeVault::Integration::Gen5::BagPocket5Count);
    QP::Game game{};
    if (pocket >= kPockets.size() || !QP::gameFromId(screen.sourceGameId, game)) return {};
    return QP::quantityRule(game, kPockets[pocket], id);
}
bool handleBag(TrainerViewScreen& screen,u64 keys,int tap,const TouchInput& touch) {
    namespace G=PokeVault::Integration::Gen5;
    using BagSurface=G::Gen5BagScreen::Surface;
    auto& trainer=nativeTrainer(screen);
    auto& ws=trainer.stagedPokemon();
    auto& bag=trainer.bagScreen();
    auto& state=trainer.sharedScreen();
    const auto refresh=[&](std::string& why) {
        return trainer.refreshStagedPokemonPresentation(why);
    };
    std::string error;
    switch(bag.surface()) {
        case BagSurface::AddPicker: {
            const int count=static_cast<int>(bag.addChoices().size());
            // Shared scrollbar first: the window is centred on the cursor, so the thumb moves the
            // cursor to the row that puts the window where the finger put it; touch is then owned.
            // R2-BUG-006: the shared compact grid (UI/SharedItemGridPicker.h); input builds it from
            // the same labels the draw uses, so taps, scrollbar and D-pad agree with what is shown.
            namespace AddGrid=UI::SharedItemGridPicker;
            const auto& addChoices=bag.addChoices();
            const auto addGrid=UI::SharedBagAddPicker::pickerGrid(count,[&](int index) {
                return bagItemLabel(screen,addChoices[static_cast<size_t>(index)]);
            });
            const int addSelected=static_cast<int>(bag.addIndex());
            const auto addBar=SharedScrollbar::handleTouch(editorStateFor(screen).bagAddScrollbar,
                SharedScrollbar::sample(touch),
                AddGrid::scrollModel(addGrid,AddGrid::firstRowFor(addGrid,addSelected)));
            if(addBar.changed && count>0)
                bag.moveAddCursor(AddGrid::indexForFirstRow(addGrid,addSelected,addBar.scroll)-addSelected);
            if(!addBar.consumed && tap>=kGen5BagAddTouchBase && tap<kGen5BagAddTouchBase+count &&
               bag.tapAddChoice(static_cast<size_t>(tap-kGen5BagAddTouchBase)))
                keys|=HidNpadButton_A;
            // Grid navigation: D-pad / stick cell to cell, L/R one screen of rows, ZL/ZR five.
            {
                const int from=static_cast<int>(bag.addIndex());
                const int to=AddGrid::navigate(addGrid,from,AddGrid::Keys{
                    (keys&HidNpadButton_Up)!=0,(keys&HidNpadButton_Down)!=0,
                    (keys&HidNpadButton_Left)!=0,(keys&HidNpadButton_Right)!=0,
                    (keys&HidNpadButton_L)!=0,(keys&HidNpadButton_R)!=0,
                    (keys&HidNpadButton_ZL)!=0,(keys&HidNpadButton_ZR)!=0});
                if(to!=from)bag.moveAddCursor(to-from);
            }
            if(keys&HidNpadButton_B) {
                bag.cancel();
            } else if(keys&HidNpadButton_A) {
                // Shared Gen I-V Add (UI/SharedBagAddFlow.h): stage x1 with no keypad, or
                // focus the existing stack; A on the row edits the quantity afterwards.
                const uint16_t id=bag.chosenAddItem();
                const auto decision=bag.pickAdd(ws,gen5PolicyRule(screen,bag.pocket(),id),
                                                refresh,error);
                screen.controllerNavigation.reset();
                if(!error.empty())screen.postStatus(error,320);
                else screen.postStatus(decision.message+"; SAV5 unchanged",300);
            }
            syncBagCursor(screen);
            return true;
        }
        case BagSurface::ConfirmRemove:
            // Deliberate Y -> Y removal; A never removes and B always cancels.
            if(keys&HidNpadButton_B)bag.cancel();
            else if(keys&HidNpadButton_Y) {
                if(!bag.confirmRemove(ws,refresh,error))screen.postStatus(error,320);
                else screen.postStatus("Gen V item removed from staged bag ONLY; SAV5 unchanged",300);
                screen.controllerNavigation.reset();
            }
            syncBagCursor(screen);
            return true;
        case BagSurface::Browse:break;
    }
    // Shared ItemsPanel hitboxes: pocket arrows and visible-row ids.
    using CategoryTouch=PokeBank::UIModel::InventoryCategoryTouch;
    if(tap==CategoryTouch::PreviousId)keys|=HidNpadButton_L;
    else if(tap==CategoryTouch::NextId)keys|=HidNpadButton_R;
    else if(tap>=0 && bag.tapRow(static_cast<size_t>(tap),ws))keys|=HidNpadButton_A;
    if(keys&HidNpadButton_B) {
        if(screen.itemsShortcutActive) {
            // Opened from Product Home Items: B leaves the session through the
            // same staged-changes confirmation as the Browse exit.
            if(state.requestExit(ws))screen.goBack=true;
            screen.controllerNavigation.reset();
            return true;
        }
        bag.reset();
        screen.detailViewActive=false; // back to the hub with Items focused
        screen.controllerNavigation.reset();
        return true;
    }
    if(keys&HidNpadButton_L)bag.changePocket(-1);
    if(keys&HidNpadButton_R)bag.changePocket(1);
    // Same list conventions as the Gen I-IV Items panel: Up/Down wrap,
    // Left/Right page, a vertical swipe that starts on a row scrolls.
    if(touch.justReleased() && touch.dragged()) {
        const int dy=touch.deltaY();
        const bool startedOnItem=std::any_of(screen.touchButtons.begin(),screen.touchButtons.end(),
            [&](const auto& hit) {
                return hit.id>=0 && hit.id<1000 &&
                       touch.startX()>=hit.x && touch.startX()<hit.x+hit.w &&
                       touch.startY()>=hit.y && touch.startY()<hit.y+hit.h;
            });
        if(startedOnItem && std::abs(dy)>=24 && std::abs(dy)>std::abs(touch.deltaX())) {
            const int steps=std::max(1,std::abs(dy)/52);
            bag.moveRow(dy<0?steps:-steps,ws);
        }
    }
    if(keys&HidNpadButton_Up)bag.stepRow(-1,ws);
    if(keys&HidNpadButton_Down)bag.stepRow(1,ws);
    if(keys&HidNpadButton_Left)bag.stepPage(-1,static_cast<size_t>(bagItemsPerPage()),ws);
    if(keys&HidNpadButton_Right)bag.stepPage(1,static_cast<size_t>(bagItemsPerPage()),ws);
    if(keys&HidNpadButton_Plus) {
        if(!state.openReviewFromBrowse(ws))
            screen.postStatus("No staged Gen V changes to review",220);
        screen.controllerNavigation.reset();
    } else if(keys&HidNpadButton_X) {
        if(!bag.openAdd(ws,error))screen.postStatus(error,300);
        screen.controllerNavigation.reset();
    } else if(keys&HidNpadButton_Y) {
        if(!bag.requestRemove(ws,error))screen.postStatus(error,300);
        screen.controllerNavigation.reset();
    } else if(keys&HidNpadButton_A) {
        if(const auto request=bag.beginQuantity(ws,error)) {
            const auto row=PokeBank::UIModel::BagAdd::decideRow(
                gen5PolicyRule(screen,request->pocket,request->itemId),request->current,
                request->maximum);
            if(row.outcome!=PokeBank::UIModel::BagAdd::RowOutcome::EditQuantity) {
                // Unique items get no quantity editor; nothing is staged.
                screen.postStatus(row.message,260);
                return true;
            }
            const auto r=Utils::promptNumber("Edit "+bagItemLabel(screen,request->itemId),
                static_cast<int>(row.current),static_cast<int>(row.minimum),
                static_cast<int>(row.maximum));
            screen.controllerNavigation.reset();
            if(r.accepted) { // Cancel cannot mutate the stage.
                if(!bag.applyQuantity(ws,*request,r.value,refresh,error))
                    screen.postStatus(error,320);
                else if(r.value!=static_cast<int>(request->current))
                    screen.postStatus("Gen V item quantity staged; SAV5 unchanged",280);
            }
        } else screen.postStatus(error,260);
    }
    syncBagCursor(screen);
    return true;
}
// ---- Native Trainer fields in the shared Trainer panel ----
constexpr int kGen5TrainerRowTouchBase=81800;
constexpr int kGen5DexRowTouchBase=82000;
bool handleTrainerEditor(TrainerViewScreen& screen,u64 keys,int tap) {
    namespace G=PokeVault::Integration::Gen5;
    using Row=G::Gen5TrainerScreen::Row;
    auto& trainer=nativeTrainer(screen);
    auto& ws=trainer.stagedPokemon();
    auto& editor=trainer.trainerScreen();
    auto& state=trainer.sharedScreen();
    const auto refresh=[&](std::string& why) {
        return trainer.refreshStagedPokemonPresentation(why);
    };
    if(editor.dex().open()) {
        // Read-only Pokedex list: browsing never stages or writes anything.
        auto& dex=editor.dex();
        if(tap>=kGen5DexRowTouchBase && tap<kGen5DexRowTouchBase+649)
            dex.move(tap-kGen5DexRowTouchBase-static_cast<int>(dex.cursor()));
        if(keys&HidNpadButton_B) {dex.close();screen.controllerNavigation.reset();return true;}
        if(keys&HidNpadButton_Up)dex.move(-1);
        if(keys&HidNpadButton_Down)dex.move(1);
        if(keys&HidNpadButton_Left)dex.move(-10);
        if(keys&HidNpadButton_Right)dex.move(10);
        if(const auto decoded=G::decodePokedex5(ws.sourceSave())) {
            if(keys&HidNpadButton_L)dex.jumpRegistered(*decoded,-1);
            if(keys&HidNpadButton_R)dex.jumpRegistered(*decoded,1);
        }
        return true;
    }
    if(tap>=kGen5TrainerRowTouchBase &&
       tap<kGen5TrainerRowTouchBase+static_cast<int>(G::Gen5TrainerScreen::kRows.size()) &&
       editor.tapRow(static_cast<size_t>(tap-kGen5TrainerRowTouchBase)))
        keys|=HidNpadButton_A;
    if(keys&HidNpadButton_B) {
        editor.leave(); // focused row is remembered for the next visit
        screen.detailViewActive=false;
        screen.controllerNavigation.reset();
        return true;
    }
    if(keys&HidNpadButton_Up)editor.moveRow(-1);
    if(keys&HidNpadButton_Down)editor.moveRow(1);
    if(keys&HidNpadButton_Plus) {
        if(!state.openReviewFromBrowse(ws))
            screen.postStatus("No staged Gen V changes to review",220);
        screen.controllerNavigation.reset();
        return true;
    }
    if(!(keys&HidNpadButton_A))return true;
    std::string error;
    if(editor.selectedRow()==Row::Pokedex) {
        if(!editor.openPokedex(ws,error))screen.postStatus(error,280);
        screen.controllerNavigation.reset();
        return true;
    }
    if(!editor.beginEdit(ws,error)) {
        screen.postStatus(error,280);
        return true;
    }
    const auto current=ws.trainerPresentation();
    bool staged=false,accepted=false;
    switch(editor.selectedRow()) {
        case Row::Name: {
            const auto r=Utils::promptText("Generation V Trainer Name",
                "1-7 printable ASCII characters",
                G::asciiTrainerName5(current->rawName),G::TrainerLayout5::MaxNameChars);
            if((accepted=r.accepted))staged=editor.applyName(ws,r.text,refresh,error);
            break;
        }
        case Row::Money: {
            const auto r=Utils::promptNumber("Generation V Money",
                static_cast<int>(current->money),0,static_cast<int>(G::TrainerLayout5::MaxMoney));
            if((accepted=r.accepted))
                staged=editor.applyMoney(ws,static_cast<uint32_t>(r.value),refresh,error);
            break;
        }
        case Row::PlayTime: {
            const auto h=Utils::promptNumber("Generation V Play Time - Hours",
                current->hours<=G::TrainerLayout5::MaxHours?current->hours:0,0,G::TrainerLayout5::MaxHours);
            if(!h.accepted)break;
            const auto m=Utils::promptNumber("Generation V Play Time - Minutes",
                current->minutes<=59?current->minutes:0,0,59);
            if(!m.accepted)break;
            const auto sec=Utils::promptNumber("Generation V Play Time - Seconds",
                current->seconds<=59?current->seconds:0,0,59);
            if((accepted=sec.accepted))
                staged=editor.applyPlayTime(ws,static_cast<uint32_t>(h.value),
                    static_cast<uint32_t>(m.value),static_cast<uint32_t>(sec.value),refresh,error);
            break;
        }
        default:break;
    }
    screen.controllerNavigation.reset();
    if(!accepted)return true; // Cancel at any prompt is byte-for-byte inert.
    if(!staged)screen.postStatus(error,320);
    else screen.postStatus("Gen V trainer field staged in app memory; SAV5 unchanged",280);
    return true;
}
void drawTrainerEditor(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    namespace G=PokeVault::Integration::Gen5;
    auto& trainer=nativeTrainer(screen);
    const auto& ws=trainer.stagedPokemon();
    const auto& editor=trainer.trainerScreen();
    if(editor.dex().open()) {
        screen.drawGSCOverlay(fb);
        fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,150));
        constexpr int w=760,h=540,rowH=44,rows=9;
        const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
        fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
        fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
        const auto decoded=G::decodePokedex5(ws.sourceSave());
        fb.drawText(x+24,y+18,"Pokedex - Generation V (read-only)",Colors::Text,TextStyle::Heading);
        fb.drawText(x+24,y+56,decoded
            ? "Seen "+std::to_string(decoded->seen)+" / Caught "+std::to_string(decoded->caught)+
              " of 649  |  native flags, never edited"
            : std::string("Pokedex block unsupported"),Colors::Accent,TextStyle::Caption);
        const int cursor=editor.dex().cursor();
        const int first=std::clamp(cursor-rows/2,0,649-rows);
        for(int r=0;decoded && r<rows;++r) {
            const int i=first+r;
            const int yy=y+92+r*rowH;
            SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x+16,yy,w-32,rowH-6), {i==cursor});  // R2-BUG-012 boxed row
            screen.touchButtons.push_back({kGen5DexRowTouchBase+i,x+16,yy,w-32,rowH-6});
            const auto& e=decoded->entries[static_cast<size_t>(i)];
            fb.drawText(x+32,yy+9,"#"+std::to_string(i+1)+"  "+
                ((e.seen||e.caught)?std::string(Names::getSpeciesName(static_cast<uint16_t>(i+1))):"-----"),
                Colors::Text,TextStyle::Body);
            fb.drawText(x+w-200,yy+11,e.caught?"Caught":e.seen?"Seen":"Unregistered",
                        e.caught?Colors::Accent:Colors::TextDim,TextStyle::Caption);
        }
        std::vector<ControllerHint> hints;
        for(const auto& entry:editor.footer(ws))hints.push_back({entry.glyph,entry.label});
        drawNavBar(fb,hints);
        return;
    }
    // The shared Trainer panel: drawn in the same content rect and HOME-style
    // card as the base Trainer view (no modal). Name / Money / Play Time /
    // Pokedex are cards with a value pill; the read-only identity rows sit
    // below a divider (R2-BUG-003). The page always draws its own footer.
    screen.drawGSCOverlay(fb);
    const int x=LEFT_PANEL_X,y=CONTENT_PANEL_Y;
    const int w=fb.getWidth()-x,h=CONTENT_PANEL_HEIGHT;
    fb.drawFilledRoundedRect(x,y,w,h,16,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,16,Colors::Border,1);
    constexpr int hH=46;
    fb.drawFilledRoundedRect(x,y,w,hH,16,Colors::AccentDim);
    fb.drawFilledRect(x,y+hH-16,w,16,Colors::AccentDim);
    fb.drawText(x+22,y+(hH-fb.lineHeight(TextStyle::Heading))/2,"Trainer",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+170,y+(hH-fb.lineHeight(TextStyle::Caption))/2,
                "Name, Money and Play Time are staged in app memory; the source save is unchanged",
                Colors::TextDim,TextStyle::Caption);
    // R2-BUG-003: the Gen I-IV Trainer hierarchy. Selectable rows first (the staged Name / Money /
    // Play Time edits, and the Pokedex view) as cards with a value pill; a divider; then the
    // read-only identity rows, compact and unselectable, as Gen IV shows them (Gender, Trainer ID,
    // Secret ID as two separate native 16-bit numbers, plus Gen V's Badges). The raw language id is
    // secondary information: one unboxed detail line, by name. Every value is still read from the
    // save; nothing is merged or recomputed. Touch ids stay kGen5TrainerRowTouchBase + kRows index.
    using TRow=G::Gen5TrainerScreen::Row;
    constexpr int cardW=680,rowH=52,rowGap=8,infoH=42,infoGap=6,top=hH+18;
    const int cardX=x+(w-cardW)/2;
    const auto indexOf=[](TRow row) {
        for(size_t i=0;i<G::Gen5TrainerScreen::kRows.size();++i)
            if(G::Gen5TrainerScreen::kRows[i]==row)return i;
        return G::Gen5TrainerScreen::kRows.size();
    };
    int cy=y+top;
    for(const TRow row:{TRow::Name,TRow::Money,TRow::PlayTime,TRow::Pokedex}) {
        const size_t i=indexOf(row);
        const bool sel=editor.open() && i==editor.row();
        fb.drawSoftShadow(cardX,cy,cardW,rowH,14);
        fb.drawFilledRoundedRect(cardX,cy,cardW,rowH,14,Colors::PanelAlt);
        if(sel)fb.drawRoundedRect(cardX,cy,cardW,rowH,14,Colors::FocusBorder,2);
        screen.touchButtons.push_back({kGen5TrainerRowTouchBase+static_cast<int>(i),cardX,cy,cardW,rowH});
        fb.drawText(cardX+24,cy+(rowH-fb.lineHeight(TextStyle::Body))/2,G::Gen5TrainerScreen::label(row),
                    sel?Colors::SelectedText:Colors::TextDim,TextStyle::Body);
        const std::string value=G::Gen5TrainerScreen::value(ws,row);
        int vw=0,vh=0;
        fb.measureText(value,vw,vh,TextStyle::Body);
        const int pillW=vw+44,pillH=36;
        const int px=cardX+cardW-pillW-20,py=cy+(rowH-pillH)/2;
        fb.drawPill(px,py,pillW,pillH,Colors::Background);
        fb.drawText(px+(pillW-vw)/2,py+(pillH-vh)/2,value,Colors::Text,TextStyle::Body);
        cy+=rowH+rowGap;
    }
    cy+=4;
    fb.drawHDivider(cardX+20,cy,cardW-40);
    cy+=14;
    for(const TRow row:{TRow::Gender,TRow::TrainerId,TRow::SecretId,TRow::Badges}) {
        fb.drawFilledRoundedRect(cardX,cy,cardW,infoH,12,Colors::PanelAlt);
        fb.drawText(cardX+24,cy+(infoH-fb.lineHeight(TextStyle::Body))/2,G::Gen5TrainerScreen::label(row),
                    Colors::TextDim,TextStyle::Body);
        const std::string value=G::Gen5TrainerScreen::value(ws,row);
        int vw=0,vh=0;
        fb.measureText(value,vw,vh,TextStyle::Body);
        fb.drawText(cardX+cardW-24-vw,cy+(infoH-vh)/2,value,Colors::Text,TextStyle::Body);
        cy+=infoH+infoGap;
    }
    if(const auto t=ws.trainerPresentation())
        fb.drawText(cardX+24,cy+2,std::string("Game language: ")+Enums::getLanguageName(t->language)+
                    "  (stored id "+std::to_string(t->language)+")",Colors::TextDim,TextStyle::Caption);
    std::vector<ControllerHint> hints;
    for(const auto& entry:editor.footer(ws))hints.push_back({entry.glyph,entry.label});
    drawNavBar(fb,hints);
}
} // namespace

// Clone and Move Pokemon use the SAME box / slot keypads as the Gen IV Clone
// action (Gen I-IV share them), defaulting to the first verified empty slot.
// Cancelling either keypad stages nothing.
void stageCloneOrMoveByKeypad(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    auto& state=trainer.sharedScreen();
    auto& staged=trainer.stagedPokemon();
    const bool move=state.moveDestination();
    const int first=static_cast<int>(state.cloneDestinationIndex());
    const auto box=Utils::promptNumber(move?"Move to Gen V Box":"Clone to Gen V Box",
                                       first/30+1,1,24);
    if(!box.accepted) {state.cancelBoxAction();return;}
    const auto slot=Utils::promptNumber(move?"Move to Slot":"Clone to Slot",first%30+1,1,30);
    if(!slot.accepted) {state.cancelBoxAction();return;}
    const int target=(box.value-1)*30+(slot.value-1);
    state.moveCloneCursor(target-static_cast<int>(state.cloneDestinationIndex()));
    std::string error;
    if(static_cast<int>(state.cloneDestinationIndex())!=target ||
       !state.confirmClone(staged,[&](std::string& why) {
           return trainer.refreshStagedPokemonPresentation(why);
       },error)) {
        state.cancelBoxAction();
        screen.postStatus(error.empty()?"Box "+std::to_string(box.value)+" / Slot "+
            std::to_string(slot.value)+" is not an empty verified Gen V box slot":error,320);
        return;
    }
    screen.postStatus(move?"Gen V Pokemon moved in the staged copy; source SAV5 unchanged"
                          :"Gen V cloned to empty staged box slot; source SAV5 unchanged",260);
}

bool handleInput(TrainerViewScreen& screen,u64 down,u64 held,
                 int stickX,int stickY,const TouchInput& touch) {
    if(!isGen5Source(screen))return false;

    // Refuse all inherited controls on a mismatched Trainer type/family.
    const auto family=screen.trainer.getGameGroup();
    if(family!=Enums::GameVersion::BW && family!=Enums::GameVersion::B2W2) {
        screen.postStatus("Generation V source presentation mismatch",180);
        if(down&HidNpadButton_B)screen.goBack=true;
        return true;
    }
    auto& trainer=nativeTrainer(screen);
    if(trainer.sourceGameId()!=screen.sourceGameId) {
        screen.postStatus("Generation V exact title identity mismatch",180);
        if(down&HidNpadButton_B)screen.goBack=true;
        return true;
    }
    auto& staged=trainer.stagedPokemon();
    auto& state=trainer.sharedScreen();
    if(screen.storageStatusFrames>0)--screen.storageStatusFrames;
    // Consume only Gen V-owned touch hitboxes published by the last frame.
    // Never forward a tap to inherited bank/source/create handlers.
    const int tap=screen.touchedButtonId(touch);
    // Loaded-game hub (same as Gen I-IV). The base hub keeps cursor/focus and
    // help; Gen V intercepts only activation, Exit and Review so that the
    // inherited Bank/Storage and source writers stay unreachable.
    if(state.surface()==Surface::Browse && !screen.detailViewActive) {
        if(screen.helpOverlayActive)return false;
        namespace G=PokeVault::Integration::Gen5;
        const auto decision=G::routeHub5(screen.homeMenuIndex,down&HidNpadButton_A,
                                         down&HidNpadButton_B,down&HidNpadButton_Plus,tap);
        switch(decision.command) {
            case G::HubCommand5::PassToBase:return false;
            case G::HubCommand5::Exit:
                if(state.requestExit(staged))screen.goBack=true;
                return true;
            case G::HubCommand5::Review:
                if(!state.openReviewFromBrowse(staged))
                    screen.postStatus("No staged Gen V changes to review",220);
                return true;
            case G::HubCommand5::StorageRefused:
                screen.homeMenuIndex=decision.tile;
                screen.selectedMode=TrainerViewScreen::ViewMode::Storage;
                screen.postStatus("Legacy Storage (Bank) does not accept Gen V PK5 records in this build. "
                                  "Use Pokemon or Party; the source save is unchanged.",360);
                return true;
            case G::HubCommand5::Enter:break;
        }
        screen.homeMenuIndex=decision.tile;
        screen.details.active=false;
        screen.controllerNavigation.reset();
        switch(static_cast<G::HubTile5>(decision.tile)) {
            case G::HubTile5::Pokemon: {
                const int boxes=static_cast<int>(screen.trainer.getBoxCount());
                const int slots=static_cast<int>(screen.trainer.getSlotsPerBox());
                screen.selectedMode=TrainerViewScreen::ViewMode::Boxes;
                screen.selectedBoxIndex=std::clamp(screen.selectedBoxIndex,0,std::max(0,boxes-1));
                screen.selectedItemIndex=std::clamp(screen.selectedItemIndex,0,std::max(0,slots-1));
                break;
            }
            case G::HubTile5::Party: {
                const int count=static_cast<int>(screen.trainer.getPartySize());
                screen.selectedMode=TrainerViewScreen::ViewMode::Party;
                screen.selectedPartyIndex=std::clamp(screen.selectedPartyIndex,0,std::max(0,count-1));
                break;
            }
            case G::HubTile5::Items:
                screen.selectedMode=TrainerViewScreen::ViewMode::Items;
                trainer.bagScreen().reset();
                syncBagCursor(screen);
                break;
            case G::HubTile5::Trainer: {
                screen.selectedMode=TrainerViewScreen::ViewMode::Trainer;
                std::string error;
                if(!trainer.trainerScreen().enter(staged,error)) {
                    screen.postStatus(error,300);
                    return true; // stay on the hub; nothing is guessed
                }
                break;
            }
            case G::HubTile5::Storage:return true;
        }
        screen.detailViewActive=true;
        return true;
    }
    // View / Edit / Create: the ONE shared editor workspace (Gen IV code path).
    if(workspaceSurface(state.surface())) {
        screen.details.active=false;
        const u64 workspaceKeys=screen.controllerNavigation.apply(
            down,held,stickX,stickY,
            HidNpadButton_Up,HidNpadButton_Down,
            HidNpadButton_Left,HidNpadButton_Right);
        return handleSharedWorkspace(screen,workspaceKeys,touch);
    }
    // A Move carry (shared ClassicPackedMove overlay) owns Y / B / A / X and
    // cell taps; navigation keys fall through to the box grid below.
    if(state.surface()==Surface::Browse && screen.detailViewActive &&
       screen.selectedMode==TrainerViewScreen::ViewMode::Boxes &&
       ClassicPackedMove::gen5CarryActive(screen)) {
        if(ClassicPackedMove::handleGen5CarryInput(screen,down,tap))return true;
    }
    if(state.surface()==Surface::Actions) {
        const auto actions=state.availableActions(staged);
        if(tap>=kGen5ActionTouchBase &&
           tap<kGen5ActionTouchBase+static_cast<int>(actions.count)) {
            const int row=tap-kGen5ActionTouchBase;
            state.moveAction(row-state.actionRow(),staged);
            down|=HidNpadButton_A;
        }

    } else if(state.surface()==Surface::Browse &&
              screen.selectedMode==TrainerViewScreen::ViewMode::Party &&
              tap>=kGen5PartyTouchBase &&
              tap<kGen5PartyTouchBase+static_cast<int>(screen.trainer.getPartySize())) {
        screen.selectedPartyIndex=tap-kGen5PartyTouchBase;
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::Browse &&
              screen.selectedMode==TrainerViewScreen::ViewMode::Boxes) {
        // Decode only the existing box panel's native hit IDs. All other
        // inherited storage, header-rename and source-write hits stay inert.
        const auto boxTouch=PokeVault::Integration::Gen5::decodeGen5BoxTouch(
            tap,screen.selectedItemIndex,
            static_cast<int>(screen.trainer.getSlotsPerBox()));
        using TouchKind=PokeVault::Integration::Gen5::BoxTouchKind5;
        switch(boxTouch.kind) {
            case TouchKind::PreviousBox:down|=HidNpadButton_L;break;
            case TouchKind::NextBox:down|=HidNpadButton_R;break;
            case TouchKind::FocusSlot:
                screen.selectedItemIndex=boxTouch.slot;break;
            case TouchKind::OpenSlot:
                screen.selectedItemIndex=boxTouch.slot;
                down|=HidNpadButton_A;break;
            case TouchKind::OpenSummary:down|=HidNpadButton_A;break;
            case TouchKind::Ignore:break;
        }
    }
    const u64 keys=screen.controllerNavigation.apply(
        down,held,stickX,stickY,
        HidNpadButton_Up,HidNpadButton_Down,
        HidNpadButton_Left,HidNpadButton_Right);

    if(state.surface()==Surface::Browse &&
       screen.selectedMode==TrainerViewScreen::ViewMode::Trainer &&
       trainer.trainerScreen().open()) {
        screen.details.active=false;
        return handleTrainerEditor(screen,keys,tap);
    }
    if(state.surface()==Surface::Browse &&
       screen.selectedMode==TrainerViewScreen::ViewMode::Items) {
        screen.details.active=false;
        return handleBag(screen,keys,tap,touch);
    }

    switch(state.surface()) {
        case Surface::Actions:
            if(keys&HidNpadButton_Up)state.moveAction(-1,staged);
            if(keys&HidNpadButton_Down)state.moveAction(1,staged);
            if(keys&HidNpadButton_B)(void)state.back();
            else if(keys&HidNpadButton_Y) {
                std::string error;
                if(!state.undoSelectedBoxReplacement(staged,
                   [&](std::string& why) {
                       return trainer.refreshStagedPokemonPresentation(why);
                   },error))
                    screen.postStatus(error,300);
                else screen.postStatus(
                    "Selected staged box operation undone; original SAV5 unchanged",260);
                screen.controllerNavigation.reset();
            } else if(keys&HidNpadButton_A) {
                std::string error;
                const auto allowed=state.availableActions(staged);
                const bool move=state.actionRow()>=0 &&
                    static_cast<size_t>(state.actionRow())<allowed.count &&
                    allowed[static_cast<size_t>(state.actionRow())]==
                        PokeBank::UIModel::SharedPokemonEditor::Action::Move;
                if(move) {
                    // Move Pokemon: the SHARED Gen I-IV carry (Round 3 Part 9),
                    // not Box / Slot number keypads. Nothing is staged until Y.
                    const auto from=state.target();
                    if(!state.beginMoveCarry(staged,error))screen.postStatus(error,300);
                    else (void)ClassicPackedMove::beginGen5FromAction(
                        screen,static_cast<int>(from.box),static_cast<int>(from.slot));
                } else if(!state.activateSelected(staged,error))
                    screen.postStatus(error,300);
                else if(state.surface()==Surface::ConfirmCreateBox &&
                        !beginSharedCreate(screen,error)) {
                    state.cancelBoxAction();
                    screen.postStatus(error,360);
                } else if(state.surface()==Surface::CloneDestination)
                    stageCloneOrMoveByKeypad(screen);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::ConfirmCreateBox:
            if(keys&HidNpadButton_B)state.cancelBoxAction();
            else if(keys&(HidNpadButton_A|HidNpadButton_Y)) {
                std::string error;
                if(!beginSharedCreate(screen,error))screen.postStatus(error,360);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::ConfirmRemoveBox:
            if(keys&HidNpadButton_B)state.cancelBoxAction();
            else if(keys&HidNpadButton_Y) {
                std::string error;
                if(!state.confirmRemoveBox(staged,[&](std::string& why) {
                    return trainer.refreshStagedPokemonPresentation(why);
                },error))
                    screen.postStatus(error,320);
                else screen.postStatus(
                    "Generation V Release staged; source SAV5 unchanged",300);
            }
            return true;
        case Surface::Review:
            if(keys&HidNpadButton_B)(void)state.back();
            else if(keys&HidNpadButton_X) {
                if(!state.requestExport(staged))
                    screen.postStatus("Gen V needs verified staged changes to export",300);
            }
            else if(keys&HidNpadButton_Y)state.requestDiscardAll();
            else if(keys&(HidNpadButton_Left|HidNpadButton_Up|
                         HidNpadButton_L))state.moveReviewPage(-1,staged);
            else if(keys&(HidNpadButton_Right|HidNpadButton_Down|
                         HidNpadButton_R))state.moveReviewPage(1,staged);
            return true;
        case Surface::ConfirmExport:
            if(keys&HidNpadButton_B) {
                (void)state.back();
            } else if(keys&HidNpadButton_Y) {
                // Re-read the exact physical source in read-only mode before
                // serializing an app-owned backup and edited working copy.
                // Candidate construction separately rejects ANY baseline
                // byte change, including the unselected retail save copy.
                // The backup is the source FILE exactly as found (a .dsv keeps
                // its metadata and DeSmuME footer); edited.sav stays raw.
                std::vector<uint8_t> observed,observedFile;
                std::string error,container;
                if(!PokeVault::Integration::Gen5::readSourceFileReadOnly(
                       screen.backupDir,observedFile,observed,error,&container)) {
                    screen.postStatus("Gen V export blocked: "+error,400);
                    return true;
                }
                PokeVault::Integration::Gen5::WorkingCopyExportRequest request;
                request.profileIdentity=PokeBank::Paths::accountUidHex(screen.userUid);
                // AccountUid{0,0} is the existing offline "Pokemon Saves"
                // source profile. Use that same explicit identity instead
                // of failing an otherwise validated external save export.
                if(request.profileIdentity.empty())
                    request.profileIdentity=std::string(32,'0');
                request.timestamp=Utils::getTimestamp();
                request.sourcePath=screen.backupDir;
                request.observedSourceBytes=observed;
                request.observedSourceFile=observedFile;
                request.sourceContainer=container;
#ifdef POKEVAULT_GIT_COMMIT
                request.applicationSha=POKEVAULT_GIT_COMMIT;
#endif
                const auto result=
                    PokeVault::Integration::Gen5::publishGen5WorkingCopy(staged,request);
                if(!result.success) {
                    screen.postStatus("Gen V export blocked: "+result.error,480);
                    return true;
                }
                state.finishExport();
                screen.postStatus("Verified app-owned Gen V copy: "+result.directory+
                                  " (original SAV5 unchanged)",480);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::ConfirmDiscardAll:
            if(keys&HidNpadButton_B)(void)state.back();
            else if(keys&HidNpadButton_Y) {
                std::string error;
                if(!state.discardAllWithPresentation(staged,
                    [&](std::string& refreshError) {
                        return trainer.refreshStagedPokemonPresentation(refreshError);
                    },error)) {
                    screen.postStatus(error.empty()
                        ?"Discard rejected; previous staged PK5 records preserved":error,360);
                } else {
                    screen.postStatus("All app-memory PK5 changes discarded; SAV5 unchanged",260);
                }
            }
            return true;
        case Surface::ConfirmExit:
            if(keys&HidNpadButton_B)(void)state.back();
            else if(keys&HidNpadButton_X)state.reviewFromExit(staged);
            else if(keys&HidNpadButton_Y) {
                if(state.confirmDiscardAndExit(staged))screen.goBack=true;
            }
            return true;
        case Surface::CloneDestination:
            state.cancelBoxAction(); // only transient while the keypads are open
            return true;
        case Surface::Legality:
            if(keys&HidNpadButton_B)(void)state.back();
            return true;
        case Surface::View:
        case Surface::Edit:
        case Surface::ConfirmDraft:
            return true; // owned by the shared editor workspace above
        case Surface::Browse:break;
    }

    // The same underlying Party/Boxes renderer is reused inside an entered hub panel.
    // All inherited generic storage/bank/source mutation routes stay barred.
    screen.details.active=false;
    if(keys&HidNpadButton_B) {
        if(PokeVault::Integration::Gen5::panelBack5(screen.itemsShortcutActive)==
           PokeVault::Integration::Gen5::PanelBack5::ExitSession) {
            if(state.requestExit(staged))screen.goBack=true;
        } else screen.detailViewActive=false; // back to the hub, tile still focused
        screen.controllerNavigation.reset();
        return true;
    }
    if(keys&HidNpadButton_Plus) {
        if(!state.openReviewFromBrowse(staged))
            screen.postStatus("No staged Gen V changes to review",220);
        return true;
    }
    if(screen.selectedMode!=TrainerViewScreen::ViewMode::Party &&
       screen.selectedMode!=TrainerViewScreen::ViewMode::Boxes)
        return true;
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        const int count=static_cast<int>(screen.trainer.getPartySize());
        if(count>0) {
            if(keys&(HidNpadButton_Down|HidNpadButton_Right))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+1)%count;
            if(keys&(HidNpadButton_Up|HidNpadButton_Left))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+count-1)%count;
            if(keys&HidNpadButton_A)beginActions(screen);
        }
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes) {
        constexpr int columns=6;
        const int capacity=static_cast<int>(screen.trainer.getSlotsPerBox());
        const int count=static_cast<int>(screen.trainer.getBoxCount());
        if(capacity<=0 || count<=0)return true;
        screen.selectedBoxIndex=std::clamp(screen.selectedBoxIndex,0,count-1);
        screen.selectedItemIndex=std::clamp(screen.selectedItemIndex,0,capacity-1);
        if(keys&HidNpadButton_L)
            screen.selectedBoxIndex=(screen.selectedBoxIndex+count-1)%count;
        if(keys&HidNpadButton_R)
            screen.selectedBoxIndex=(screen.selectedBoxIndex+1)%count;
        if(keys&HidNpadButton_ZL)
            screen.selectedBoxIndex=PokeVault::Integration::Gen5::advanceGen5BoxIndex(
                screen.selectedBoxIndex,count,-5);
        if(keys&HidNpadButton_ZR)
            screen.selectedBoxIndex=PokeVault::Integration::Gen5::advanceGen5BoxIndex(
                screen.selectedBoxIndex,count,5);
        if(keys&HidNpadButton_Up)
            screen.selectedItemIndex=(screen.selectedItemIndex+capacity-columns)%capacity;
        if(keys&HidNpadButton_Down)
            screen.selectedItemIndex=(screen.selectedItemIndex+columns)%capacity;
        if(keys&HidNpadButton_Left)
            screen.selectedItemIndex=(screen.selectedItemIndex+capacity-1)%capacity;
        if(keys&HidNpadButton_Right)
            screen.selectedItemIndex=(screen.selectedItemIndex+1)%capacity;
        if(keys&HidNpadButton_A)beginActions(screen);
    }
    return true;
}

// A narrow passive PK5 panel within the shared Trainer screen. It displays
// only verified record fields; unlike the generic details modal it NEVER
// calls Legality::analyze or synthesizes unknown history/stat calculations.
void dimShared(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,140));
}
// Pokemon Actions: the CURRENT narrow Gen I-IV action menu (same size, spacing,
// focus and labels). Only actions Gen V can perform are listed.
const char* narrowActionLabel5(PokeBank::UIModel::SharedPokemonEditor::Action action) noexcept {
    using A=PokeBank::UIModel::SharedPokemonEditor::Action;
    switch(action) {
        case A::View:return "View";
        case A::Edit:return "Edit";
        case A::Move:return "Move";
        case A::Clone:return "Clone";
        case A::Remove:return "Release";
        case A::LegalityProvenance:return PokeBank::UIModel::NarrowPokemonMenu::kOriginLegalityLabel;
        case A::Add:return "Add Pokemon";
        case A::Review:return "Review Pending Changes";
        case A::Close:return "Cancel";
        default:return "";
    }
}
void drawActions(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                 const ScreenState& state,
                 const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& workspace) {
    screen.drawGSCOverlay(fb);
    const auto choices=state.availableActions(workspace);
    PokeBank::UIModel::NarrowPokemonMenu::Model model{};
    for(size_t i=0;i<choices.count;++i)
        model.append(narrowActionLabel5(choices[i]),static_cast<int>(i));
    const auto& slot=state.target();
    const auto pokemon=slot.region==Region::Party?workspace.viewParty(slot.slot):
        workspace.viewBox(slot.box,slot.slot);
    const std::string title=pokemon && pokemon->valid() && !pokemon->empty()
        ? std::string(Names::getSpeciesName(pokemon->species())) : "Empty Slot";
    drawNarrowPokemonActions(fb,screen.touchButtons,model,state.actionRow(),title,
                             screen.legacyProviderLabel()+" NDS SOURCE / READ ONLY",
                             kGen5ActionTouchBase);
    SharedEditorChrome::Presentation::PickerFooterSpec footer;
    footer.page=false;
    if(state.selectedBoxUndoAvailable(workspace))footer.yLabel="Undo This Slot";
    SharedEditorChrome::drawPickerFooter(fb,footer);
}
const char* reviewedStatName(size_t stat) noexcept {
    constexpr const char* names[]={"HP","Atk","Def","Spe","SpA","SpD"};
    return stat<6?names[stat]:"?";
}
std::string reviewedFieldLabel(const PokeVault::Integration::Gen5::VerifiedFieldChange& c) {
    using Field=PokeVault::Integration::Gen5::StagedPokemon5Record::Field;
    if(c.field==Field::NicknameText)return "Nickname changed";
    if(c.field==Field::OriginalTrainerText)return "OT Name changed";
    std::string name;
    switch(c.field) {
        case Field::Nature:name="Nature";break;
        case Field::Level:name="Level";break;
        case Field::Experience:name="Experience";break;
        case Field::Species:name="Species";break;
        case Field::CreateBox:name="Create in Box";break;
        case Field::CloneBox:name="Clone to Box";break;
        case Field::RemoveBox:name="Remove from Box";break;
        case Field::NicknameText:name="Nickname";break;
        case Field::OriginalTrainerText:name="OT Name";break;
        case Field::Friendship:name="Friendship";break;
        case Field::IV:name=std::string("IV ")+reviewedStatName(c.stat);break;
        case Field::EV:name=std::string("EV ")+reviewedStatName(c.stat);break;
        case Field::HeldItem:name="Held Item";break;
        case Field::Ball:name="Ball";break;
        case Field::MetLevel:name="Met Level";break;
        case Field::TrainerId:name="Trainer ID";break;
        case Field::SecretId:name="Secret ID";break;
        case Field::OriginalTrainerGender:name="OT Gender";break;
        case Field::Pokerus:name="Pokerus";break;
        case Field::MetLocation:name="Met Location";break;
        case Field::FatefulEncounter:name="Fateful";break;
        case Field::Form:name="Form";break;
        case Field::AbilitySlot:name="Ability Slot";break;
        case Field::Gender:name="Gender";break;
        case Field::Move:name="Move "+std::to_string(c.stat+1);break;
        case Field::MovePPUps:name="PP Ups "+std::to_string(c.stat+1);break;
        case Field::MovePP:name="PP "+std::to_string(c.stat+1);break;
        default:name="Unsupported";break;
    }
    const auto display = [&](uint32_t value) -> std::string {
        switch (c.field) {
            case Field::Move: return Names::getMoveName(static_cast<uint16_t>(value));
            case Field::HeldItem: return Names::getItemName(static_cast<uint16_t>(value));
            case Field::Species: return Names::getSpeciesName(static_cast<uint16_t>(value));
            case Field::Nature: return Trainer::getNatureName(static_cast<uint8_t>(value));
            case Field::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
            default: return std::to_string(value);
        }
    };
    return name+": "+display(c.before)+" -> "+display(c.after);
}
void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                const ScreenState& state,
                const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    dimShared(screen,fb);
    constexpr int x=150,y=59,w=980,h=534;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+24,y+18,"Review Pending Changes - Generation V",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+57,
        "Verified app-memory PK5 fields and native Bag slots; original SAV5 unchanged",
        Colors::Accent,TextStyle::Caption);
    const auto pending=ws.pendingReview();
    // A staged Move is ONE row ("Move"), not its Clone + Remove halves (Gen5ReviewRows.h).
    const auto records=PokeVault::Integration::Gen5::groupReviewRows5(pending);
    const auto bagChanges=ws.bagReviewChanges();
    const auto trainerChanges=ws.trainerReviewChanges();
    const size_t total=records.size()+bagChanges.size()+trainerChanges.size();
    constexpr size_t pageSize=ScreenState::kReviewPageSize;
    const size_t pages=std::max(size_t{1},(total+pageSize-1)/pageSize);
    const size_t page=std::min(state.reviewPage(),pages-1);
    fb.drawText(x+24,y+94,std::to_string(records.size())+" staged Pokemon  /  "+
        std::to_string(bagChanges.size())+" Bag  /  "+std::to_string(trainerChanges.size())+
        " Trainer  /  Page "+std::to_string(page+1)+
        " of "+std::to_string(pages),Colors::Text,TextStyle::Body);
    for(size_t row=0;row<pageSize;++row) {
        const size_t i=page*pageSize+row;
        if(i>=total)break;
        if(i>=records.size()+bagChanges.size()) {
            const auto& c=trainerChanges[i-records.size()-bagChanges.size()];
            using TField=PokeVault::Integration::Gen5::TrainerChange5::Field;
            const int yy=y+133+static_cast<int>(row)*91;
            fb.drawFilledRoundedRect(x+18,yy,w-36,83,8,Colors::PanelAlt);
            fb.drawText(x+32,yy+8,std::string("Trainer / ")+
                (c.field==TField::Name?"Name":c.field==TField::Money?"Money":"Play Time"),
                Colors::Text,TextStyle::Body);
            fb.drawText(x+32,yy+43,c.before+" -> "+c.after,Colors::TextSecondary,TextStyle::Caption);
            continue;
        }
        if(i>=records.size()) {
            const auto& c=bagChanges[i-records.size()];
            const int yy=y+133+static_cast<int>(row)*91;
            fb.drawFilledRoundedRect(x+18,yy,w-36,83,8,Colors::PanelAlt);
            fb.drawText(x+32,yy+8,std::string("Bag / ")+
                PokeVault::Integration::Gen5::bagPocketName5(c.pocket)+"  -  "+
                bagItemLabel(screen,c.itemId),Colors::Text,TextStyle::Body);
            const std::string what=c.before==0?"Added x"+std::to_string(c.after):
                c.after==0?"Removed (was x"+std::to_string(c.before)+")":
                "Quantity: "+std::to_string(c.before)+" -> "+std::to_string(c.after);
            fb.drawText(x+32,yy+43,what,Colors::TextSecondary,TextStyle::Caption);
            continue;
        }
        const auto& record=records[i];
        const auto& change=pending[record.change];
        const auto& loc=change.location;
        const int yy=y+133+static_cast<int>(row)*91;
        if(record.kind==PokeVault::Integration::Gen5::ReviewRow5::Kind::Move) {
            const auto& removal=pending[record.removal];
            const auto boxLabel=[](const auto& s) {
                return "Box "+std::to_string(s.box+1)+" / Slot "+std::to_string(s.slot+1);
            };
            fb.drawFilledRoundedRect(x+18,yy,w-36,83,8,Colors::PanelAlt);
            fb.drawText(x+32,yy+8,"Move  "+boxLabel(removal.location)+"  ->  "+boxLabel(loc),
                        Colors::Text,TextStyle::Body);
            std::string error;
            const auto moved=PokeVault::Integration::Gen5::verifyStagedReview(change,&error);
            const auto vacated=PokeVault::Integration::Gen5::verifyStagedReview(removal,&error);
            uint16_t species=0;
            if(moved)
                for(const auto& f:moved->fields)
                    if(f.field==PokeVault::Integration::Gen5::StagedPokemon5Record::Field::CloneBox)
                        species=static_cast<uint16_t>(f.after);
            fb.drawText(x+32,yy+43,
                moved && vacated
                    ? std::string(Names::getSpeciesName(species))+" moved; the original slot is now empty"
                    : std::string("Unverified staged difference - no legality conclusion"),
                moved && vacated ? Colors::TextSecondary : Colors::Warning,TextStyle::Caption);
            continue;
        }
        const std::string label=loc.region==Region::Party
            ? "Party slot "+std::to_string(loc.slot+1)
            : "Box "+std::to_string(loc.box+1)+" / Slot "+std::to_string(loc.slot+1);
        fb.drawFilledRoundedRect(x+18,yy,w-36,83,8,Colors::PanelAlt);
        fb.drawText(x+32,yy+8,label,Colors::Text,TextStyle::Body);
        std::string error;
        const auto verified=PokeVault::Integration::Gen5::verifyStagedReview(change,&error);
        if(!verified) {
            fb.drawText(x+32,yy+42,
                "Unverified staged difference - no legality conclusion",
                Colors::Warning,TextStyle::Caption);
            continue;
        }
        std::string line;
        for(size_t field=0;field<verified->fields.size() && field<3;++field) {
            if(!line.empty())line+="  |  ";
            line+=reviewedFieldLabel(verified->fields[field]);
        }
        if(verified->fields.size()>3)
            line+="  |  +"+std::to_string(verified->fields.size()-3)+" more";
        fb.drawText(x+32,yy+43,line,Colors::TextSecondary,TextStyle::Caption);
    }
    fb.drawText(x+24,y+h-31,"Box clone/remove staged only; no source writes or inferred legality",
                Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"L/R","Review Pages"},{"X","Export Working Copy"},
                   {"Y","Discard All..."},{"B","Back"}});
}
// Origin & Legality (Round 3 Part 9): the neutral Gen I-IV panel. It reports the
// PK5 structure and the record's stored origin only, and never a legality
// verdict: Gen V has no legality checker here, so Legality is "Incomplete"
// and the encounter check is "Unknown".
void drawLegality5(TrainerViewScreen& screen,PKSEFramebuffer& fb,const ScreenState& state,
                   const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& workspace) {
    namespace P=PokeBank::UIModel::SharedEditorPresentation;
    dimShared(screen,fb);
    SharedEditorChrome::beginModal(screen.touchButtons);
    constexpr int x=210,y=82,w=860,h=500;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Divider,1);
    fb.drawText(x+24,y+18,P::kOriginLegalityHeading,Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+52,"PK5 structure checked • no full legality checker",
                Colors::TextDim,TextStyle::Caption);
    const auto& slot=state.target();
    const auto pk=slot.region==Region::Party?workspace.viewParty(slot.slot):
        workspace.viewBox(slot.box,slot.slot);
    if(!pk || !pk->valid() || pk->empty()) {
        fb.drawText(x+34,y+110,"No valid Pokemon selected",Colors::Warning,TextStyle::Body);
        drawNavBar(fb,{{"B","Back"}});
        return;
    }
    const uint8_t origin=pk->originVersion();
    const std::string originName=origin==20?"White":origin==21?"Black":
        origin==22?"White 2":origin==23?"Black 2":"Other (version id "+std::to_string(origin)+")";
    const auto date=pk->metDate();
    const std::pair<std::string,std::string> rows[]={
        {"Exact source",gen5GameName(screen.sourceGameId)},
        {"PK5 structure","PASS"},
        {"Source mutation","HARD DISABLED"},
        {"Origin game",originName},
        {"Met location",metLocationText5(origin,pk->metLocation())},
        {"Met level / date",std::to_string(pk->metLevel())+"  /  "+std::to_string(date.year)+"-"+
            std::to_string(date.month)+"-"+std::to_string(date.day)},
        {"Fateful encounter",pk->fatefulEncounter()?"Yes":"No"},
        {"Encounter check",P::legalityLabel(P::LegalityState::Unknown)},
        {"Legality",P::legalityLabel(P::LegalityState::Incomplete)},
    };
    int rowY=y+96;
    for(const auto& [label,value]:rows) {
        fb.drawText(x+34,rowY,label,Colors::TextDim);
        const bool flag=label=="Encounter check" || label=="Legality";
        fb.drawText(x+290,rowY,value,label=="PK5 structure"?Colors::Success:
                    label=="Source mutation"?Colors::Accent:flag?Colors::Warning:Colors::Text);
        rowY+=40;
    }
    drawNavBar(fb,{{"B","Back"}});
}
void drawDecision(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                  std::string title,std::string message,
                  const std::vector<ControllerHint>& keys) {
    dimShared(screen,fb);
    // The decision owns the frame's touch; the covered hub's footer is never tappable.
    SharedEditorChrome::beginModal(screen.touchButtons);
    const int w=870,h=250,x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+26,y+24,title,Colors::Text,TextStyle::Heading);
    fb.drawText(x+26,y+93,message,Colors::TextSecondary,TextStyle::Body);
    fb.drawText(x+26,y+142,"Source SAV5 is never modified.",
                Colors::Accent,TextStyle::Caption);
    // R2-BUG-012: each choice is its own boxed row; a tap presses exactly that button.
    std::vector<SharedBoxedRows::GlyphChoice> choices;
    for(const auto& key:keys)choices.push_back({key.button,key.label});
    SharedBoxedRows::drawGlyphChoices(fb,x+26,y+h-SharedBoxedRows::kChoiceHeight-18,w-52,choices);
    if(!keys.empty())drawNavBar(fb,keys);
}
void drawBag(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    namespace G=PokeVault::Integration::Gen5;
    using BagSurface=G::Gen5BagScreen::Surface;
    auto& trainer=nativeTrainer(screen);
    const auto& ws=trainer.stagedPokemon();
    const auto& bag=trainer.bagScreen();
    syncBagCursor(screen);
    screen.drawGSCOverlay(fb); // The shared Items panel with Gen V pockets.
    std::vector<ControllerHint> hints;
    for(const auto& entry:bag.footer(ws))hints.push_back({entry.glyph,entry.label});
    if(bag.surface()==BagSurface::Browse) {
        // A glyph-less segment is a state marker (accent text). The vector overload drops hints
        // without a button, so serialize it here or "view only" would never be shown.
        std::string serialized;
        for(const auto& hint:hints) {
            if(hint.button.empty() || hint.label.empty()) continue;
            if(!serialized.empty()) serialized+="  |  ";
            serialized+=hint.button+": "+hint.label;
        }
        if(!G::Gen5BagScreen::mutablePocket(bag.pocket()) &&
           G::Gen5BagScreen::available(ws))
            serialized+="  |  Key Items/TMs view only";
        drawNavBar(fb,serialized);
        return;
    }
    const auto game=screen.trainer.getGameGroup();
    if(bag.surface()==BagSurface::AddPicker) {
        const auto& choices=bag.addChoices();
        const int count=static_cast<int>(choices.size());
        const int selected=static_cast<int>(bag.addIndex());
        UI::SharedBagAddPicker::drawPicker(fb,screen.touchButtons,kGen5BagAddTouchBase,game,
            "Add Item",std::string(G::bagPocketName5(bag.pocket()))+"  |  released items only",
            count,selected,
            [&](int index){return bagItemLabel(screen,choices[static_cast<size_t>(index)]);},
            [&](int index){return choices[static_cast<size_t>(index)];},
            [&](int index) {
                const auto rows=ws.bagPresentation();
                const auto id=choices[static_cast<size_t>(index)];
                if(!rows || bag.pocket()>=rows->size())return uint16_t{0};
                for(const auto& stack:(*rows)[bag.pocket()])if(stack.itemId==id)return stack.count;
                return uint16_t{0};
            },
            // R2-BUG-005: the move a focused Gen V TM/HM teaches, from the machine table
            // (Names::getMachineDescriptor; Lane I fills BW/B2W2 there).
            [&](int index){return Names::getMachineDescriptor(game,choices[static_cast<size_t>(index)]).moveId;},
            editorStateFor(screen).bagAddScrollbar);
    } else {
        const auto& item=bag.pendingRemove();
        UI::SharedBagAddPicker::drawRemoveConfirm(fb,screen.touchButtons,game,"Remove Gen V Item?",
            item?item->itemId:0,item?bagItemLabel(screen,item->itemId):std::string(),
            item?item->count:0,"Staged bag only; Discard All in Review restores it.");
        return; // drawRemoveConfirm draws its own Y Remove / B Cancel footer.
    }
    drawNavBar(fb,hints);
}
bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!isGen5Source(screen))return false;
    auto& trainer=nativeTrainer(screen);
    auto& state=trainer.sharedScreen();
    const auto& staged=trainer.stagedPokemon();
    // The inherited modal unconditionally invokes an unsupported Gen V
    // legality analyzer; the shared Gen V adapter never opens that modal.
    screen.details.active=false;
    switch(state.surface()) {
        case Surface::Actions:drawActions(screen,fb,state,staged);return true;
        case Surface::CloneDestination:
            // Transient while the Gen IV box / slot keypads are open.
            dimShared(screen,fb);return true;
        case Surface::Review:drawReview(screen,fb,state,staged);return true;
        case Surface::Legality:drawLegality5(screen,fb,state,staged);return true;
        case Surface::Edit:
        case Surface::View:
        case Surface::ConfirmDraft:
            // The ONE shared editor workspace, including its unsaved-changes dialog.
            drawSharedWorkspace(screen,fb);return true;
        case Surface::ConfirmCreateBox:
            drawDecision(screen,fb,"Create A Pokemon In This Empty Box Slot?",
                "Opens the shared editor. Nothing is added until Keep; SAV5 untouched.",
                {{"A","Create"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmRemoveBox:
            // The Gen IV Release dialog: same title, box/slot line and Y confirm.
            drawDecision(screen,fb,"Release this Pokemon?",
                "Box "+std::to_string(state.target().box+1)+" / Slot "+
                    std::to_string(state.target().slot+1)+
                    ": removed only from the app-owned staged copy.",
                {{"Y","Confirm Release"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmExport:
            drawDecision(screen,fb,"Export Verified Generation V Working Copy?",
                "Creates an untouched backup + edited SAV5 under PokeBank NX/exports/gen5.",
                {{"Y","Export Copy"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmDiscardAll:
            drawDecision(screen,fb,"Discard All Staged Changes?",
                "Y permanently clears app-memory edits; SAV5 stays unchanged.",
                {{"Y","Confirm Discard"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmExit:
            drawDecision(screen,fb,"Staged Changes Remain In Memory",
                "Exit discards in-memory edits. Exported working copies remain safe.",
                {{"Y","Discard & Exit"},{"X","Review"},{"B","Continue"}});
            return true;
        case Surface::Browse:break;
    }
    if(!screen.detailViewActive) {
        // The same loaded-game hub as Gen I-IV, drawn by the base renderer.
        screen.drawGSCOverlay(fb);
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Items) {
        drawBag(screen,fb);
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Trainer) {
        drawTrainerEditor(screen,fb);
        return true;
    }
    screen.drawGSCOverlay(fb);
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        // Match the established two-column Party panel hit geometry, but
        // route through Gen V's own guarded Actions instead of the base sheet.
        constexpr int x=LEFT_PANEL_X,y=CONTENT_PANEL_Y;
        constexpr int gutter=16,slotGap=12;
        const int width=fb.getWidth()-x;
        const int top=y+58;
        const int colW=(width-3*gutter)/2;
        const int slotH=(CONTENT_PANEL_HEIGHT-(top-y)-2*slotGap-gutter)/3;
        const int size=static_cast<int>(screen.trainer.getPartySize());
        for(int i=0;i<std::min(size,6);++i) {
            const int column=i>=3?1:0;
            const int row=i>=3?i-3:i;
            screen.touchButtons.push_back({
                kGen5PartyTouchBase+i,
                x+gutter+column*(colW+gutter),
                top+row*(slotH+slotGap),colW,slotH});
        }
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes &&
       ClassicPackedMove::gen5CarryActive(screen))
        ClassicPackedMove::draw(screen,fb); // carried sprite + "Y Drop / L/R Box / B Cancel"
    else if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes)
        drawNavBar(fb,{{"D-pad","Navigate"},{"L/R","Box"},
                       {"ZL/ZR","Jump 5"},{"A","Actions"},{"+","Review"},{"B","Back"}});
    else
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"A","Actions"},
                       {"+","Review"},{"B","Back"}});
    return true;
}
} // namespace UI::Gen5SharedReadOnlySurface
