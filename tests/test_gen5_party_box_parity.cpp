// Gen V Party/Box parity with the Gen IV shared editor:
// - Box "Move Pokemon" (exact clone to a verified empty slot + removal of the
//   original) staged atomically, rolled back on refusal, exported and reparsed.
// - Party keeps View/Edit/Clone; no party Move/Remove (no party compaction).
// - Party Level edits regenerate stored party data (level byte and stats).
// - Read-only origin fields (met/egg date, egg location) from the cited PK5
//   layout, shown like Gen IV's read-only rows and never editable.
#include "Pokemon/SpeciesMoveDefaults.h"
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5OriginSummary.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "UI/SharedPokemonEditorContract.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
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
using Slot = G::Gen5StagedPokemonWorkspace::Slot;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Surface = G::Gen5SharedScreenState::Surface;

namespace {

void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

// Synthetic SAV5: party Pikachu (Lv 50), Box 1 slot 1 Eevee with native
// met/egg dates, Box 1 slot 2 Snivy. Checksums stamped on the primary copy.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N'; sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
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
    std::vector<uint8_t> eevee(C::StoredSize, 0);
    write32(eevee, 0, 0xABCDEF01);
    C::write16(eevee, 8, 133);
    eevee[0x17] = 2;                        // language
    eevee[0x5F] = version;                  // origin game
    eevee[0x78] = 10; eevee[0x79] = 3; eevee[0x7A] = 6;   // egg date 2010-03-06
    eevee[0x7B] = 11; eevee[0x7C] = 12; eevee[0x7D] = 25; // met date 2011-12-25
    C::write16(eevee, 0x7E, 60003);         // egg location (raw)
    C::write16(eevee, 0x80, 16);            // met location (raw)
    const auto box0 = C::encryptCandidate(eevee);
    std::copy(box0.begin(), box0.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset));
    std::vector<uint8_t> snivy(C::StoredSize, 0);
    write32(snivy, 0, 0x01020304);
    C::write16(snivy, 8, 495);
    const auto box1 = C::encryptCandidate(snivy);
    std::copy(box1.begin(), box1.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + C::StoredSize));
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
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

