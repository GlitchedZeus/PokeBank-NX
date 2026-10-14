// PokeBank NX application QA: export-safety and source-immutability contracts for EVERY export path
// (Thread 4, QA-owned). Each path is driven exactly the way its UI caller drives it:
//
//   gen1-pokemon+bag  Gen1PokemonEditorOverlayUX exportStagedCopy: Pokemon finalize with Bag candidate,
//                     Common::publishVerifiedStagedExport + gen1StrictValidator (.srm)
//   gen1-bag          ClassicInventoryOverlay exportStagedCopy (Gen I)            (.srm)
//   gen2-trainer      TrainerViewScreenGSCOverlay: Gen2::publishVerifiedStagedEditorExport (.srm)
//   gen2-bag          ClassicInventoryOverlay exportStagedCopy (Gen II)           (.srm)
//   gen3-pokemon+bag  Gen3SharedPokemonSurface: Gen3::publishVerifiedGen3Export(editor, request, inventory)
//   gen3-bag          ClassicInventoryOverlay exportStagedCopy (Gen III)          (.srm)
//   gen4-pokemon      Gen4SharedPokemonSurface: Gen4::publishVerifiedGen4Export
//   gen5-pk5          Gen5SharedReadOnlySurface: readNormalizedSourceReadOnly + publishGen5WorkingCopy
//   gen5-bag          the same Gen V path with only a native Bag edit staged
//   gen5-trainer      the same Gen V path with native Trainer Name + Money staged
//
// Contracts checked for every path:
//   C1 the source file's bytes, size and mtime never change (also when the export root IS the
//      source's own directory, and across failed exports);
//   C2 the published edited save strictly re-parses as the same exact game, and the published
//      backup is byte-identical to the source;
//   C3 only whitelisted bytes differ between source and edited save (field/block + its checksums,
//      layout constants from the owning production header or the accepted fixture that pins them);
//   C4 an existing export is never overwritten: a second export lands in a new directory and the
//      first is untouched; a directory or a plain file squatting on the destination is untouched;
//   C5 a failed export (every injected failure point, and an export root that is a plain file)
//      leaves no new file and no temporary/pending directory behind.
#include "fixtures/gsc_pokemon_fixture.h"
#include "fuzz_synthetic_saves.h"
#include "test_qa_support.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"

#include <functional>
#include <iostream>
#include <memory>

using namespace qacontract;
namespace Common = PokeVault::Integration::Common;
namespace G1 = PokeVault::Integration::Gen1;
namespace G2 = PokeVault::Integration::Gen2;
namespace G3 = PokeVault::Integration::Gen3;
namespace G4 = PokeVault::Integration::Gen4;
namespace G5 = PokeVault::Integration::Gen5;

