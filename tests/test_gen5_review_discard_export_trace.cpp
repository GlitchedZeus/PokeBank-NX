// S04 hardware-item trace: "Review / Discard / Export Copy work - changes are
// only staged until explicitly exported" (Generation V).
//
// One end-to-end host trace per exact title (W, B, W2, B2) through the REAL
// production Gen V staged-workspace code, driven the way the shared Gen V
// screen (src/UI/Gen5SharedReadOnlySurface.inc) drives it:
//   Gen5ReadOnlyTrainer (shared Trainer bridge + presentation refresh)
//   Gen5SharedScreenState (Actions / Add / Edit / Clone / Move / Release /
//                          Review / Discard All / Export confirmation)
//   Gen5SharedEditorBridge stageSharedField / stageSharedName
//   Gen5BagScreen (shared Items panel quantity keypad)
//   verifyStagedReview / bagReviewChanges (what Review lists)
//   readSourceFileReadOnly + publishGen5WorkingCopy (Export Copy)
//   readNormalizedSourceReadOnly + Gen5ReadOnlySave::parse (reopen export)
//
// The fixture save is the same synthetic, checksum-valid SAV5 the existing
// accepted tests build (tests/test_gen5_source_e2e.cpp writeCopy() for the
// trainer/party/bag, tests/test_gen5_staged_save_candidate.cpp fixture() for
// boxed PK5 records). No offsets, block tables or checksums are invented:
// layout comes from Gen5::LayoutInfo / Gen5Bag / Gen5Trainer headers.
#include "Integration/Gen5/Gen5BagScreen.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Utils/SHA256.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace B = PokeVault::Legacy;
namespace fs = std::filesystem;

namespace {

int failures = 0;
int checks = 0;
void require(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

using Bytes = std::vector<uint8_t>;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Kind = G::Gen5StagedPokemonWorkspace::ChangeKind;
using Field = G::StagedPokemon5Record::Field;
using Surface = G::Gen5SharedScreenState::Surface;
using Action = G::Gen5SharedScreenState::Action;
using FieldId = G::Gen5SharedScreenState::Field;

std::span<const L::SaveBlock> blocksFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                               : std::span<const L::SaveBlock>(L::Black2White2);
}
size_t copySizeFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
}
void stampCopy(Bytes& sav, size_t base, G::SaveFamily family) {
    auto copy = std::span<uint8_t>(sav).subspan(base, copySizeFor(family));
    for (const auto& b : blocksFor(family)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}
void write32(Bytes& s, size_t o, uint32_t v) {
    C::write16(s, o, static_cast<uint16_t>(v));
    C::write16(s, o + 2, static_cast<uint16_t>(v >> 16));
}
void putBoxRecord(Bytes& sav, size_t base, size_t slot, uint32_t pid, uint16_t species) {
    // Same boxed record shape as test_gen5_staged_save_candidate.cpp fixture().
    Bytes box(C::StoredSize, 0);
    write32(box, 0, pid);
    C::write16(box, 8, species);
    const auto boxed = C::encryptCandidate(box);
    std::copy(boxed.begin(), boxed.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + slot * C::StoredSize));
}
// test_gen5_source_e2e.cpp writeCopy(): trainer `initial`+"X", one party
// Pikachu, Medicine pocket Potion x`potions`, money. Optionally three boxed
// PK5 in Box 1 slots 1-3 (Eevee, Pikachu, Eevee).
void writeCopy(Bytes& sav, size_t base, G::SaveFamily family, uint8_t version, char initial,
               uint16_t potions, bool boxed) {
    const size_t player = base + 0x19400;
    C::write16(sav, player + 0x04, static_cast<uint16_t>(initial));
    C::write16(sav, player + 0x06, 'X');
    C::write16(sav, player + 0x08, 0xFFFF);
    C::write16(sav, player + 0x14, 12345);
    C::write16(sav, player + 0x16, 54321);
    sav[player + 0x1F] = version;
    C::write16(sav, player + 0x24, 5);
    const size_t misc = base + G::trainerBlock5(family, G::TrainerLayout5::MiscBlockIndex).offset;
    C::write16(sav, misc, 1000);
    Bytes pk(C::PartySize, 0);
    C::write16(pk, 0, 0x1357);
    C::write16(pk, 8, 25);
    C::write16(pk, 12, 12345);
    C::write16(pk, 14, 54321);
    pk[0x8C] = 20;
    const auto party = C::encryptCandidate(pk);
    sav[base + L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    const size_t medicine = base + G::itemsBlockOffset(family) + G::bagPocketLayout5(family, 3).offset;
    C::write16(sav, medicine, 17);
    C::write16(sav, medicine + 2, potions);
    if (boxed) {
        putBoxRecord(sav, base, 0, 0x00002123u, 133);
        putBoxRecord(sav, base, 1, 0x00004321u, 25);
        putBoxRecord(sav, base, 2, 0x00006543u, 133);
    }
    stampCopy(sav, base, family);
}

Bytes diskBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool writeFile(const std::string& path, const Bytes& bytes) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    return std::fclose(f) == 0 && ok;
}
std::string sha256(const Bytes& bytes) {
    Utils::SHA256 h;
    h.update(bytes.data(), bytes.size());
    uint8_t out[Utils::PKSE_SHA256_HASH_SIZE];
    h.finalize(out);
    static const char* hex = "0123456789abcdef";
    std::string s;
    for (uint8_t b : out) { s.push_back(hex[b >> 4]); s.push_back(hex[b & 15]); }
    return s;
}
// Every regular file under root -> SHA-256 of its content.
std::map<std::string, std::string> tree(const std::string& root) {
    std::map<std::string, std::string> out;
    for (const auto& e : fs::recursive_directory_iterator(root))
        if (e.is_regular_file())
            out[fs::relative(e.path(), root).string()] = sha256(diskBytes(e.path().string()));
    return out;
}

