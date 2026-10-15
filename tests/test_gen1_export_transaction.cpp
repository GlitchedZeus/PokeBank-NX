// Phase 4 safety for Generation I (Red/Blue/Yellow): Open -> Validate -> Working copy -> Edit ->
// Review -> Export -> Verify through the same production transaction the Gen I editor uses
// (publishVerifiedGen1Export). Synthetic fixtures only.
//
// Asserted: the source file and the editor's original bytes never change; Box and Bag edits are
// exported separately and together; checksums are recomputed; exports reparse strictly; regions
// outside the edited records stay byte-identical; stale/foreign Bag workspaces, unfinished packed
// moves and empty sessions are refused; every fault point and racing destination leaves no
// residue; malformed, truncated and checksum-broken inputs are refused before a workspace opens.
#include "Integration/Gen1/Gen1ExportTransaction.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace PokeVault::Integration::Gen1;
namespace Common = PokeVault::Integration::Common;
using PokeVault::Inventory::ClassicPocket;

namespace {

// International R/B/Y SRAM layout, as used by test_rby_pokemon_editor (same synthetic fixture).
constexpr size_t kMainStart = 0x2598, kMainLength = 0xF8B, kMainChecksum = 0x3523;
constexpr size_t kBag = 0x25C9, kPc = 0x27E6, kParty = 0x2F2C, kCurrentBox = 0x30C0;

uint8_t sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return static_cast<uint8_t>((~s) & 255);
}
size_t bank(size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; }
void checksums(std::vector<uint8_t>& v) {
    v[kMainChecksum] = sum(v, kMainStart, kMainLength);
    if (v[0x284C] & 0x80)
        for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
            v[b + 0x1A4C] = sum(v, b, 0x1A4C);
            for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = sum(v, b + i * 0x462, 0x462);
        }
}
std::vector<uint8_t> fixture(SourceGame game, uint8_t moneyLow = 0x00, uint8_t bagRepels = 0) {
    std::vector<uint8_t> v(0x8000, 0);
    // Scratch/sprite area below the main block: non-zero so any stray write is visible.
    for (size_t i = 0; i < kMainStart; ++i) v[i] = static_cast<uint8_t>(i * 7 + 3);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50; // RED
    v[0x25F5] = moneyLow;
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[kBag + 1] = v[kPc + 1] = 0xFF;
    if (bagRepels) { v[kBag] = 1; v[kBag + 1] = 30; v[kBag + 2] = bagRepels; v[kBag + 3] = 0xFF; }
    v[kParty] = 0; v[kParty + 1] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    const size_t box = kCurrentBox, body = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF; // Pikachu, internal 0x54
    v[body] = 0x54; v[body + 1] = 0; v[body + 2] = 20; v[body + 3] = 5;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 33;
    v[body + 12] = 0x12; v[body + 13] = 0x34; v[body + 16] = 125;
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 35;
    std::fill_n(v.begin() + box + 682, 11, 0x50);
    v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50);
    v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80; // PIKA
    std::copy_n(v.begin() + box, 0x462, v.begin() + bank(2));
    checksums(v);
    return v;
}

