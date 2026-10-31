// Gen VI through the shared Modern::NativeSession contract: the adapter must present and stage
// exactly what Gen6StagedSession verifies, refuse what it cannot verify (with a reason), keep the
// source bytes immutable and export a separately verified copy. These are the behaviours the ONE
// shared surface relies on for every Gen VI-IX title.
#include "Integration/Gen6/Gen6NativeSession.h"
#include "Integration/Gen6/Gen6Bag.h"
#include "Pokemon/ModernRecordView.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G6 = PokeVault::Integration::Gen6;
namespace Modern = PokeVault::Integration::Modern;
namespace Layout = PokeVault::Integration::Gen6::Layout;
using Modern::SlotRef;
using Field = Modern::Field;
using State = Modern::FieldState;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::string makeTempDir() {
    char pattern[] = "/tmp/gen6-native-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::vector<uint8_t> saveWithBag() {
    auto save = Gen6Fixture::makeSave({});
    // Items pocket (offset 0): two stacks; Medicine pocket at its XY offset.
    const auto medicine = G6::bagPocketLayout6(Layout::Family::XY, static_cast<size_t>(G6::BagPocket6::Medicine)).offset;
    Gen6Fixture::writeBag(save, Layout::Family::XY, {{0, 1, 3}, {0, 4, 10}, {medicine, 17, 2}});
    Gen6Fixture::resignAll(save, Layout::Family::XY);
    return save;
}
} // namespace

