// Round 5 (owner Oct 24): Review must open from every normal entry. Party Actions keeps its
// hardware-accepted View / Edit / Origin & Legality / Cancel sheet (E02 / NEW-20), so Party edits
// reach Review through the staged-exit menu: Party -> Edit -> keep -> B (leave) -> A Review
// Changes. The Bag's Options -> Review opens the same shared paged Review.
//
// Drives a real Gold / Silver / Crystal GSCReadOnlyTrainer: a staged Party edit makes the session
// dirty (the shell then asks to leave), the four-action exit maps A / touch 2 to Review, the
// Review it opens is standalone (B returns to the screen underneath), and its rows are the named
// Party changes the shared presenter draws (never a numeric move id). Paging and the swipe gesture
// are the shared models. The verified export carries the Party edit; the source never changes.
// Synthetic fixture only.
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "UI/ClassicReviewOrigin.h"
#include "UI/ClassicStagedExit.h"
#include "UI/SharedReviewPresentation.h"
#include "UI/SharedReviewTouch.h"
#include "tests/fixtures/gsc_pokemon_fixture.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G2 = PokeVault::Integration::Gen2;
namespace Exit = PokeBank::UIModel::ClassicStagedExit;
namespace Origin = PokeBank::UIModel::ClassicReviewOrigin;
namespace Review = PokeBank::UIModel::SharedReview;
namespace Touch = PokeBank::UIModel::SharedReviewTouch;

namespace {

struct FakeScreen { std::string sourceGameId; };

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

void partyEditReachesReview(const L& layout, G2::SourceGame game, const char* gameId) {
    const auto raw = fixture(layout);
    const auto parsed = G2::parse(raw, game);
    assert(parsed);
    std::string error;
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    assert(trainer && error.empty() && !trainer->hasStagedChanges());
    auto* editor = trainer->stagedEditor();
    assert(editor);

    // Party -> Edit -> change Move 1 to Thunderbolt -> Keep.
    const auto before = editor->partyPokemon(0, error);
    assert(before);
    G2::BoxPokemonEdit edit;
    auto moves = before->moves;
    moves[0] = 85; // Thunderbolt (pret pokecrystal constants/move_constants.asm)
    edit.moves = moves;
    assert(editor->stagePartyPokemonEdit(0, edit, error));
    assert(trainer->hasStagedChanges()); // the shell's B / + now asks before leaving

    // The four-action exit: A or its touch row opens Review Changes.
    assert(Exit::choose(true, false, false, false) == Exit::Action::Review);
    assert(Exit::choose(false, false, false, false, 2) == Exit::Action::Review);
    assert(Exit::choose(true, true, false, false) == Exit::Action::ContinueEditing);

    // openStagedReview marks it standalone: B returns to the screen underneath, once.
    FakeScreen screen{gameId};
    Origin::markStandalone(screen);
    assert(Origin::takeStandalone(screen) && !Origin::takeStandalone(screen));

    // The rows the shared presenter draws: named Party changes, page 1 of 1.
    Review::Rows rows;
    for (const auto& change : editor->pendingChanges())
        rows.emplace_back(change.label, change.beforeValue + " -> " + change.afterValue);
    assert(!rows.empty() && Review::pages(rows.size()) == 1);
    bool moveRow = false;
    for (const auto& [label, detail] : rows) {
        if (label.rfind("Party 1", 0) == 0 && label.find("Move 1") != std::string::npos) {
            moveRow = true;
            assert(detail.find(Names::getMoveName(85)) != std::string::npos);
            assert(detail.find(" 85") == std::string::npos); // never the raw move id
        }
    }
    assert(moveRow);

    // Export Copy from that Review: the Party edit is in the verified copy; the source is not.
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / ("pokebank-party-review-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    G2::StagedEditorExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId) + "_party";
    request.sourcePath = "/retroarch/saves/fixture.srm";
    request.gameId = gameId;
    request.timestamp = "20261025-010000";
    const auto result = G2::publishVerifiedStagedEditorExport(*editor, request);
    assert(result.success);
    const auto edited = read((fs::path(result.directory) / "edited.srm").c_str());
    const auto reparsed = G2::parse(std::vector<uint8_t>(edited.begin(), edited.end()), game);
    assert(reparsed && reparsed.save->party().size() == 1 && reparsed.save->party()[0].moves[0] == 85);
    fs::remove_all(root);
    assert(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(), raw.begin()));
}

void swipePaging() {
    const auto p = Touch::kPanel;
    const int cx = p.x + p.w / 2, cy = p.y + p.h / 2;
    assert(p.x == 90 && p.y == 40 && p.w == 1100 && p.h == 584); // drawReviewPanel on 1280x720
    assert(Touch::swipePage(true, true, cx, cy + 60, cx, cy - 60) == 1);   // up: next page
    assert(Touch::swipePage(true, true, cx, cy - 60, cx, cy + 60) == -1);  // down: previous
    assert(Touch::swipePage(false, true, cx, cy + 60, cx, cy - 60) == 0);  // still touching
    assert(Touch::swipePage(true, false, cx, cy, cx, cy) == 0);            // a tap selects nothing
    assert(Touch::swipePage(true, true, cx, cy, cx + 80, cy - 30) == 0);   // sideways
    assert(Touch::swipePage(true, true, cx, cy, cx, cy - 20) == 0);        // too short
    assert(Touch::swipePage(true, true, cx, p.y + 10, cx, p.y - 30) == 0); // leaves the panel
    assert(Touch::swipePage(true, true, 10, cy + 60, 10, cy - 60) == 0);   // outside the panel
    // Paging clamps exactly like L/R on 9 rows (3 pages).
    size_t page = 0;
    page = Review::move(page, Touch::swipePage(true, true, cx, cy + 60, cx, cy - 60), 9);
    page = Review::move(page, 1, 9);
    page = Review::move(page, 1, 9);
    assert(page == 2);
}

void routes() {
    // Bag -> Options -> Review is the generation's shared Review; the private list is gone.
    const auto bag = read("src/UI/ClassicInventoryOverlay.cpp");
    assert(bag.find("if (!screen.openStagedReview(false))") != std::string::npos);
    assert(bag.find("Pending Classic Save Changes") == std::string::npos);
    // Party Actions keeps the accepted four commands (no Review row).
    assert(read("src/UI/ClassicPartyActions.inc").find("actionsForSlot(true,{false,false,true},false)") !=
           std::string::npos);
    // Gen I and Gen II Review both page by swipe through the shared models.
    assert(read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc").find("SharedReviewTouch::swipePage(") !=
           std::string::npos);
    assert(read("src/UI/Gen2HardwareWorkspaceFix.inc").find("PokeBank::UIModel::SharedReview::move(") !=
           std::string::npos);
}

} // namespace

int main() {
    partyEditReachesReview(GS, G2::SourceGame::Gold, "gold_gbc");
    partyEditReachesReview(GS, G2::SourceGame::Silver, "silver_gbc");
    partyEditReachesReview(C, G2::SourceGame::Crystal, "crystal_gbc");
    swipePaging();
    routes();
    std::cout << "Party edit -> leave -> Review Changes, Bag -> shared Review, swipe paging (round 5): PASS\n";
    return 0;
}