// What Review lists, in production order: staged PK5 records (each certified
// by verifyStagedReview), then Bag changes, then Trainer changes.
struct ReviewRow {
    G::Gen5StagedPokemonWorkspace::Slot location{};
    Kind kind = Kind::FieldEdit;
    std::vector<G::VerifiedFieldChange> fields;
    bool verified = false;
};
std::vector<ReviewRow> reviewRows(const G::Gen5StagedPokemonWorkspace& ws) {
    std::vector<ReviewRow> rows;
    for (const auto& change : ws.pendingReview()) {
        std::string error;
        const auto v = G::verifyStagedReview(change, &error);
        rows.push_back({change.location, change.kind, v ? v->fields : std::vector<G::VerifiedFieldChange>{},
                        static_cast<bool>(v)});
    }
    return rows;
}
const ReviewRow* findRow(const std::vector<ReviewRow>& rows, Region region, size_t box, size_t slot) {
    for (const auto& r : rows)
        if (r.location.region == region && r.location.box == box && r.location.slot == slot) return &r;
    return nullptr;
}
bool hasField(const ReviewRow* r, Field f, uint32_t before, uint32_t after, bool checkValues = true) {
    if (!r) return false;
    for (const auto& c : r->fields)
        if (c.field == f && (!checkValues || (c.before == before && c.after == after))) return true;
    return false;
}

// A complete, comparable fingerprint of everything the workspace stages.
Bytes stagedFingerprint(const G::Gen5StagedPokemonWorkspace& ws) {
    Bytes out;
    for (const auto& c : ws.pendingReview()) {
        out.push_back(static_cast<uint8_t>(c.location.region));
        out.push_back(static_cast<uint8_t>(c.location.box));
        out.push_back(static_cast<uint8_t>(c.location.slot));
        out.push_back(static_cast<uint8_t>(c.kind));
        out.insert(out.end(), c.before.begin(), c.before.end());
        out.insert(out.end(), c.after.begin(), c.after.end());
    }
    if (const auto* bag = ws.stagedBagIfOpened()) out.insert(out.end(), bag->staged().begin(), bag->staged().end());
    for (const auto& t : ws.trainerReviewChanges()) out.insert(out.end(), t.after.begin(), t.after.end());
    return out;
}

