#include "test_backup_session_trace.h"
// PokeBank NX application QA: Gen I-V acceptance matrix, Generation V rows (Thread 4, QA-owned).
//
// Rows black, white, black2, white2: both SAV5 layout families (BlackWhite and Black2White2, see
// Gen5SaveBlockLayout.h LayoutInfo tables). Every cell is driven through the production Gen V entry
// points exactly as the app wires them: discoverSources / inspectSourceFile / inspectSourceChoices
// (Gen5SourceDiscovery.h), assignValidatedSource + LegacySourceBindings + openAssignedSource
// (Gen5AssignedSource.h), the Gen5ReadOnlyTrainer bridge and its Gen5SharedScreenState /
// Gen5StagedPokemonWorkspace (the shared Trainer/Party/Boxes screen), the shared editor bridge and
// SharedPokemonWorkspace::navigate, buildStagedSaveCandidateForObservedSource, the verified
// working-copy export (publishGen5WorkingCopy), and HomeSnapshotCache + homeCardSnapshot (the
// SaveSelectScreen::refreshAssignedSnapshot wiring, as in tests/test_home_card_gen5_snapshot.cpp).
//
// Synthetic saves only, built in memory and written below qacontract::scratchRoot. Layout sources:
//   * base image, bag stacks and block checksums: qasynth::gen5 in tests/fuzz_synthetic_saves.h
//     (PKHeX SaveBlockAccessor5BW/B2W2 via Gen5SaveBlockLayout.h);
//   * retail-consistent party/box PK5 records and encrypted blank box slots: richSave() and
//     plainRecord() in tests/fuzz_gen5_edits.cpp (Gen5BlankSlot.h);
//   * trainer PlayerData / Misc fields: Gen5Trainer.h TrainerLayout5 (PKHeX PlayerData5/Misc5), as
//     written by tests/test_gen5_source_e2e.cpp writeCopy() and tests/test_home_card_gen5_snapshot.cpp;
//   * Pokedex flags: Gen5Pokedex.h DexLayout5 (PKHeX Zukan5);
//   * PK5 form bits (0x40 >> 3) and the shiny PID rule: tests/test_qa_known_failures.cpp (QA-F5)
//     and tests/test_home_card_gen5_snapshot.cpp shinyPid().
#include "test_qa_matrix.h"
#include "test_qa_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5BagScreen.h"
#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5CreateValidation.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5GameCardPreview.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5Pokedex.h"
#include "Integration/Gen5/Gen5SharedEditorBridge.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5Trainer.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/LegacySourceBindings.h"
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveProviderIdentity.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/SharedBagAddFlow.h"
#include "UI/SharedStatReroll.h"
#include "test_qa_matrix_round2.h"

#include <sys/stat.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace qamatrix {
namespace {

namespace fs = std::filesystem;
namespace G5 = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace T = PokeVault::Integration::Gen5::TrainerLayout5;
namespace F = PokeVault::Integration::Gen5::EditorFields;
namespace S = PokeVault::Source;
namespace B = PokeVault::Legacy;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Nav = PokeBank::UIModel::SharedPokemonWorkspace;
using Bytes = std::vector<uint8_t>;
using Family = G5::SaveFamily;
using Region = G5::Gen5StagedPokemonWorkspace::Region;
using Slot = G5::Gen5StagedPokemonWorkspace::Slot;
using Screen = G5::Gen5SharedScreenState;
using NF = G5::StagedPokemon5Record::Field;

// --------------------------------------------------------------------------------------------
// Cell plumbing: a cell body throws CellFailure (or any exception) to record Fail with the reason.
// --------------------------------------------------------------------------------------------
struct CellFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};
void need(bool condition, const std::string& what) {
    if (!condition) throw CellFailure(what);
}
struct Outcome {
    Status status = Status::Pass;
    std::string note;
};
Outcome passed(std::string note = {}) { return {Status::Pass, std::move(note)}; }

void runCell(Matrix& m, const std::string& version, std::string_view cell,
             const std::function<Outcome()>& body) {
    Outcome o;
    try {
        o = body();
    } catch (const std::exception& e) {
        o = {Status::Fail, e.what()};
    }
    m.set(version, cell, o.status, o.note);
}

// --------------------------------------------------------------------------------------------
// Synthetic SAV5
// --------------------------------------------------------------------------------------------
constexpr uint16_t kTid = 12345, kSid = 54321;  // as tests/fuzz_gen5_edits.cpp
const std::u16string kTrainer = u"QATEST";
constexpr uint8_t kTrainerGender = 1;            // female (PlayerData5 0x21)
constexpr uint8_t kLanguage = 2;                 // English, createLanguageAllowed5
constexpr uint32_t kMoney = 4321;

// tests/fuzz_gen5_edits.cpp plainRecord(): retail-consistent ability for the PID's ability slot,
// stored gender, origin version; form in the upper five bits of 0x40 (PKHeX PK5.Form, as in
// tests/test_qa_known_failures.cpp QA-F5).
Bytes plainRecord(bool party, uint32_t pid, uint16_t species, uint8_t origin, uint8_t gender,
                  uint8_t form = 0, uint8_t level = 10) {
    Bytes r(party ? C::PartySize : C::StoredSize, 0);
    qasynth::w32(r, 0, pid);
    qasynth::w16(r, 0x08, species);
    qasynth::w16(r, 0x0C, kTid);
    qasynth::w16(r, 0x0E, kSid);
    qasynth::w32(r, 0x10, 1000);
    r[0x14] = 70;
    const auto personal = G5::personal5(species, 0, false);
    r[0x15] = personal.info ? G5::abilityForSlot5(*personal.info, (pid >> 16) & 1u) : 0;
    r[0x17] = 2;
    r[0x40] = static_cast<uint8_t>((form << 3) | (gender << 1));
    r[0x41] = 3;
    r[0x5F] = origin;
    r[0x83] = 4;
    r[0x84] = 0x80 | 5;
    if (party) r[0x8C] = level;
    return r;
}

void put(Bytes& sav, size_t at, const Bytes& plain) {
    const auto enc = C::encryptCandidate(plain);
    std::copy(enc.begin(), enc.end(), sav.begin() + static_cast<std::ptrdiff_t>(at));
}

// tests/test_home_card_gen5_snapshot.cpp shinyPid(): hi ^ lo ^ TID ^ SID == 0 is shiny in Gen V.
uint32_t shinyPid() {
    const uint16_t lo = 0x1111;
    const uint16_t hi = static_cast<uint16_t>(lo ^ kTid ^ kSid);
    return (static_cast<uint32_t>(hi) << 16) | lo;
}

constexpr uint16_t kBasculin = 550, kDeerling = 585;

// richSave() of tests/fuzz_gen5_edits.cpp on the qasynth::gen5 base image (bag stacks from
// BagCatalog5), plus a trainer, money and Pokedex flags. Primary copy only.
// homeVariant replaces party slot 0 with a shiny Blue-Striped Basculin (form 1) at level 33.
Bytes buildSave(Family family, uint8_t version, bool homeVariant = false) {
    const size_t copy = qasynth::gen5::copySize(family);
    auto sav = qasynth::gen5::save(family, version, 1);
    // PlayerData5 (TrainerLayout5, block 27 at 0x19400 for both families).
    const size_t player = G5::trainerBlock5(family, T::PlayerBlockIndex).offset;
    for (size_t i = 0; i < kTrainer.size(); ++i)
        C::write16(sav, player + T::NameOffset + i * 2, static_cast<uint16_t>(kTrainer[i]));
    C::write16(sav, player + T::NameOffset + kTrainer.size() * 2, 0xFFFF);
    C::write16(sav, player + T::TidOffset, kTid);
    C::write16(sav, player + T::SidOffset, kSid);
    sav[player + T::LanguageOffset] = kLanguage;
    sav[player + T::GenderOffset] = kTrainerGender;
    C::write16(sav, player + T::HoursOffset, 12);
    sav[player + T::MinutesOffset] = 34;
    // Misc5 money (block 52).
    const size_t misc = G5::trainerBlock5(family, T::MiscBlockIndex).offset;
    qasynth::w32(sav, misc + T::MoneyOffset, kMoney);
    // Party: Gen V origin Pikachu (ratio 127, PID low byte 0x80 -> male) and a Gen IV origin Eevee.
    sav[L::PartyOffset + 4] = 2;
    if (homeVariant)
        put(sav, L::PartyOffset + 8, plainRecord(true, shinyPid(), kBasculin, version, 0, 1, 33));
    else
        put(sav, L::PartyOffset + 8, plainRecord(true, 0x2A3B0080u, 25, version, 0));
    put(sav, L::PartyOffset + 8 + C::PartySize, plainRecord(true, 0x77770010u, 133, 10, 1));
    // Box 0: slot 0 Gen V origin Eevee, slot 2 Gen V origin Deerling (seasonal forms), the rest
    // retail encrypted blanks.
    const auto& blank = G5::encryptedBlankBoxSlot5();
    for (size_t s = 1; s < L::BoxSlots; ++s)
        std::copy(blank.begin(), blank.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + s * C::StoredSize));
    put(sav, L::BoxOffset, plainRecord(false, 0x11220090u, 133, version, 0));
    put(sav, L::BoxOffset + 2 * C::StoredSize, plainRecord(false, 0x33440090u, kDeerling, version, 0));
    // Pokedex (DexLayout5): caught Pikachu; seen Pikachu and Eevee in the first seen region.
    namespace D = G5::DexLayout5;
    const size_t dex = family == Family::BlackWhite ? L::BlackWhite[D::BlackWhiteBlockIndex].offset
                                                    : L::Black2White2[D::Black2White2BlockIndex].offset;
    const auto flag = [&](size_t region, uint16_t species) {
        const size_t bit = species - 1u;
        sav[dex + region + (bit >> 3)] |= static_cast<uint8_t>(1u << (bit & 7));
    };
    flag(D::CaughtOffset, 25);
    flag(D::SeenOffset, 25);
    flag(D::SeenOffset, 133);
    qasynth::gen5::stamp(std::span<uint8_t>(sav).subspan(0, copy), qasynth::gen5::blocks(family));
    return sav;
}