void exercise(G::SaveFamily family, std::string_view game, uint8_t version) {
    const auto sav = fixture(family, version);
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    assert(parsed && parsed->boxPokemon(0, 0)->species() == 133 && parsed->boxPokemon(0, 1)->species() == 495);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    const auto ok = [](std::string&) { return true; };
    const auto refuse = [](std::string& why) { why = "presentation refused"; return false; };

    // Party: View/Edit/Clone only (Gen IV has no party Move/Release either).
    assert(ui.openActions(ws, Slot{Region::Party, 0, 0}, error));
    auto actions = ui.availableActions(ws);
    assert(has(actions, Action::View) && has(actions, Action::Edit) && has(actions, Action::Clone));
    assert(!has(actions, Action::Move) && !has(actions, Action::Remove));
    assert(!ui.activate(ws, Action::Move, error));
    assert(ui.activate(ws, Action::Close, error) && ui.surface() == Surface::Browse);

    // Box: Move Pokemon is offered next to Clone and Remove.
    assert(ui.openActions(ws, Slot{Region::Box, 0, 0}, error));
    actions = ui.availableActions(ws);
    assert(has(actions, Action::Move) && has(actions, Action::Clone) && has(actions, Action::Remove));
    assert(actions.count <= 8);
    assert(std::string_view(Shared::actionLabel(Action::Move)) == "Move Pokemon");
    assert(ui.activate(ws, Action::Move, error));
    assert(ui.surface() == Surface::CloneDestination && ui.moveDestination());
    assert(ui.cloneDestinationIndex() == 2);           // first verified empty slot
    ui.moveCloneCursor(-1);                             // Box 1 slot 2 is occupied
    assert(!ui.cloneDestinationAllowed(ws));
    assert(!ui.confirmClone(ws, ok, error) && !ws.hasChanges());
    ui.moveCloneCursor(31);                             // Box 2 slot 3
    assert(ui.cloneDestinationAllowed(ws));
    // A refused presentation rolls BOTH halves back.
    assert(!ui.confirmClone(ws, refuse, error) && !ws.hasChanges());
    assert(ui.confirmClone(ws, ok, error) && ui.surface() == Surface::Browse);
    assert(ws.viewBox(0, 0)->empty());
    assert(ws.viewBox(1, 2)->species() == 133 && ws.viewBox(1, 2)->pid() == 0xABCDEF01u);
    assert(ws.changedRecordCount() == 2);
    // The moved source no longer offers Move/Clone (it is a staged slot).
    assert(ui.openActions(ws, Slot{Region::Box, 0, 0}, error));
    assert(!has(ui.availableActions(ws), Action::Move));
    assert(ui.activate(ws, Action::Close, error));
    // Review certifies a byte-exact clone plus a retail encrypted-blank removal.
    for (const auto& change : ws.pendingReview()) {
        const auto verified = G::verifyStagedReview(change, &error);
        assert(verified && verified->fields.size() == 1);
        const auto f = verified->fields[0].field;
        assert(f == G::StagedPokemon5Record::Field::CloneBox || f == G::StagedPokemon5Record::Field::RemoveBox);
    }

    // Workspace-level guards: party sources and staged sources are refused.
    G::Gen5StagedPokemonWorkspace direct(*parsed);
    assert(!direct.moveToEmptyBox(Slot{Region::Party, 0, 0}, 0, 5, &error));
    assert(!direct.moveToEmptyBox(Slot{Region::Box, 0, 0}, 0, 1, &error)); // occupied dest
    assert(!direct.hasChanges());
    assert(direct.moveToEmptyBox(Slot{Region::Box, 0, 1}, 0, 2, &error));
    assert(!direct.moveToEmptyBox(Slot{Region::Box, 0, 1}, 0, 3, &error)); // already moved
    assert(direct.changedRecordCount() == 2);

    // Export: moved record round-trips, nothing else in the boxes changes.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && out->changedRecords == 2 && !out->bagChanged);
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(reopened);
    assert(reopened->boxPokemon(0, 0)->empty());
    assert(reopened->boxPokemon(1, 2)->species() == 133);
    assert(reopened->boxPokemon(0, 1)->species() == 495);
    assert(reopened->partyCount() == 1);
    assert(sav == pristine);
    // Stale source is refused.
    auto stale = sav;
    stale[L::BoxOffset + 5 * C::StoredSize] ^= 1;
    assert(!G::buildStagedSaveCandidateForObservedSource(ws, stale, &error));

    // Party Level edit regenerates stored level and battle stats.
    const auto before = ws.viewParty(0);
    assert(before && before->partyLevel() == 50);
    using Field = G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0, Field::Level, 0, 60, &error));
    const auto after = ws.viewParty(0);
    assert(after && after->partyLevel() == 60);
    const auto oldBytes = before->decodedBytes();
    const auto newBytes = after->decodedBytes();
    const auto stat = [](std::span<const uint8_t> b, size_t off) { return uint16_t(b[off] | (b[off + 1] << 8)); };
    assert(stat(newBytes, 0x90) > stat(oldBytes, 0x90));    // max HP regenerated
    const auto both = G::buildStagedSaveCandidate(ws, &error);
    assert(both && both->changedRecords == 3);
    const auto both2 = G::Gen5ReadOnlySave::parse(both->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(both2 && both2->partyPokemon(0)->partyLevel() == 60);

    // Read-only origin fields from the cited PK5 layout.
    const auto eevee = parsed->boxPokemon(0, 0);
    assert(eevee->metDate().year == 2011 && eevee->metDate().month == 12 && eevee->metDate().day == 25);
    assert(eevee->eggDate().year == 2010 && eevee->eggDate().month == 3 && eevee->eggDate().day == 6);
    assert(eevee->eggLocation() == 60003 && eevee->metLocation() == 16);
    const auto line = G::originSummary5(*eevee);
    assert(line.find("Met 2011-12-25") != std::string::npos);
    assert(line.find("Egg 2010-03-06 @ 60003") != std::string::npos);
    assert(line.find("Language 2") != std::string::npos);
    assert(G::originSummary5(*parsed->boxPokemon(0, 1)).find("Met none") != std::string::npos);
}