bool sameSpan(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
bool sameBag(const G::ValidatedBag5& a, const G::ValidatedBag5& b) {
    for (size_t p = 0; p < G::BagPocket5Count; ++p) {
        if (a[p].size() != b[p].size()) return false;
        for (size_t i = 0; i < a[p].size(); ++i)
            if (a[p][i].itemId != b[p][i].itemId || a[p][i].count != b[p][i].count) return false;
    }
    return true;
}

G::CreateRequest5 createRequest(uint8_t version) {
    // Same explicit native request as tests/test_gen5_create_integration.cpp.
    G::CreateRequest5 r;
    r.game = static_cast<G::CreateGame5>(version);
    r.species = 25; r.level = 70;
    r.nature = 3; r.gender = 0; r.abilitySlot = 0; r.pid = 0x123400ff; r.friendship = 70; r.heldItem = 638;
    r.trainer = G::CreateTrainer5{100, 200, 0, 2, "TEST"};
    r.nickname = "CREATED";
    r.encounter = {G::CreateHistory5::CallerSuppliedNative, 1, 1, 4, 2024, 1, 1};
    r.moves[0] = {1, 0, std::nullopt};
    return r;
}

constexpr uint16_t kHeldItem = 234;   // Leftovers (Gen V range <= 638)
constexpr uint16_t kPotions = 3;
constexpr uint16_t kNewPotions = 42;
constexpr uint32_t kFirstMoney = 4321;   // staged, then discarded
constexpr uint32_t kRedoMoney = 98765;   // staged after Discard All and exported
constexpr uint32_t kEditLevel = 30;

void trace(const std::string& dir, G::SaveFamily family, const char* game, uint8_t version, bool backup) {
    const std::string T = std::string("S04 [") + game + (backup ? " backup] " : " primary] ");
    const size_t copySize = copySizeFor(family);
    const size_t base = backup ? copySize : 0;
    const size_t otherBase = backup ? 0 : copySize;
    const auto selection = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    auto blocks = blocksFor(family);

    // ---- Source file on disk --------------------------------------------
    Bytes raw(L::FullSaveSize, 0);
    writeCopy(raw, base, family, version, 'S', kPotions, true);
    writeCopy(raw, otherBase, family, version, 'O', 9, false);
    const std::string srcDir = dir + "/source";
    const std::string exportRoot = dir + "/exports";
    fs::create_directories(srcDir);
    fs::create_directories(exportRoot);
    const std::string sourcePath = srcDir + "/" + game + ".sav";
    require(writeFile(sourcePath, raw), T + "fixture source written");
    const Bytes sourceBefore = diskBytes(sourcePath);
    const std::string sourceShaBefore = sha256(sourceBefore);
    const auto treeBefore = tree(dir);
    const auto mtimeBefore = fs::last_write_time(sourcePath);

    // ---- Open through the real reader + shared Trainer bridge -------------
    Bytes payload;
    std::string error, container;
    require(G::readNormalizedSourceReadOnly(sourcePath, payload, error, &container), T + "source opens read-only: " + error);
    const auto save = G::Gen5ReadOnlySave::parse(payload, game, &error, selection);
    require(save.has_value(), T + "Gen V reader validates fixture: " + error);
    if (!save) return;
    require(save->selectedBackupPartition() == backup, T + "selected copy");
    auto trainer = B::Gen5ReadOnlyTrainer::create(*save, game, error);
    require(trainer != nullptr, T + "shared Trainer bridge: " + error);
    if (!trainer) return;
    auto& ws = trainer->stagedPokemon();
    auto& ui = trainer->sharedScreen();
    auto& bagUi = trainer->bagScreen();
    const auto refresh = [&](std::string& why) { return trainer->refreshStagedPokemonPresentation(why); };
    require(!ws.hasChanges() && G::Gen5SharedScreenState::reviewRowCount(ws) == 0, T + "fresh workspace is clean");
    require(!ui.openReviewFromBrowse(ws), T + "Review unavailable with no staged change");
    const auto sourceBag = G::decodeBag5(*save);
    require(sourceBag && (*sourceBag)[3].size() == 1 && (*sourceBag)[3][0].count == kPotions, T + "source bag decodes");
    const auto r = createRequest(version);

    // Staging helpers: every step uses the shared screen state machine.
    const auto editSlot = [&](G::Gen5StagedPokemonWorkspace::Slot slot, auto&& stageOps, const std::string& what) {
        std::string e;
        bool ok = ui.openActions(ws, slot, e) && ui.activate(ws, Action::Edit, e) && ui.isEditing();
        ok = ok && stageOps(e);
        ok = ok && ui.keepWithPresentation(ws, refresh, e);
        require(ok && ui.surface() == Surface::Browse, T + what + ": " + e);
    };
    const auto createAt = [&](size_t box, size_t slot) {
        std::string e;
        bool ok = ui.openActions(ws, {Region::Box, box, slot}, e) && ui.activate(ws, Action::Add, e) &&
                  ui.surface() == Surface::ConfirmCreateBox && ui.confirmCreateBox(ws, r, refresh, e);
        require(ok && ui.surface() == Surface::Browse, T + "Create in empty Box 1/" + std::to_string(slot + 1) + ": " + e);
    };
    const auto editLevelNick = [&]() {
        editSlot({Region::Box, 0, 0}, [&](std::string& e) {
            return G::stageSharedField(ui.editDraft(), FieldId::Level, 0, kEditLevel, e) &&
                   G::stageSharedName(ui.editDraft(), FieldId::Nickname, "TRACE", e);
        }, "Edit Level + Nickname of Box 1/1");
    };
    const auto editHeld = [&]() {
        editSlot({Region::Party, 0, 0}, [&](std::string& e) {
            return G::stageSharedField(ui.editDraft(), FieldId::HeldItem, 0, kHeldItem, e);
        }, "Edit Held Item of Party 1");
    };
    const auto release = [&](size_t box, size_t slot) {
        std::string e;
        bool ok = ui.openActions(ws, {Region::Box, box, slot}, e) && ui.activate(ws, Action::Remove, e) &&
                  ui.surface() == Surface::ConfirmRemoveBox;
        // Release is a two-step Y confirm; first check B cancels with no stage.
        const auto pre = stagedFingerprint(ws);
        (void)ui.back();
        ok = ok && ui.surface() == Surface::Actions && stagedFingerprint(ws) == pre;
        ok = ok && ui.activate(ws, Action::Remove, e) && ui.confirmRemoveBox(ws, refresh, e);
        (void)ui.back(); // Close if still on Actions (no-op from Browse)
        require(ok && ui.surface() == Surface::Browse, T + "Release (Y-confirmed) Box 1/" + std::to_string(slot + 1) + ": " + e);
    };
    const auto bagQuantity = [&](uint16_t value) {
        std::string e;
        bagUi.reset();
        bagUi.changePocket(3); // Medicine
        bool ok = bagUi.tapRow(0, ws);
        const auto req = bagUi.beginQuantity(ws, e);
        ok = ok && req && req->itemId == 17 && bagUi.applyQuantity(ws, *req, value, refresh, e);
        require(ok, T + "Bag Medicine/Potion quantity -> " + std::to_string(value) + ": " + e);
    };

    // Trainer page (R2-BUG-003 layout): Money through a tap on its row, then Name.
    const auto trainerEdit = [&](uint32_t money, const char* name) {
        std::string e;
        auto& page = trainer->trainerScreen();
        using TRow = G::Gen5TrainerScreen::Row;
        bool ok = page.openEditor(ws, e) && page.tapRow(1) && page.selectedRow() == TRow::Money &&
                  page.beginEdit(ws, e) && page.applyMoney(ws, money, refresh, e);
        page.moveRow(-1);
        ok = ok && page.selectedRow() == TRow::Name && page.applyName(ws, name, refresh, e);
        page.close();
        require(ok, T + "Trainer Name + Money staged: " + e);
    };

    // ---- 1. Full staged sequence ------------------------------------------
    createAt(0, 3);
    editLevelNick();
    {   // Clone Party 1 -> first verified empty box slot (Box 1/5).
        std::string e;
        bool ok = ui.openActions(ws, {Region::Party, 0, 0}, e) && ui.activate(ws, Action::Clone, e) &&
                  ui.surface() == Surface::CloneDestination && !ui.moveDestination() &&
                  ui.cloneDestinationIndex() == 4 && ui.confirmClone(ws, refresh, e);
        require(ok && ui.surface() == Surface::Browse, T + "Clone Party 1 -> Box 1/5: " + e);
    }
    {   // Move Box 1/2 -> Box 2/1 through the shared destination picker.
        std::string e;
        bool ok = ui.openActions(ws, {Region::Box, 0, 1}, e) && ui.activate(ws, Action::Move, e) &&
                  ui.moveDestination() && ui.cloneDestinationIndex() == 5;
        ui.moveCloneCursor(25);
        ok = ok && ui.cloneDestinationIndex() == 30 && ui.confirmClone(ws, refresh, e);
        require(ok && ui.surface() == Surface::Browse, T + "Move Box 1/2 -> Box 2/1: " + e);
    }
    release(0, 2);
    editHeld();
    bagQuantity(kNewPotions);
    trainerEdit(kFirstMoney, "FIRST");

    // Presentation followed the staged workspace, source did not move.
    require(ws.viewBox(0, 1)->empty() && ws.viewBox(1, 0)->species() == 25 &&
            ws.viewBox(0, 2)->empty() && ws.viewBox(0, 3)->species() == 25 &&
            ws.viewBox(0, 4)->species() == 25 && ws.viewParty(0)->heldItem() == kHeldItem,
            T + "workspace shows every staged change");
    require(save->boxPokemon(0, 1)->species() == 25 && save->boxPokemon(0, 2)->species() == 133 &&
            save->boxPokemon(1, 0)->empty() && save->partyPokemon(0)->heldItem() == 0,
            T + "validated source save object untouched by staging");
    require(diskBytes(sourcePath) == sourceBefore, T + "source file untouched by staging");

    // ---- 2. Review --------------------------------------------------------
    require(ui.openReviewFromBrowse(ws) && ui.surface() == Surface::Review, T + "Review opens from Browse");
    const auto rows = reviewRows(ws);
    const auto bagRows = ws.bagReviewChanges();
    require(rows.size() == 7, T + "Review lists 7 staged PK5 records (got " + std::to_string(rows.size()) + ")");
    require(bagRows.size() == 1, T + "Review lists 1 Bag change");
    // S04 follow-up: the staged Move (Clone to Box 2/1 + Remove of Box 1/2) is ONE Review row.
    {
        const auto pending = ws.pendingReview();
        const auto grouped = G::groupReviewRows5(pending);
        require(grouped.size() == 6, T + "Review shows 6 Pokemon rows: the Move is one row (got " +
                                         std::to_string(grouped.size()) + ")");
        std::size_t moves = 0;
        for (const auto& row : grouped) {
            if (row.kind != G::ReviewRow5::Kind::Move) continue;
            ++moves;
            const auto& to = pending[row.change].location;
            const auto& from = pending[row.removal].location;
            require(to.region == Region::Box && to.box == 1 && to.slot == 0 &&
                    from.region == Region::Box && from.box == 0 && from.slot == 1,
                    T + "Move row reads Box 1 / Slot 2 -> Box 2 / Slot 1");
        }
        require(moves == 1, T + "exactly one Move row (the Clone from Party 1 stays a Clone)");
    }
    require(G::Gen5SharedScreenState::reviewRowCount(ws) == 9,
            T + "Review row count = 6 Pokemon rows + 1 Bag + 2 Trainer");
    require(std::all_of(rows.begin(), rows.end(), [](const ReviewRow& x) { return x.verified; }),
            T + "every Review PK5 row independently verifies");
    {
        const auto* created = findRow(rows, Region::Box, 0, 3);
        require(created && created->kind == Kind::CreateBox && hasField(created, Field::CreateBox, 0, 25),
                T + "Review names Create in Box 1/4 (species 25)");
        const auto* edited = findRow(rows, Region::Box, 0, 0);
        require(edited && edited->kind == Kind::FieldEdit && hasField(edited, Field::Level, 1, kEditLevel) &&
                hasField(edited, Field::NicknameText, 0, 1) && edited->fields.size() == 2,
                T + "Review names Box 1/1 Level 1->30 and Nickname changed (only)");
        const auto* clone = findRow(rows, Region::Box, 0, 4);
        require(clone && clone->kind == Kind::CloneBox && hasField(clone, Field::CloneBox, 0, 25) && clone->fields.size() == 1,
                T + "Review names Clone to Box 1/5");
        const auto* moveTo = findRow(rows, Region::Box, 1, 0);
        const auto* moveFrom = findRow(rows, Region::Box, 0, 1);
        // Move has no distinct Review kind: it is listed as its two verified
        // halves (Clone to Box 2/1 + Remove from Box 1/2).
        require(moveTo && moveTo->kind == Kind::CloneBox && hasField(moveTo, Field::CloneBox, 0, 25),
                T + "Review names Move destination (Clone to Box 2/1)");
        require(moveFrom && moveFrom->kind == Kind::RemoveBox && hasField(moveFrom, Field::RemoveBox, 25, 0),
                T + "Review names Move source (Remove from Box 1/2)");
        const auto* released = findRow(rows, Region::Box, 0, 2);
        require(released && released->kind == Kind::RemoveBox && hasField(released, Field::RemoveBox, 133, 0),
                T + "Review names Release of Box 1/3");
        const auto* held = findRow(rows, Region::Party, 0, 0);
        require(held && held->kind == Kind::FieldEdit && hasField(held, Field::HeldItem, 0, kHeldItem) &&
                held->fields.size() == 1, T + "Review names Party 1 Held Item 0->234 (only)");
        require(!bagRows.empty() && bagRows[0].pocket == 3 && bagRows[0].itemId == 17 &&
                bagRows[0].before == kPotions && bagRows[0].after == kNewPotions,
                T + "Review names Bag Medicine Potion 3->42");
        const auto trainerRows = ws.trainerReviewChanges();
        using TField = G::TrainerChange5::Field;
        require(trainerRows.size() == 2 &&
                std::any_of(trainerRows.begin(), trainerRows.end(), [](const auto& c) { return c.field == TField::Name; }) &&
                std::any_of(trainerRows.begin(), trainerRows.end(), [](const auto& c) { return c.field == TField::Money; }),
                T + "Review lists exactly the Trainer Name and Money changes");
    }
    // Paging is presentation only.
    const auto staged1 = stagedFingerprint(ws);
    const auto candidate1 = G::buildStagedSaveCandidate(ws, &error);
    require(candidate1.has_value(), T + "staged candidate builds: " + error);
    ui.moveReviewPage(1, ws);
    require(ui.reviewPage() == 1, T + "Review page 2 of 3");
    ui.moveReviewPage(1, ws);
    require(ui.reviewPage() == 2, T + "Review page 3 of 3 (9 rows, 4 per page)");
    ui.moveReviewPage(1, ws);
    require(ui.reviewPage() == 2, T + "Review paging clamps");
    ui.moveReviewPage(-1, ws);
    // Expanded semantic-field rows may outnumber grouped records; paging uses
    // the presented row count without changing any native or staged byte.
    for(int i=0;i<10;++i)ui.moveReviewPage(1,ws,25);
    require(ui.reviewPage()==6,T+"all 25 semantic rows are reachable on 7 pages");
    for(int i=0;i<10;++i)ui.moveReviewPage(-1,ws,25);
    require(ui.reviewPage()==0,T+"semantic Review pages clamp at the first page");
    require(stagedFingerprint(ws)==staged1,T+"semantic paging is byte-immutable");

    // ---- 3. Cancel paths: nothing changes -----------------------------------
    {
        const auto treeMid = tree(dir);
        // Cancel Export confirmation.
        require(ui.requestExport(ws) && ui.surface() == Surface::ConfirmExport, T + "Export asks for confirmation");
        (void)ui.back();
        require(ui.surface() == Surface::Review, T + "B on Export confirmation returns to Review");
        // Cancel Discard All confirmation.
        ui.requestDiscardAll();
        require(ui.surface() == Surface::ConfirmDiscardAll, T + "Discard All asks for confirmation");
        (void)ui.back();
        require(ui.surface() == Surface::Review, T + "B on Discard confirmation returns to Review");
        // Cancel Review itself.
        (void)ui.back();
        require(ui.surface() == Surface::Browse, T + "B closes Review");
        require(stagedFingerprint(ws) == staged1, T + "cancel Review/Export/Discard keeps every staged byte");
        const auto candidate2 = G::buildStagedSaveCandidate(ws, &error);
        require(candidate1 && candidate2 && candidate2->bytes == candidate1->bytes,
                T + "cancel paths leave the identical export candidate");
        require(tree(dir) == treeMid, T + "cancel paths write no file");
        require(diskBytes(sourcePath) == sourceBefore, T + "cancel paths leave source untouched");
    }
    // Staged-edit candidate keeps the other copy byte-identical (pre-export).
    if (candidate1)
        require(std::equal(candidate1->bytes.begin() + static_cast<std::ptrdiff_t>(otherBase),
                           candidate1->bytes.begin() + static_cast<std::ptrdiff_t>(otherBase + copySize),
                           payload.begin() + static_cast<std::ptrdiff_t>(otherBase)),
                T + "candidate never touches the unselected copy");

    // ---- 4. Discard All -----------------------------------------------------
    require(ui.openReviewFromBrowse(ws), T + "Review reopens");
    ui.requestDiscardAll();
    {
        // A rejected presentation refresh keeps everything (atomic Discard).
        std::string e;
        require(!ui.discardAllWithPresentation(ws, [](std::string& why) { why = "simulated"; return false; }, e) &&
                ui.surface() == Surface::ConfirmDiscardAll && stagedFingerprint(ws) == staged1,
                T + "Discard All with refused refresh preserves staged changes");
        std::string restore;
        (void)trainer->refreshStagedPokemonPresentation(restore);
        require(ui.discardAllWithPresentation(ws, refresh, e) && ui.surface() == Surface::Browse,
                T + "Discard All (Y-confirmed): " + e);
    }
    require(!ws.hasChanges() && !trainer->hasStagedChanges(), T + "Discard All: workspace has no changes");
    require(ws.pendingReview().empty() && ws.bagReviewChanges().empty() && ws.trainerReviewChanges().empty() &&
            G::Gen5SharedScreenState::reviewRowCount(ws) == 0, T + "Discard All: Review empty");
    require(!ui.openReviewFromBrowse(ws), T + "Discard All: Review no longer opens");
    require(!G::buildStagedSaveCandidate(ws, &error), T + "Discard All: nothing to export");
    {
        bool exact = true;
        for (size_t s = 0; s < save->partyCount(); ++s)
            exact = exact && sameSpan(ws.viewParty(s)->originalEncryptedBytes(), save->partyPokemon(s)->originalEncryptedBytes());
        for (size_t b = 0; b < 24; ++b)
            for (size_t s = 0; s < 30; ++s)
                exact = exact && sameSpan(ws.viewBox(b, s)->originalEncryptedBytes(),
                                          save->boxPokemon(b, s)->originalEncryptedBytes());
        require(exact, T + "Discard All: every party/box PK5 is byte-identical to the source");
        const auto shownBag = ws.bagPresentation();
        require(shownBag && sourceBag && sameBag(*shownBag, *sourceBag), T + "Discard All: Bag equals source bag");
        require(ws.emptyCloneDestination(0, 3) && ws.emptyCloneDestination(0, 4) && ws.emptyCloneDestination(1, 0) &&
                ws.originalOccupied({Region::Box, 0, 1}) && ws.originalOccupied({Region::Box, 0, 2}),
                T + "Discard All: Create/Clone/Move/Release slots return to source state");
        require(trainer->getPartySize() == 1 && trainer->items.size() == G::BagPocket5Count &&
                trainer->items[3].size() == 1 && trainer->items[3][0].count == kPotions,
                T + "Discard All: shared presentation shows the source again");
        require(sameSpan(ws.sourceSave().sourceBytes(), payload), T + "Discard All: source image intact");
        const auto shownTrainer = ws.trainerPresentation();
        require(!ws.trainerDirty() && shownTrainer && shownTrainer->money == 1000 && trainer->money == 1000,
                T + "Discard All: Trainer Money back to the source value");
    }
    require(diskBytes(sourcePath) == sourceBefore && tree(dir) == treeBefore, T + "Discard All writes no file");

    // ---- 5. Redo a small set and Export Copy ---------------------------------
    createAt(0, 3);
    editLevelNick();
    release(0, 2);
    editHeld();
    bagQuantity(kNewPotions);
    trainerEdit(kRedoMoney, "REDO");
    require(ui.openReviewFromBrowse(ws) && G::Gen5SharedScreenState::reviewRowCount(ws) == 7,
            T + "redo: Review lists 4 PK5 + 1 Bag + 2 Trainer");
    require(ui.requestExport(ws) && ui.surface() == Surface::ConfirmExport, T + "redo: Export confirmation");
    // Exactly the production ConfirmExport (Y) path.
    Bytes observed, observedFile;
    require(G::readSourceFileReadOnly(sourcePath, observedFile, observed, error, &container),
            T + "export re-reads source read-only: " + error);
    G::WorkingCopyExportRequest req;
    req.profileIdentity = "0123456789abcdef0123456789abcdef";
    req.timestamp = backup ? "20261017_120001" : "20261017_120000";
    req.sourcePath = sourcePath;
    req.applicationSha = "host-s04-trace";
    req.observedSourceBytes = observed;
    req.observedSourceFile = observedFile;
    req.sourceContainer = container;
    req.rootDirectory = exportRoot;
    const auto result = G::publishGen5WorkingCopy(ws, req);
    require(result.success && result.error.empty(), T + "Export Copy publishes: " + result.error);
    ui.finishExport();
    require(ui.surface() == Surface::Review && ws.hasChanges(), T + "after export Review stays, staged changes kept");
    require(result.changedRecords == 4, T + "export reports 4 changed PK5 records");
    if (!result.success) return;

    // ---- 6. Reopen the exported file through the real reader -----------------
    const std::string editedPath = result.directory + "/edited.sav";
    const std::string backupPath = result.directory + "/" + result.backupName;
    require(result.backupName == "original_backup.sav", T + "raw source backup name");
    require(diskBytes(backupPath) == sourceBefore, T + "backup is the untouched source verbatim");
    Bytes edited;
    std::string editedContainer;
    require(G::readNormalizedSourceReadOnly(editedPath, edited, error, &editedContainer), T + "export reopens: " + error);
    require(sha256(diskBytes(editedPath)) == result.editedSha256, T + "edited.sav SHA-256 matches manifest result");
    require(result.sourceSha256 == sha256(payload), T + "export records source SHA-256");
    const auto reopened = G::Gen5ReadOnlySave::parse(edited, game, &error, selection);
    require(reopened.has_value(), T + "exported SAV5 passes strict Gen V reader (all CRCs): " + error);
    if (reopened) {
        // Independent CRC check of every block in the selected copy.
        bool crcOk = true;
        for (const auto& b : blocks) {
            const uint16_t crc = Utils::crc16ccitt(edited.data() + base + b.offset, b.length);
            crcOk = crcOk && C::read16(edited, base + b.checksumOffset) == crc &&
                    C::read16(edited, base + b.mirrorOffset) == crc;
        }
        require(crcOk, T + "every exported block checksum + mirror verifies");
        const auto lvl = reopened->boxPokemon(0, 0);
        const auto p = G::personal5(133, 0, family == G::SaveFamily::Black2White2);
        require(lvl && lvl->valid() && p &&
                G::levelForExperience5(p.info->growthRate, lvl->experience()) == kEditLevel,
                T + "export has Box 1/1 Level 30");
        const auto nick = lvl ? G::asciiName5(lvl->decodedBytes(), true) : std::nullopt;
        require(nick && *nick == "TRACE", T + "export has Box 1/1 Nickname TRACE");
        require(reopened->boxPokemon(0, 2)->valid() && reopened->boxPokemon(0, 2)->empty(), T + "export has Box 1/3 released");
        const auto made = reopened->boxPokemon(0, 3);
        require(made && made->valid() && made->species() == 25 && made->pid() == *r.pid &&
                made->originVersion() == version, T + "export has created Pikachu in Box 1/4");
        require(reopened->partyPokemon(0)->heldItem() == kHeldItem, T + "export has Party 1 Held Item 234");
        const auto bag = G::decodeBag5(*reopened);
        require(bag && (*bag)[3].size() == 1 && (*bag)[3][0].count == kNewPotions, T + "export has Potion x42");
        require(sameSpan(reopened->boxPokemon(0, 1)->originalEncryptedBytes(), save->boxPokemon(0, 1)->originalEncryptedBytes()) &&
                reopened->boxPokemon(0, 4)->empty() && reopened->boxPokemon(1, 0)->empty(),
                T + "export has no discarded change (no Clone/Move residue)");
        const auto exportedTrainer = G::decodeTrainer5(*reopened);
        require(exportedTrainer && exportedTrainer->money == kRedoMoney &&
                G::asciiTrainerName5(exportedTrainer->rawName) == std::string("REDO"),
                T + "export has the redone Trainer Name REDO and Money (not the discarded ones)");
        require(std::equal(edited.begin() + static_cast<std::ptrdiff_t>(otherBase),
                           edited.begin() + static_cast<std::ptrdiff_t>(otherBase + copySize),
                           sourceBefore.begin() + static_cast<std::ptrdiff_t>(otherBase)),
                T + "export unselected copy byte-identical");
        require(G::inspectSourceFile(editedPath, "Manual", game, selection).ready(),
                T + "exported file reopens as a ready source");
    }

    // ---- 7. Source and filesystem audit --------------------------------------
    const Bytes sourceAfter = diskBytes(sourcePath);
    require(sourceAfter == sourceBefore && sha256(sourceAfter) == sourceShaBefore,
            T + "ORIGINAL source byte-identical / same SHA-256 after the whole trace");
    require(fs::last_write_time(sourcePath) == mtimeBefore, T + "source mtime unchanged");
    const auto treeAfter = tree(dir);
    const std::string exportRel = fs::relative(result.directory, dir).string();
    std::set<std::string> added;
    bool preexistingIntact = true;
    for (const auto& [path, hash] : treeBefore) {
        const auto it = treeAfter.find(path);
        preexistingIntact = preexistingIntact && it != treeAfter.end() && it->second == hash;
    }
    for (const auto& [path, hash] : treeAfter)
        if (!treeBefore.count(path)) added.insert(path);
    require(preexistingIntact, T + "no pre-existing file changed");
    // publishGen5WorkingCopy writes exactly backup + edited + its manifest.
    const std::set<std::string> expected{exportRel + "/original_backup.sav", exportRel + "/edited.sav",
                                         exportRel + "/EXPORT_MANIFEST.txt"};
    require(added == expected, T + "only the export directory's backup/edited/manifest were written");
    for (const auto& a : added)
        if (!expected.count(a)) std::cerr << "  unexpected file: " << a << '\n';
}

} // namespace