// tests/test_qa_export_contracts.cpp gen5RecordWhitelist(): the record range plus each containing
// block's checksum and mirror words, followed transitively into the footer block.
std::vector<qacontract::Range> recordWhitelist(Family family, size_t copyBase, size_t recordOffset,
                                               size_t length) {
    std::vector<qacontract::Range> out{{copyBase + recordOffset, copyBase + recordOffset + length}};
    std::vector<size_t> pending{recordOffset};
    std::vector<size_t> seen;
    while (!pending.empty()) {
        const size_t at = pending.back();
        pending.pop_back();
        for (const auto& block : qasynth::gen5::blocks(family)) {
            if (at < block.offset || at >= block.offset + block.length) continue;
            if (std::find(seen.begin(), seen.end(), block.offset) != seen.end()) continue;
            seen.push_back(block.offset);
            out.push_back({copyBase + block.checksumOffset, copyBase + block.checksumOffset + 2});
            out.push_back({copyBase + block.mirrorOffset, copyBase + block.mirrorOffset + 2});
            pending.push_back(block.checksumOffset);
            pending.push_back(block.mirrorOffset);
        }
    }
    return out;
}

// --------------------------------------------------------------------------------------------
// Per-version context
// --------------------------------------------------------------------------------------------
struct Spec {
    std::string row;
    std::string game;
    std::string sibling;   // same-family other title (exact-version check)
    uint8_t version;
    Family family;
    std::string provider;  // raw provider label discovery records
    std::string providerDisplay;
    std::string fileName;  // deliberately names ANOTHER title
};

struct Ctx {
    Spec spec;
    fs::path root, savesDir, path, bindingsPath, exportsRoot;
    Bytes original;
    qacontract::FileState initialFile;
    qacontract::TreeState initialTree;
    std::string profile = std::string(32, 'a');
    std::optional<S::SaveInstance> row;
    std::optional<G5::Gen5ReadOnlySave> parsed;  // from openAssignedSource after the assignment
    std::string fingerprint;
    std::string newFingerprint;

    bool b2w2() const { return spec.family == Family::Black2White2; }
    const G5::Gen5ReadOnlySave& save() const {
        need(parsed.has_value(), "prerequisite failed: the assigned source did not open (see linking)");
        return *parsed;
    }
    std::unique_ptr<B::Gen5ReadOnlyTrainer> trainer() const {
        std::string error;
        auto t = B::Gen5ReadOnlyTrainer::create(save(), spec.game, error);
        need(t != nullptr, "Gen5ReadOnlyTrainer::create refused: " + error);
        return t;
    }
    bool sourceUntouched() const { return qacontract::fileState(path) == initialFile; }
};

std::function<bool(std::string&)> refresher(B::Gen5ReadOnlyTrainer& t) {
    return [&t](std::string& why) { return t.refreshStagedPokemonPresentation(why); };
}

bool hasAction(const Shared::ActionSet& set, Shared::Action a) {
    for (size_t i = 0; i < set.count; ++i)
        if (set[i] == a) return true;
    return false;
}
std::string actionList(const Shared::ActionSet& set) {
    static const char* names[] = {"View", "Edit", "Clone", "Remove", "Legality", "Add",
                                  "Review", "Close", "None", "Move", "ImportFile", "ExportFile"};
    std::string out;
    for (size_t i = 0; i < set.count; ++i) {
        if (i) out += ",";
        out += names[static_cast<size_t>(set[i])];
    }
    return out;
}

Nav::Keys key(char k) {  // as tests/test_shared_editor_contract.cpp
    Nav::Keys keys{};
    keys.l = k == 'L'; keys.r = k == 'R'; keys.up = k == 'U'; keys.down = k == 'D';
    keys.left = k == '<'; keys.right = k == '>';
    return keys;
}

Bytes bytesOf(const std::optional<G5::Pokemon5ReadOnly>& p) {
    if (!p) return {};
    const auto b = p->originalEncryptedBytes();
    return Bytes(b.begin(), b.end());
}

Bytes observedSource(const Ctx& c) {
    Bytes observed;
    std::string error, container;
    need(G5::readNormalizedSourceReadOnly(c.path.string(), observed, error, &container),
         "readNormalizedSourceReadOnly failed: " + error);
    return observed;
}

// Same request/parse wiring as SaveSelectScreen::refreshAssignedSnapshot
// (tests/test_home_card_gen5_snapshot.cpp Fixture::refresh).
const S::HomeCardSnapshot* homeRefresh(const Ctx& c, B::LegacySourceBindings& bindings,
                                        S::HomeSnapshotCache& cache, S::SnapshotRefresh& outcome) {
    const auto assigned = bindings.resolveFileForGame(c.profile, c.spec.game);
    if (assigned.status != B::AssignedFileStatus::Ready) return nullptr;
    return &cache.refresh(S::assignedSnapshotRequest(c.profile, c.spec.game, assigned), S::statRegularFile,
                          [&] { return G5::homeCardSnapshot(G5::openAssignedSource(bindings, c.profile, c.spec.game)); },
                          &outcome);
}

void setMtime(const fs::path& path, long seconds) {
    struct timespec times[2] = {{seconds, 0}, {seconds, 0}};
    ::utimensat(AT_FDCWD, path.c_str(), times, 0);
}

// --------------------------------------------------------------------------------------------
// Cells
// --------------------------------------------------------------------------------------------
Outcome discovery(Ctx& c) {
    G5::DiscoveryRoot root{c.savesDir.string(), c.spec.provider, 2};
    const auto found = G5::discoverSources(std::span<const G5::DiscoveryRoot>(&root, 1));
    need(found.rootsOpened == 1 && found.filesExamined == 1,
         "discoverSources did not open the root / examine the file");
    const auto it = std::find_if(found.instances.begin(), found.instances.end(),
                                 [&](const S::SaveInstance& s) { return s.path() == c.path.string(); });
    need(it != found.instances.end(), "discoverSources returned no row for the synthetic save");
    need(it->ready() && it->readOnly() && it->generation == 5,
         "discovered row not a ready read-only Gen V row: " + it->diagnostic);
    need(found.instances.size() == 1, "single-copy save produced " + std::to_string(found.instances.size()) + " rows");
    const auto probe = G5::inspectSourceFile(c.path.string(), c.spec.provider);
    need(probe.ready(), "inspectSourceFile not ready: " + probe.diagnostic);
    need(probe.contentFingerprint == it->contentFingerprint, "discovery and inspection fingerprints differ");
    c.row = probe;
    // Two valid but different copies: two labelled choice rows, no automatic pick.
    const fs::path twoDir = c.root / "twocopies";
    fs::create_directories(twoDir);
    const fs::path two = twoDir / "two.sav";
    // Backup copy = the primary copy with different money, restamped (layout as buildSave()).
    auto twoCopies = c.original;
    const size_t copySize = qasynth::gen5::copySize(c.spec.family);
    std::copy(twoCopies.begin(), twoCopies.begin() + static_cast<std::ptrdiff_t>(copySize),
              twoCopies.begin() + static_cast<std::ptrdiff_t>(copySize));
    qasynth::w32(twoCopies, copySize + G5::trainerBlock5(c.spec.family, T::MiscBlockIndex).offset + T::MoneyOffset,
                 kMoney + 1);
    qasynth::gen5::stamp(std::span<uint8_t>(twoCopies).subspan(copySize, copySize),
                         qasynth::gen5::blocks(c.spec.family));
    qacontract::writeSource(two, twoCopies);
    const auto rows = G5::inspectSourceChoices(two.string(), c.spec.provider);
    need(rows.size() == 2 && rows[0].ready() && rows[1].ready(),
         "two-copy save did not yield two ready choice rows: " + std::to_string(rows.size()) + " row(s), " +
             (rows.empty() ? std::string() : rows[0].diagnostic));
    need((rows[0].sourceVariant == "gen5-copy-0-primary" && rows[1].sourceVariant == "gen5-copy-1-backup") ||
             (rows[1].sourceVariant == "gen5-copy-0-primary" && rows[0].sourceVariant == "gen5-copy-1-backup"),
         "two-copy rows are not labelled primary/backup");
    const auto automatic = G5::inspectSourceFile(two.string(), c.spec.provider);
    need(!automatic.ready(), "Automatic copy selection picked one of two different valid copies");
    fs::remove_all(twoDir);
    return passed(std::string(c.spec.family == Family::BlackWhite ? "BW" : "B2W2") +
                  " layout; discoverSources + inspectSourceFile ready; two different copies -> 2 labelled rows, "
                  "Automatic refused");
}

Outcome identity(Ctx& c) {
    need(c.row.has_value(), "prerequisite failed: discovery");
    need(c.row->gameId == c.spec.game,
         "exact game " + c.row->gameId + " != " + c.spec.game + " (file named " + c.spec.fileName + ")");
    const auto sibling = G5::inspectSourceFile(c.path.string(), c.spec.provider, c.spec.sibling);
    need(sibling.validation == S::ValidationStatus::AssignmentMismatch,
         "assigning the same-family sibling " + c.spec.sibling + " was not refused as AssignmentMismatch");
    need(c.save().exactGameId() == c.spec.game, "opened save exact id differs");
    need(c.save().family() == c.spec.family, "opened save layout family differs");
    const auto t = c.trainer();
    const auto group = c.b2w2() ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;
    need(t->getGameGroup() == group, "Trainer bridge game group differs from the layout family");
    return passed("PlayerData5 version byte " + std::to_string(c.spec.version) + " -> " + c.spec.game +
                  " although the file is named '" + c.spec.fileName + "'; sibling " + c.spec.sibling +
                  " refused (AssignmentMismatch)");
}

Outcome linking(Ctx& c) {
    need(c.row.has_value(), "prerequisite failed: discovery");
    {
        B::LegacySourceBindings bindings(c.bindingsPath.string());
        need(bindings.load(), "bindings load");
        const auto assigned = G5::assignValidatedSource(bindings, *c.row, c.profile, c.spec.game);
        need(assigned.success && assigned.instance.rememberedSource,
             "assignValidatedSource failed: " + assigned.diagnostic);
    }
    B::LegacySourceBindings reloaded(c.bindingsPath.string());
    need(reloaded.load(), "bindings reload from disk");
    const auto resolved = reloaded.resolveFileForGame(c.profile, c.spec.game);
    need(resolved.status == B::AssignedFileStatus::Ready, "resolveFileForGame not Ready after reload");
    need(resolved.binding.sourcePath == c.path.string(), "binding resolves to another path");
    need(resolved.binding.sourceType == c.spec.provider, "binding provider differs");
    need(resolved.binding.expectedRawFamily == (c.b2w2() ? "B2W2" : "BW"), "binding family differs");
    need(resolved.binding.sourceVariant == "gen5-copy-0-primary", "binding copy is not the primary copy");
    auto opened = G5::openAssignedSource(reloaded, c.profile, c.spec.game);
    need(opened.ready(), "openAssignedSource after reload: " + opened.diagnostic);
    need(opened.instance.path() == c.path.string(), "opened instance path differs");
    need(G5::openAssignedSource(reloaded, std::string(32, 'b'), c.spec.game).status ==
             G5::AssignedOpenStatus::Unassigned,
         "another profile resolved the assignment");
    need(!G5::openAssignedSource(reloaded, c.profile, c.spec.sibling).ready(),
         "the sibling title opened this profile's save");
    c.fingerprint = opened.instance.contentFingerprint;
    c.parsed = std::move(opened.save);
    return passed("assignValidatedSource -> reload -> resolveFileForGame Ready (same path, copy 0) -> "
                  "openAssignedSource ready; other profile Unassigned, sibling title refused");
}