// Owner hardware failure 2: on a real Black 2 save an empty box slot offered
// only "Cancel". Retail boxes store the ENCRYPTED blank PK5 (PKHeX
// SaveFile.ClearBoxes writes WriteSlotBox(BlankPKM)), not zero bytes, and the
// old empty-destination check accepted raw zeros only.
void retailEncryptedBlankSlots(G::SaveFamily family, std::string_view game, uint8_t version) {
    auto sav = fixture(family, version);
    const auto& blank = G::encryptedBlankBoxSlot5();
    bool nonzero = false;
    for (auto b : blank) nonzero |= b != 0;
    assert(nonzero && G::nativeBlankSlot5(blank));
    // Every slot except the two real Pokemon becomes the retail blank.
    for (size_t box = 0; box < L::BoxCount; ++box)
        for (size_t slot = 0; slot < L::BoxSlots; ++slot) {
            if (box == 0 && slot < 2) continue;
            std::copy(blank.begin(), blank.end(),
                      sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + box * L::BoxStride + slot * C::StoredSize));
        }
    // A garbage (non-blank, invalid) slot must never become a destination.
    sav[L::BoxOffset + 2 * L::BoxStride + 5 * C::StoredSize + 40] ^= 0x5A;
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    auto copy = std::span<uint8_t>(sav).subspan(0, family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize
                                                                                        : L::Black2White2CopySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    assert(parsed && parsed->diagnostics().invalidBoxRecords == 1);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    const auto ok = [](std::string&) { return true; };

    // Encrypted-blank slot: Add is offered (it was "Cancel" only before).
    assert(ws.emptyCloneDestination(0, 5));
    assert(ui.openActions(ws, Slot{Region::Box, 0, 5}, error));
    auto actions = ui.availableActions(ws);
    assert(has(actions, Action::Add) && !has(actions, Action::View));
    assert(ui.activate(ws, Action::Close, error));
    // Damaged slot: never a destination, no Add.
    assert(!ws.emptyCloneDestination(2, 5));
    assert(ui.openActions(ws, Slot{Region::Box, 2, 5}, error));
    assert(!has(ui.availableActions(ws), Action::Add));
    assert(ui.activate(ws, Action::Close, error));

    // Create into the encrypted blank, Clone and Move into blanks, Remove.
    G::CreateRequest5 r;
    r.game = static_cast<G::CreateGame5>(version);
    r.species = 25; r.level = 12; r.nature = 3; r.gender = 0; r.abilitySlot = 0;
    r.pid = 0x00340012; r.friendship = 70;
    r.gender = G::createGenderForPID5(G::personal5(25, 0, family == G::SaveFamily::Black2White2).info->genderRatio, *r.pid);
    r.trainer = G::CreateTrainer5{12345, 54321, 1, 2, "ROSA"};
    r.nickname = "PIKA";
    r.encounter = {G::CreateHistory5::CallerSuppliedNative, 1, 5, 4, 2024, 1, 1};
    r.moves[0] = {84, 0, std::nullopt};
    assert(ws.createInEmptyBox(r, 0, 5, &error));
    assert(ws.viewBox(0, 5)->species() == 25);
    assert(ws.cloneToEmptyBox(Slot{Region::Box, 0, 0}, 3, 7, &error));
    assert(ws.moveToEmptyBox(Slot{Region::Box, 0, 1}, 4, 9, &error));
    for (const auto& change : ws.pendingReview()) assert(G::verifyStagedReview(change, &error));
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && out->changedRecords == 4);
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(reopened);
    assert(reopened->boxPokemon(0, 5)->species() == 25);
    assert(reopened->boxPokemon(3, 7)->species() == 133);
    assert(reopened->boxPokemon(4, 9)->species() == 495);
    // The moved-out slot is written as the retail encrypted blank, not zeros.
    const auto vacated = reopened->boxPokemon(0, 1);
    assert(vacated->valid() && vacated->empty());
    assert(std::equal(vacated->originalEncryptedBytes().begin(), vacated->originalEncryptedBytes().end(),
                      blank.begin()));
    assert(sav == pristine);
    (void)ok;
}