// Round 6 (P1-RETURN): Review opened from Party/Boxes Browse, the Edit workspace or the Create
// workspace returns to that surface on B, with the draft (and Create's staged record) intact;
// Discard All from a workspace Review never returns into a record it just removed. Same real
// reader, shared Trainer bridge and shared screen state machine as the trace above.
void workspaceReviewReturn(const std::string& dir, G::SaveFamily family, const char* game, uint8_t version,
                           bool backup) {
    const std::string T = std::string("R6 [") + game + "] ";
    const size_t copySize = copySizeFor(family);
    const size_t base = backup ? copySize : 0;
    const size_t otherBase = backup ? 0 : copySize;
    Bytes raw(L::FullSaveSize, 0);
    writeCopy(raw, base, family, version, 'S', kPotions, true);
    writeCopy(raw, otherBase, family, version, 'O', 9, false);
    fs::create_directories(dir + "/r6");
    const std::string sourcePath = dir + "/r6/" + game + ".sav";
    require(writeFile(sourcePath, raw), T + "fixture written");
    const Bytes sourceBefore = diskBytes(sourcePath);
    Bytes payload;
    std::string error;
    require(G::readNormalizedSourceReadOnly(sourcePath, payload, error), T + "opens read-only: " + error);
    const auto save = G::Gen5ReadOnlySave::parse(payload, game, &error,
                                                 backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary);
    require(save.has_value(), T + "validates: " + error);
    if (!save) return;
    auto trainer = B::Gen5ReadOnlyTrainer::create(*save, game, error);
    require(trainer != nullptr, T + "Trainer bridge: " + error);
    if (!trainer) return;
    auto& ws = trainer->stagedPokemon();
    auto& ui = trainer->sharedScreen();
    const auto refresh = [&](std::string& why) { return trainer->refreshStagedPokemonPresentation(why); };
    std::string e;

    // Nothing staged: no workspace can open Review either.
    require(ui.openActions(ws, {Region::Box, 0, 0}, e) && ui.activate(ws, Action::Edit, e) && ui.isEditing(),
            T + "Edit opens: " + e);
    require(!ui.openReviewFrom(ws) && ui.surface() == Surface::Edit, T + "no Review with nothing staged");
    require(G::stageSharedField(ui.editDraft(), FieldId::Level, 0, kEditLevel, e) &&
            ui.keepWithPresentation(ws, refresh, e) && ui.surface() == Surface::Browse && ws.hasChanges(),
            T + "one staged edit: " + e);

    // Browse (Party / Boxes): Review, then B, back on Browse.
    require(ui.openReviewFrom(ws) && ui.surface() == Surface::Review && ui.reviewReturn() == Surface::Browse,
            T + "Review from Browse");
    (void)ui.back();
    require(ui.surface() == Surface::Browse, T + "B returns to Browse");

    // Edit workspace with an unsaved draft: Review, then B, back in Edit with the draft as it was.
    require(ui.openActions(ws, {Region::Party, 0, 0}, e) && ui.activate(ws, Action::Edit, e) && ui.isEditing(),
            T + "Edit Party 1: " + e);
    require(G::stageSharedField(ui.editDraft(), FieldId::HeldItem, 0, kHeldItem, e), T + "draft change: " + e);
    require(ui.editDraft().dirty(), T + "draft is dirty");
    const auto draftItem = ui.editDraft().current()->heldItem();
    require(ui.openReviewFrom(ws) && ui.surface() == Surface::Review && ui.reviewReturn() == Surface::Edit,
            T + "Review from the Edit workspace");
    require(!ws.viewParty(0) || ws.viewParty(0)->heldItem() != kHeldItem,
            T + "opening Review stages nothing from the draft");
    (void)ui.back();
    require(ui.surface() == Surface::Edit && ui.isEditing() && ui.editDraft().dirty() &&
            ui.editDraft().current()->heldItem() == draftItem && ui.reviewReturn() == Surface::Browse,
            T + "B returns to Edit with the draft intact");
    require(ui.keepWithPresentation(ws, refresh, e) && ws.viewParty(0)->heldItem() == kHeldItem,
            T + "the kept draft still stages normally: " + e);

    // Create workspace: the created record is staged; Review, then B, back in Create.
    const auto r = createRequest(version);
    require(ui.openActions(ws, {Region::Box, 0, 5}, e) && ui.activate(ws, Action::Add, e) &&
            ui.surface() == Surface::ConfirmCreateBox && ui.beginCreateDraft(ws, r, refresh, e) &&
            ui.surface() == Surface::Edit && ui.createPending(), T + "Create opens its workspace: " + e);
    require(ui.openReviewFrom(ws) && ui.reviewReturn() == Surface::Edit, T + "Review from Create");
    (void)ui.back();
    require(ui.surface() == Surface::Edit && ui.createPending() && !ws.viewBox(0, 5)->empty(),
            T + "B returns to Create with its staged record");

    // Discard All from that Review: everything goes, and Review never returns into the removed record.
    require(ui.openReviewFrom(ws), T + "Review from Create again");
    ui.requestDiscardAll();
    require(ui.surface() == Surface::ConfirmDiscardAll, T + "Discard All asks first");
    (void)ui.back();
    require(ui.surface() == Surface::Review && ui.reviewReturn() == Surface::Edit,
            T + "B on the confirmation keeps Review and its return");
    ui.requestDiscardAll();
    require(ui.discardAllWithPresentation(ws, refresh, e) && ui.surface() == Surface::Browse &&
            !ui.createPending() && !ui.isEditing() && !ws.hasChanges() && ws.viewBox(0, 5)->empty(),
            T + "Discard All from a workspace Review lands on Browse with no draft: " + e);
    (void)ui.back();
    require(ui.surface() == Surface::Browse, T + "a later B stays on Browse");

    require(diskBytes(sourcePath) == sourceBefore, T + "source file untouched");
}