std::vector<uint8_t> slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void spit(const fs::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
std::string text(const fs::path& path) {
    const auto b = slurp(path);
    return {b.begin(), b.end()};
}
size_t entries(const fs::path& dir) {
    return static_cast<size_t>(std::distance(fs::directory_iterator(dir), fs::directory_iterator()));
}

bool inRange(size_t i, size_t begin, size_t length) { return i >= begin && i < begin + length; }
bool isChecksum(size_t i) {
    if (i == kMainChecksum) return true;
    for (size_t b : {size_t(0x4000), size_t(0x6000)})
        if (i >= b + 0x1A4C && i <= b + 0x1A4C + 6) return true;
    return false;
}

struct Session {
    std::vector<uint8_t> raw;
    ParseResult parsed;
    std::unique_ptr<StagedPokemonEditor> pokemon;
    std::unique_ptr<StagedInventoryEditor> inventory;
};

// Open + Validate + Working copy from the bytes actually read from the source file on disk.
Session open(const fs::path& sourceFile, SourceGame game) {
    Session s;
    s.raw = slurp(sourceFile);
    s.parsed = parse(s.raw, game);
    assert(s.parsed);
    std::string error;
    s.pokemon = StagedPokemonEditor::create(*s.parsed.save, error);
    assert(s.pokemon);
    s.inventory = StagedInventoryEditor::create(*s.parsed.save, error);
    assert(s.inventory);
    return s;
}

Gen1ExportRequest requestFor(const fs::path& root, const std::string& stem, const fs::path& source) {
    Gen1ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = stem;
    request.sourcePath = source.string();
    request.timestamp = "20261015-120000";
    request.applicationSha = "test-sha";
    return request;
}

// Export -> Verify: exact backup, strict reparse, manifest, and no temporary residue.
std::vector<uint8_t> verifyPublished(const Common::VerifiedExportResult& result, const Session& s,
                                     SourceGame game, const fs::path& root) {
    if (!result.success) std::cerr << "Gen I export failed: " << result.error << '\n';
    assert(result.success && result.changedBytes > 0);
    const fs::path dir(result.directory);
    assert(slurp(dir / "original_backup.srm") == s.raw);
    const auto edited = slurp(dir / "edited.srm");
    assert(edited.size() == s.raw.size());
    const auto reparsed = parse(edited, game);
    assert(reparsed && reparsed.save->metadata().sourceGame == game);
    assert(edited[kMainChecksum] == sum(edited, kMainStart, kMainLength));
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        assert(edited[b + 0x1A4C] == sum(edited, b, 0x1A4C));
        for (size_t i = 0; i < 6; ++i) assert(edited[b + 0x1A4D + i] == sum(edited, b + i * 0x462, 0x462));
    }
    const auto manifest = text(dir / "EDIT_MANIFEST.txt");
    assert(manifest.find(std::string("GAME_ID=") + sourceGameId(game)) != std::string::npos);
    assert(manifest.find("SOURCE_SHA256=" + result.originalSha256) != std::string::npos);
    for (const auto& entry : fs::directory_iterator(root))
        assert(entry.path().filename().string().find(".tmp") == std::string::npos);
    return edited;
}

void boxEdit(const fs::path& root, const fs::path& source, SourceGame game) {
    auto s = open(source, game);
    std::string error;
    BoxPokemonEdit edit;
    edit.level = 30;
    edit.dvs = std::array<uint8_t, 4>{15, 14, 13, 12};
    edit.nickname = "Sparky";
    assert(s.pokemon->stageEdit(2, 0, edit, error));
    // Review: every change is listed before export.
    const auto lines = gen1ExportChangeLines(*s.pokemon, s.inventory.get());
    assert(!lines.empty() && lines.size() == s.pokemon->pendingChanges().size());

    const auto result = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "box", source), s.inventory.get());
    const auto edited = verifyPublished(result, s, game, root);
    const auto reparsed = parse(edited, game);
    const auto& slot = reparsed.save->boxes()[2].slots[0];
    assert(slot && slot->level == 30 && slot->nickname == "Sparky");
    assert((slot->dvs == std::array<uint8_t, 5>{
        StagedPokemonEditor::derivedHPDV({15, 14, 13, 12}), 15, 14, 13, 12}));
    // Box-only export: only the current box copy, its banked copy and checksums may change.
    // Party, Bag, PC items, money and the scratch area are untouched.
    for (size_t i = 0; i < edited.size(); ++i)
        if (edited[i] != s.raw[i])
            assert(inRange(i, kCurrentBox, 0x462) || inRange(i, bank(2), 0x462) || isChecksum(i));
    assert(std::equal(edited.begin() + kParty, edited.begin() + kParty + 404, s.raw.begin() + kParty));
    const auto manifest = text(fs::path(result.directory) / "EDIT_MANIFEST.txt");
    assert(manifest.find("Party edit deferred") != std::string::npos);
    // Source never changes: in memory, in the editor and on disk.
    assert(slurp(source) == s.raw);
    assert(std::equal(s.raw.begin(), s.raw.end(), s.pokemon->originalBytes().begin()));
    assert(std::equal(s.raw.begin(), s.raw.end(), s.parsed.save->sourceBytes().begin()));
    fs::remove_all(result.directory);
}