// Hardware failure fa9cfcea: Create showed only Cancel and was a raw prompt
// wizard. Route: empty retail-blank Box slot -> Actions -> Add -> Create ->
// the SAME Edit draft as Edit (species, nickname, level, nature, IVs, EVs,
// moves, ability, gender, held item) -> Keep -> Review -> Export -> reopen.
void sharedCreateRoute(G::SaveFamily family, std::string_view game, uint8_t version) {
    using NF = G::StagedPokemon5Record::Field;
    auto sav = fixture(family, version);
    sav[0x19400 + 0x1E] = 2;  // English
    sav[0x19400 + 0x21] = 1;  // female trainer
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
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    const auto ok = [](std::string&) { return true; };
    const auto refuse = [](std::string& why) { why = "renderer refused"; return false; };
    const G::Gen5SharedScreenState::CreateDate5 date{2026, 10, 15};

    // Defaults copy the save's own trainer; nothing is invented from a filename.
    const auto defaults = G::Gen5SharedScreenState::sharedCreateDefaults(ws, 0x00340000u, "Pikachu", date, error);
    assert(defaults && defaults->trainer && defaults->trainer->name == "NX");
    assert(std::all_of(defaults->ivs.begin(), defaults->ivs.end(), [](auto value) { return value == 0; }));
    assert(defaults->trainer->tid == 12345 && defaults->trainer->sid == 54321);
    assert(defaults->trainer->gender == 1 && defaults->trainer->language == 2);
    assert(static_cast<uint32_t>(defaults->game) == version && defaults->gender == 1);

    // Occupied slot: Create refused.
    assert(ui.openActions(ws, Slot{Region::Box, 0, 0}, error));
    assert(!has(ui.availableActions(ws), Action::Add));
    assert(ui.activate(ws, Action::Close, error));

    // Empty slot -> Add -> Create opens the shared Edit draft on Species.
    assert(ui.openActions(ws, Slot{Region::Box, 0, 5}, error));
    assert(ui.activate(ws, Action::Add, error) && ui.surface() == Surface::ConfirmCreateBox);
    // A renderer refusal rolls the whole Create back.
    assert(!ui.beginCreateDraft(ws, *defaults, refuse, error) && !ws.hasChanges());
    assert(ui.surface() == Surface::ConfirmCreateBox && ws.emptyCloneDestination(0, 5));
    assert(ui.beginCreateDraft(ws, *defaults, ok, error));
    assert(ui.surface() == Surface::Edit && ui.createPending());
    assert(ui.draft().current()->species() == 25);

    // A pending Create has no history: any BW/B2W2 species rebuilds it natively,
    // even across EXP growth groups (Bulbasaur), and a default nickname follows.
    // R2-BUG-004: each real species change brings that species' own native level-up moves
    // (PKHeX lvlmove_bw / _b2w2 at the draft level, full PP, no PP Ups); Pikachu's Thunder
    // Shock never survives into Bulbasaur or Eevee. A same-species (shiny) rebuild keeps them.
    const auto table = PokeBank::Moves::levelUpTableForGen45Version(ws.sourceSave().trainer().gameVersion);
    const auto nativeMoves = [&](uint16_t species) {
        const auto level = static_cast<uint8_t>(ui.draft().current()->partyRecord()
            ? ui.draft().current()->partyLevel() : 5);
        const auto d = PokeBank::Moves::defaultMoves(table, species, 0, level);
        assert(d.known && d.count > 0);
        return d.moves;
    };
    const auto movesNow = [&]() {
        const auto m = ui.draft().current()->moves();
        return std::array<uint16_t, 4>{m[0], m[1], m[2], m[3]};
    };
    assert(ui.recreateCreateSpecies(ws, 1, false, "Bulbasaur", "Pikachu", ok, error));
    assert(ui.draft().current()->species() == 1 && ui.createPending());
    assert(G::asciiName5(ui.draft().current()->decodedBytes(), true) == "Bulbasaur");
    assert(movesNow() == nativeMoves(1));
    for (size_t i = 0; i < 4; ++i) {
        assert(ui.draft().current()->ppUps()[i] == 0);
        assert(ui.draft().current()->pp()[i] == G::basePP5(movesNow()[i]));
    }
    assert(!ui.recreateCreateSpecies(ws, 650, false, "X", "Bulbasaur", ok, error));
    assert(movesNow() == nativeMoves(1));  // a refused rebuild changes nothing
    assert(ui.recreateCreateSpecies(ws, 133, true, "Eevee", "Bulbasaur", ok, error));
    assert(ui.draft().current()->species() == 133 && ui.draft().current()->shiny());
    assert(G::asciiName5(ui.draft().current()->decodedBytes(), true) == "Eevee");
    const auto eeveeMoves = nativeMoves(133);
    assert(movesNow() == eeveeMoves);
    assert(ui.recreateCreateSpecies(ws, 133, false, "Eevee", "Eevee", ok, error));
    assert(!ui.draft().current()->shiny());
    assert(movesNow() == eeveeMoves);  // same species: moves untouched

    // Edit every owner-listed field through the same native draft the shared
    // editor workspace stages into.
    auto& draft = ui.editDraft();
    assert(draft.stageName(NF::NicknameText, "SPARKY", error));
    assert(draft.stage(NF::Level, 0, 20, error));
    assert(draft.stage(NF::Nature, 0, 10, error));
    assert(draft.stage(NF::IV, 1, 31, error));
    assert(draft.stage(NF::EV, 3, 252, error));
    assert(draft.stage(NF::HeldItem, 0, 1, error));
    assert(draft.stage(NF::Move, 1, 98, error));  // Quick Attack
    assert(G::EditorFields::toggleShiny(draft, error) && draft.current()->shiny());
    assert(ui.surface() == Surface::Edit && ui.draft().dirty());

    // B never drops a pending Create: Keep/Discard decision first.
    assert(!ui.back() && ui.surface() == Surface::ConfirmDraft);
    ui.continueEditing();
    assert(ui.surface() == Surface::Edit && ui.draft().dirty());
    assert(ui.keepWithPresentation(ws, ok, error));
    assert(ui.surface() == Surface::Browse && !ui.createPending());
    assert(ws.changedRecordCount() == 1);

    // Clean Create + Keep just finalizes the default record.
    assert(ui.openActions(ws, Slot{Region::Box, 0, 7}, error));
    assert(ui.activate(ws, Action::Add, error));
    assert(ui.beginCreateDraft(ws, *defaults, ok, error));
    assert(ui.keepWithPresentation(ws, ok, error) && !ui.createPending());
    assert(ws.changedRecordCount() == 2);

    // Create then Discard restores the native empty slot exactly.
    assert(ui.openActions(ws, Slot{Region::Box, 0, 6}, error));
    assert(ui.activate(ws, Action::Add, error));
    assert(ui.beginCreateDraft(ws, *defaults, ok, error));
    assert(ws.changedRecordCount() == 3);
    ui.discardDraft();  // cannot orphan a created record
    assert(ui.surface() == Surface::Edit && ui.createPending());
    assert(!ui.back() && ui.surface() == Surface::ConfirmDraft);
    assert(ui.discardCreateDraft(ws, ok, error));
    assert(ui.surface() == Surface::Browse && !ui.createPending());
    assert(ws.changedRecordCount() == 2 && ws.emptyCloneDestination(0, 6));

    // Review -> Export -> reopen.
    for (const auto& change : ws.pendingReview()) assert(G::verifyStagedReview(change, &error));
    assert(ui.openReviewFromBrowse(ws) && ui.surface() == Surface::Review);
    assert(ui.requestExport(ws) && ui.surface() == Surface::ConfirmExport);
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && out->changedRecords == 2);
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(reopened);
    const auto made = reopened->boxPokemon(0, 5);
    assert(made && made->valid() && made->species() == 133 && made->nature() == 10);
    assert(made->ivs()[1] == 31 && made->evs()[3] == 252 && made->heldItem() == 1);
    assert(made->moves()[0] == eeveeMoves[0] && made->moves()[1] == 98 && made->tid() == 12345);
    assert(G::asciiName5(made->decodedBytes(), true).value_or("") == "SPARKY");
    assert(reopened->boxPokemon(0, 7)->species() == 25);
    const auto vacated = reopened->boxPokemon(0, 6);
    assert(vacated->valid() && vacated->empty());
    assert(sav == pristine);
}

} // namespace

int main() {
    using Shared::FieldAccess;
    using Shared::FieldIdentity;
    using Shared::Generation;
    for (const auto field : {FieldIdentity::MetDate, FieldIdentity::Egg, FieldIdentity::EggLocation,
                             FieldIdentity::EggDate}) {
        assert(Shared::fieldAccessForGeneration(Generation::Gen4, field) == FieldAccess::ReadOnly);
        assert(Shared::fieldAccessForGeneration(Generation::Gen5, field) == FieldAccess::ReadOnly);
    }
    retailEncryptedBlankSlots(G::SaveFamily::BlackWhite, "black_nds", 21);
    retailEncryptedBlankSlots(G::SaveFamily::Black2White2, "black2_nds", 23);
    sharedCreateRoute(G::SaveFamily::BlackWhite, "white_nds", 20);
    sharedCreateRoute(G::SaveFamily::BlackWhite, "black_nds", 21);
    sharedCreateRoute(G::SaveFamily::Black2White2, "white2_nds", 22);
    sharedCreateRoute(G::SaveFamily::Black2White2, "black2_nds", 23);
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20);
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23);
    std::cout << "Gen V Party/Box parity: Move Pokemon, party actions, party stat regeneration, "
                 "read-only origin fields PASS\n";
}