Outcome provider(Ctx& c) {
    need(c.row.has_value(), "prerequisite failed: discovery");
    need(c.row->providerLabel == c.spec.provider, "providerLabel '" + c.row->providerLabel + "'");
    need(c.row->providerId == S::providerIdFor(c.spec.provider), "providerId '" + c.row->providerId + "'");
    need(S::identifyProvider(c.row->providerLabel) != S::ProviderIdentity::Unknown, "provider unidentified");
    need(S::providerDisplayName(c.row->providerLabel) == c.spec.providerDisplay,
         "display name " + S::providerDisplayName(c.row->providerLabel));
    need(S::saveInstanceSourceLine(*c.row).rfind(c.spec.providerDisplay + " / ", 0) == 0,
         "Save Instance line '" + S::saveInstanceSourceLine(*c.row) + "'");
    B::LegacySourceBindings bindings(c.bindingsPath.string());
    need(bindings.load(), "bindings load");
    const auto assigned = bindings.resolveFileForGame(c.profile, c.spec.game);
    need(assigned.status == B::AssignedFileStatus::Ready, "prerequisite failed: linking");
    const auto card = S::homeCardSourceText("REMEMBERED", assigned.binding.sourceType);
    need(card == c.spec.providerDisplay, "Home card source text '" + card + "'");
    need(S::homeSourceLine("Nintendo DS", card) == "Nintendo DS \xE2\x80\xA2 " + c.spec.providerDisplay,
         "Home source line");
    return passed("'" + c.spec.provider + "' -> '" + c.spec.providerDisplay +
                  "' on the Save Instance row and the Home card (from the persisted binding)");
}

Outcome trainerName(Ctx& c) {
    const auto decoded = G5::decodeTrainer5(c.save());
    need(decoded && decoded->rawName == kTrainer, "decodeTrainer5 name differs");
    need(c.row && c.row->trainerName == "QATEST", "Save Instance trainer label '" +
                                                       (c.row ? c.row->trainerName : std::string()) + "'");
    const auto t = c.trainer();
    need(t->trainerName == "QATEST", "Trainer bridge name '" + t->trainerName + "'");
    need(decoded->tid == kTid && decoded->sid == kSid && decoded->money == kMoney, "TID/SID/money differ");
    return passed("QATEST via decodeTrainer5, Save Instance row and Gen5ReadOnlyTrainer; TID/SID/money decoded");
}

Outcome trainerGender(Ctx& c) {
    const auto decoded = G5::decodeTrainer5(c.save());
    need(decoded && decoded->gender == kTrainerGender, "decodeTrainer5 gender differs");
    const auto t = c.trainer();
    need(t->trainerGender == kTrainerGender, "Trainer bridge gender differs");
    B::LegacySourceBindings bindings(c.bindingsPath.string());
    need(bindings.load(), "bindings load");
    const auto preview = G5::previewAssignedGame(G5::openAssignedSource(bindings, c.profile, c.spec.game));
    need(preview && preview->trainerGenderKnown && preview->trainerGender == kTrainerGender,
         "Home/card preview gender not known female");
    return passed("PlayerData5 0x21 = female through decodeTrainer5, Trainer bridge and card preview");
}

Outcome party(Ctx& c) {
    const auto& s = c.save();
    need(s.partyCount() == 2, "party count " + std::to_string(s.partyCount()));
    const auto p0 = s.partyPokemon(0), p1 = s.partyPokemon(1);
    need(p0 && p0->valid() && p0->species() == 25 && p0->partyLevel() == 10, "party slot 1 not Pikachu L10");
    need(p1 && p1->valid() && p1->species() == 133, "party slot 2 not Eevee");
    const auto t = c.trainer();
    need(t->getPartySize() == 2 && t->party.size() == 2 && t->party[0] && t->party[1],
         "Trainer bridge party presentation size differs");
    return passed("2 members (Pikachu L10, Gen IV origin Eevee) via parse and the Trainer bridge");
}

Outcome boxes(Ctx& c) {
    const auto& s = c.save();
    const auto b0 = s.boxPokemon(0, 0), b1 = s.boxPokemon(0, 1), b2 = s.boxPokemon(0, 2);
    need(b0 && b0->valid() && b0->species() == 133, "box 1 slot 1 not Eevee");
    need(b1 && b1->valid() && b1->empty() && G5::nativeBlankSlot5(b1->originalEncryptedBytes()),
         "box 1 slot 2 not a native blank");
    need(b2 && b2->valid() && b2->species() == kDeerling, "box 1 slot 3 not Deerling");
    need(s.diagnostics().occupiedBoxRecords == 2 && s.diagnostics().invalidBoxRecords == 0,
         "box diagnostics occupied/invalid differ");
    const auto t = c.trainer();
    need(t->getBoxCount() == 24 && t->getSlotsPerBox() == 30 && t->boxes.size() == 24, "box geometry");
    need(t->boxes[0][0] && !t->boxes[0][1] && t->boxes[0][2], "Trainer bridge box presentation differs");
    return passed("24x30; 2 occupied + retail encrypted blanks, 0 quarantined; Trainer bridge agrees");
}

Outcome pokedex(Ctx& c) {
    const auto dex = G5::decodePokedex5(c.save());
    need(dex.has_value(), "decodePokedex5 failed");
    need(dex->seen == 2 && dex->caught == 1, "seen/caught " + std::to_string(dex->seen) + "/" +
                                                 std::to_string(dex->caught) + " != 2/1");
    need(dex->entries[24].caught && dex->entries[24].seen && dex->entries[132].seen && !dex->entries[132].caught,
         "per-species flags differ");
    const auto progress = c.save().dexProgress();
    need(progress.seen == 2 && progress.caught == 1 && progress.total == 649, "dexProgress differs");
    G5::Gen5PokedexView view;
    view.openAt(25);
    view.jumpRegistered(*dex, 1);
    need(view.cursor() == 132, "Pokedex view jump did not land on Eevee");
    return passed("seen 2 / caught 1 / total 649 (Zukan5 block " +
                  std::string(c.b2w2() ? "54 @0x21400" : "55 @0x21600") + ")");
}

Outcome bag(Ctx& c) {
    const auto decoded = G5::decodeBag5(c.save());
    need(decoded.has_value(), "decodeBag5 refused the synthetic Items block");
    const auto& b = *decoded;
    need(b[0].size() == 2 && b[0][0].count == 3 && b[0][1].count == 999, "Items pocket differs");
    const uint16_t key = c.b2w2() ? G5::BagCatalog5::kKeyB2W2[0] : G5::BagCatalog5::kKeyBW[0];
    need(b[1].size() == 1 && b[1][0].itemId == key, "Key Items pocket differs");
    need(b[2].size() == 1 && b[3].size() == 1 && b[3][0].count == 12 && b[4].size() == 1, "TM/Medicine/Berries differ");
    const auto t = c.trainer();
    need(t->items.size() == G5::BagPocket5Count && t->items[3].size() == 1 && t->items[3][0].count == 12,
         "Trainer bridge Items panel differs");
    return passed("5 pockets decoded (family-specific key item list) and shown by the Trainer bridge");
}

Outcome navigation(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    // Actions menu cursor moves are pure.
    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error), "openActions: " + error);
    for (int d : {1, 1, -1, 7, -3}) ui.moveAction(d, ws);
    need(!ws.hasChanges(), "moving the Actions cursor staged a change");
    (void)ui.back();
    for (const auto mode : {Shared::Action::Edit, Shared::Action::View}) {
        need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error), "openActions: " + error);
        need(ui.activate(ws, mode, error), "activate View/Edit: " + error);
        auto& draft = ui.editDraft();
        const auto opened = draft.current();
        need(opened.has_value(), "no draft");
        const Bytes pristine(opened->decodedBytes().begin(), opened->decodedBytes().end());
        const bool editable = mode == Shared::Action::Edit;
        const auto detailEditable = [&](int row) { return F::detailEditable(draft, row, c.b2w2()); };
        const auto pidEditable = [&](int row) { return F::pidRowEditable(draft, row, c.b2w2()); };
        Shared::Focus focus{};
        const char walk[] = "DDDDDDDDDDDDDDDDDDDDDDD>>>>DDDDDDDDDDDDD<<<<<URLRLUUUD>>>D>D>D<<<<RRRLLL";
        for (const char k : walk) {
            focus = Nav::navigate(Shared::Generation::Gen5, focus, key(k), !editable, editable, F::RowCount,
                                  detailEditable, pidEditable).focus;
            const auto now = draft.current();
            need(now && std::equal(now->decodedBytes().begin(), now->decodedBytes().end(), pristine.begin(),
                                   pristine.end()),
                 std::string("focus move '") + k + "' changed the draft");
        }
        need(!draft.dirty() && !ws.hasChanges(), "navigation left a dirty draft or staged change");
        if (editable) {
            // B on an Edit draft always asks Keep/Discard (PokemonEditorExitGuard); Discard stages nothing.
            need(!ui.back() && ui.surface() == Screen::Surface::ConfirmDraft, "B on Edit did not ask Keep/Discard");
            ui.discardDraft();
            need(ui.surface() == Screen::Surface::Browse, "Discard did not return to Browse");
        } else {
            need(ui.back() && ui.surface() == Screen::Surface::Browse, "B on View did not close it");
        }
    }
    need(!ws.hasChanges() && c.sourceUntouched(), "navigation changed the workspace or the source file");
    return passed("SharedPokemonWorkspace::navigate over Gen V Edit and View drafts (" +
                  std::to_string(F::RowCount) + " detail rows) and Actions cursor: no byte changed; B on Edit asks "
                  "Keep/Discard, Discard stages nothing");
}

