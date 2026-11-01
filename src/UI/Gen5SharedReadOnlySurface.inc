// Generation V's guarded adapter for the EXISTING shared Trainer/Party/Boxes/
// Items renderer. View, verified PK5 staged Edit/Review, shared empty-box Add
// and native Bag edits are in app memory only. Gen V never calls the inherited
// source-save writer, Storage/Bank or generic uncontrolled mutation handlers.
// Touch targets are isolated to verified read-only and native-field staged
// actions; native bank, source-writing and unsupported mutation routes remain barred.
#include "Inventory/ItemQuantityPolicy.h"
#include "Integration/Gen1/Gen12CreateReview.h"
#include "UI/SharedBagAddPicker.h"
#include "UI/SharedBagTopUp.h"
#include "Inventory/BulkItemAdaptersNds.h"
#include "Names/LocationDisplayNames.h"
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Integration/Gen5/Gen5BoxTouchNavigation.h"
#include "UI/ReviewEntry.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "UI/ClassicStagedExit.h"
#include "UI/ClassicTrainerSession.h"
#include "UI/SharedTrainerPanel.h"
#include "Integration/Gen5/Gen5HubRouting.h"
#include "Integration/Gen5/Gen5DisplayStats.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5PersonalTypes.h"
#include "Integration/Gen5/Gen5Progression.h"
#include "UI/SharedPokemonShell.h"
#include "UI/Gen5LanguageCycle.h"
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
PokeBank::UIModel::SharedReview::Rows sharedReviewRows(TrainerViewScreen& screen,
    const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws);
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
// Round 11 X-IMPEXP: X on the Boxes grid exports an occupied slot / imports into an empty one,
// exactly when the A sheet lists that file row (Gen5SharedEditorBridge::fileCapabilities). A carry
// keeps priority: handleGen5CarryInput refuses X while a Pokemon is held.
namespace Bridge5=PokeVault::Integration::Gen5;
std::optional<Bridge5::SharedEditorSlot> boxFileSlot(TrainerViewScreen& screen,
                                                     const Bridge5::Gen5StagedPokemonWorkspace& staged) {
    if(screen.selectedMode!=TrainerViewScreen::ViewMode::Boxes)return std::nullopt;
    return Bridge5::selectedSlot(staged,selectedNativeSlot(screen));
}
const char* boxFileLabel(TrainerViewScreen& screen,const Bridge5::Gen5StagedPokemonWorkspace& staged) {
    const auto slot=boxFileSlot(screen,staged);
    if(!slot)return nullptr;
    return PokeBank::UIModel::SharedPokemonEditor::boxXLabel(slot->occupied,Bridge5::fileCapabilities(staged,*slot));
}
void boxFileShortcut(TrainerViewScreen& screen,const Bridge5::Gen5StagedPokemonWorkspace& staged) {
    namespace Shared=PokeBank::UIModel::SharedPokemonEditor;
    const auto slot=boxFileSlot(screen,staged);
    if(!slot)return;
    const auto activation=Shared::boxActivation(slot->occupied,false,true,Bridge5::fileCapabilities(staged,*slot));
    if(activation==Shared::BoxActivation::ImportFile)
        PokemonFileMenus::beginImport(screen,5,screen.selectedBoxIndex,screen.selectedItemIndex);
    else if(activation==Shared::BoxActivation::ExportFile)
        PokemonFileMenus::exportPokemon(screen,5,false,screen.selectedBoxIndex,screen.selectedItemIndex);
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

// A PID-changing edit (Ability slot, TID/SID keeping shininess, Species with a
// shiny choice) waits here until the player confirms a dialog that states the
// PID change. Gender (owner Oct 28 #6) and Shiny (Round 11 H29) are inline
// toggles instead, behind the same strict re-verification (verifiedPidProbe);
// Review lists every PID change ("PID: 0x.. -> 0x..").
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
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    bool workspaceOpen = false;
    PendingPidChange5 pid;
    SharedScrollbar::State bagAddScrollbar;   // R2-SCROLLBAR: draggable thumb on the Add Item list
    UI::SharedBagTopUp::Ui bagTopUp;          // Top Up / Add Missing (owner Oct 28, Feature B): ZR
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
    if ((state.owner != &screen || state.ownerSerial != screen.instanceSerial)) {
        state = {};
        state.owner = &screen; state.ownerSerial = screen.instanceSerial;
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
    static constexpr int kGeneration = 5;   // ReviewEntry's button rule
    static constexpr int kDetailRows = Fields::RowCount;
    static constexpr int kMaxSpecies = Fields::kMaxSpecies;
    static constexpr bool kCompactForm = true;
    static constexpr const char* kDetailReadOnly =
        "Origin is read-only for a Generation V record in this editor";
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
    // Round 6: + opens the shared Review; B there returns to this workspace (reviewReturn).
    static void openReview(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        if (!backend(screen).openReviewFrom(nativeTrainer(screen).stagedPokemon()))
            screen.postStatus("No staged Gen V changes to review", 220);
        screen.controllerNavigation.reset();
    }

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
    // An inline PID-row toggle (Gender, Shiny) applies its probe only when it
    // passes the same strict re-verification the "Change PID?" dialog's confirm
    // uses; otherwise it says so and the draft is untouched. The caller assigns.
    static std::optional<Record> verifiedPidProbe(TrainerViewScreen& screen,
                                                  const PokeVault::Integration::Gen5::Gen5SharedPokemonSession& probe) {
        auto after = probe.current();
        if (!after || !pidLinksHold(*after, b2w2(screen))) {
            screen.postStatus("The new PID failed strict re-verification; nothing was changed", 320);
            return std::nullopt;
        }
        return after;
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
            case Species: return std::string(Names::getSpeciesName(p.species())) + " #" + std::to_string(p.species());
            case Nickname: case OriginalTrainer: {
                // Round 11 H27: the name as stored, in the record's language (display only).
                const auto name = PokeVault::Integration::Gen5::displayName5(p.decodedBytes(), row == Nickname);
                return name ? *name : std::string("(unreadable)");
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
            case MetLocation:
                // Owner item 4: a new Pokemon's location is not chosen until the user picks one.
                if (backend(screen).createNeedsMetLocation()) return "Choose a location";
                return metLocationText5(p.originVersion(), p.metLocation());
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
                // The ASCII write gate seeds the prompt: a stored Japanese / Korean / accented name is
                // never re-encoded. It can only be replaced, and the guide says so (Round 11 H27).
                const auto current = PokeVault::Integration::Gen5::asciiName5(p.decodedBytes(), nickname);
                const char* guide = current ? (nickname ? "1-10 ASCII characters" : "1-7 ASCII characters")
                                            : (nickname ? "Native name: replace with 1-10 ASCII characters"
                                                        : "Native name: replace with 1-7 ASCII characters");
                const auto r = Utils::promptText(nickname ? "Nickname" : "Original Trainer", guide,
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
            case Language: {
                // A/touch cycles native PK5 language IDs inline, without a modal.
                // The existing guarded staging path maintains encryption/checksum,
                // Review/Discard and source-save immutability. The changed value is the
                // feedback: no status toast (owner Oct 25 7C, matching Gen III Language).
                stage(screen, Field::Language, 0, PokeBank::UIModel::Gen5LanguageCycle::next(p.language()));
                return true;
            }
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
                EditorSurface::openContextualMetLocationPicker(screen, state, p.originVersion(),
                    p.species(), p.metLocation(), Fields::metLocationChoices(),
                    {p.eggLocation(), p.egg(), p.fatefulEncounter()});
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
    // True when this generation has its own View message for the row.
    static bool passiveActivate(TrainerViewScreen& screen, EditorSurface::EditorState&, int row) {
        const auto p = draft(screen).current();
        if (row == Fields::Form && p && singleForm(screen, *p)) {
            screen.postStatus("This Gen V Pokémon has only its default form", 135);
            return true;
        }
        return false;
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
    // Fields::pidRowEditable without the mode gate: rows Edit cannot change for this record either.
    static bool pidRowLocked(const TrainerViewScreen& screen, const EditorSurface::EditorState&,
                             int row, const Record& p) {
        if (Fields::earlierOrigin(p)) return true;
        const auto personal = PokeVault::Integration::Gen5::personal5(p.species(), p.form(), b2w2(screen));
        if (!personal) return true;
        switch (row) {
            case Fields::NatureRow: case Fields::ShinyRow: return false;
            case Fields::GenderRow: return !PokeVault::Integration::Gen5::variableGenderRatio5(personal.info->genderRatio);
            case Fields::AbilityRow: return Fields::abilityChoices(p, b2w2(screen)).size() <= 1;
            default: return true;
        }
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
            // Owner Oct 28 (#6): Gender is an inline Male / Female toggle, as on Gen IV; the PID it
            // changes is stated inline instead of in a separate "Change PID?" dialog. The probe is
            // applied only when it passes the same strict re-verification that dialog used.
            auto probe = d;
            if (!Fields::toggleGender(probe, error)) {
                screen.postStatus(error, 300);
            } else if (const auto after = verifiedPidProbe(screen, probe)) {
                const std::string note = std::string("Gender ") + EditorSurface::genderText(p.gender()) + " -> " +
                    EditorSurface::genderText(after->gender()) + "  |  PID " + pidHex5(p.pid()) + " -> " +
                    pidHex5(after->pid()) + " (nature, shininess and ability kept)";
                d = std::move(probe);
                screen.postStatus(note, 420);
            }
        } else if (row == Fields::ShinyRow) {
            // Round 11 H29 (owner: "inline like every other gen (1-4)"): Shiny cycles in place like
            // Gen I-IV and the Gender row above; no "Change PID?" dialog. Field::Shiny (withShiny5)
            // rewrites only the PID's high half, so gender, nature and the ability slot are kept,
            // and the probe passes the same strict re-verification as Gender before it is applied.
            // A routine edit (H22, UI/EditFeedback.h): the Shiny value, sprite and PID row are the
            // feedback; Review lists the PID change.
            namespace EditFeedback = PokeBank::UIModel::EditFeedback;
            auto probe = d;
            if (!Fields::toggleShiny(probe, error)) {
                screen.postStatus(error, 300);
            } else if (const auto after = verifiedPidProbe(screen, probe)) {
                if (after->shiny() == p.shiny()) {
                    screen.postStatus("Shiny could not change while keeping this PID's traits; nothing was changed", 320);
                } else {
                    d = std::move(probe);
                    EditFeedback::applied(screen, EditFeedback::Kind::Shiny);
                }
            }
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
    static std::vector<uint16_t> moveChoices(TrainerViewScreen& screen, EditorSurface::EditorState&) {
        const auto p = draft(screen).current();
        return p ? Fields::moveChoices(screen.sourceGameId,p->species(),p->form(),p->moves())
                 : std::vector<uint16_t>{0};
    }
    static void moveStatus(TrainerViewScreen& screen, const EditorSurface::EditorState&,
                           const Record& raw, uint16_t move, const char*& status, Color& color) {
        namespace C = PokeVault::Integration::Gen5MoveCompatibility;
        const auto current = draft(screen).current();
        const auto currentMoves = current ? current->moves() : std::array<uint16_t,4>{};
        const bool existing = std::find(currentMoves.begin(),currentMoves.end(),move) != currentMoves.end();
        const auto availability = C::classify(screen.sourceGameId,raw.species(),raw.form(),move,existing);
        status = move==0 ? "-" : "Unknown";
        color = move==0 ? Colors::TextDim : Colors::Error;
        if(availability==C::Availability::Direct) {
            status = move==0 ? "-" : "OK";   // owner Oct 25 7B: the Gen I-IV treatment
            color = move==0 ? Colors::TextDim : Colors::Success;
        } else if(availability==C::Availability::Transfer) {
            status = "Transfer";
            color = Colors::Warning;
        } else if(availability==C::Availability::FormHistory) {
            status = "Prior form";
            color = Colors::Warning;
        } else if(availability==C::Availability::Preserved) {
            status = "Preserved";
            color = Colors::Warning;
        }
    }
    // Black/White or Black 2/White 2 for the shared Moves panel and chooser facts.
    static Enums::GameVersion moveGame(const TrainerViewScreen& screen) {
        return b2w2(screen) ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;
    }
    static std::string pickerLabel(TrainerViewScreen& screen, const EditorSurface::EditorState& state,
                                   PickerTarget target, uint16_t value) {
        const auto p = draft(screen).current();
        switch (target) {
            case PickerTarget::Species: return EditorSurface::dexLabel(value);
            case PickerTarget::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
            case PickerTarget::HeldItem:
                return value == 0 ? "(none)" :
                    Names::machineDisplayLabel(screen.trainer.getGameGroup(), value, Names::getItemName(value));
            case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
            case PickerTarget::MetLocation: {
                std::string label = Names::getMetLocationChoiceName(p ? p->originVersion() : 20, value);   // P5
                if (state.pickerMetContextActive)
                    label += "  |  " + state.pickerMetContext.detail(value,
                        [](uint16_t species) { return Names::getSpeciesName(species); });
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
    static void metLocationAccepted(TrainerViewScreen& screen) { backend(screen).noteCreateMetLocationChosen(); }
    static bool applyPicker(TrainerViewScreen& screen, EditorSurface::EditorState& state,
                            PickerTarget target, uint16_t value) {
        auto& d = draft(screen);
        const auto p = d.current();
        if (!p) return false;
        switch (target) {
            case PickerTarget::Species: return applySpecies(screen, state, *p, value);
            case PickerTarget::HeldItem: return stage(screen, Field::HeldItem, 0, value);
            case PickerTarget::Language: return stage(screen, Field::Language, 0, value);
            case PickerTarget::Ball: return stage(screen, Field::Ball, 0, value);
            case PickerTarget::MetLocation: {
                // Picking a location (even the placeholder's own value) is the Create choice Keep needs.
                if (p && p->metLocation() == value) { backend(screen).noteCreateMetLocationChosen(); return true; }
                if (!stage(screen, Field::MetLocation, 0, value)) return false;
                backend(screen).noteCreateMetLocationChosen();
                return true;
            }
            case PickerTarget::Pokerus: return stage(screen, Field::Pokerus, 0, value);
            case PickerTarget::Form: return stage(screen, Field::Form, 0, value);
            case PickerTarget::Move: return stage(screen, Field::Move, static_cast<size_t>(state.moveEditorSlot), value);
            case PickerTarget::Nature: return stage(screen, Field::Nature, 0, value);
            case PickerTarget::Ability: {
                // The ability the record already stores: nothing to change, nothing staged.
                if (const auto now = d.current(); now && Fields::abilityChoiceKeepsStored(*now, b2w2(screen), value)) {
                    screen.postStatus("Ability unchanged", 180);
                    return true;
                }
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
            bool encounterReset = false;
            if (!backendState.recreateCreateSpecies(trainer.stagedPokemon(), species,
                    state.speciesPreviewShiny, Names::getSpeciesName(species),
                    Names::getSpeciesName(p.species()),
                    [&](std::string& why) { return trainer.refreshStagedPokemonPresentation(why); },
                    error, encounterReset)) {
                screen.postStatus(error, 360);
                return false;
            }
            // A Create reset is visible: the status names what was reset (Pokerus, Met Location).
            if (const auto reset = backendState.createSpeciesResetStatus(encounterReset); !reset.empty())
                screen.postStatus(reset, 300);
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
            // Round 11 (owner H28): keep only the moves the new species can use in this exact game,
            // with their PP and PP Ups; never add one. Staged in Keep/Review's replay order (Move,
            // PP Ups, PP) so the replay reproduces these bytes exactly. An existing Pokemon keeps
            // its Met Location and Pokerus (SpeciesChangeHistoryPolicy Edit).
            if (!PokeBank::UIModel::SpeciesMoveDefaults::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,
                    PokeVault::Integration::Gen5::speciesChangeMoves5(screen.sourceGameId, species, 0, p), error)) {
                screen.postStatus("Could not update moves for the new species: " + error, 360);
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
    static uint8_t nature(const Record& p) { return p.nature(); }
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
        // Round 11 H27: the stored name in every Gen V language, as Gen III / IV headers already
        // do, so cycling Language visibly changes an un-nicknamed name. Display only: the edit and
        // replay paths keep the asciiName5 write gate. Unreadable -> the species-name fallback.
        const auto nickname = PokeVault::Integration::Gen5::displayName5(p.decodedBytes(), true);
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
                PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete)},
        };
        // Round 11 (owner H25 clarification 2): an existing Pokemon whose species changed keeps its
        // history; a misfit is a caution line right under the met rows, never a change.
        namespace History = PokeBank::UIModel::HistoryConsistency;
        if (const auto caution = History::joined(backend(screen).historyFlags()); !caution.empty())
            rows.insert(rows.begin() + 6, {History::kCautionLabel, caution});
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
        bool encounterReset = false;
        applied = current && trainer.sharedScreen().recreateCreateSpecies(
            trainer.stagedPokemon(), pending.species, pending.shiny,
            Names::getSpeciesName(pending.species), Names::getSpeciesName(current->species()),
            [&](std::string& why) { return trainer.refreshStagedPokemonPresentation(why); }, error,
            encounterReset);
        if (applied)
            if (const auto reset = trainer.sharedScreen().createSpeciesResetStatus(encounterReset); !reset.empty())
                screen.postStatus(reset, 300);
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
    fb.drawText(x + 28, cy + 98, std::string("Gender, ability and shininess stay linked to the new PID. Origin & Legality: ")+
                PokeBank::UIModel::SharedEditorPresentation::legalityLabel(PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete), Colors::TextDim, TextStyle::Caption);
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
    const auto& shared = nativeTrainer(screen).sharedScreen();
    // A Review opened from this workspace returns to it: keep its focus and pending PID step.
    const bool reviewOverWorkspace =
        shared.surface() == Surface::Review && shared.reviewReturn() == Surface::Edit;
    if (!workspaceSurface(shared.surface()) && !reviewOverWorkspace) {
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
// ---- Top Up / Add Missing (owner Oct 28, Feature B) ----------------------------------------
// ZR on the Bag opens the shared flow (UI/SharedBagTopUp.h). The plan reads Gen5StagedBag through
// Inventory/BulkItemAdaptersNds.h with gen5BagChoices (the Add picker's list); A in the preview
// stages it all or nothing; a refused refresh restores the whole workspace; B stages nothing.
namespace BI=PokeVault::Inventory::BulkItems;
constexpr int kGen5TopUpTouchBase=81900;
std::string gen5TopUpPocketName(BI::Pocket pocket) {
    const auto native=BI::Gen5Bag::nativePocket(pocket);
    return native?PokeVault::Integration::Gen5::bagPocketName5(*native):"Bag";
}
bool planGen5TopUp(TrainerViewScreen& screen,BI::Plan& out,std::vector<BI::Pocket>& covered,std::string& error) {
    auto& trainer=nativeTrainer(screen);
    auto& ws=trainer.stagedPokemon();
    const auto game=BI::Gen5Bag::game(screen.sourceGameId);
    auto* bag=ws.stagedBag(&error);
    if(!game || !bag) {if(error.empty())error="Gen V bag is unavailable";return false;}
    const auto pockets=BI::Gen5Bag::pockets(*bag);
    const size_t pocket=trainer.bagScreen().pocket();
    if(pocket>=BI::Gen5Bag::kPockets.size()) {error="Gen V pocket is unavailable";return false;}
    const auto request=PokeBank::UIModel::BagTopUp::request(editorStateFor(screen).bagTopUp.model,
        BI::Gen5Bag::kPockets[pocket],pockets);
    covered=request.pockets;
    out=BI::plan(*game,request,pockets);
    return true;
}
void handleGen5TopUp(TrainerViewScreen& screen,u64 keys,int tap,const TouchInput& touch) {
    namespace Model=PokeBank::UIModel::BagTopUp;
    auto& trainer=nativeTrainer(screen);
    auto& ui=editorStateFor(screen).bagTopUp;
    const auto action=UI::SharedBagTopUp::handleInput(ui,keys,tap,kGen5TopUpTouchBase,touch);
    screen.controllerNavigation.reset();
    std::string error;
    switch(action) {
        case Model::Action::None:
        case Model::Action::Close:
            return;
        case Model::Action::PromptAmount:
            (void)UI::SharedBagTopUp::promptAmount(ui);
            return;
        case Model::Action::BuildPreview: {
            BI::Plan plan;
            std::vector<BI::Pocket> covered;
            if(!planGen5TopUp(screen,plan,covered,error)) {screen.postStatus(error,320);return;}
            Model::showPreview(ui.model,std::move(plan),covered,
                [&](BI::Pocket,uint16_t id){return bagItemLabel(screen,id);},
                [](BI::Pocket p){return gen5TopUpPocketName(p);});
            return;
        }
        case Model::Action::Confirm: {
            BI::Plan again;
            std::vector<BI::Pocket> covered;
            if(!planGen5TopUp(screen,again,covered,error) || !Model::samePlan(again,ui.model.plan)) {
                screen.postStatus(error.empty()?"The Bag changed; preview again":error,320);
                Model::close(ui.model);
                return;
            }
            auto& ws=trainer.stagedPokemon();
            auto snapshot=ws;
            auto* bag=ws.stagedBag(&error);
            const auto result=bag?BI::apply(*bag,ui.model.plan,BI::Gen5Bag::Adapter{}):BI::ApplyResult{};
            Model::close(ui.model);
            ui.scrollbar.reset();
            if(!result.applied) {
                screen.postStatus("Nothing staged: "+(result.error.empty()?error:result.error),400);
                return;
            }
            if(!trainer.refreshStagedPokemonPresentation(error)) {
                ws=std::move(snapshot);
                std::string restored;
                (void)trainer.refreshStagedPokemonPresentation(restored);
                screen.postStatus("Gen V Top Up refresh failed; nothing staged: "+error,360);
                return;
            }
            syncBagCursor(screen);
            screen.postStatus(Model::appliedStatus(result.toppedUp,result.added,again),400);   // again == the applied plan
            return;
        }
    }
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
    if(editorStateFor(screen).bagTopUp.active()) {
        // Top Up / Add Missing owns every input while it is open (SETUP or PREVIEW).
        handleGen5TopUp(screen,keys,tap,touch);
        syncBagCursor(screen);
        return true;
    }
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
                const auto rule=gen5PolicyRule(screen,bag.pocket(),id);
                if(bag.requestAddWarning(ws,rule)) {
                    screen.controllerNavigation.reset();
                    return true;
                }
                const auto decision=bag.pickAdd(ws,gen5PolicyRule(screen,bag.pocket(),id),
                                                refresh,error);
                screen.controllerNavigation.reset();
                if(!error.empty())screen.postStatus(error,320);
                else screen.postStatus(decision.message+"; SAV5 unchanged",300);
            }
            syncBagCursor(screen);
            return true;
        }
        case BagSurface::KeyItemAddWarning:
            if(keys&HidNpadButton_B)bag.cancel();
            else if(keys&HidNpadButton_A) {
                const auto decision=bag.pickAdd(ws,gen5PolicyRule(screen,bag.pocket(),bag.chosenAddItem()),
                                                refresh,error);
                if(!error.empty())screen.postStatus(error,320);
                else screen.postStatus(decision.message+"; event flags unchanged",300);
                screen.controllerNavigation.reset();
            }
            syncBagCursor(screen);
            return true;
        case BagSurface::ConfirmRemove:
            // Y opened the explicit Bag Remove question; A confirms it, B always cancels.
            if(keys&HidNpadButton_B)bag.cancel();
            else if(keys&HidNpadButton_A) {
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
    } else if(keys&HidNpadButton_ZR) {
        // Top Up / Add Missing (owner Oct 28, Feature B): offered where X Add is.
        if(!bag.availability(ws).addItem)
            screen.postStatus(G::Gen5BagScreen::available(ws)?"Top Up is unavailable in this pocket"
                                                             :"Gen V bag is unavailable",300);
        else PokeBank::UIModel::BagTopUp::open(editorStateFor(screen).bagTopUp.model);
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
    // Y (or the Review Changes button): the Trainer page's one Review key (ReviewEntry.h).
    if(keys&HidNpadButton_Y ||
       tap==PokeBank::UIModel::ClassicTrainerSession::kReviewTouchId) {
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
        case Row::Gender: {
            // Owner Oct 28 (#5): A flips Male / Female in place, no prompt (Gen III / IV parity).
            accepted=true;
            staged=editor.applyGender(ws,current->gender==0?1:0,refresh,error);
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
    // Round 7: painted by UI/SharedTrainerPanel.h, the painter the Gen I-IV Trainer page uses too.
    screen.drawGSCOverlay(fb);
    const int x=LEFT_PANEL_X,y=CONTENT_PANEL_Y;
    const int w=fb.getWidth()-x,h=CONTENT_PANEL_HEIGHT;
    namespace TPanel=UI::SharedTrainerPanel;
    namespace TPage=PokeBank::UIModel::ClassicTrainerPage;
    TPanel::drawPanel(fb,x,y,w,h,"Name, Money, Play Time and Gender are staged in app memory; the source save is unchanged");
    // R2-BUG-003: the Gen I-IV Trainer hierarchy. Selectable rows first (the staged Name / Money /
    // Play Time edits, and the Pokedex view) as cards with a value pill; a divider; then the
    // read-only identity rows, compact and unselectable, as Gen IV shows them (Gender, Trainer ID,
    // Secret ID as two separate native 16-bit numbers, plus Gen V's Badges). The raw language id is
    // secondary information: one unboxed detail line, by name. Every value is still read from the
    // save; nothing is merged or recomputed. Touch ids stay kGen5TrainerRowTouchBase + kRows index.
    // Round 7 (owner B06 / C06, QA CG90): the rects come from the one Trainer page model
    // (UI/ClassicTrainerPage.h) the Gen I-IV page uses: the Gen V heights, a compact Review button
    // in its measured slot below the last row, and the language line beside it in that band, so
    // neither can overlap a row.
    using TRow=G::Gen5TrainerScreen::Row;
    using F=TPage::Field;
    // Owner Oct 28 (#5): Gender is a staged card (its native writer exists); the Pokedex summary sits
    // with the read-only rows as on Gen I-IV, and still opens the read-only list with A.
    const std::vector<TPage::Row> model{{F::Name,true,-1},{F::Money,true,-1},{F::PlayTime,true,-1},
        {F::Gender,true,-1},{F::Pokedex,false,-1},{F::TrainerId,false,-1},{F::SecretId,false,-1},
        {F::Badges,false,-1}};
    const auto page=PokeBank::UIModel::ClassicTrainerPage::layout(model,x,y,w,h,true);
    const auto indexOf=[](TRow row) {
        for(size_t i=0;i<G::Gen5TrainerScreen::kRows.size();++i)
            if(G::Gen5TrainerScreen::kRows[i]==row)return i;
        return G::Gen5TrainerScreen::kRows.size();
    };
    size_t placed=0;
    for(const TRow row:{TRow::Name,TRow::Money,TRow::PlayTime,TRow::Gender}) {
        const size_t i=indexOf(row);
        const bool sel=editor.open() && i==editor.row();
        const auto& c=page.rows[placed++].card;
        screen.touchButtons.push_back({kGen5TrainerRowTouchBase+static_cast<int>(i),c.x,c.y,c.w,c.h});
        TPanel::drawEditableRow(fb,c.x,c.y,c.w,c.h,G::Gen5TrainerScreen::label(row),
                                G::Gen5TrainerScreen::value(ws,row),sel);
    }
    {
        const int cardX=page.rows.front().card.x,cy=page.dividerY,cardW=TPage::kCardW;
        TPanel::drawDivider(fb,cardX,cy,cardW);
    }
    for(const TRow row:{TRow::Pokedex,TRow::TrainerId,TRow::SecretId,TRow::Badges}) {
        const auto& c=page.rows[placed++].card;
        TPanel::drawInfoRow(fb,c.x,c.y,c.w,c.h,G::Gen5TrainerScreen::label(row),
                            G::Gen5TrainerScreen::value(ws,row));
        if(row==TRow::Pokedex) {
            // Read-only, but A (or a tap) still opens the read-only Pokedex list.
            const size_t i=indexOf(row);
            screen.touchButtons.push_back({kGen5TrainerRowTouchBase+static_cast<int>(i),c.x,c.y,c.w,c.h});
            if(editor.open() && i==editor.row())fb.drawRoundedRect(c.x,c.y,c.w,c.h,10,Colors::Accent,2);
        }
    }
    if(const auto t=ws.trainerPresentation()) {
        const auto& line=page.language;
        fb.drawText(line.x,line.y+(line.h-fb.lineHeight(TextStyle::Caption))/2,std::string("Game language: ")+
                    Enums::getLanguageName(t->language)+"  (stored id "+std::to_string(t->language)+")",
                    Colors::TextDim,TextStyle::Caption);
    }
    drawGlyphButton(fb,page.review.x,page.review.y,page.review.w,page.review.h,"Y","Review Changes");
    screen.touchButtons.push_back({PokeBank::UIModel::ClassicTrainerSession::kReviewTouchId,
        page.review.x,page.review.y,page.review.w,page.review.h});
    std::vector<ControllerHint> hints;
    for(const auto& entry:editor.footer(ws))hints.push_back({entry.glyph,entry.label});
    drawNavBar(fb,hints);
}
} // namespace

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
    if(state.surface()==Surface::Browse && (screen.helpOverlayActive || (down&HidNpadButton_Minus)))
        return false;
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
    // A Move or Clone carry (shared ClassicPackedMove overlay) owns A / B / Y / X
    // and cell taps; navigation keys fall through to the box grid below.
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
                const bool clone=state.actionRow()>=0 &&
                    static_cast<size_t>(state.actionRow())<allowed.count &&
                    allowed[static_cast<size_t>(state.actionRow())]==
                        PokeBank::UIModel::SharedPokemonEditor::Action::Clone;
                const auto selected=state.actionRow()>=0 &&
                    static_cast<size_t>(state.actionRow())<allowed.count ?
                    allowed[static_cast<size_t>(state.actionRow())] :
                    PokeBank::UIModel::SharedPokemonEditor::Action::None;
                using FileAction=PokeBank::UIModel::SharedPokemonEditor::Action;
                if(selected==FileAction::ImportFile || selected==FileAction::ExportFile) {
                    // Feature A: the shared file dialog / file export (UI/PokemonFileMenus.h).
                    const auto where=state.target();
                    (void)state.activate(staged,FileAction::Close,error);
                    if(selected==FileAction::ImportFile)
                        PokemonFileMenus::beginImport(screen,5,static_cast<int>(where.box),static_cast<int>(where.slot));
                    else
                        PokemonFileMenus::exportPokemon(screen,5,where.region==Region::Party,
                            static_cast<int>(where.box),static_cast<int>(where.slot));
                } else if(move) {
                    // Move Pokemon: the SHARED Gen I-IV carry (Round 3 Part 9),
                    // not Box / Slot number keypads. Nothing is staged until A.
                    const auto from=state.target();
                    if(!state.beginMoveCarry(staged,error))screen.postStatus(error,300);
                    else (void)ClassicPackedMove::beginGen5FromAction(
                        screen,static_cast<int>(from.box),static_cast<int>(from.slot));
                } else if(clone) {
                    // Round 11 H31 (owner): Clone is the same shared visual carry, never
                    // number keypads. The menu closes, the copy rides the box cursor
                    // (a Party Pokemon is carried over the current box), A places it on
                    // a verified empty slot, B cancels; nothing is staged until A.
                    const auto from=state.target();
                    if(!state.beginCloneCarry(staged,error))screen.postStatus(error,300);
                    else if(!ClassicPackedMove::beginCloneFromAction(
                        screen,static_cast<int>(from.box),static_cast<int>(from.slot),
                        from.region==Region::Party))
                        screen.postStatus("Clone could not start from here",240);
                } else if(!state.activateSelected(staged,error))
                    screen.postStatus(error,300);
                else if(state.surface()==Surface::ConfirmCreateBox &&
                        !beginSharedCreate(screen,error)) {
                    state.cancelBoxAction();
                    screen.postStatus(error,360);
                }
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
                         HidNpadButton_L))state.moveReviewPage(-1,staged,sharedReviewRows(screen,staged).size());
            else if(keys&(HidNpadButton_Right|HidNpadButton_Down|
                         HidNpadButton_R))state.moveReviewPage(1,staged,sharedReviewRows(screen,staged).size());
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
        case Surface::ConfirmExit: {
            namespace Exit=PokeBank::UIModel::ClassicStagedExit;
            const auto action=Exit::choose(keys&HidNpadButton_A,keys&HidNpadButton_B,
                keys&HidNpadButton_X,keys&HidNpadButton_Y,tap);
            if(action==Exit::Action::ContinueEditing)(void)state.back();
            else if(action==Exit::Action::Review)state.reviewFromExit(staged);
            else if(action==Exit::Action::ExportCopy) {
                state.reviewFromExit(staged);
                (void)state.requestExport(staged);
            } else if(action==Exit::Action::DiscardExit && state.confirmDiscardAndExit(staged))
                screen.goBack=true;
            return true;
        }
        case Surface::CloneDestination:
            // Never entered from this screen (Round 11 H31: Clone and Move are the shared
            // carry); leave it defensively without staging anything.
            state.cancelBoxAction();
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
    // Round 7 (ReviewEntry.h): one Review key per screen. Party and Boxes: Y, where Y has no other
    // job in Browse (carrying is handled above); + is not a second key there. The save hub keeps
    // its + Review. B in Review returns here with the cursor and box intact.
    namespace Entry=PokeBank::UIModel::ReviewEntry;
    const bool party=screen.selectedMode==TrainerViewScreen::ViewMode::Party;
    const bool boxes=screen.selectedMode==TrainerViewScreen::ViewMode::Boxes;
    const bool panel=screen.detailViewActive && (party || boxes);
    if((!screen.detailViewActive && (keys&HidNpadButton_Plus)) ||
       (panel && Entry::opensReview(party?Entry::Screen::Party:Entry::Screen::Boxes,5,keys))) {
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
        else if(keys&HidNpadButton_X)boxFileShortcut(screen,staged);
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
        case A::ImportFile:return PokeBank::UIModel::NarrowPokemonMenu::kImportFileLabel;
        case A::ExportFile:return PokeBank::UIModel::NarrowPokemonMenu::kExportFileLabel;
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
std::string reviewedFieldName(const PokeVault::Integration::Gen5::VerifiedFieldChange& c) {
    using Field=PokeVault::Integration::Gen5::StagedPokemon5Record::Field;
    std::string name;
    switch(c.field) {
        case Field::Nature:name="Nature";break;
        case Field::Personality:name="PID";break;
        case Field::Shiny:name="Shiny";break;
        case Field::Level:name="Level";break;
        case Field::Experience:name="Experience";break;
        case Field::Species:name="Species";break;
        case Field::CreateBox:name="Create in Box";break;
        case Field::CloneBox:name="Clone to Box";break;
        case Field::RemoveBox:name="Remove from Box";break;
        case Field::NicknameText:name="Nickname";break;
        case Field::OriginalTrainerText:name="OT Name";break;
        case Field::Friendship:name="Friendship";break;
        case Field::Language:name="Language";break;
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
    return name;
}
std::string reviewedFieldValue(PokeVault::Integration::Gen5::StagedPokemon5Record::Field field,
                               uint32_t value) {
    using Field=PokeVault::Integration::Gen5::StagedPokemon5Record::Field;
        switch (field) {
            case Field::Move: return Names::getMoveName(static_cast<uint16_t>(value));
            case Field::HeldItem: return Names::getItemName(static_cast<uint16_t>(value));
            case Field::Species: case Field::CreateBox: case Field::CloneBox: case Field::RemoveBox:
                return value?Names::getSpeciesName(static_cast<uint16_t>(value)):"Empty";
            case Field::Shiny: return value?"Yes":"No";
            case Field::Nature: return Trainer::getNatureName(static_cast<uint8_t>(value));
            case Field::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
            case Field::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
            // Round 6 L06: Review names values instead of raw native modes ("Pokerus: 2 -> 1").
            case Field::Pokerus: return Fields::pokerusLabel(value);
            case Field::OriginalTrainerGender: return value == 0 ? "Male" : "Female";
            case Field::Personality: return pidHex5(value);   // Round 11 H29: the inline Shiny / Gender PID
            case Field::Gender: return value == 0 ? "Male" : value == 1 ? "Female" : "Genderless";
            case Field::FatefulEncounter: return value ? "Yes" : "No";
            default: return std::to_string(value);
        }
}
std::string reviewedFieldLabel(const PokeVault::Integration::Gen5::VerifiedFieldChange& c) {
    using Field=PokeVault::Integration::Gen5::StagedPokemon5Record::Field;
    if(c.field==Field::NicknameText)return "Nickname changed";
    if(c.field==Field::OriginalTrainerText)return "OT Name changed";
    return reviewedFieldName(c)+": "+reviewedFieldValue(c.field,c.before)+" -> "+
           reviewedFieldValue(c.field,c.after);
}
PokeBank::UIModel::SharedReview::Rows sharedReviewRows(TrainerViewScreen& screen,
    const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    namespace G=PokeVault::Integration::Gen5;
    PokeBank::UIModel::SharedReview::Rows rows;
    const auto pending=ws.pendingReview();
    const auto location=[](const auto& loc) {
        return loc.region==Region::Party?"Party "+std::to_string(loc.slot+1):
            "Box "+std::to_string(loc.box+1)+" / Slot "+std::to_string(loc.slot+1);
    };
    for(const auto& grouped:G::groupReviewRows5(pending)) {
        const auto& c=pending[grouped.change];std::string error;
        const auto verified=G::verifyStagedReview(c,&error);
        if(grouped.kind==G::ReviewRow5::Kind::Move) {
            const auto& removed=pending[grouped.removal];
            const auto original=G::verifyStagedReview(removed,&error);
            rows.push_back({"Move Pokemon",verified && original?
                location(removed.location)+" -> "+location(c.location):"Unverified staged difference"});
            continue;
        }
        if(!verified) {rows.push_back({location(c.location),"Unverified staged difference"});continue;}
        const G::Pokemon5ReadOnly before(std::span<const uint8_t>(c.before));
        const G::Pokemon5ReadOnly after(std::span<const uint8_t>(c.after));
        if(!verified->created.empty()) {
            // CG74: the shared Create Review shape (Gen12CreateReview.h, as Gen I-IV): a
            // "<loc> / Add Pokémon" header with "Empty -> SPECIES Lv n", then one "<loc> / Field" row
            // per value of the certified staged record (later edits are already in those values).
            using F=G::StagedPokemon5Record::Field;
            namespace CR=PokeVault::Integration::Gen12CreateReview;
            const std::string loc=location(c.location);
            rows.push_back({loc+" / "+std::string(CR::kAddPokemon),"Empty -> "+CR::header(
                Names::getSpeciesName(after.species()),
                Fields::level(after,nativeTrainer(screen).sharedScreen().black2White2()))});
            std::string moves;
            std::array<uint32_t,6> ivs{},evs{};
            for(const auto& v:verified->created) {
                if(v.field==F::Move)moves+=(moves.empty()?"":", ")+std::string(Names::getMoveName(static_cast<uint16_t>(v.after)));
                if(v.field==F::IV && v.stat<6)ivs[v.stat]=v.after;
                if(v.field==F::EV && v.stat<6)evs[v.stat]=v.after;
            }
            if(!moves.empty())rows.push_back({loc+" / Moves",moves});
            const auto ot=G::displayName5(after.decodedBytes(),false);
            rows.push_back({loc+" / OT / TID / SID",(ot && !ot->empty()?*ot:std::string("(none)"))+" / "+
                std::to_string(after.tid())+" / "+std::to_string(after.sid())});
            for(const auto& v:verified->created) {
                if(v.field==F::Move || v.field==F::IV || v.field==F::EV ||
                   v.field==F::TrainerId || v.field==F::SecretId)continue;
                std::string value=reviewedFieldValue(v.field,v.after);
                if(v.field==F::AbilitySlot)value=Trainer::getAbilityName(static_cast<uint16_t>(v.after));
                if(v.field==F::MetLocation)value=Names::getMetLocationDisplayName(after.originVersion(),after.metLocation());
                std::string name=reviewedFieldName(v);
                if(v.field==F::AbilitySlot)name="Ability";
                rows.push_back({loc+" / "+name,value});
            }
            constexpr std::array<const char*,6> statLabels{{"HP","Atk","Def","Spe","SpA","SpD"}};
            if(const auto line=CR::nonZero(statLabels,ivs);!line.empty())rows.push_back({loc+" / IVs",line});
            if(const auto line=CR::nonZero(statLabels,evs);!line.empty())rows.push_back({loc+" / EVs",line});
            continue;
        }
        for(const auto& field:verified->fields) {
            using F=G::StagedPokemon5Record::Field;
            std::string detail=reviewedFieldLabel(field);
            if(field.field==F::NicknameText || field.field==F::OriginalTrainerText) {
                const bool nickname=field.field==F::NicknameText;
                const auto from=G::displayName5(before.decodedBytes(),nickname);
                const auto to=G::displayName5(after.decodedBytes(),nickname);
                detail=std::string(nickname?"Nickname: ":"OT Name: ")+(from?*from:"(unreadable)")+
                    " -> "+(to?*to:"(unreadable)");
            }
            if(field.field==F::AbilitySlot)
                detail=std::string("Ability: ")+Trainer::getAbilityName(before.ability())+" -> "+Trainer::getAbilityName(after.ability());
            if(field.field==F::MetLocation)
                detail="Met Location: "+Names::getMetLocationDisplayName(before.originVersion(),before.metLocation())+
                    " -> "+Names::getMetLocationDisplayName(after.originVersion(),after.metLocation());
            rows.push_back({location(c.location),detail});
        }
        // Round 11 (owner H25 clarification 2): the species changed and the record kept its own
        // history; a misfit is one caution note on this entry, never a change or a refusal.
        if(before.species()!=after.species()) {
            namespace History=PokeBank::UIModel::HistoryConsistency;
            if(const auto caution=History::joined(G::historyFlags5(after,
                   nativeTrainer(screen).sharedScreen().black2White2()));!caution.empty())
                rows.push_back({location(c.location),std::string(History::kCautionLabel)+": "+caution});
        }
    }
    for(const auto& c:ws.bagReviewChanges())rows.push_back({std::string("Bag / ")+G::bagPocketName5(c.pocket)+" / "+bagItemLabel(screen,c.itemId),
        std::to_string(c.before)+" -> "+std::to_string(c.after)});
    for(const auto& c:ws.trainerReviewChanges()) {
        using F=G::TrainerChange5::Field;
        rows.push_back({std::string("Trainer / ")+(c.field==F::Name?"Name":c.field==F::Money?"Money":
                        c.field==F::Gender?"Gender":"Play Time"),c.before+" -> "+c.after});
    }
    return rows;
}

void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                const ScreenState& state,
                const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    const auto rows=sharedReviewRows(screen,ws);
    SharedEditorChrome::drawReviewPanel(fb,screen.touchButtons,rows,state.reviewPage(),ws.hasChanges(),"Generation V");
}
// Origin & Legality (Round 3 Part 9): the neutral Gen I-IV panel. It reports the
// PK5 structure and the record's stored origin only, and never a legality
// verdict: Gen V has no legality checker here, so Legality is "Incomplete"
// and the encounter check is "Unknown".
void drawLegality5(TrainerViewScreen& screen,PKSEFramebuffer& fb,const ScreenState& state,
                   const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& workspace) {
    namespace P=PokeBank::UIModel::SharedEditorPresentation;
    dimShared(screen,fb);
    const auto& slot=state.target();
    const auto p=slot.region==Region::Party?workspace.viewParty(slot.slot):workspace.viewBox(slot.box,slot.slot);
    std::vector<std::pair<std::string,std::string>> rows{
        {"Source game",gen5GameName(screen.sourceGameId)},
        {"Native format",p && p->valid()?"PK5 checksum and structure valid":"PK5 unavailable or invalid"},
        {"Encounter / evolution / transfers","Full history not evaluated"},
        {"Rule coverage","Structure and editor guardrails; full legality incomplete"}};
    if(p && p->valid() && !p->empty()) {
        const auto origin=p->originVersion();const auto date=p->metDate();
        rows.push_back({"Pokemon",std::string(Names::getSpeciesName(p->species()))+" #"+std::to_string(p->species())});
        const auto nickname=PokeVault::Integration::Gen5::displayName5(p->decodedBytes(),true);
        const auto ot=PokeVault::Integration::Gen5::displayName5(p->decodedBytes(),false);
        rows.push_back({"Nickname",nickname?*nickname:"(unreadable)"});
        rows.push_back({"Original Trainer",ot?*ot:"(unreadable)"});
        rows.push_back({"Trainer / Secret ID",std::to_string(p->tid())+" / "+std::to_string(p->sid())});
        rows.push_back({"Origin game",Enums::getOriginGameName(origin)});
        rows.push_back({"Met location",metLocationText5(origin,p->metLocation())});
        if(const auto event=Names::metLocationEventEvidence(origin,p->metLocation());!event.empty())
            rows.push_back({"Event provenance",event});
        rows.push_back({"Met level",std::to_string(p->metLevel())});
        rows.push_back({"Met date (stored)",(date.month==0 && date.day==0)?std::string("Not recorded"):std::to_string(date.year)+"-"+std::to_string(date.month)+"-"+std::to_string(date.day)});
        rows.push_back({"Egg / fateful",std::string(p->egg()?"Unhatched":"Not egg")+" / "+(p->fatefulEncounter()?"Fateful":"Not fateful")});
        for(size_t i=0;i<4;++i)rows.push_back({"Move "+std::to_string(i+1),p->moves()[i]?Names::getMoveName(p->moves()[i]):"Empty"});
    }
    SharedEditorChrome::drawOriginLegalityPanel(fb,screen.touchButtons,rows,P::LegalityState::Incomplete);
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
    if(editorStateFor(screen).bagTopUp.active()) {
        UI::SharedBagTopUp::drawTopUpModal(fb,screen.touchButtons,editorStateFor(screen).bagTopUp,
            kGen5TopUpTouchBase,G::bagPocketName5(bag.pocket()),"Gen V Bag");
        return;
    }
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
    } else if(bag.surface()==BagSurface::KeyItemAddWarning) {
        const auto id=bag.chosenAddItem();
        UI::SharedBagAddPicker::drawKeyItemWarning(fb,screen.touchButtons,game,id,
            bagItemLabel(screen,id),G::bagPocketName5(bag.pocket()),false);
        return;
    } else {
        const auto& item=bag.pendingRemove();
        if(item && PokeBank::UIModel::BagAdd::needsKeyItemWarning(
                gen5PolicyRule(screen,bag.pocket(),item->itemId))) {
            UI::SharedBagAddPicker::drawKeyItemWarning(fb,screen.touchButtons,game,item->itemId,
                bagItemLabel(screen,item->itemId),G::bagPocketName5(bag.pocket()),true);
            return;
        }
        UI::SharedBagAddPicker::drawRemoveConfirm(fb,screen.touchButtons,game,"Remove Gen V Item?",
            item?item->itemId:0,item?bagItemLabel(screen,item->itemId):std::string(),
            item?item->count:0,"Staged bag only; Discard All in Review restores it.");
        return; // The modal owns its A/B choices; do not repeat them in a footer.
    }
    drawNavBar(fb,hints);
}
bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!isGen5Source(screen))return false;
    auto& trainer=nativeTrainer(screen);
    auto& state=trainer.sharedScreen();
    const auto& staged=trainer.stagedPokemon();
    if(screen.helpOverlayActive){screen.drawGSCOverlay(fb);return true;}
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
            SharedEditorChrome::drawReviewDiscardConfirm(fb,screen.touchButtons);
            return true;
        case Surface::ConfirmExit:
            SharedEditorChrome::drawStagedExitPanel(fb,screen.touchButtons);
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
        ClassicPackedMove::draw(screen,fb); // carried sprite + "A Place / L/R Box / B Cancel"
    else if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes) {
        // Round 11 X-IMPEXP: X Export / Import after A, before Y, from the same decision as the key.
        std::vector<ControllerHint> hints{{"D-pad","Navigate"},{"L/R","Box"},{"ZL/ZR","Jump 5"},{"A","Actions"}};
        if(const char* x=boxFileLabel(screen,nativeTrainer(screen).stagedPokemon()))hints.push_back({"X",x});
        hints.push_back({"Y","Review"});
        hints.push_back({"B","Back"});
        drawNavBar(fb,hints);
    }
    else
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"A","Actions"},
                       {"Y","Review"},{"B","Back"}});
    return true;
}
} // namespace UI::Gen5SharedReadOnlySurface