int main() {
    const auto original = saveWithBag();
    const auto pristine = original;

    // ---- open: exact-title assignment is authoritative ----
    check(!G6::Gen6NativeSession::open(original, "y_3ds").ready(), "an X save refuses a Y assignment");
    check(!G6::Gen6NativeSession::open(original, "y_3ds").diagnostic.empty(), "mismatch names why");
    auto opened = G6::Gen6NativeSession::open(original, "x_3ds");
    check(opened.ready(), "a valid X save opens: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    auto& s = *opened.session;
    check(s.exactGameId() == "x_3ds", "exact id");
    check(s.capabilities().id == "x_3ds" && s.capabilities().generation == 6, "registry row attached");
    check(s.boxCount() == 31 && s.slotsPerBox() == 30 && s.partyCount() == 2, "native geometry 31x30, party 2");
    check(s.familyLabel() == "Generation VI", "family label");
    check(!s.boxName(0).empty() && !s.boxName(30).empty(), "every box has a presentable name (native or Box N fallback)");

    // ---- record views ----
    const auto party0 = s.view(SlotRef::party(0));
    check(party0 && party0->valid && !party0->empty && party0->partyRecord, "party 0 is a valid party record");
    check(party0 && party0->species == 1 && party0->level == 12, "party 0 species/level from the native record");
    check(party0 && party0->currentHp == 20 && party0->stats[0] == 21 && party0->stats[5] == 26, "party cached stats");
    check(party0 && party0->nickname == "SYNTH" && party0->otName == "TESTER", "UTF-16 names decoded");
    check(party0 && party0->tid == 12345 && party0->sid == 54321 && party0->heldItem == 1, "ids and held item");
    check(party0 && party0->ivs[0] == 31 && party0->evs[0] == 4 && party0->moves[0] == 33 && party0->pp[0] == 35, "IV/EV/move/PP");
    check(party0 && party0->metLocation == 6 && party0->ball == 4 && party0->metLevel == 5 && party0->language == 2, "met data");
    check(party0 && party0->metDate.year == 2014 && party0->metDate.month == 1 && party0->metDate.day == 2, "met date");
    check(party0 && party0->originGame == Layout::VersionX, "origin version");
    check(party0 && !party0->nativeFacts.empty(), "native facts carried for the shared editor");
    const auto box2 = s.view(SlotRef::boxed(0, 2));
    check(box2 && box2->valid && !box2->empty && !box2->partyRecord, "box 0/2 is a stored record");
    check(box2 && box2->species == 676 && box2->form == 2, "Furfrou form 2 kept");
    check(box2 && box2->level >= 1 && box2->stats[0] > 0 && box2->baseStats[0] > 0, "boxed stats computed from personal data");
    const auto blank = s.view(SlotRef::boxed(5, 5));
    check(blank && blank->valid && blank->empty, "blank slot is valid and empty");
    check(!s.view(SlotRef::boxed(31, 0)).has_value(), "non-canonical slot has no view");
    check(s.originalOccupied(SlotRef::boxed(0, 0)) && !s.originalOccupied(SlotRef::boxed(5, 5)), "originalOccupied");
    check(s.emptyDestination(SlotRef::boxed(5, 5)) && !s.emptyDestination(SlotRef::party(5)), "emptyDestination is box-only");
    check(s.firstEmptyBoxSlot() == SlotRef::boxed(0, 3), "first blank box slot is Box 1 slot 4");

    // ModernRecordView presents the same data to the shared panels.
    {
        Pokemon::ModernRecordView pk(*party0, Enums::GameVersion::XY);
        check(pk.speciesID() == 1 && pk.level() == 12 && pk.statHPMax() == 21 && pk.statHPCurrent() == 20, "presentation Pokemon mirrors the view");
        check(pk.ivHP() == 31 && pk.move(0) == 33 && pk.getGameGroup() == Enums::GameVersion::XY, "presentation IV/move/group");
        check(Utils::utf16ToUtf8(pk.nickname()) == "SYNTH", "presentation nickname");
    }

    // ---- field model ----
    check(s.fieldState(Field::Nickname, Modern::Region::Box) == State::Editable, "Nickname editable");
    check(s.fieldState(Field::Species, Modern::Region::Box) == State::ReadOnly, "Species read-only (no verified species writer)");
    check(s.fieldState(Field::Move, Modern::Region::Box) == State::ReadOnly, "Moves read-only (no pinned XY learnset)");
    check(s.fieldState(Field::DV, Modern::Region::Box) == State::Hidden, "DV hidden");
    check(s.fieldState(Field::CalculatedStats, Modern::Region::Party) == State::Derived &&
              s.fieldState(Field::CalculatedStats, Modern::Region::Box) == State::Hidden,
          "calculated stats derived on party, hidden on box");
    check(s.fieldMaximum(Field::HeldItem) == 717 && s.fieldMaximum(Field::EV) == 252 && s.fieldMaximum(Field::Ball) == 25, "XY limits");
    check(s.heldItemChoices().size() > 100 && s.heldItemChoices().front() == 0, "held item choices come from the XY catalog");
    check(s.ballChoices().size() == 25, "25 balls");

    // ---- staging ----
    std::string error;
    check(s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 5, 0, error), "stage held item: " + error);
    check(s.view(SlotRef::boxed(0, 0))->heldItem == 5, "staged view shows the new held item");
    check(s.stagedAt(SlotRef::boxed(0, 0)) && !s.stagedAt(SlotRef::boxed(0, 1)), "stagedAt");
    check(s.hasChanges() && s.changeCount() == 1, "one staged change");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::Species, 150, 0, error) && !error.empty(), "species refused with reason");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 800, 0, error), "held item beyond XY range refused");
    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "NEW", error), "stage nickname: " + error);
    check(s.view(SlotRef::boxed(0, 0))->nickname == "NEW", "nickname staged");
    check(!s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "Pok\xC3\xA9", error), "non-ASCII nickname refused");
    check(s.stageValue(SlotRef::party(0), Field::IV, 7, 0, error), "party IV stages: " + error);
    check(s.view(SlotRef::party(0))->ivs[0] == 7 && s.view(SlotRef::party(0))->stats[0] != 21, "party stats recomputed after IV edit");
    const auto rows = s.reviewRows();
    check(rows.size() >= 3, "review rows list every staged field");
    bool sawHeld = false;
    for (const auto& r : rows) if (r.label == "Box 1 / Slot 1" && r.value.find("held item") != std::string::npos) sawHeld = true;
    check(sawHeld, "review names the held item change at its slot");

    // ---- clone / move / remove / create ----
    check(s.cloneSupport(SlotRef::boxed(0, 1)).available, "clone available for an unmodified occupied box record");
    check(!s.cloneSupport(SlotRef::boxed(0, 0)).available, "clone refused for a staged source");
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(0, 3), error), "clone to Box 1 slot 4: " + error);
    check(s.view(SlotRef::boxed(0, 3))->species == 101, "clone landed");
    check(!s.clone(SlotRef::boxed(0, 2), SlotRef::boxed(0, 3), error), "clone onto a staged destination refused");
    check(!s.moveSupport(SlotRef::party(1)).available && !s.moveSupport(SlotRef::party(1)).reason.empty(), "party Move blocked with reason");
    check(s.moveSupport(SlotRef::boxed(0, 1)).available, "box Move available");
    check(s.move(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error), "move Box 1/2 -> Box 2/1: " + error);
    check(s.view(SlotRef::boxed(1, 0))->species == 101 && s.view(SlotRef::boxed(0, 1))->empty, "move = clone + remove");
    check(!s.removeSupport(SlotRef::party(0)).available, "party Remove blocked");
    check(s.removeSupport(SlotRef::boxed(0, 2)).available, "box Remove available");
    check(s.remove(SlotRef::boxed(0, 2), error), "remove Box 1/3: " + error);
    check(s.view(SlotRef::boxed(0, 2))->empty, "removed slot reads empty");
    check(!s.createSupport(SlotRef::boxed(5, 5)).available && !s.createSupport(SlotRef::boxed(5, 5)).reason.empty(),
          "Create blocked with the named prerequisite");
    check(s.undo(SlotRef::boxed(0, 2)) && !s.view(SlotRef::boxed(0, 2))->empty, "undo restores the removed record");

    // ---- bag ----
    check(s.bagAvailable(), "bag decodes");
    auto bag = s.bag();
    check(bag.size() == 5 && bag[0].name == std::string("Items") && bag[0].items.size() == 2, "five pockets, two Items stacks");
    check(bag[0].editable && !bag[1].editable && !bag[1].reason.empty(), "story pockets read-only with reason");
    check(bag[0].items[1].itemId == 4 && bag[0].items[1].count == 10, "item stack decoded");
    check(s.bagSetQuantity(0, 1, 42, error), "set quantity: " + error);
    check(s.bag()[0].items[1].count == 42, "quantity staged");
    check(!s.bagAdd(1, 450, 1, error), "key item add refused");
    check(s.bagAdd(0, 2, 7, error), "add Ultra Ball: " + error);
    check(s.bag()[0].items.size() == 3, "stack added");
    check(!s.bagAddChoices(0).empty() && s.bagAddChoices(1).empty(), "add choices only for addable pockets");
    bool sawBag = false;
    for (const auto& r : s.reviewRows()) if (r.label.rfind("Bag / Items", 0) == 0) sawBag = true;
    check(sawBag, "review lists bag changes");
    s.discardBag();
    check(s.bag()[0].items.size() == 2 && s.bag()[0].items[1].count == 10, "bag discard restores the source bag");

    // ---- trainer / dex ----
    check(s.trainerName() == "TESTER" && s.trainerGender() == 1, "trainer name and gender");
    bool sawTid = false;
    for (const auto& f : s.trainerFacts()) if (f.label == "Trainer ID" && f.value == "12345") sawTid = true;
    check(sawTid, "trainer facts include the TID");
    check(s.pokedex().total == 721, "Kalos dex total");

    // ---- export: separate verified copy; source untouched ----
    check(s.exportSupport().available, "export available with staged changes");
    const std::string root = makeTempDir();
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.sourcePath = "sdmc:/3ds/Checkpoint/saves/0x0055D X/b1/main";
    req.timestamp = "20261031_120000";
    req.applicationSha = "test";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export succeeds: " + result.error);
    if (result.success) {
        const auto edited = slurp(result.directory + "/edited.sav");
        const auto backup = slurp(result.directory + "/original_backup.sav");
        check(backup == pristine, "backup is the pristine source");
        check(edited.size() == Layout::XYSize && edited != pristine, "edited copy differs");
        auto reopened = G6::Gen6NativeSession::open(edited, "x_3ds");
        check(reopened.ready(), "exported copy strictly reopens: " + reopened.diagnostic);
        if (reopened.ready()) {
            check(reopened.session->view(SlotRef::boxed(0, 0))->heldItem == 5 && reopened.session->view(SlotRef::boxed(0, 0))->nickname == "NEW",
                  "edits survive the round trip");
            check(reopened.session->view(SlotRef::boxed(1, 0))->species == 101 && reopened.session->view(SlotRef::boxed(0, 1))->empty,
                  "move survives the round trip");
            check(!reopened.session->hasChanges(), "reopened copy has no pending changes");
        }
    }
    check(original == pristine, "the source bytes never changed");
    check(s.sourceBytes().size() == pristine.size() && std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin()),
          "adapter holds the immutable source");

    s.discardAll();
    check(!s.hasChanges() && s.view(SlotRef::boxed(0, 0))->heldItem == 1, "discard all restores the source view");

    // ---- draft bracket (shared View/Edit workspace) ----
    check(!s.beginDraft(SlotRef::boxed(5, 5), error) && !error.empty(), "draft on an empty slot refused");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.draftActive() && s.draftSlot() == SlotRef::boxed(0, 0) && !s.draftDirty(),
          "draft begins clean");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 9, 0, error) && s.draftDirty() && s.view(SlotRef::boxed(0, 0))->heldItem == 9,
          "edit inside the draft is visible and dirty");
    s.discardDraft();
    check(!s.draftActive() && !s.stagedAt(SlotRef::boxed(0, 0)) && s.view(SlotRef::boxed(0, 0))->heldItem == 1 && !s.hasChanges(),
          "discard draft restores the pre-draft state exactly");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 200, 0, error), "second draft");
    s.keepDraft();
    check(!s.draftActive() && s.stagedAt(SlotRef::boxed(0, 0)) && s.view(SlotRef::boxed(0, 0))->friendship == 200, "keep draft retains the edit");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && !s.draftDirty(), "a draft over a kept edit starts clean");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 150, 0, error) && s.draftDirty(), "further edit dirties it");
    s.discardDraft();
    check(s.view(SlotRef::boxed(0, 0))->friendship == 200 && s.stagedAt(SlotRef::boxed(0, 0)), "discard returns to the KEPT edit, not the source");
    // Review finding W5-01: Discard All closes an open bracket; a later discardDraft cannot re-stage.
    check(s.beginDraft(SlotRef::boxed(0, 0), error), "draft over a kept edit");
    s.discardAll();
    check(!s.draftActive() && !s.hasChanges(), "Discard All ends the open draft");
    s.discardDraft();
    check(!s.hasChanges() && s.view(SlotRef::boxed(0, 0))->friendship == 70, "discardDraft after Discard All re-stages nothing");
    // Review finding W5-02: slot operations touching the open draft are refused, so a Move can never
    // be half-reverted into a Clone by a later discardDraft.
    check(s.beginDraft(SlotRef::boxed(0, 1), error), "draft on Box 1/2");
    check(!s.move(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error) && error.find("open editor draft") != std::string::npos,
          "Move of the drafted slot refused with a reason");
    check(!s.remove(SlotRef::boxed(0, 1), error) && !s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error),
          "Remove and Clone of the drafted slot refused");
    check(!s.undo(SlotRef::boxed(0, 1)), "undo of the drafted slot refused");
    s.discardDraft();
    check(!s.hasChanges() && s.view(SlotRef::boxed(1, 0))->empty, "nothing half-applied");
    // W5-11: switching drafts discards the first draft's unkept edit (documented behaviour).
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 9, 0, error), "dirty draft");
    check(s.beginDraft(SlotRef::boxed(0, 1), error) && !s.stagedAt(SlotRef::boxed(0, 0)), "a new draft discards the previous unkept draft");
    s.discardDraft();
    s.discardAll();
    check(!s.exportSupport().available && !s.exportSupport().reason.empty(), "export unavailable without changes, with reason");

    std::cout << "test_gen6_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