Outcome creation(Ctx& c, Matrix& m) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    const auto request = Screen::sharedCreateDefaults(ws, 0x00A1B2C3u, "PIKACHU", {2026, 10, 15}, error);
    need(request.has_value(), "sharedCreateDefaults refused: " + error);
    m.fact(c.spec.row, "create.default.species", std::to_string(request->species));
    m.fact(c.spec.row, "create.default.level", std::to_string(request->level));
    need(request->species == Screen::kSharedCreateSpecies && request->level == 5, "unexpected defaults");
    need(static_cast<uint32_t>(request->game) == c.spec.version, "Create default game differs from the save");
    need(request->trainer && request->trainer->name == "QATEST" && request->trainer->tid == kTid &&
             request->trainer->sid == kSid && request->trainer->gender == kTrainerGender &&
             request->trainer->language == kLanguage,
         "Create OT not copied from the save trainer");
    // Validation rejects out-of-title / out-of-range requests.
    auto bad = *request;
    bad.species = 650;
    need(G5::validateCreate5(bad).error == G5::CreateError5::SpeciesForm, "species 650 accepted");
    bad = *request;
    bad.level = 0;
    need(G5::validateCreate5(bad).error == G5::CreateError5::Level, "level 0 accepted");
    bad = *request;
    bad.format = G5::CreateFormat5::Party;
    need(G5::validateCreate5(bad).error == G5::CreateError5::Format, "party-format Create accepted");
    bad = *request;
    bad.game = static_cast<G5::CreateGame5>(c.spec.version == 20 ? 21 : c.spec.version == 21 ? 20
                                                                      : c.spec.version == 22 ? 23 : 22);
    need(!ws.createInEmptyBox(bad, 0, 1, &error), "Create for the sibling title accepted");
    // Add is offered only on the empty native slot, then Create -> Edit draft -> Keep.
    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error), "openActions occupied: " + error);
    need(!hasAction(ui.availableActions(ws), Shared::Action::Add), "Add offered on an occupied slot");
    (void)ui.back();
    need(ui.openActions(ws, Slot{Region::Box, 0, 1}, error), "openActions empty: " + error);
    need(hasAction(ui.availableActions(ws), Shared::Action::Add), "Add not offered on the empty slot");
    need(ui.activate(ws, Shared::Action::Add, error) && ui.surface() == Screen::Surface::ConfirmCreateBox,
         "Add did not open the Create confirmation: " + error);
    need(ui.beginCreateDraft(ws, *request, refresher(*t), error), "beginCreateDraft: " + error);
    need(ui.surface() == Screen::Surface::Edit && ui.createPending(), "Create did not open the Edit draft");
    // Owner Oct 28 (item 4): Keep is refused until a Met Location is chosen; the picker records it.
    // Round 11 (owner H28): the new Pokemon also starts with no moves; one status names both.
    need(ui.createNeedsMetLocation() && !ui.keepWithPresentation(ws, refresher(*t), error) &&
             error == "Choose a Met Location and at least one move before Keep",
         "Create kept without a Met Location choice");
    need(ui.createPending(), "a refused Keep left the Create draft");
    ui.noteCreateMetLocationChosen();
    need(!ui.keepWithPresentation(ws, refresher(*t), error) && error == "Choose at least one move before Keep" &&
             ui.createPending(), "Create kept without a move");
    need(ui.editDraft().stage(NF::Move, 0, 84, error), "choose Thunder Shock: " + error);
    need(ui.keepWithPresentation(ws, refresher(*t), error), "Keep: " + error);
    const auto made = ws.viewBox(0, 1);
    need(made && made->valid() && !made->empty() && made->species() == 25 && !made->partyRecord() &&
             made->originVersion() == c.spec.version,
         "created PK5 differs (species/origin/boxed)");
    need(t->boxes[0][1] != nullptr, "created Pokemon not presented");
    const auto pending = ws.pendingReview();
    need(pending.size() == 1 && pending[0].kind == G5::Gen5StagedPokemonWorkspace::ChangeKind::CreateBox,
         "Create not staged as one CreateBox change");
    const auto verified = G5::verifyStagedReview(pending[0], &error);
    need(verified.has_value(), "Create certificate failed independent verification: " + error);
    need(c.sourceUntouched(), "Create touched the source file");
    return passed("defaults Pikachu L5 with the save's OT/TID/SID/gender/language and exact game; species 650, "
                  "level 0, party format and sibling title refused; Add -> Create draft -> Keep -> verified");
}

struct FieldCase {
    Shared::FieldIdentity field;
    size_t stat;
    uint32_t value;
    std::string name;
    std::function<bool(const G5::Pokemon5ReadOnly&)> check;
};

Outcome fields(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    const bool b2 = c.b2w2();
    using FI = Shared::FieldIdentity;
    const uint16_t item = F::heldItemChoices().at(1);
    const auto locations = F::metLocationChoices();
    const uint16_t location = locations.at(locations.size() / 2);
    // Order matters only where the native transaction requires it (Species needs a nickname;
    // Level supersedes the Experience edit, which is checked when staged).
    const std::vector<FieldCase> cases = {
        {FI::Nature, 0, 7, "Nature", [](auto& p) { return p.nature() == 7; }},
        {FI::Friendship, 0, 200, "Friendship", [](auto& p) { return p.friendship() == 200; }},
        {FI::IV, 2, 30, "IV", [](auto& p) { return p.ivs()[2] == 30; }},
        {FI::EV, 1, 100, "EV", [](auto& p) { return p.evs()[1] == 100; }},
        {FI::HeldItem, 0, item, "HeldItem", [item](auto& p) { return p.heldItem() == item; }},
        {FI::Language, 0, 3, "Language", [](auto& p) { return p.language() == 3; }},
        {FI::Ball, 0, 1, "Ball", [](auto& p) { return p.ball() == 1; }},
        {FI::MetLevel, 0, 3, "MetLevel", [](auto& p) { return p.metLevel() == 3; }},
        {FI::Move, 0, 33, "Move", [](auto& p) { return p.moves()[0] == 33; }},
        {FI::MovePPUps, 0, 2, "MovePPUps", [](auto& p) { return p.ppUps()[0] == 2; }},
        {FI::MovePP, 0, 1, "MovePP", [](auto& p) { return p.pp()[0] == 1; }},
        {FI::TrainerId, 0, 999, "TrainerId", [](auto& p) { return p.tid() == 999; }},
        {FI::SecretId, 0, 1000, "SecretId", [](auto& p) { return p.sid() == 1000; }},
        {FI::OriginalTrainerGender, 0, 1, "OTGender", [](auto& p) { return p.originalTrainerGender() == 1; }},
        {FI::Pokerus, 0, 2, "Pokerus", [](auto& p) { return p.pokerusMode() == 2; }},
        {FI::MetLocation, 0, location, "MetLocation", [location](auto& p) { return p.metLocation() == location; }},
        {FI::FatefulEncounter, 0, 1, "Fateful", [](auto& p) { return p.fatefulEncounter(); }},
        {FI::Ability, 0, 1, "Ability", [](auto& p) { return F::abilitySlot(p) == 1; }},
        {FI::Gender, 0, 1, "Gender", [](auto& p) { return p.gender() == 1; }},
        {FI::Experience, 0, 2000, "Experience", [](auto& p) { return p.experience() == 2000; }},
        {FI::Level, 0, 20, "Level", [b2](auto& p) { return F::level(p, b2) == 20; }},
        {FI::Nickname, 0, 0, "Nickname", [](auto& p) {
             return G5::asciiName5(p.decodedBytes(), true).value_or("") == "QANICK"; }},
        {FI::OriginalTrainer, 0, 0, "OriginalTrainer", [](auto& p) {
             return G5::asciiName5(p.decodedBytes(), false).value_or("") == "QAOT"; }},
        {FI::Species, 0, 25, "Species", [](auto& p) { return p.species() == 25; }},
    };
    // Coverage: every Editable Gen V shared-editor field has a case here (Form is staged below).
    std::vector<std::string> uncovered;
    for (int f = 0; f <= static_cast<int>(FI::FatefulEncounter); ++f) {
        const auto field = static_cast<FI>(f);
        // Form and Shiny are staged separately below (Deerling form; the PID-linked Shiny row).
        if (G5::fieldAccess(field) != Shared::FieldAccess::Editable || field == FI::Form ||
            field == FI::Shiny) continue;
        if (std::none_of(cases.begin(), cases.end(), [&](const FieldCase& k) { return k.field == field; }))
            uncovered.push_back(std::to_string(f));
    }
    need(uncovered.empty(), "Editable shared field(s) without a QA case: " +
                                std::accumulate(uncovered.begin(), uncovered.end(), std::string()));

    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error) && ui.activate(ws, Shared::Action::Edit, error),
         "open Edit on box 1 slot 1: " + error);
    auto& draft = ui.editDraft();
    for (const auto& k : cases) {
        const auto before=draft.current();
        const Bytes originalNames(before->decodedBytes().begin()+0x48,before->decodedBytes().begin()+0x5E);
        const Bytes originalOt(before->decodedBytes().begin()+0x68,before->decodedBytes().begin()+0x78);
        bool ok;
        if (k.field == FI::Nickname) ok = G5::stageSharedName(draft, k.field, "QANICK", error);
        else if (k.field == FI::OriginalTrainer) ok = G5::stageSharedName(draft, k.field, "QAOT", error);
        else ok = G5::stageSharedField(draft, k.field, k.stat, k.value, error);
        need(ok, k.name + " refused: " + error);
        need(k.check(*draft.current()), k.name + " did not stage the requested value");
        if(k.field==FI::Language) {
            // M1 D3 (PKHeX PK5 / StringConverter5): the OT never changes; a nicknamed record keeps its
            // nickname bytes; an unnicknamed one gets exactly the species' default name in the new
            // language. Anything else is a silent name change.
            const auto after=draft.current();
            const auto bytes=after->decodedBytes();
            Bytes expectedNames=originalNames;
            if(!(G5::Crypto::read32(before->decodedBytes(),0x38)&0x80000000u) && before->language()!=after->language()) {
                Bytes plain(bytes.begin(),bytes.end());
                std::copy(originalNames.begin(),originalNames.end(),plain.begin()+0x48);
                need(G5::writeUnicodeName5(plain,true,Names::defaultSpeciesName(after->species(),
                         static_cast<uint8_t>(after->language()),5),static_cast<uint8_t>(after->language())),
                     "Language: default name does not fit");
                expectedNames.assign(plain.begin()+0x48,plain.begin()+0x5E);
            }
            need(std::equal(expectedNames.begin(),expectedNames.end(),bytes.begin()+0x48) &&
                 std::equal(originalOt.begin(),originalOt.end(),bytes.begin()+0x68),
                 "Language silently changed native OT/nickname bytes");
        }
    }
    // The STATS PID-linked Shiny row (coordinated PID, Gen5EditorFields.h).
    need(F::toggleShiny(draft, error) && draft.current()->shiny(), "Shiny toggle: " + error);
    need(ui.keepWithPresentation(ws, refresher(*t), error), "Keep: " + error);
    // Form on the Deerling (seasonal forms) in box 1 slot 3.
    need(ui.openActions(ws, Slot{Region::Box, 0, 2}, error) && ui.activate(ws, Shared::Action::Edit, error),
         "open Edit on Deerling: " + error);
    need(G5::stageSharedField(ui.editDraft(), FI::Form, 0, 1, error) && ui.editDraft().current()->form() == 1,
         "Form refused: " + error);
    need(ui.keepWithPresentation(ws, refresher(*t), error), "Keep Form: " + error);
    for (const auto& change : ws.pendingReview())
        need(G5::verifyStagedReview(change, &error).has_value(), "independent review replay failed: " + error);
    // Round trip: the export candidate re-parses strictly with the staged bytes and values.
    const auto observed = observedSource(c);
    const auto candidate = G5::buildStagedSaveCandidateForObservedSource(ws, observed, &error);
    need(candidate.has_value(), "export candidate: " + error);
    const auto again = G5::Gen5ReadOnlySave::parse(candidate->bytes, c.spec.game, &error, G5::SaveCopySelection::Primary);
    need(again.has_value(), "candidate strict reparse: " + error);
    need(bytesOf(again->boxPokemon(0, 0)) == bytesOf(ws.viewBox(0, 0)) &&
             bytesOf(again->boxPokemon(0, 2)) == bytesOf(ws.viewBox(0, 2)),
         "re-parsed records differ from the staged records");
    const auto back = *again->boxPokemon(0, 0);
    for (const auto& k : cases)
        if (k.field != FI::Experience) need(k.check(back), k.name + " lost in the round trip");
    need(back.shiny() && again->boxPokemon(0, 2)->form() == 1, "Shiny/Form lost in the round trip");
    need(c.sourceUntouched(), "field staging touched the source file");
    return passed(std::to_string(cases.size() + 2) +
                  " fields (every Editable FieldIdentity + Shiny row + Form) via stageSharedField/Name -> Keep "
                  "-> verifyStagedReview -> candidate strict reparse");
}