int main() {
    char pattern[] = "/tmp/pokebank_s04_trace_XXXXXX";
    const char* root = ::mkdtemp(pattern);
    if (!root) { std::cerr << "FAIL: S04 mkdtemp\n"; return 1; }
    struct Game { G::SaveFamily family; const char* id; uint8_t version; bool backup; };
    const Game games[] = {
        {G::SaveFamily::BlackWhite, "white_nds", 20, false},
        {G::SaveFamily::BlackWhite, "black_nds", 21, true},
        {G::SaveFamily::Black2White2, "white2_nds", 22, false},
        {G::SaveFamily::Black2White2, "black2_nds", 23, true},
    };
    for (const auto& g : games) {
        const std::string dir = std::string(root) + "/" + g.id;
        fs::create_directories(dir);
        trace(dir, g.family, g.id, g.version, g.backup);
        workspaceReviewReturn(dir, g.family, g.id, g.version, g.backup);
    }
    std::error_code ec;
    fs::remove_all(root, ec);
    if (failures) {
        std::cerr << "S04 Gen V Review/Discard/Export trace: " << failures << " of " << checks << " checks FAILED\n";
        return 1;
    }
    std::cout << "S04 Gen V Review/Discard/Export trace (W, B, W2, B2): " << checks << " checks PASS\n";
    return 0;
}