void bagEdit(const fs::path& root, const fs::path& source, SourceGame game) {
    auto s = open(source, game);
    std::string error;
    assert(!s.pokemon->hasPendingChanges());
    assert(s.inventory->stageSetQuantity(ClassicPocket::Balls, 4, 10, error));
    const auto result = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "bag", source), s.inventory.get());
    const auto edited = verifyPublished(result, s, game, root);
    // Bag-only export: only the Bag list and the main checksum change. Boxes are untouched.
    for (size_t i = 0; i < edited.size(); ++i)
        if (edited[i] != s.raw[i]) assert(inRange(i, kBag, 0x25F3 - kBag) || i == kMainChecksum);
    std::string reviewError;
    auto reread = StagedInventoryEditor::create(*parse(edited, game).save, reviewError);
    assert(reread);
    bool found = false;
    for (const auto& entry : reread->entries(ClassicPocket::Balls, reviewError))
        found = found || (entry.itemId == 4 && entry.quantity == 10);
    assert(found);
    assert(slurp(source) == s.raw);
    fs::remove_all(result.directory);
}

void combinedEdit(const fs::path& root, const fs::path& source, SourceGame game) {
    auto s = open(source, game);
    std::string error;
    BoxPokemonEdit edit;
    edit.otName = "Will";
    assert(s.pokemon->stageEdit(2, 0, edit, error));
    assert(s.inventory->stageSetQuantity(ClassicPocket::Medicines, 20, 25, error));
    const auto lines = gen1ExportChangeLines(*s.pokemon, s.inventory.get());
    assert(lines.size() == s.pokemon->pendingChanges().size() + s.inventory->pendingChanges().size());
    const auto result = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "both", source), s.inventory.get());
    const auto edited = verifyPublished(result, s, game, root);
    const auto reparsed = parse(edited, game);
    assert(reparsed.save->boxes()[2].slots[0]->originalTrainer == "Will");
    for (size_t i = 0; i < edited.size(); ++i)
        if (edited[i] != s.raw[i])
            assert(inRange(i, kCurrentBox, 0x462) || inRange(i, bank(2), 0x462) ||
                   inRange(i, kBag, 0x25F3 - kBag) || isChecksum(i));
    const auto manifest = text(fs::path(result.directory) / "EDIT_MANIFEST.txt");
    assert(manifest.find("Inventory item 20: 0 -> 25") != std::string::npos);
    assert(slurp(source) == s.raw);
    fs::remove_all(result.directory);
}