Outcome pickers(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    need(F::kMaxSpecies == 649, "species bound");
    // The production picker must honor the exact save game, Pikachu's species
    // and its existing source moves; it must never fall back to every native move.
    namespace Moves = PokeVault::Integration::Gen5MoveCompatibility;
    const auto moves = F::moveChoices(c.spec.game, 25, 0, {});
    need(moves.size() > 40 && moves.size() < 180 && moves.front() == 0 &&
             std::is_sorted(moves.begin(), moves.end()) &&
             std::find(moves.begin(), moves.end(), 85) != moves.end() &&
             std::find(moves.begin(), moves.end(), 459) == moves.end() &&
             std::all_of(moves.begin() + 1, moves.end(), [](uint16_t m) {
                 return G5::validNativeMove5(m);
             }), "species/game move picker included an unsupported native move");
    need(Moves::classify(c.spec.game, 25, 0, 459, false) == Moves::Availability::Unavailable,
         "unsupported Pikachu move was certified selectable");
    const auto originalOnly = F::moveChoices(c.spec.game, 25, 0, {459, 0, 0, 0});
    need(std::find(originalOnly.begin(), originalOnly.end(), 459) != originalOnly.end() &&
             Moves::classify(c.spec.game, 25, 0, 459, true) == Moves::Availability::Preserved,
         "an unusual source move must survive without being certified legal");
    size_t direct = 0;
    for (uint16_t m = 1; m <= G5::MaxNativeMove5; ++m)
        direct += Moves::direct(c.spec.game, 25, 0, m) ? 1u : 0u;
    need(direct >= 40 && direct < moves.size(), "source-pinned direct learnset unexpectedly absent");
    const auto items = F::heldItemChoices();
    need(!items.empty() && items.front() == 0 &&
             std::all_of(items.begin(), items.end(), [](uint16_t i) { return i <= 638; }),
         "held item picker list differs");
    need(F::ballChoices().size() == 25, "ball picker list differs");
    need(F::metLocationChoices().size() == G5::NativeMetLocations5.size(), "met location list differs");
    need(ui.openActions(ws, Slot{Region::Box, 0, 2}, error) && ui.activate(ws, Shared::Action::Edit, error),
         "open Edit: " + error);
    need(F::formChoices(ui.editDraft(), c.b2w2()).size() == 4, "Deerling form picker does not list 4 seasons");
    need(!F::abilityChoices(*ui.editDraft().current(), c.b2w2()).empty(), "ability picker empty");
    need(!ui.speciesRefusal(650).empty() && ui.speciesRefusal(kDeerling).empty(),
         "species picker bound / identity refusal differs");
    ui.discardDraft();
    // Every native pocket has a real add catalogue. Unique Key Items/TMs/HMs are x1,
    // not stackable; the actual warning/add/remove transactions are exercised by bagAdd.
    for (const auto pocket : {G5::BagPocket5::Items, G5::BagPocket5::Medicine, G5::BagPocket5::Berries,
                             G5::BagPocket5::KeyItems,G5::BagPocket5::Machines})
        need(!G5::gen5BagChoices(c.spec.family, static_cast<size_t>(pocket)).empty(), "empty Bag add picker");
    for(const size_t pocket:{size_t{1},size_t{2}})
        for(const auto id:G5::gen5BagChoices(c.spec.family,pocket))
            need(G5::gen5BagItemInPocket(c.spec.family,pocket,id) &&
                 G5::bagPocketLayout5(c.spec.family,pocket).maxCount==1,"invalid / stackable unique item");
    const uint16_t own = c.b2w2() ? G5::BagCatalog5::kKeyB2W2.back() : G5::BagCatalog5::kKeyBW.back();
    need(G5::gen5BagItemInPocket(c.spec.family, 1, own), "family key-item list not recognised");
    need(!ws.hasChanges(), "picker queries staged a change");
    return passed("species 1-649 (650 refused), Pikachu " + std::to_string(moves.size()-1) +
                  " compatibility-filtered moves + none (" + std::to_string(direct) + " direct in exact game), items <=638, 25 balls, " +
                  std::to_string(G5::NativeMetLocations5.size()) +
                  " native met locations (not encounter proof), Deerling 4 forms, ability slots; all five native Bag add lists, unique Key/TM/HM x1");
}

Outcome actions(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    using A = Shared::Action;
    const auto expect = [&](Slot s, std::vector<A> want, const char* what) {
        need(ui.openActions(ws, s, error), std::string("openActions ") + what + ": " + error);
        const auto got = ui.availableActions(ws);
        bool same = got.count == want.size();
        for (size_t i = 0; same && i < want.size(); ++i) same = got[i] == want[i];
        need(same, std::string(what) + " actions [" + actionList(got) + "]");
        (void)ui.back();
    };
    // The one shared action order (NarrowPokemonActionMenu.h; owner-accepted E02): View, Edit, Clone,
    // Move (where supported), Release, Origin & Legality, Review Pending Changes, Cancel. Feature A
    // (owner Oct 28) adds one file row to that accepted order and moves nothing else: Export to file
    // after Origin & Legality, Import from file after Add Pokemon. The exact full list is checked.
    const auto withFileRow = [](std::vector<A> accepted, A file, std::size_t at) {
        accepted.insert(accepted.begin() + static_cast<std::ptrdiff_t>(at), file);
        return accepted;
    };
    expect(Slot{Region::Party, 0, 0},
           withFileRow({A::View, A::Edit, A::Clone, A::LegalityProvenance, A::Close}, A::ExportFile, 4), "party");
    expect(Slot{Region::Box, 0, 0},
           withFileRow({A::View, A::Edit, A::Clone, A::Move, A::Remove, A::LegalityProvenance, A::Close}, A::ExportFile, 6),
           "box");
    expect(Slot{Region::Box, 0, 1}, withFileRow({A::Add, A::Close}, A::ImportFile, 1), "empty box");
    // Cursor wraps; Cancel (Close) returns to Browse without staging.
    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error), "openActions: " + error);
    ui.moveAction(-1, ws);
    need(ui.activateSelected(ws, error) && ui.surface() == Screen::Surface::Browse && !ws.hasChanges(),
         "wrapped cursor did not select Cancel");
    // Release -> confirm -> staged blank; Undo restores.
    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error) && ui.activate(ws, A::Remove, error) &&
             ui.confirmRemoveBox(ws, refresher(*t), error),
         "Release: " + error);
    need(ws.viewBox(0, 0)->empty() && !t->boxes[0][0], "Release did not stage a blank slot");
    need(ui.openActions(ws, Slot{Region::Box, 0, 0}, error) && ui.selectedBoxUndoAvailable(ws) &&
             ui.undoSelectedBoxReplacement(ws, refresher(*t), error),
         "Undo: " + error);
    need(ws.viewBox(0, 0)->species() == 133 && !ws.hasChanges(), "Undo did not restore the original");
    // Clone party 1 into the first empty slot; Move Deerling to the next empty slot.
    need(ui.openActions(ws, Slot{Region::Party, 0, 0}, error) && ui.activate(ws, A::Clone, error) &&
             ui.cloneDestinationIndex() == 1 && ui.confirmClone(ws, refresher(*t), error),
         "Clone: " + error);
    need(ws.viewBox(0, 1)->species() == 25 && !ws.viewBox(0, 1)->partyRecord(), "Clone result differs");
    need(ui.openActions(ws, Slot{Region::Box, 0, 2}, error) && ui.activate(ws, A::Move, error), "Move: " + error);
    const size_t dest = ui.cloneDestinationIndex();
    need(ui.confirmClone(ws, refresher(*t), error), "Move confirm: " + error);
    need(ws.viewBox(0, 2)->empty() && ws.viewBox(dest / 30, dest % 30)->species() == kDeerling,
         "Move result differs");
    // Part 9 item 1: a staged slot (the Clone copy in box 0/1, the Move result) keeps the full shared
    // list, with Review before Cancel; the party keeps Clone and Origin & Legality as well.
    expect(Slot{Region::Box, 0, 1},
           withFileRow({A::View, A::Edit, A::Clone, A::Move, A::Remove, A::LegalityProvenance, A::Review, A::Close},
                       A::ExportFile, 6), "cloned copy");
    expect(Slot{Region::Box, dest / 30, dest % 30},
           withFileRow({A::View, A::Edit, A::Clone, A::Move, A::Remove, A::LegalityProvenance, A::Review, A::Close},
                       A::ExportFile, 6), "moved Pokemon");
    expect(Slot{Region::Party, 0, 0},
           withFileRow({A::View, A::Edit, A::Clone, A::LegalityProvenance, A::Review, A::Close}, A::ExportFile, 4),
           "party after staging");
    need(c.sourceUntouched(), "actions touched the source file");
    return passed("party [View,Edit,Clone,Origin & Legality,Cancel], box [View,Edit,Clone,Move,Release,Origin & Legality,"
                  "Cancel] (shared order), empty [Add,Cancel]; wrap, Release+Undo, Clone, Move; staged clone / moved "
                  "slot keep the full list + Review");
}

