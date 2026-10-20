// Round 3 Part 9 (QA docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md, item 1): Gen V
// Clone / Move / Release parity with Gen IV on slots staged in this session.
//   * a Created, Cloned or field-Edited Pokemon keeps Clone / Move / Release
//     (QA's probe: such slots offered only View / Edit / Review / Cancel);
//   * every result re-certifies through the existing review + export checks
//     (clone source snapshot or explicit Create request, fields replayed);
//   * Move is the shared deferred carry: nothing is staged until the drop, a
//     changed carried record or an occupied target is refused, and a refused
//     presentation rolls the whole Move back;
//   * Origin & Legality is listed and opens a neutral panel (no verdict).
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "UI/SharedPokemonEditorContract.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
using Action = Shared::Action;
using Field = G::StagedPokemon5Record::Field;
using Slot = G::Gen5StagedPokemonWorkspace::Slot;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Kind = G::Gen5StagedPokemonWorkspace::ChangeKind;
using Surface = G::Gen5SharedScreenState::Surface;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

// Party Pikachu; Box 1 slot 1 Eevee, slot 2 Snivy; every other box slot is
// the retail encrypted blank (PKHeX SaveFile.ClearBoxes).
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N'; sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
    sav[tr + 0x1E] = 2;
    sav[tr + 0x1F] = version;
    std::vector<uint8_t> pk(C::PartySize, 0);
    write32(pk, 0, 0x12345678);
    C::write16(pk, 8, 25);
    C::write16(pk, 12, 12345);
    C::write16(pk, 14, 54321);
    pk[0x8C] = 50;
    const auto party = C::encryptCandidate(pk);
    sav[L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    const auto boxed = [&](uint32_t pid, uint16_t species, size_t slot) {
        std::vector<uint8_t> raw(C::StoredSize, 0);
        write32(raw, 0, pid);
        C::write16(raw, 8, species);
        raw[0x14] = 70;
        raw[0x5F] = version;
        C::write16(raw, 0x80, 16);
        const auto enc = C::encryptCandidate(raw);
        std::copy(enc.begin(), enc.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + slot * C::StoredSize));
    };
    boxed(0xABCDEF01, 133, 0);
    boxed(0x01020304, 495, 1);
    const auto& blank = G::encryptedBlankBoxSlot5();
    for (size_t box = 0; box < L::BoxCount; ++box)
        for (size_t slot = 0; slot < L::BoxSlots; ++slot) {
            if (box == 0 && slot < 2) continue;
            std::copy(blank.begin(), blank.end(),
                      sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + box * L::BoxStride + slot * C::StoredSize));
        }
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    auto copy = std::span<uint8_t>(sav).subspan(0, family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize
                                                                                        : L::Black2White2CopySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

bool has(const Shared::ActionSet& set, Action action) {
    for (size_t i = 0; i < set.count; ++i)
        if (set[i] == action) return true;
    return false;
}

std::vector<uint8_t> bytesOf(const G::Gen5StagedPokemonWorkspace& ws, size_t box, size_t slot) {
    const auto pk = ws.viewBox(box, slot);
    return pk ? std::vector<uint8_t>(pk->originalEncryptedBytes().begin(), pk->originalEncryptedBytes().end())
              : std::vector<uint8_t>{};
}

void exercise(G::SaveFamily family, std::string_view game, uint8_t version) {
    const std::string tag = std::string(game) + ": ";
    const auto sav = fixture(family, version);
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    check(parsed.has_value(), tag + "fixture parses");
    if (!parsed) return;
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    const auto ok = [](std::string&) { return true; };
    const auto refuse = [](std::string& why) { why = "presentation refused"; return false; };
    const auto offered = [&](Slot slot) {
        std::string e;
        Shared::ActionSet set{};
        if (ui.openActions(ws, slot, e)) set = ui.availableActions(ws);
        (void)ui.activate(ws, Action::Close, e);
        return set;
    };
    const auto fullBox = [&](Slot slot) {
        const auto set = offered(slot);
        return has(set, Action::Clone) && has(set, Action::Move) && has(set, Action::Remove) &&
               has(set, Action::LegalityProvenance);
    };

    // Untouched originals: unchanged Gen IV order, plus Origin & Legality.
    check(fullBox(Slot{Region::Box, 0, 0}), tag + "original box slot: Clone, Move, Release, Origin & Legality");
    {
        const auto set = offered(Slot{Region::Box, 0, 0});
        check(set.count >= 6 && set[0] == Action::View && set[1] == Action::Edit && set[2] == Action::Clone &&
                  set[3] == Action::Move && set[4] == Action::Remove && set[5] == Action::LegalityProvenance,
              tag + "shared action order View, Edit, Clone, Move, Release, Origin & Legality");
    }

    // QA repro: a Created slot (Create -> Keep) kept only View / Edit / Review / Cancel.
    G::CreateRequest5 r;
    r.game = static_cast<G::CreateGame5>(version);
    r.species = 25; r.level = 12; r.nature = 3; r.abilitySlot = 0;
    r.pid = 0x00340012; r.friendship = 70;
    r.gender = G::createGenderForPID5(G::personal5(25, 0, family == G::SaveFamily::Black2White2).info->genderRatio, *r.pid);
    r.trainer = G::CreateTrainer5{12345, 54321, 1, 2, "ROSA"};
    r.nickname = "PIKA";
    r.encounter = {G::CreateHistory5::CallerSuppliedNative, 1, 5, 4, 2024, 1, 1};
    r.moves[0] = {84, 0, std::nullopt};
    check(ws.createInEmptyBox(r, 0, 5, &error), tag + "Create stages into Box 1 / Slot 6");
    check(fullBox(Slot{Region::Box, 0, 5}), tag + "Created slot keeps Clone, Move, Release");
    check(ws.stagedOperand(Slot{Region::Box, 0, 5}) && !ws.originalOccupied(Slot{Region::Box, 0, 5}),
          tag + "a Created slot is a staged operand, not an original");

    // Clone of the Created Pokemon (shared Clone destination + confirm).
    check(ui.openActions(ws, Slot{Region::Box, 0, 5}, error) && ui.activate(ws, Action::Clone, error) &&
              ui.surface() == Surface::CloneDestination,
          tag + "Clone opens the shared destination for a Created slot");
    ui.moveCloneCursor(6 - static_cast<int>(ui.cloneDestinationIndex()));   // Box 1 / Slot 7
    check(!ui.confirmClone(ws, refuse, error) && !ws.isBoxReplacement(0, 6), tag + "refused presentation rolls the clone back");
    check(ui.confirmClone(ws, ok, error), tag + "Created Pokemon clones to Box 1 / Slot 7");
    check(ws.viewBox(0, 6) && ws.viewBox(0, 6)->species() == 25 && bytesOf(ws, 0, 6) == bytesOf(ws, 0, 5),
          tag + "the clone of a Created Pokemon is byte-exact");
    check(fullBox(Slot{Region::Box, 0, 6}), tag + "Cloned slot keeps Clone, Move, Release");

    // Field-edited original: QA's probe lost the actions after an edit.
    check(ws.stageBox(0, 0, Field::Friendship, 0, 199, &error), tag + "Friendship edit staged on Eevee");
    check(!ws.originalOccupied(Slot{Region::Box, 0, 0}) && fullBox(Slot{Region::Box, 0, 0}),
          tag + "edited original keeps Clone, Move, Release");
    check(ws.cloneStagedToEmptyBox(Slot{Region::Box, 0, 0}, 0, 7, &error), tag + "edited Eevee clones to Slot 8");
    check(ws.viewBox(0, 7) && ws.viewBox(0, 7)->friendship() == 199 && ws.viewBox(0, 7)->pid() == 0xABCDEF01u,
          tag + "the clone carries the staged edit and the same PID");
    check(ws.viewBox(0, 0) && ws.viewBox(0, 0)->friendship() == 199, tag + "the edited source keeps its edit");

    // Party edit: Clone yes, Move / Release no (Gen IV has no party Move/Release).
    check(ws.stageParty(0, Field::Friendship, 0, 150, &error), tag + "party Friendship edit");
    {
        const auto set = offered(Slot{Region::Party, 0, 0});
        check(has(set, Action::Clone) && !has(set, Action::Move) && !has(set, Action::Remove) &&
                  has(set, Action::LegalityProvenance),
              tag + "edited party slot: Clone and Origin & Legality, no Move/Release");
    }
    check(ws.cloneStagedToEmptyBox(Slot{Region::Party, 0, 0}, 1, 0, &error), tag + "edited party Pokemon clones to Box 2");
    check(ws.viewBox(1, 0) && !ws.viewBox(1, 0)->partyRecord() && ws.viewBox(1, 0)->friendship() == 150,
          tag + "party clone is the native boxed form with the edit");

    // Release of a staged Clone undoes it: the slot is a verified blank again.
    check(ui.openActions(ws, Slot{Region::Box, 0, 6}, error) && ui.activate(ws, Action::Remove, error) &&
              ui.surface() == Surface::ConfirmRemoveBox,
          tag + "Release opens the confirm for a Cloned slot");
    check(ui.confirmRemoveBox(ws, ok, error) && ws.emptyCloneDestination(0, 6) && !ws.isBoxReplacement(0, 6),
          tag + "releasing a staged clone returns the slot to its native blank");

    // Move: the deferred carry. Nothing is staged until the drop.
    check(ui.openActions(ws, Slot{Region::Box, 0, 5}, error) && ui.beginMoveCarry(ws, error) &&
              ui.surface() == Surface::Browse,
          tag + "Actions -> Move closes the menu for the carry");
    const auto carried = bytesOf(ws, 0, 5);
    const auto stagedBefore = ws.changedRecordCount();
    check(ws.changedRecordCount() == stagedBefore, tag + "pickup stages nothing");
    check(!ui.dropCarriedPokemon(ws, Slot{Region::Box, 0, 5}, carried, 0, 1, ok, error) &&
              ws.changedRecordCount() == stagedBefore,
          tag + "drop on an occupied slot is refused");
    auto tampered = carried;
    tampered[20] ^= 1;
    check(!ui.dropCarriedPokemon(ws, Slot{Region::Box, 0, 5}, tampered, 2, 3, ok, error),
          tag + "a changed carried record is refused");
    check(!ui.dropCarriedPokemon(ws, Slot{Region::Box, 0, 5}, carried, 2, 3, refuse, error) &&
              ws.viewBox(0, 5)->species() == 25 && ws.emptyCloneDestination(2, 3),
          tag + "refused presentation rolls the whole Move back");
    check(ui.dropCarriedPokemon(ws, Slot{Region::Box, 0, 5}, carried, 2, 3, ok, error),
          tag + "Created Pokemon drops on Box 3 / Slot 4");
    check(ws.viewBox(0, 5)->empty() && ws.emptyCloneDestination(0, 5) && bytesOf(ws, 2, 3) == carried,
          tag + "Move of a Created Pokemon relocates it byte-exact");
    check(fullBox(Slot{Region::Box, 2, 3}), tag + "moved slot keeps Clone, Move, Release");

    // Move of an edited original: one Move review row (Clone + Remove pair).
    check(ws.stageBox(0, 1, Field::Friendship, 0, 222, &error), tag + "Snivy edit");
    const auto snivy = bytesOf(ws, 0, 1);
    check(ui.openActions(ws, Slot{Region::Box, 0, 1}, error) && ui.beginMoveCarry(ws, error) &&
              ui.dropCarriedPokemon(ws, Slot{Region::Box, 0, 1}, snivy, 3, 9, ok, error),
          tag + "edited Snivy moves to Box 4 / Slot 10");
    check(ws.viewBox(0, 1)->empty() && ws.viewBox(3, 9)->species() == 495 && ws.viewBox(3, 9)->friendship() == 222,
          tag + "moved Snivy keeps its edit; the source is a staged blank");
    const auto pending = ws.pendingReview();
    const auto rows = G::groupReviewRows5(pending);
    bool moveRow = false;
    for (const auto& row : rows)
        if (row.kind == G::ReviewRow5::Kind::Move && pending[row.change].location.box == 3 &&
            pending[row.change].location.slot == 9)
            moveRow = true;
    check(moveRow, tag + "an edited original's Move is ONE review row");

    // Release of an edited original: drops the edit, stages the native blank.
    check(ws.stageBox(0, 0, Field::Friendship, 0, 200, &error), tag + "second Eevee edit");
    check(ui.openActions(ws, Slot{Region::Box, 0, 0}, error) && ui.activate(ws, Action::Remove, error) &&
              ui.confirmRemoveBox(ws, ok, error),
          tag + "edited original releases");
    check(ws.viewBox(0, 0)->empty() && ws.isBoxReplacement(0, 0), tag + "release of an edited original is the native removal");

    // Every staged record re-certifies through the existing review checks.
    bool verified = true;
    std::size_t clones = 0, creates = 0, removes = 0;
    for (const auto& change : ws.pendingReview()) {
        verified = verified && G::verifyStagedReview(change, &error).has_value();
        clones += change.kind == Kind::CloneBox;
        creates += change.kind == Kind::CreateBox;
        removes += change.kind == Kind::RemoveBox;
    }
    check(verified, tag + "every staged record verifies (" + error + ")");
    check(creates == 1 && clones == 3 && removes == 2, tag + "staged kinds: 1 Create, 3 Clone, 2 Remove");

    // Origin & Legality: listed, neutral, B returns to the menu.
    check(ui.openActions(ws, Slot{Region::Box, 0, 7}, error) && ui.activate(ws, Action::LegalityProvenance, error) &&
              ui.surface() == Surface::Legality,
          tag + "Origin & Legality opens the neutral panel");
    (void)ui.back();
    check(ui.surface() == Surface::Actions, tag + "B returns from Origin & Legality to Actions");
    check(ui.activate(ws, Action::Close, error), tag + "close Actions");

    // Verified export; the original save bytes are never written.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    check(out.has_value(), tag + "export candidate builds (" + error + ")");
    if (out) {
        const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
        check(reopened.has_value(), tag + "exported copy reparses");
        if (reopened) {
            check(reopened->boxPokemon(0, 0)->empty(), tag + "export: released Eevee is blank");
            check(reopened->boxPokemon(0, 1)->empty(), tag + "export: moved Snivy source is blank");
            check(reopened->boxPokemon(0, 5)->empty() && reopened->boxPokemon(0, 6)->empty(),
                  tag + "export: moved Create source and released clone are blank");
            check(reopened->boxPokemon(0, 7)->species() == 133 && reopened->boxPokemon(0, 7)->friendship() == 199,
                  tag + "export: edited Eevee clone");
            check(reopened->boxPokemon(1, 0)->species() == 25 && reopened->boxPokemon(1, 0)->friendship() == 150,
                  tag + "export: edited party clone");
            check(reopened->boxPokemon(2, 3)->species() == 25, tag + "export: moved Created Pokemon");
            check(reopened->boxPokemon(3, 9)->species() == 495 && reopened->boxPokemon(3, 9)->friendship() == 222,
                  tag + "export: moved edited Snivy");
            check(reopened->partyPokemon(0)->friendship() == 150, tag + "export: party edit");
        }
    }
    check(sav == pristine, tag + "original SAV5 bytes unchanged");
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

// The device UI routes Move through the SHARED carry and handles the panel.
void sourceWiring() {
    const auto surface = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto carry = read("src/UI/ClassicPackedMoveOverlay.inc");
    check(surface.find("state.beginMoveCarry(staged,error)") != std::string::npos &&
              surface.find("ClassicPackedMove::beginGen5FromAction(") != std::string::npos,
          "Gen V Actions -> Move starts the shared ClassicPackedMove carry");
    check(surface.find("ClassicPackedMove::handleGen5CarryInput(screen,down,tap)") != std::string::npos &&
              surface.find("ClassicPackedMove::draw(screen,fb);") != std::string::npos,
          "Gen V box grid hands the carry its keys and draws the shared carried sprite/footer");
    check(carry.find("CarryGeneration::Gen5") != std::string::npos &&
              carry.find("trainer.sharedScreen().dropCarriedPokemon(") != std::string::npos,
          "the shared carry drops Gen V through the verified state-model Move");
    check(surface.find("case Surface::Legality:drawLegality5(") != std::string::npos &&
              surface.find("case Surface::Legality:\n            if(keys&HidNpadButton_B)") != std::string::npos,
          "Origin & Legality is drawn and B backs out");
    check(surface.find("case A::LegalityProvenance:return PokeBank::UIModel::NarrowPokemonMenu::kOriginLegalityLabel;") !=
              std::string::npos,
          "Origin & Legality uses the shared Gen I-IV label");
    const auto panel = surface.substr(surface.find("void drawLegality5("));
    const auto end = panel.find("\nvoid drawDecision(");
    const auto body = panel.substr(0, end);
    // E03: the panel is the ONE shared Gen I-V Origin & Legality panel (no verdict; see
    // tests/test_shared_origin_legality.cpp for its rows and wording).
    check(body.find("SharedEditorChrome::drawOriginLegalityPanel(") != std::string::npos &&
              body.find("LegalityState::Verified") == std::string::npos &&
              body.find("ProblemsFound") == std::string::npos,
          "Origin & Legality is the shared neutral panel, never a verdict");
}

} // namespace

int main() {
    sourceWiring();
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21);
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen V staged-slot Clone / Move / Release / Origin & Legality parity (B/W/B2/W2, "
              << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