void refusals(const fs::path& root, const fs::path& source, SourceGame game) {
    auto s = open(source, game);
    std::string error;

    // Nothing staged: refused, nothing written.
    auto none = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "none", source), s.inventory.get());
    assert(!none.success && none.error.find("No pending") != std::string::npos);
    assert(entries(root) == 0);

    // A Bag workspace opened from a different (stale or newer) copy of the save is never spliced
    // in. The first copy differs only INSIDE the Bag byte range (plus the main checksum), which the
    // per-byte baseline check in finalizedBytes cannot see; the second differs in money as well.
    for (const auto& otherRaw : {fixture(game, 0x00, 7), fixture(game, 0x42)}) {
        const auto otherParsed = parse(otherRaw, game);
        assert(otherParsed);
        auto foreignBag = StagedInventoryEditor::create(*otherParsed.save, error);
        assert(foreignBag && foreignBag->stageSetQuantity(ClassicPocket::Balls, 4, 3, error));
        auto stale = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "stale", source), foreignBag.get());
        assert(!stale.success && stale.error.find("different saves") != std::string::npos);
        assert(entries(root) == 0);
    }

    // An unfinished packed move must be placed or cancelled before export.
    BoxPokemonEdit edit;
    edit.level = 9;
    assert(s.pokemon->stageEdit(2, 0, edit, error));
    assert(s.pokemon->beginPackedMove(2, 0, error));
    auto carrying = publishVerifiedGen1Export(*s.pokemon, requestFor(root, "carry", source), s.inventory.get());
    assert(!carrying.success && carrying.error.find("Place or cancel") != std::string::npos);
    assert(entries(root) == 0);
    assert(s.pokemon->cancelPackedMove(error));

    // Every fault point and racing destination fails closed without residue, and the other
    // writer's path is preserved untouched.
    for (const auto point : {Common::ExportFailurePoint::AfterBackupWrite,
                             Common::ExportFailurePoint::AfterEditedWrite,
                             Common::ExportFailurePoint::AfterReadbackVerify,
                             Common::ExportFailurePoint::AfterManifestWrite,
                             Common::ExportFailurePoint::AfterFirstPublishedFile}) {
        auto request = requestFor(root, "fault", source);
        request.failurePoint = point;
        assert(!publishVerifiedGen1Export(*s.pokemon, request, s.inventory.get()).success);
        assert(entries(root) == 0);
    }
    auto racedDir = requestFor(root, "raced", source);
    racedDir.failurePoint = Common::ExportFailurePoint::DestinationAppearsAsEmptyDirectory;
    auto raced = publishVerifiedGen1Export(*s.pokemon, racedDir, s.inventory.get());
    assert(!raced.success && raced.error.find("appeared before publication") != std::string::npos);
    assert(fs::is_directory(root / "raced") && fs::is_empty(root / "raced") && entries(root) == 1);
    fs::remove(root / "raced");
    auto racedFile = requestFor(root, "raced", source);
    racedFile.failurePoint = Common::ExportFailurePoint::DestinationAppearsAsFile;
    assert(!publishVerifiedGen1Export(*s.pokemon, racedFile, s.inventory.get()).success);
    assert(fs::is_regular_file(root / "raced") && fs::file_size(root / "raced") == 0 && entries(root) == 1);
    fs::remove(root / "raced");

    // Cancel (discard) returns the working copy to the exact source; nothing can then be exported.
    s.pokemon->discard();
    s.inventory->discard();
    assert(std::equal(s.raw.begin(), s.raw.end(), s.pokemon->stagedBytes().begin()));
    assert(!publishVerifiedGen1Export(*s.pokemon, requestFor(root, "after-discard", source),
                                      s.inventory.get()).success);
    assert(entries(root) == 0);
    assert(slurp(source) == s.raw);
}

// Malformed, truncated, checksum-broken and foreign inputs never reach a working copy.
void malformedInputs(SourceGame game) {
    const auto good = fixture(game);
    assert(parse(good, game));
    auto truncated = good;
    truncated.resize(0x7FFF);
    assert(!parse(truncated, game));
    assert(!parse(std::vector<uint8_t>{}, game));
    auto oversized = good;
    oversized.push_back(0);
    assert(!parse(oversized, game));
    auto badChecksum = good;
    badChecksum[kMainChecksum] ^= 0x01;
    assert(!parse(badChecksum, game));
    auto badBody = good;
    badBody[0x2605] ^= 0x40; // trainer id inside the checksum-covered main block
    assert(!parse(badBody, game));
}

} // namespace

int main() {
    const fs::path base = fs::temp_directory_path() / ("pokebank-gen1-export-" + std::to_string(::getpid()));
    fs::remove_all(base);
    for (const auto game : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow}) {
        const fs::path root = base / sourceGameId(game) / "exports";
        const fs::path source = base / sourceGameId(game) / "source.srm";
        fs::create_directories(root);
        spit(source, fixture(game));
        boxEdit(root, source, game);
        bagEdit(root, source, game);
        combinedEdit(root, source, game);
        refusals(root, source, game);
        malformedInputs(game);
        assert(entries(root) == 0);
    }
    fs::remove_all(base);
    std::cout << "Gen I verified export (Box, Bag, combined, stale/foreign Bag, packed move, faults, races, "
                 "malformed input, source immutability): PASS\n";
    return 0;
}