Outcome workingCopy(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    std::string error;
    const Bytes sourceBefore(ws.sourceSave().sourceBytes().begin(), ws.sourceSave().sourceBytes().end());
    need(ws.stageParty(0, NF::Friendship, 0, 222, &error), "stage Friendship: " + error);
    need(ws.stagedBag(&error) && ws.stagedBag(&error)->setQuantity(3, 0, 42, &error), "stage Bag: " + error);
    need(ws.stagedTrainer(&error) && ws.stagedTrainer(&error)->setMoney(31337, &error), "stage money: " + error);
    need(t->refreshStagedPokemonPresentation(error), "presentation: " + error);
    need(t->money == 31337 && t->items[3][0].count == 42, "Trainer bridge does not present the working copy");
    need(Bytes(ws.sourceSave().sourceBytes().begin(), ws.sourceSave().sourceBytes().end()) == sourceBefore &&
             ws.sourceSave().partyPokemon(0)->friendship() == 70,
         "staging changed the in-memory source image");
    need(c.sourceUntouched(), "staging touched the source file");
    const auto candidate = G5::buildStagedSaveCandidateForObservedSource(ws, observedSource(c), &error);
    need(candidate.has_value(), "candidate: " + error);
    const auto again = G5::Gen5ReadOnlySave::parse(candidate->bytes, c.spec.game, &error, G5::SaveCopySelection::Primary);
    need(again && again->partyPokemon(0)->friendship() == 222 && (*G5::decodeBag5(*again))[3][0].count == 42 &&
             G5::decodeTrainer5(*again)->money == 31337,
         "working copy does not carry the PK5/Bag/Trainer edits");
    const size_t copy = qasynth::gen5::copySize(c.spec.family);
    need(std::equal(candidate->bytes.begin() + static_cast<std::ptrdiff_t>(copy), candidate->bytes.end(),
                    c.original.begin() + static_cast<std::ptrdiff_t>(copy)),
         "working copy changed the unselected backup copy");
    return passed("PK5 + Bag + Trainer staged in app memory; source image/file unchanged; candidate carries all "
                  "three, backup copy byte-identical");
}

Outcome review(Ctx& c) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    need(!ui.openReviewFromBrowse(ws), "Review opened with nothing staged");
    need(ws.stageParty(0, NF::Friendship, 0, 222, &error), "stage: " + error);
    need(ws.stagedBag(&error)->setQuantity(3, 0, 42, &error), "bag: " + error);
    need(ws.stagedTrainer(&error)->setName("QAREV", &error), "name: " + error);
    need(t->refreshStagedPokemonPresentation(error), "presentation: " + error);
    need(Screen::reviewRowCount(ws) == 3, "review rows " + std::to_string(Screen::reviewRowCount(ws)));
    const auto pending = ws.pendingReview();
    need(pending.size() == 1, "pending PK5 records");
    const auto verified = G5::verifyStagedReview(pending[0], &error);
    need(verified && verified->fields.size() == 1 && verified->fields[0].field == NF::Friendship &&
             verified->fields[0].before == 70 && verified->fields[0].after == 222,
         "verified review row differs: " + error);
    const auto bagRows = ws.bagReviewChanges();
    need(bagRows.size() == 1 && bagRows[0].pocket == 3 && bagRows[0].before == 12 && bagRows[0].after == 42,
         "Bag review row differs");
    const auto trainerRows = ws.trainerReviewChanges();
    need(trainerRows.size() == 1 && trainerRows[0].field == G5::TrainerChange5::Field::Name &&
             trainerRows[0].before == "QATEST" && trainerRows[0].after == "QAREV",
         "Trainer review row differs");
    need(ui.openReviewFromBrowse(ws) && ui.surface() == Screen::Surface::Review, "Review surface");
    need(ui.requestExport(ws) && ui.surface() == Screen::Surface::ConfirmExport, "export confirmation");
    (void)ui.back();
    ui.requestDiscardAll();
    need(ui.discardAllWithPresentation(ws, refresher(*t), error) && !ws.hasChanges() &&
             t->trainerName == "QATEST",
         "Discard All: " + error);
    need(c.sourceUntouched(), "review touched the source file");
    return passed("3 rows (Friendship 70->222 independently replayed, Bag 12->42, Trainer name); "
                  "export confirmation; Discard All restores");
}

Outcome exportCell(Ctx& c, Matrix& m) {
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    std::string error;
    need(ws.stageBox(0, 0, NF::Level, 0, 40, &error), "stage Level: " + error);
    need(ws.stagedBag(&error)->setQuantity(static_cast<size_t>(G5::BagPocket5::Items), 0, 7, &error), "bag: " + error);
    need(ws.stagedTrainer(&error)->setName("QAEXP", &error) && ws.stagedTrainer(&error)->setMoney(123456, &error),
         "trainer: " + error);
    const auto observed = observedSource(c);
    fs::create_directories(c.exportsRoot);
    const auto before = qacontract::treeState(c.exportsRoot);
    // A stale observation (one byte of the unselected copy differs) is refused before any I/O.
    auto stale = observed;
    stale[qasynth::gen5::copySize(c.spec.family) + 0x100] ^= 0xFF;
    G5::WorkingCopyExportRequest request;
    request.profileIdentity = c.profile;
    request.timestamp = "20261015_120000";
    request.sourcePath = c.path.string();
    request.applicationSha = "qa-matrix";
    request.observedSourceBytes = stale;
    request.rootDirectory = c.exportsRoot.string();
    const auto refused = G5::publishGen5WorkingCopy(ws, request);
    need(!refused.success && qacontract::treeState(c.exportsRoot) == before,
         "export over a stale source observation was not refused cleanly");
    request.observedSourceBytes = observed;
    const auto result = G5::publishGen5WorkingCopy(ws, request);
    need(result.success, "publishGen5WorkingCopy: " + result.error);
    const fs::path dir = result.directory;
    need(!before.count(fs::relative(dir, c.exportsRoot).string() + "/<dir>"), "export reused an existing directory");
    need(qacontract::slurp(dir / "original_backup.sav") == c.original, "original_backup.sav differs from the source");
    need(fs::exists(dir / "EXPORT_MANIFEST.txt"), "manifest missing");
    const auto edited = qacontract::slurp(dir / "edited.sav");
    const auto again = G5::Gen5ReadOnlySave::parse(edited, c.spec.game, &error, G5::SaveCopySelection::Primary);
    need(again && again->selectedCopyOffset() == 0 && again->exactGameId() == c.spec.game,
         "edited.sav strict reparse: " + error);
    need(G5::decodeTrainer5(*again)->money == 123456 && G5::decodeTrainer5(*again)->rawName == u"QAEXP",
         "edited.sav lacks the Trainer edits");
    // Byte whitelist (tests/test_qa_export_contracts.cpp): box record, Items block, trainer name, money.
    const auto fam = c.spec.family;
    auto allowed = recordWhitelist(fam, 0, L::BoxOffset, C::StoredSize);
    for (const auto& extra :
         {recordWhitelist(fam, 0, G5::itemsBlockOffset(fam), G5::itemsBlockLength(fam)),
          recordWhitelist(fam, 0, G5::trainerBlock5(fam, T::PlayerBlockIndex).offset + T::NameOffset, T::NameBytes),
          recordWhitelist(fam, 0, G5::trainerBlock5(fam, T::MiscBlockIndex).offset + T::MoneyOffset, 4)})
        allowed.insert(allowed.end(), extra.begin(), extra.end());
    size_t offender = 0, changed = 0;
    need(qacontract::onlyWhitelistedChanges(c.original, edited, allowed, &offender, &changed),
         "edited.sav changed non-whitelisted byte " + qacontract::hex(offender));
    // S06: corrupt the first byte of the money field (inside the Misc block, which its block
    // checksum covers); the primary copy must stop reparsing with the Trainer edits.
    const size_t moneyAt = G5::trainerBlock5(fam, T::MiscBlockIndex).offset + T::MoneyOffset;
    const auto s06 = qamatrix::exportCorruptionProbe(edited, moneyAt + 3, [&](const auto& bytes) {
        std::string why;
        const auto r = G5::Gen5ReadOnlySave::parse(bytes, c.spec.game, &why, G5::SaveCopySelection::Primary);
        if (!r || r->selectedCopyOffset() != 0 || r->exactGameId() != c.spec.game) return false;
        const auto tr = G5::decodeTrainer5(*r);
        return tr && tr->rawName == u"QAEXP";
    });
    need(s06.empty(), s06);
    m.fact(c.spec.row, std::string(qamatrix::kExportChecksumFact), std::string(qamatrix::kExportChecksumDetected));
    need(c.sourceUntouched(), "export touched the source file");
    m.fact(c.spec.row, "export.output", "new-file-only");
    m.fact(c.spec.row, "export.reparse", "strict");
    return passed("new export dir (backup == source, edited.sav strict reparse, manifest); " +
                  std::to_string(changed) + " changed bytes all whitelisted; stale observation refused, no files");
}

Outcome immutability(Ctx& c) {
    need(qacontract::fileState(c.path) == c.initialFile, "source bytes/size/mtime changed");
    need(qacontract::treeState(c.savesDir) == c.initialTree, "saves directory changed");
    need(fs::file_size(c.path) == L::FullSaveSize, "source size changed");
    return passed("bytes, size and ns mtime of the source (and its directory) unchanged after discovery, "
                  "assignment, opens, all staging, candidates and export");
}