namespace {

constexpr int kFailurePoints = 4;  // every path exposes four host-only injected failure points
const char* kTimestamp = "20261014_120000";

struct Outcome {
    bool success = false;
    std::string directory;
    std::string error;
};

struct ExportPath {
    std::string name;
    std::string sourceFile;  // name of the synthetic source on disk
    Bytes original;
    std::string backupName, editedName;
    // failurePoint 0 = none, 1..4 = injected failure after backup / edited / readback / manifest.
    // The source path is passed so paths that re-read the source (Gen V) do so for real.
    std::function<Outcome(const fs::path& root, const fs::path& source, int failurePoint)> publish;
    std::function<bool(std::span<const uint8_t>, std::string&)> strictReparse;
    std::vector<Range> whitelist;
};

Common::ExportFailurePoint commonPoint(int p) {
    switch (p) {
        case 1: return Common::ExportFailurePoint::AfterBackupWrite;
        case 2: return Common::ExportFailurePoint::AfterEditedWrite;
        case 3: return Common::ExportFailurePoint::AfterReadbackVerify;
        case 4: return Common::ExportFailurePoint::AfterManifestWrite;
        default: return Common::ExportFailurePoint::None;
    }
}

Outcome fromCommon(const Common::VerifiedExportResult& r) { return {r.success, r.directory, r.error}; }

// Files only (directories are allowed to be created by ensureDirectory-style setup).
std::map<std::string, FileState> filesOnly(const TreeState& tree) {
    std::map<std::string, FileState> out;
    for (const auto& [k, v] : tree)
        if (k.size() < 6 || k.substr(k.size() - 6) != "/<dir>") out[k] = v;
    return out;
}
bool hasTemporary(const TreeState& tree) {
    for (const auto& [k, v] : tree)
        if (k.find(".tmp") != std::string::npos || k.find(".pending") != std::string::npos) return true;
    return false;
}

void runContract(const ExportPath& path) {
    currentCase() = path.name;
    const fs::path base = scratchRoot(path.name);
    const fs::path sourceDir = base / "source";
    const fs::path source = sourceDir / path.sourceFile;
    writeSource(source, path.original);
    const auto sourceBefore = fileState(source);
    auto checkSource = [&](const char* when) {
        QA_EXPECT(fileState(source) == sourceBefore, std::string("C1 source file changed ") + when);
    };

    // C5: every injected failure point leaves nothing behind.
    const fs::path root = base / "exports";
    for (int point = 1; point <= kFailurePoints; ++point) {
        const auto before = treeState(root);
        const auto out = path.publish(root, source, point);
        const auto after = treeState(root);
        QA_EXPECT(!out.success && out.directory.empty(), "C5 injected failure " + std::to_string(point) + " reported success");
        QA_EXPECT(filesOnly(after) == filesOnly(before), "C5 injected failure " + std::to_string(point) + " left a file");
        QA_EXPECT(!hasTemporary(after), "C5 injected failure " + std::to_string(point) + " left a temporary directory");
        checkSource("after an injected failure");
    }
    // C5: an export root that is a plain file fails without touching it.
    {
        const fs::path fileRoot = base / "root-is-a-file";
        const Bytes marker{'q', 'a'};
        spit(fileRoot, marker);
        const auto before = fileState(fileRoot);
        const auto out = path.publish(fileRoot, source, 0);
        QA_EXPECT(!out.success, "C5 export into a plain-file root reported success");
        QA_EXPECT(fileState(fileRoot) == before, "C5 plain-file root was modified");
        checkSource("after a plain-file root");
    }

    // C2/C3: a real export.
    const auto first = path.publish(root, source, 0);
    QA_EXPECT(first.success, "C2 export failed: " + first.error);
    if (!first.success) return;
    const fs::path dir(first.directory);
    QA_EXPECT(dir.parent_path().string().rfind(root.string(), 0) == 0, "C4 export left the export root");
    const auto backup = slurp(dir / path.backupName);
    const auto edited = slurp(dir / path.editedName);
    QA_EXPECT(backup == path.original, "C2 backup differs from source");
    std::string error;
    QA_EXPECT(path.strictReparse(edited, error), "C2 edited save failed strict reparse: " + error);
    size_t offender = 0, changed = 0;
    const bool whitelisted = onlyWhitelistedChanges(path.original, edited, path.whitelist, &offender, &changed);
    QA_EXPECT(whitelisted, "C3 byte " + hex(offender) + " changed outside the whitelist");
    QA_EXPECT(changed > 0, "C3 export did not change any byte");
    checkSource("after a successful export");

    // C4: a second export never overwrites the first.
    const auto firstTree = treeState(dir);
    const auto second = path.publish(root, source, 0);
    QA_EXPECT(second.success && second.directory != first.directory, "C4 second export reused the first directory");
    QA_EXPECT(treeState(dir) == firstTree, "C4 first export was modified by the second");

    // C4: squatters (a directory with a sentinel, then a plain file) on the exact destination.
    const auto relative = fs::relative(dir, root);
    for (const bool squatAsFile : {false, true}) {
        const fs::path squatRoot = base / (squatAsFile ? "squat-file" : "squat-dir");
        const fs::path squat = squatRoot / relative;
        fs::create_directories(squat.parent_path());
        if (squatAsFile) {
            spit(squat, Bytes{'s', 'q', 'u', 'a', 't'});
        } else {
            fs::create_directories(squat);
            spit(squat / path.editedName, Bytes{'s', 'e', 'n', 't', 'i', 'n', 'e', 'l'});
        }
        const auto squatBefore = treeState(squatRoot);
        const auto squatFile = fileState(squat);
        const auto out = path.publish(squatRoot, source, 0);
        QA_EXPECT(out.directory != squat.string(), "C4 export replaced an existing destination");
        QA_EXPECT(fileState(squat) == squatFile, "C4 existing destination was modified");
        for (const auto& [k, v] : squatBefore)
            QA_EXPECT(treeState(squatRoot).count(k) && treeState(squatRoot).at(k) == v,
                      "C4 existing entry changed: " + k);
    }

    // C1: exporting INTO the source's own directory still never touches the source.
    const auto inSource = path.publish(sourceDir, source, 0);
    QA_EXPECT(inSource.success, "export into the source directory failed: " + inSource.error);
    checkSource("after exporting into the source directory");
    fs::remove_all(base);
    std::cout << "  " << path.name << ": " << changed << " byte(s) changed, all whitelisted\n";
}

// ------------------------------------------------------------------------------------------------
// Gen I
// ------------------------------------------------------------------------------------------------
// International R/B/Y bag list (count, id/qty pairs, 0xFF terminator) at 0x25C9 with 20 slots and the
// main-data checksum at 0x3523: tests/test_rby_staged_inventory.cpp INTL layout.
constexpr size_t kGen1Bag = 0x25C9, kGen1BagBytes = 1 + 20 * 2 + 1, kGen1Pc = 0x27E6, kGen1Sum = 0x3523;

Bytes gen1Source() {
    auto d = qasynth::gen1::save();
    std::fill_n(d.begin() + kGen1Bag, kGen1BagBytes, 0);
    d[kGen1Bag] = 1;
    d[kGen1Bag + 1] = 20;  // item id 20 x3, as in the accepted inventory fixture
    d[kGen1Bag + 2] = 3;
    d[kGen1Bag + 3] = 0xFF;
    d[kGen1Pc] = 0;  // empty PC item list with its terminator
    d[kGen1Pc + 1] = 0xFF;
    qasynth::gen1::forge(d);
    return d;
}

ExportPath gen1Path(bool withPokemonEditor) {
    const auto original = gen1Source();
    auto parsed = G1::parse(original, G1::SourceGame::Red);
    if (!parsed) throw std::runtime_error("gen1 synthetic source rejected: " + parsed.detail);
    std::string error;
    std::shared_ptr<G1::StagedInventoryEditor> inventory = G1::StagedInventoryEditor::create(*parsed.save, error);
    if (!inventory || !inventory->stageSetQuantity(PokeVault::Inventory::ClassicPocket::Medicines, 20, 25, error))
        throw std::runtime_error("gen1 bag staging failed: " + error);
    std::shared_ptr<G1::StagedPokemonEditor> pokemon;
    if (withPokemonEditor) {
        pokemon = G1::StagedPokemonEditor::create(*parsed.save, error);
        if (!pokemon) throw std::runtime_error("gen1 pokemon editor failed: " + error);
    }
    ExportPath path;
    path.name = withPokemonEditor ? "gen1-pokemon+bag" : "gen1-bag";
    path.sourceFile = "Pokemon Red.srm";
    path.original = original;
    path.backupName = "original_backup.srm";
    path.editedName = "edited.srm";
    path.publish = [=](const fs::path& root, const fs::path&, int point) {
        std::string err;
        Bytes edited = inventory->finalize(err);
        if (pokemon) edited = pokemon->finalizedBytes(err, edited);
        if (edited.empty()) return Outcome{false, {}, err};
        Common::VerifiedExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = std::string("red_gb_") + kTimestamp;
        request.generationLabel = withPokemonEditor ? "Generation I" : "Generation I Bag";
        request.fileExtension = ".srm";
        request.gameId = "red_gb";
        request.timestamp = kTimestamp;
        request.originalBytes = original;
        request.editedBytes = edited;
        request.changeLines = {"Inventory item 20: 3 -> 25"};
        request.validator = Common::gen1StrictValidator(G1::SourceGame::Red);
        request.failurePoint = commonPoint(point);
        return fromCommon(Common::publishVerifiedStagedExport(request));
    };
    path.strictReparse = Common::gen1StrictValidator(G1::SourceGame::Red);
    path.whitelist = {{kGen1Bag, kGen1Bag + kGen1BagBytes}, {kGen1Sum, kGen1Sum + 1}};
    return path;
}

// ------------------------------------------------------------------------------------------------
// Gen II
// ------------------------------------------------------------------------------------------------
ExportPath gen2Path(bool bag) {
    // The accepted Gen II editor fixture (tests/fixtures/gsc_pokemon_fixture.h, international Gold/Silver
    // layout GS with Bag pockets, box Pokemon and the secondary checksum at 0x7E6D).
    // A real save's secondary copy already mirrors the primary; make the synthetic one consistent
    // too (same mirror table and secondary checksum as cited below), so C3 measures the edit only.
    auto consistent = fixture(GS);
    struct MirrorRange { size_t begin, end, destination; };
    constexpr MirrorRange table[] = {{0x2009, 0x222F, 0x15C7}, {0x222F, 0x23D9, 0x3D69}, {0x23D9, 0x2856, 0x0C6B},
                                     {0x2856, 0x288A, 0x7E39}, {0x288A, 0x2D69, 0x10E8}};
    for (const auto& m : table)
        std::copy(consistent.begin() + static_cast<std::ptrdiff_t>(m.begin),
                  consistent.begin() + static_cast<std::ptrdiff_t>(m.end),
                  consistent.begin() + static_cast<std::ptrdiff_t>(m.destination));
    consistent[GS.sum2] = consistent[GS.sum];
    consistent[GS.sum2 + 1] = consistent[GS.sum + 1];
    const auto original = consistent;
    auto parsed = G2::parse(original, G2::SourceGame::Gold);
    if (!parsed) throw std::runtime_error("gen2 synthetic source rejected: " + parsed.detail);
    std::string error;
    std::shared_ptr<G2::StagedEditor> editor = G2::StagedEditor::create(*parsed.save, error);
    if (!editor) throw std::runtime_error("gen2 editor failed: " + error);
    if (bag) {
        // The first catalog item the exact Gold Items pocket accepts (no id is assumed here).
        bool staged = false;
        for (unsigned id = 1; id < 256 && !staged; ++id)
            staged = editor->stageInventoryQuantity(G2::InventoryPocket::Items, static_cast<uint8_t>(id), 5, error);
        if (!staged) throw std::runtime_error("gen2 bag staging failed: " + error);
    } else if (!editor->stageMoney(123457, error)) {
        throw std::runtime_error("gen2 money staging failed: " + error);
    }
    ExportPath path;
    path.name = bag ? "gen2-bag" : "gen2-trainer";
    path.sourceFile = "Pokemon Gold.srm";
    path.original = original;
    path.backupName = "original_backup.srm";
    path.editedName = "edited.srm";
    if (bag) {
        path.publish = [=](const fs::path& root, const fs::path&, int point) {
            std::string err;
            const auto edited = editor->finalizedBytes(err);
            if (edited.empty()) return Outcome{false, {}, err};
            Common::VerifiedExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = std::string("gold_gbc_") + kTimestamp;
            request.generationLabel = "Generation II Bag";
            request.fileExtension = ".srm";
            request.gameId = "gold_gbc";
            request.timestamp = kTimestamp;
            request.originalBytes = original;
            request.editedBytes = edited;
            request.validator = Common::gen2StrictValidator(G2::SourceGame::Gold);
            request.failurePoint = commonPoint(point);
            return fromCommon(Common::publishVerifiedStagedExport(request));
        };
    } else {
        path.publish = [=](const fs::path& root, const fs::path& source, int point) {
            if (point != 0) {
                // The UI-facing helper exposes no fault injection; drive the same transaction with it.
                std::string err;
                const auto edited = editor->finalizedBytes(err);
                G2::ExportTransactionRequest request;
                request.rootDirectory = root.string();
                request.directoryStem = std::string("gold_gbc_") + kTimestamp;
                request.sourcePath = source.string();
                request.gameId = "gold_gbc";
                request.timestamp = kTimestamp;
                request.sourceGame = G2::SourceGame::Gold;
                request.originalBytes = original;
                request.editedBytes = edited;
                request.pendingChanges = editor->pendingChanges();
                request.failurePoint = static_cast<G2::ExportFailurePoint>(point);
                const auto r = G2::publishVerifiedExport(request);
                return Outcome{r.success, r.directory, r.error};
            }
            G2::StagedEditorExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = std::string("gold_gbc_") + kTimestamp;
            request.sourcePath = source.string();
            request.gameId = "gold_gbc";
            request.timestamp = kTimestamp;
            const auto r = G2::publishVerifiedStagedEditorExport(*editor, request);
            return Outcome{r.success, r.directory, r.error};
        };
    }
    path.strictReparse = Common::gen2StrictValidator(G2::SourceGame::Gold);
    // Primary field ranges: money (3 bytes) or the Bag pockets (TM/HM pocket through the 50-slot PC
    // list), plus BOTH checksum words, plus each primary range's image in the international Gold/Silver
    // secondary copy (mirror table in src/Integration/Gen2/Gen2StagedEditor.cpp
    // synchronizeInternationalMirrors; layout in tests/fixtures/gsc_pokemon_fixture.h GS).
    const std::vector<Range> primary = bag ? std::vector<Range>{{GS.tm, GS.pc + 1 + 50 * 2 + 1}}
                                           : std::vector<Range>{{GS.money, GS.money + 3}};
    struct Mirror { size_t begin, end, destination; };
    constexpr Mirror mirrors[] = {{0x2009, 0x222F, 0x15C7}, {0x222F, 0x23D9, 0x3D69}, {0x23D9, 0x2856, 0x0C6B},
                                  {0x2856, 0x288A, 0x7E39}, {0x288A, 0x2D69, 0x10E8}};
    path.whitelist = primary;
    for (const auto& r : primary)
        for (const auto& m : mirrors) {
            const size_t b = std::max(r.begin, m.begin), e = std::min(r.end, m.end);
            if (b < e) path.whitelist.push_back({m.destination + (b - m.begin), m.destination + (e - m.begin)});
        }
    path.whitelist.push_back({GS.sum, GS.sum + 2});
    path.whitelist.push_back({GS.sum2, GS.sum2 + 2});
    return path;
}

// ------------------------------------------------------------------------------------------------
// Gen III
// ------------------------------------------------------------------------------------------------
// Active-slot sector bodies and sector checksums only; sector ids, signatures and save counters of
// both slots, and the whole inactive slot, must be untouched (Gen3::Detail layout constants).
std::vector<Range> gen3Whitelist(uint8_t activeSlot) {
    namespace D = G3::Detail;
    std::vector<Range> out;
    for (size_t s = 0; s < D::kSectorCount; ++s) {
        const size_t off = D::kSlotBases[activeSlot] + s * D::kSectorSize;
        out.push_back({off, off + 0xFF4});
        out.push_back({off + 0xFF6, off + 0xFF8});
    }
    return out;
}

ExportPath gen3Path(bool pokemon) {
    const auto original = qasynth::gen3::fixture();
    if (original.empty()) throw std::runtime_error("gen3 fixture missing (build-host/frlg_fixture.sav)");
    const auto game = G3::SourceGame::FireRedGBA;
    std::string error;
    std::shared_ptr<G3::StagedInventoryEditor> inventory = G3::StagedInventoryEditor::create(original, game, error);
    if (!inventory || !inventory->stageInventoryQuantity(G3::InventoryPouch::Items, 13, 50, error))
        throw std::runtime_error("gen3 bag staging failed: " + error);
    std::shared_ptr<G3::StagedPokemonEditor> editor;
    if (pokemon) {
        editor = G3::StagedPokemonEditor::create(original, game, error);
        if (!editor || !editor->stageCloneBoxPokemon(0, 0, 0, 1, error))
            throw std::runtime_error("gen3 pokemon staging failed: " + error);
    }
    ExportPath path;
    path.name = pokemon ? "gen3-pokemon+bag" : "gen3-bag";
    path.sourceFile = "Pokemon FireRed.sav";
    path.original = original;
    path.backupName = pokemon ? "original_backup.sav" : "original_backup.srm";
    path.editedName = pokemon ? "edited.sav" : "edited.srm";
    path.publish = [=](const fs::path& root, const fs::path& source, int point) {
        if (editor) {
            G3::Gen3ExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = std::string("firered_gba_") + kTimestamp;
            request.sourcePath = source.string();
            request.timestamp = kTimestamp;
            request.failurePoint = commonPoint(point);
            return fromCommon(G3::publishVerifiedGen3Export(*editor, request, inventory.get()));
        }
        std::string err;
        const auto edited = inventory->finalizedBytes(err);
        if (edited.empty()) return Outcome{false, {}, err};
        Common::VerifiedExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = std::string("firered_gba_") + kTimestamp;
        request.generationLabel = "Generation III Bag";
        request.fileExtension = ".srm";
        request.gameId = "firered_gba";
        request.timestamp = kTimestamp;
        request.originalBytes = original;
        request.editedBytes = edited;
        request.validator = Common::gen3StrictValidator(game);
        request.failurePoint = commonPoint(point);
        return fromCommon(Common::publishVerifiedStagedExport(request));
    };
    path.strictReparse = Common::gen3StrictValidator(game);
    path.whitelist = gen3Whitelist(inventory->activeSlot());
    return path;
}

// ------------------------------------------------------------------------------------------------
// Gen IV
// ------------------------------------------------------------------------------------------------
ExportPath gen4Path(qasynth::gen4::Kind kind, G4::Layout layout, const char* gameId) {
    const auto original = qasynth::gen4::save(kind);
    std::string error;
    auto created = G4::Gen4StagedPokemonEditor::create(original, layout, gameId, &error);
    if (!created || !created->stageCloneBoxPokemon(0, 0, 0, 1, &error))
        throw std::runtime_error(std::string("gen4 staging failed: ") + error);
    auto editor = std::make_shared<G4::Gen4StagedPokemonEditor>(std::move(*created));
    const auto parsed = G4::Gen4ReadOnlySave::parse(original, layout, gameId, &error);
    const auto sp = qasynth::gen4::spec(kind);
    ExportPath path;
    path.name = std::string("gen4-pokemon-") + gameId;
    path.sourceFile = "Pokemon.sav";
    path.original = original;
    path.backupName = "original_backup.sav";
    path.editedName = "edited.sav";
    path.publish = [=](const fs::path& root, const fs::path& source, int point) {
        G4::Gen4ExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = std::string(gameId) + "_" + kTimestamp;
        request.sourcePath = source.string();
        request.timestamp = kTimestamp;
        request.failurePoint = commonPoint(point);
        return fromCommon(G4::publishVerifiedGen4Export(*editor, request));
    };
    path.strictReparse = [=](std::span<const uint8_t> bytes, std::string& err) {
        const auto again = G4::Gen4ReadOnlySave::parse(bytes, layout, gameId, &err);
        return again && again->assignmentStatus() == G4::AssignmentStatus::Match && !again->recovered();
    };
    // Only the selected storage block (boxes) of the selected partition: SAV4 spec() geometry.
    const size_t storage = parsed->storageSelection().offset;
    path.whitelist = {{storage, storage + sp.storageSize}};
    return path;
}

// ------------------------------------------------------------------------------------------------
// Gen V
// ------------------------------------------------------------------------------------------------
G5::WorkingCopyFailurePoint gen5Point(int p) {
    switch (p) {
        case 1: return G5::WorkingCopyFailurePoint::AfterBackup;
        case 2: return G5::WorkingCopyFailurePoint::AfterEdited;
        case 3: return G5::WorkingCopyFailurePoint::AfterReadback;
        case 4: return G5::WorkingCopyFailurePoint::AfterManifest;
        default: return G5::WorkingCopyFailurePoint::None;
    }
}

// The record range plus the containing block's checksum and mirror words (LayoutInfo tables).
std::vector<Range> gen5RecordWhitelist(G5::SaveFamily family, size_t copyBase, size_t recordOffset, size_t length) {
    std::vector<Range> out{{copyBase + recordOffset, copyBase + recordOffset + length}};
    // A changed block's checksum and mirror words; the mirror table itself lives inside the final
    // footer block, whose own checksum therefore changes too (followed transitively).
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

enum class Gen5Edit { Pk5, Bag, Trainer };

ExportPath gen5Path(G5::SaveFamily family, uint8_t version, bool backupCopy, Gen5Edit edit) {
    const bool bag = edit == Gen5Edit::Bag;
    const auto original = qasynth::gen5::save(family, version, backupCopy ? 2 : 1);
    std::string error;
    const auto selection = backupCopy ? G5::SaveCopySelection::Backup : G5::SaveCopySelection::Primary;
    const auto parsed = G5::Gen5ReadOnlySave::parse(original, {}, &error, selection);
    if (!parsed) throw std::runtime_error("gen5 synthetic source rejected: " + error);
    auto workspace = std::make_shared<G5::Gen5StagedPokemonWorkspace>(*parsed);
    const size_t base = parsed->selectedCopyOffset();
    ExportPath path;
    if (edit == Gen5Edit::Trainer) {
        // Native Trainer Name + Money (Gen5Trainer.h TrainerLayout5: PlayerData block 27 name at
        // 0x04..0x13, Misc block 52 money at 0x00..0x03).
        auto* trainer = workspace->stagedTrainer(&error);
        if (!trainer || !trainer->setName("QATEST", &error) || !trainer->setMoney(123456, &error))
            throw std::runtime_error("gen5 trainer staging failed: " + error);
        namespace T = G5::TrainerLayout5;
        const auto& player = G5::trainerBlock5(family, T::PlayerBlockIndex);
        const auto& misc = G5::trainerBlock5(family, T::MiscBlockIndex);
        path.whitelist = gen5RecordWhitelist(family, base, player.offset + T::NameOffset, T::NameBytes);
        const auto money = gen5RecordWhitelist(family, base, misc.offset + T::MoneyOffset, 4);
        path.whitelist.insert(path.whitelist.end(), money.begin(), money.end());
    } else if (bag) {
        auto* staged = workspace->stagedBag(&error);
        if (!staged || !staged->setQuantity(static_cast<size_t>(G5::BagPocket5::Items), 0, 7, &error))
            throw std::runtime_error("gen5 bag staging failed: " + error);
        const size_t at = G5::itemsBlockOffset(family);
        path.whitelist = gen5RecordWhitelist(family, base, at, G5::itemsBlockLength(family));
    } else {
        using Field = G5::StagedPokemon5Record::Field;
        if (!workspace->stageBox(0, 0, Field::Level, 0, 40, &error))
            throw std::runtime_error("gen5 pk5 staging failed: " + error);
        path.whitelist = gen5RecordWhitelist(family, base, G5::LayoutInfo::BoxOffset, G5::Crypto::StoredSize);
    }
    const std::string game(parsed->exactGameId());
    path.name = std::string(edit == Gen5Edit::Trainer ? "gen5-trainer-" : bag ? "gen5-bag-" : "gen5-pk5-") + game + (backupCopy ? "-backup" : "-primary");
    path.sourceFile = "POKEMON B.sav";
    path.original = original;
    path.backupName = "original_backup.sav";
    path.editedName = "edited.sav";
    path.publish = [=](const fs::path& root, const fs::path& source, int point) {
        Bytes observed;
        std::string err, container;
        if (!G5::readNormalizedSourceReadOnly(source.string(), observed, err, &container))
            return Outcome{false, {}, err};
        G5::WorkingCopyExportRequest request;
        request.profileIdentity = std::string(32, '0');
        request.timestamp = kTimestamp;
        request.sourcePath = source.string();
        request.observedSourceBytes = observed;
        request.rootDirectory = root.string();
        request.failurePoint = gen5Point(point);
        const auto r = G5::publishGen5WorkingCopy(*workspace, request);
        return Outcome{r.success, r.directory, r.error};
    };
    path.strictReparse = [=](std::span<const uint8_t> bytes, std::string& err) {
        const auto again = G5::Gen5ReadOnlySave::parse(bytes, game, &err, selection);
        return again && again->selectedCopyOffset() == base;
    };
    return path;
}

} // namespace

int main() {
    std::vector<std::function<ExportPath()>> paths = {
        [] { return gen1Path(true); },
        [] { return gen1Path(false); },
        [] { return gen2Path(false); },
        [] { return gen2Path(true); },
        [] { return gen3Path(true); },
        [] { return gen3Path(false); },
        [] { return gen4Path(qasynth::gen4::Kind::DiamondPearl, G4::Layout::DiamondPearl, "diamond_nds"); },
        [] { return gen4Path(qasynth::gen4::Kind::Platinum, G4::Layout::Platinum, "platinum_nds"); },
        [] { return gen4Path(qasynth::gen4::Kind::HeartGoldSoulSilver, G4::Layout::HeartGoldSoulSilver, "heartgold_nds"); },
        [] { return gen5Path(G5::SaveFamily::BlackWhite, 21, false, Gen5Edit::Pk5); },
        [] { return gen5Path(G5::SaveFamily::BlackWhite, 20, true, Gen5Edit::Pk5); },
        [] { return gen5Path(G5::SaveFamily::Black2White2, 23, false, Gen5Edit::Pk5); },
        [] { return gen5Path(G5::SaveFamily::Black2White2, 22, true, Gen5Edit::Pk5); },
        [] { return gen5Path(G5::SaveFamily::BlackWhite, 21, false, Gen5Edit::Bag); },
        [] { return gen5Path(G5::SaveFamily::Black2White2, 22, true, Gen5Edit::Bag); },
        [] { return gen5Path(G5::SaveFamily::BlackWhite, 20, true, Gen5Edit::Trainer); },
        [] { return gen5Path(G5::SaveFamily::Black2White2, 23, false, Gen5Edit::Trainer); },
    };
    std::cout << "Export-safety contracts (C1 source immutable, C2 strict reparse, C3 whitelist, "
                 "C4 no overwrite, C5 no partial output):\n";
    for (const auto& make : paths) {
        try {
            runContract(make());
        } catch (const std::exception& e) {
            ++failures();
            std::fprintf(stderr, "CONTRACT SETUP FAILED [%s]: %s\n", currentCase().c_str(), e.what());
        }
    }
    if (failures() != 0) {
        std::fprintf(stderr, "%d export contract violation(s)\n", failures());
        return 1;
    }
    std::cout << "All " << paths.size() << " export paths satisfy C1-C5\n";
    return 0;
}