Outcome homeRefresh(Ctx& c, Matrix& m) {
    B::LegacySourceBindings bindings(c.bindingsPath.string());
    need(bindings.load(), "bindings load");
    S::HomeSnapshotCache cache;
    S::SnapshotRefresh outcome{};
    const auto* snap = homeRefresh(c, bindings, cache, outcome);
    need(snap && outcome == S::SnapshotRefresh::Parsed && snap->current(), "first refresh not Parsed/Current");
    need(snap->trainerName == "QATEST" && snap->trainerGenderKnown && snap->trainerGender == kTrainerGender,
         "snapshot trainer differs");
    need(snap->partyCount == 2 && snap->party[0].species == 25 && snap->party[0].level == 10 &&
             !snap->party[0].shiny && snap->party[1].species == 133,
         "snapshot party differs");
    need(snap->dexSeen == 2 && snap->dexCaught == 1 && snap->dexTotal == 649, "snapshot Pokedex differs");
    need(snap->contentFingerprint == c.fingerprint, "snapshot fingerprint differs from the opened source");
    for (int poll = 0; poll < 5; ++poll) {
        (void)homeRefresh(c, bindings, cache, outcome);
        need(outcome == S::SnapshotRefresh::Reused, "unchanged file was re-parsed");
    }
    need(cache.parseCount() == 1, "parse count after unchanged polls");
    // The emulator writes a new party (same inode, newer mtime): shiny Blue-Striped Basculin L33.
    qacontract::spit(c.path, buildSave(c.spec.family, c.spec.version, true));
    setMtime(c.path, 1000000100);
    snap = homeRefresh(c, bindings, cache, outcome);
    need(snap && outcome == S::SnapshotRefresh::Parsed && snap->current() && cache.parseCount() == 2,
         "changed file not re-validated as Current");
    need(snap->party[0].species == kBasculin, "refreshed party species " + std::to_string(snap->party[0].species));
    need(snap->contentFingerprint != c.fingerprint, "fingerprint unchanged after the rewrite");
    c.newFingerprint = snap->contentFingerprint;
    const bool levelKept = snap->party[0].level == 33;
    const bool shinyShown = snap->party[0].shiny;
    const bool formKept = snap->party[0].form == 1;
    m.fact(c.spec.row, "home.refresh", "snapshot-cache");
    m.fact(c.spec.row, "home.party.level", levelKept ? "kept" : "dropped");
    m.fact(c.spec.row, "home.party.shiny", shinyShown ? "shown" : "hidden");
    m.fact(c.spec.row, "home.party.form", formKept ? "kept" : "dropped");
    {
        // Gender: the card must carry the record's own PK5 0x40 gender (strict reparse of the file).
        std::string genderError;
        const auto onDisk = G5::Gen5ReadOnlySave::parse(qacontract::slurp(c.path), c.spec.game, &genderError,
                                                       G5::SaveCopySelection::Primary);
        const auto lead = onDisk ? onDisk->partyPokemon(0) : std::nullopt;
        m.fact(c.spec.row, "home.party.gender",
               lead && lead->gender() <= 2 && snap->party[0].gender == lead->gender() ? "kept" : "dropped");
    }
    need(levelKept && shinyShown, "refreshed party level/shiny lost");
    const std::string mechanics =
        "HomeSnapshotCache + homeCardSnapshot: Parsed once, Reused while unchanged, re-validated Current after "
        "the rewrite (species 550, level 33, shiny)";
    // QA-F5 is fixed (Gen5GameCardPreview.h carries pk->form()): a dropped form is a regression.
    need(formKept, "Home party form " + std::to_string(snap->party[0].form) +
                       " for Basculin form 1 (QA-F5 regression, Gen5GameCardPreview.h homeCardSnapshot)");
    return passed(mechanics + ", form 1 kept (QA-F5 fix holds)");
}

Outcome relaunch(Ctx& c) {
    need(!c.newFingerprint.empty(), "prerequisite failed: home-refresh did not rewrite the source");
    // A fresh process: bindings reloaded from disk, a new snapshot cache, a new Trainer session.
    B::LegacySourceBindings bindings(c.bindingsPath.string());
    need(bindings.load(), "bindings load");
    S::HomeSnapshotCache cache;
    S::SnapshotRefresh outcome{};
    const auto* snap = homeRefresh(c, bindings, cache, outcome);
    need(snap && outcome == S::SnapshotRefresh::Parsed && snap->current() && snap->party[0].species == kBasculin &&
             snap->contentFingerprint == c.newFingerprint,
         "relaunched Home card does not show the current file");
    const auto stale = G5::openAssignedSource(bindings, c.profile, c.spec.game, G5::SaveCopySelection::Automatic,
                                              c.fingerprint);
    need(!stale.ready(), "an Open pinned to the pre-rewrite fingerprint was accepted");
    const auto opened = G5::openAssignedSource(bindings, c.profile, c.spec.game);
    need(opened.ready() && opened.instance.contentFingerprint == c.newFingerprint, "reopen: " + opened.diagnostic);
    std::string error;
    const auto t = B::Gen5ReadOnlyTrainer::create(*opened.save, c.spec.game, error);
    need(t && t->sourceSave().partyPokemon(0)->species() == kBasculin, "relaunched Trainer session: " + error);
    return passed("reloaded bindings + new cache + new Trainer session see the rewritten file; the old "
                  "fingerprint is refused");
}

// --------------------------------------------------------------------------------------------
// Round 2 cells (bag-add, reroll), each on a fresh Gen5ReadOnlyTrainer bridge.
//   bag-add  Gen5BagScreen openAdd / moveAddCursor / pickAdd(ws, rule, refresh, error) with the
//            rule Gen5SharedReadOnlySurface.inc gen5PolicyRule builds, then beginQuantity +
//            BagAdd::decideRow as its A-on-row handler (tests/test_bag_add_flow_gen15.cpp gen5)
//   reroll   the Y handler of SharedPokemonEditorSurface.inc with the Gen V adapter (setIv / setEv
//            = Gen5SharedPokemonSession::stage(Field::IV / Field::EV)), then keepWithPresentation
// --------------------------------------------------------------------------------------------
namespace QP = PokeVault::Inventory::QuantityPolicy;

QP::ItemQuantityRule gen5Rule(const Ctx& c, size_t pocket, uint16_t id) {
    // Gen5SharedReadOnlySurface.inc gen5PolicyRule (Gen5Bag.h pocket order).
    static constexpr std::array<QP::Pocket, 5> kPockets{{QP::Pocket::Items, QP::Pocket::KeyItems,
                                                         QP::Pocket::TMHM, QP::Pocket::Medicine, QP::Pocket::Berries}};
    QP::Game game{};
    if (pocket >= kPockets.size() || !QP::gameFromId(c.spec.game, game)) return {};
    return QP::quantityRule(game, kPockets[pocket], id);
}

uint16_t heldIn(const G5::Gen5StagedPokemonWorkspace& ws, size_t pocket, uint16_t id) {
    const auto bag = ws.bagPresentation();
    if (!bag) return 0;
    for (const auto& s : (*bag)[pocket])
        if (s.itemId == id) return s.count;
    return 0;
}

Outcome bagAdd(Ctx& c) {
    namespace BagAdd = PokeBank::UIModel::BagAdd;
    using BagSurface = G5::Gen5BagScreen::Surface;
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& bag = t->bagScreen();
    const auto refresh = refresher(*t);
    std::string error;
    constexpr size_t kMedicine = 3, kKey = 1;
    constexpr uint16_t kAntidote = 18;
    const uint16_t potion = G5::BagCatalog5::kMedicine[0];
    const auto choose = [&](uint16_t id) {
        for (int guard = 0; guard < 1000 && bag.chosenAddItem() != id; ++guard) bag.moveAddCursor(1);
        return bag.chosenAddItem() == id;
    };
    while (bag.pocket() != kMedicine) bag.changePocket(1);
    need(heldIn(ws, kMedicine, kAntidote) == 0 && heldIn(ws, kMedicine, potion) == 12, "synthetic Medicine pocket");

    // A on a new stackable item: staged x1 at once, no keypad.
    const auto antidote = gen5Rule(c, kMedicine, kAntidote);
    need(antidote.valid && antidote.editable && antidote.minimum == 1, "Antidote policy rule");
    need(bag.openAdd(ws, error) && bag.surface() == BagSurface::AddPicker, "openAdd(Medicine): " + error);
    need(choose(kAntidote), "Antidote not in the Add picker");
    const auto pick = bag.pickAdd(ws, antidote, refresh, error);
    need(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1 && pick.quantity == antidote.minimum &&
             error.empty(), "pickAdd(Antidote) is not StageNew x1: " + error);
    need(bag.surface() == BagSurface::Browse && heldIn(ws, kMedicine, kAntidote) == 1,
         "staged bag does not hold Antidote x1");
    // A on that row: the quantity editor on [1, 999].
    const auto request = bag.beginQuantity(ws, error);
    need(request && request->itemId == kAntidote && request->current == 1, "beginQuantity on Antidote: " + error);
    const auto row = BagAdd::decideRow(antidote, request->current, request->maximum);
    need(row.outcome == BagAdd::RowOutcome::EditQuantity && row.minimum == 1 && row.maximum == 999,
         "Antidote row is not EditQuantity [1,999]");

    // A on the held Potion: focus the existing stack, nothing staged.
    const auto reviewBefore = ws.bagReviewChanges().size();
    need(bag.openAdd(ws, error) && choose(potion), "openAdd / Potion not in the picker: " + error);
    const auto held = bag.pickAdd(ws, gen5Rule(c, kMedicine, potion), refresh, error);
    need(held.outcome == BagAdd::PickOutcome::FocusExisting && error.empty(), "pickAdd(Potion) is not FocusExisting");
    const auto focused = bag.selected(ws);
    need(heldIn(ws, kMedicine, potion) == 12 && ws.bagReviewChanges().size() == reviewBefore && focused &&
             focused->itemId == potion, "re-pick changed the stage or did not focus the Potion");

    // Unique Key Items: existing items focus, a new one requires its story warning,
    // cancellation changes no serialized byte, and explicit removal is staged separately.
    while (bag.pocket() != kKey) bag.changePocket(1);
    const uint16_t key = c.b2w2() ? G5::BagCatalog5::kKeyB2W2[0] : G5::BagCatalog5::kKeyBW[0];
    const auto keyRule = gen5Rule(c, kKey, key);
    need(keyRule.valid && !keyRule.editable && keyRule.maximum == 1, "Key Item policy rule not unique / max 1");
    need(BagAdd::decideRow(keyRule, 1, G5::bagPocketLayout5(c.spec.family, kKey).maxCount).outcome ==
             BagAdd::RowOutcome::FixedQuantity, "Key Item row is not FixedQuantity");
    need(!bag.beginQuantity(ws, error).has_value(), "Key Item opened a quantity editor");
    need(bag.openAdd(ws,error) && choose(key),"Existing Key Item not offered: "+error);
    need(!bag.requestAddWarning(ws,keyRule) &&
        bag.pickAdd(ws,keyRule,refresh,error).outcome==BagAdd::PickOutcome::FocusExisting,
        "Existing Key Item did not focus without duplication");
    constexpr uint16_t addedKey=447,addedMachine=618;
    const auto beforeWarning=G5::buildStagedSaveCandidate(ws,&error);need(beforeWarning.has_value(),error);
    const auto addKeyRule=gen5Rule(c,kKey,addedKey);
    need(bag.openAdd(ws,error) && choose(addedKey),"New Key Item not offered: "+error);
    need(!bag.confirmAdd(ws,addedKey,1,refresh,error),"Key Item staged without warning");
    need(bag.requestAddWarning(ws,addKeyRule) && bag.surface()==BagSurface::KeyItemAddWarning,
        "Key Item warning missing");
    bag.cancel();
    const auto cancelled=G5::buildStagedSaveCandidate(ws,&error);
    need(cancelled && cancelled->bytes==beforeWarning->bytes,"Warning cancellation changed staged bytes");
    need(bag.openAdd(ws,error) && choose(addedKey) && bag.requestAddWarning(ws,addKeyRule),"Reopen Key Item warning");
    need(bag.pickAdd(ws,addKeyRule,refresh,error).outcome==BagAdd::PickOutcome::StageNew &&
        heldIn(ws,kKey,addedKey)==1,"Key Item x1 not staged: "+error);
    need(bag.tapRow(0,ws) && bag.requestRemove(ws,error),"Key Item Remove did not open");
    bag.cancel();need(heldIn(ws,kKey,key)==1,"Cancel removed original Key Item");
    need(bag.requestRemove(ws,error) && bag.confirmRemove(ws,refresh,error) && heldIn(ws,kKey,key)==0,
        "Explicit Key Item Remove failed: "+error);
    bag.changePocket(1);
    const auto machineRule=gen5Rule(c,2,addedMachine);
    need(bag.openAdd(ws,error) && choose(addedMachine),"TM/HM picker missing native item: "+error);
    need(!bag.confirmAdd(ws,addedMachine,2,refresh,error),"TM/HM stacked above one");
    need(!bag.requestAddWarning(ws,machineRule),"TM/HM incorrectly requested Key Item warning");
    need(bag.pickAdd(ws,machineRule,refresh,error).outcome==BagAdd::PickOutcome::StageNew &&
        heldIn(ws,2,addedMachine)==1,"TM/HM x1 not staged: "+error);
    need(bag.tapRow(0,ws) && bag.requestRemove(ws,error) && bag.confirmRemove(ws,refresh,error),
        "Explicit TM/HM Remove failed: "+error);

    // The staged bag really holds it: export candidate re-parses with Antidote x1.
    const auto out = G5::buildStagedSaveCandidate(ws, &error);
    need(out && out->bagChanged, "export candidate: " + error);
    const auto reopened = G5::Gen5ReadOnlySave::parse(out->bytes, c.spec.game, &error, G5::SaveCopySelection::Primary);
    need(reopened.has_value(), "candidate strict reparse: " + error);
    const auto exported = G5::decodeBag5(*reopened);
    need(exported && std::any_of((*exported)[kMedicine].begin(), (*exported)[kMedicine].end(),
                                 [](const auto& s) { return s.itemId == kAntidote && s.count == 1; }),
         "re-parsed Medicine pocket lacks Antidote x1");
    for(const auto& [pocket,id]:{std::pair<size_t,uint16_t>{kKey,addedKey},{2,addedMachine}})
        need(std::any_of((*exported)[pocket].begin(),(*exported)[pocket].end(),
            [id](const auto& slot){return slot.itemId==id && slot.count==1;}),"Unique item did not reparse at x1");
    need(c.sourceUntouched(), "bag add touched the source file");
    return passed("Gen5BagScreen::pickAdd(ws, gen5PolicyRule, refresh): Antidote StageNew x1 (no numpad), staged bag "
                  "x1 (candidate strict reparse); beginQuantity + decideRow EditQuantity [1,999]; held Potion -> "
                  "FocusExisting, nothing staged; Key/TM/HM warning/cancel/add/remove x1 and strict reparse, no unique quantity editor");
}

Outcome reroll(Ctx& c) {
    namespace Reroll = PokeBank::UIModel::SharedStatReroll;
    using FI = Shared::FieldIdentity;
    auto t = c.trainer();
    auto& ws = t->stagedPokemon();
    auto& ui = t->sharedScreen();
    std::string error;
    const auto openEdit = [&]() -> G5::Gen5SharedPokemonSession& {
        const bool opened = ui.openActions(ws, Slot{Region::Box, 0, 0}, error) &&
                            ui.activate(ws, Shared::Action::Edit, error);
        need(opened, "open Edit on box 1 slot 1: " + error);
        return ui.editDraft();
    };
    const auto keep = [&](G5::Gen5SharedPokemonSession& draft) {
        if (!draft.dirty()) {
            ui.discardDraft();  // nothing to Keep (B asks Keep/Discard on an Edit draft; Discard)
            return;
        }
        need(ui.keepWithPresentation(ws, refresher(*t), error), "Keep: " + error);
    };
    Reroll::Rng rng(0x5EED5u);
    for (const bool shiny : {false, true})
        for (const uint8_t gender : {uint8_t{0}, uint8_t{1}}) {
            const std::string what = std::string(shiny ? "shiny" : "non-shiny") + (gender ? " female" : " male");
            auto& setup = openEdit();
            if (setup.current()->shiny() != shiny) need(F::toggleShiny(setup, error), what + ": Shiny: " + error);
            if (setup.current()->gender() != gender)
                need(G5::stageSharedField(setup, FI::Gender, 0, gender, error), what + ": Gender: " + error);
            keep(setup);
            const auto start = ws.viewBox(0, 0);
            need(start && start->shiny() == shiny && start->gender() == gender, what + ": start state not staged");
            for (int i = 0; i < round2::kRerolls; ++i) {
                auto& draft = openEdit();
                const auto cur = draft.current();
                need(cur.has_value(), "no draft");
                const auto next = Reroll::reroll(rng, {cur->ivs(), cur->evs()});
                for (size_t s = 0; s < 6; ++s) need(draft.stage(NF::EV, s, 0, error), "EV clear: " + error);
                for (size_t s = 0; s < 6; ++s) need(draft.stage(NF::IV, s, next.ivs[s], error), "IV: " + error);
                for (size_t s = 0; s < 6; ++s) need(draft.stage(NF::EV, s, next.evs[s], error), "EV: " + error);
                keep(draft);
                const auto now = ws.viewBox(0, 0);
                need(now && now->ivs() == next.ivs && now->evs() == next.evs, what + ": rolled IVs/EVs not staged");
                need(now->shiny() == shiny && now->gender() == gender && now->pid() == start->pid(),
                     what + ": reroll " + std::to_string(i) + " changed shiny/gender/PID");
            }
        }
    need(c.sourceUntouched(), "reroll touched the source file");
    return passed("Y = SharedStatReroll::reroll via Gen5SharedPokemonSession::stage(EV 0 / IV / EV) -> "
                  "keepWithPresentation, x" + std::to_string(round2::kRerolls) +
                  " per case: Gen V-origin Eevee shiny/non-shiny x male/female keep shiny, gender and PID");
}

// --------------------------------------------------------------------------------------------
void runVersion(Matrix& m, const Spec& spec) {
    Ctx c;
    c.spec = spec;
    c.root = qacontract::scratchRoot("matrix-gen5-" + spec.row);
    c.savesDir = c.root / "saves";
    c.path = c.savesDir / spec.fileName;
    c.bindingsPath = c.root / "bindings";
    c.exportsRoot = c.root / "exports";
    c.original = buildSave(spec.family, spec.version);
    qacontract::writeSource(c.path, c.original);
    std::string backupError;
    const bool backupOk=verifyBackupSessionTrace(c.original,spec.game,backupError);
    m.check(spec.row,"backup-session",backupOk,backupError,"Exact historical file -> staged edit/export/Discard");
    c.initialFile = qacontract::fileState(c.path);
    c.initialTree = qacontract::treeState(c.savesDir);
    m.fact(spec.row, "identity.source", "save-internal");
    m.fact(spec.row, "nav.mutates", "no");
    m.fact(spec.row, "trainer.gender", "stored");
    m.fact(spec.row, "bag.read", "yes");

    const auto& v = spec.row;
    runCell(m, v, "discovery", [&] { return discovery(c); });
    runCell(m, v, "linking", [&] { return linking(c); });
    runCell(m, v, "provider", [&] { return provider(c); });
    runCell(m, v, "identity", [&] { return identity(c); });
    runCell(m, v, "trainer-name", [&] { return trainerName(c); });
    runCell(m, v, "trainer-gender", [&] { return trainerGender(c); });
    runCell(m, v, "party", [&] { return party(c); });
    runCell(m, v, "boxes", [&] { return boxes(c); });
    runCell(m, v, "pokedex", [&] { return pokedex(c); });
    runCell(m, v, "bag", [&] { return bag(c); });
    runCell(m, v, "navigation", [&] { return navigation(c); });
    runCell(m, v, "creation", [&] { return creation(c, m); });
    runCell(m, v, "fields", [&] { return fields(c); });
    runCell(m, v, "pickers", [&] { return pickers(c); });
    runCell(m, v, "actions", [&] { return actions(c); });
    runCell(m, v, "working-copy", [&] { return workingCopy(c); });
    runCell(m, v, "review", [&] { return review(c); });
    runCell(m, v, "export", [&] { return exportCell(c, m); });
    runCell(m, v, "immutability", [&] { return immutability(c); });
    // Round 2 cells: before home-refresh, which rewrites the source file.
    runCell(m, v, "bag-add", [&] { return bagAdd(c); });
    runCell(m, v, "reroll", [&] { return reroll(c); });
    runCell(m, v, "home-refresh", [&] { return homeRefresh(c, m); });
    runCell(m, v, "relaunch", [&] { return relaunch(c); });
    std::error_code ignored;
    fs::remove_all(c.root, ignored);
}

} // namespace

void runGen5(Matrix& m) {
    const std::vector<Spec> specs = {
        {"black", "black_nds", "white_nds", 21, Family::BlackWhite, "DraStic", "DraStic", "POKEMON W2.sav"},
        {"white", "white_nds", "black_nds", 20, Family::BlackWhite, "melonDS", "melonDS", "POKEMON B2.sav"},
        {"black2", "black2_nds", "white2_nds", 23, Family::Black2White2, "RetroArch", "RetroArch", "POKEMON W.sav"},
        {"white2", "white2_nds", "black2_nds", 22, Family::Black2White2, "Manual", "Manual file", "POKEMON B.sav"},
    };
    for (const auto& spec : specs) runVersion(m, spec);
}

} // namespace qamatrix
