#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "test_temp_dir.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace PokeVault::Integration::Gen2;

namespace {

bool exists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

std::vector<uint8_t> readBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

std::string readText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// Every entry name directly inside the export root (no "." / "..").
std::vector<std::string> rootEntries(const std::string& root) {
    std::vector<std::string> names;
    DIR* dir = ::opendir(root.c_str());
    assert(dir);
    while (const dirent* entry = ::readdir(dir)) {
        const std::string name = entry->d_name;
        if (name != "." && name != "..") names.push_back(name);
    }
    ::closedir(dir);
    return names;
}

bool isDirectory(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool isEmptyRegularFile(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size == 0;
}

void removeExport(const std::string& directory) {
    std::remove((directory + "/original_backup.srm").c_str());
    std::remove((directory + "/edited.srm").c_str());
    std::remove((directory + "/EDIT_MANIFEST.txt").c_str());
    ::rmdir(directory.c_str());
}

void runExport(const L& layout, SourceGame game, const std::string& tag) {
    const auto source = fixture(layout, true);
    auto parsed = parse(source, game);
    assert(parsed);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor);
    // The fixture stores 123456. Stage a guaranteed different valid value so this test exercises
    // the same "pending staged work" precondition as the real Review -> Export UI.
    assert(editor->stageMoney(123457, error));
    assert(editor->hasPendingChanges());
    const auto edited = editor->finalizedBytes(error);
    assert(!edited.empty());
    assert(source == std::vector<uint8_t>(editor->originalBytes().begin(), editor->originalBytes().end()));

    const std::string root = PokeBankTest::freshTempBase("pokebank-gen2-export");  // fresh per run
    const std::string stem = "verified-" + tag;

    // This is the same staged-editor-level abstraction used by the real Gen II UI. It owns
    // finalization and pending-change capture, then delegates to the verified disk transaction.
    StagedEditorExportRequest uiRequest;
    uiRequest.rootDirectory = root;
    uiRequest.directoryStem = stem;
    uiRequest.sourcePath = "/retroarch/saves/" + tag + ".srm";
    uiRequest.gameId = sourceGameId(game);
    uiRequest.timestamp = "20260914-120000";
    uiRequest.applicationSha = "test-sha";

    const auto result = publishVerifiedStagedEditorExport(*editor, uiRequest);
    if (!result.success)
        std::fprintf(stderr, "verified staged export failed for %s: %s\n", tag.c_str(), result.error.c_str());
    assert(result.success && !result.directory.empty());
    assert(exists(result.directory + "/original_backup.srm"));
    assert(exists(result.directory + "/edited.srm"));
    assert(exists(result.directory + "/EDIT_MANIFEST.txt"));
    assert(readBytes(result.directory + "/original_backup.srm") == source);
    assert(readBytes(result.directory + "/edited.srm") == edited);
    const auto written = parse(readBytes(result.directory + "/edited.srm"), game);
    assert(written);
    const std::string manifest = readText(result.directory + "/EDIT_MANIFEST.txt");
    assert(manifest.find("GAME_ID=" + std::string(sourceGameId(game))) != std::string::npos);
    assert(manifest.find("SAVE_FORMAT=PK2 / GSC SRAM") != std::string::npos);
    assert(manifest.find("SOURCE_PATH=/retroarch/saves/" + tag + ".srm") != std::string::npos);
    assert(manifest.find("SOURCE_KIND=RetroArchLegacy_READ_ONLY") != std::string::npos);
    assert(manifest.find("SOURCE_SIZE=" + std::to_string(source.size())) != std::string::npos);
    assert(manifest.find("SOURCE_SHA256=" + result.originalSha256) != std::string::npos);
    assert(manifest.find("EDITED_SIZE=" + std::to_string(edited.size())) != std::string::npos);
    assert(manifest.find("EDITED_SHA256=" + result.editedSha256) != std::string::npos);
    assert(manifest.find("APPLICATION_SHA=test-sha") != std::string::npos);
    assert(manifest.find("WRITE_READBACK=VERIFIED") != std::string::npos);
    assert(manifest.find("STRICT_REPARSE=VERIFIED") != std::string::npos);
    assert(manifest.find("LIVE_RETROARCH_WRITE=DISABLED") != std::string::npos);
    assert(manifest.find("LIVE_INSTALLED_GAME_WRITE=DISABLED") != std::string::npos);

    // Low-level request remains public only for deterministic failure/corruption injection tests.
    ExportTransactionRequest request;
    request.rootDirectory = root;
    request.directoryStem = stem;
    request.sourcePath = uiRequest.sourcePath;
    request.gameId = uiRequest.gameId;
    request.timestamp = uiRequest.timestamp;
    request.applicationSha = uiRequest.applicationSha;
    request.sourceGame = game;
    request.originalBytes = editor->originalBytes();
    request.editedBytes = edited;
    request.pendingChanges = editor->pendingChanges();

    // Failure after files have been created must leave neither a final directory nor a temp tree.
    ExportTransactionRequest failed = request;
    failed.directoryStem = "fail-" + tag;
    failed.failurePoint = ExportFailurePoint::AfterEditedWrite;
    const auto failResult = publishVerifiedExport(failed);
    assert(!failResult.success);
    assert(!exists(root + "/fail-" + tag));
    assert(!exists(root + "/fail-" + tag + ".tmp"));

    // Every pre-publication fault point fails closed: the export root gains no final directory
    // and no temporary tree (only the successful UI export above remains).
    for (const auto point : {ExportFailurePoint::AfterBackupWrite, ExportFailurePoint::AfterEditedWrite,
                             ExportFailurePoint::AfterReadbackVerify, ExportFailurePoint::AfterManifestWrite,
                             ExportFailurePoint::AfterFirstPublishedFile}) {
        ExportTransactionRequest faulty = request;
        faulty.directoryStem = "fault-" + tag;
        faulty.failurePoint = point;
        assert(!publishVerifiedExport(faulty).success);
        assert(rootEntries(root).size() == 1);
    }

    // QA-F3b: a destination that appears after the unique name was chosen (an empty directory,
    // which rename(2) would silently replace, or a plain file) is never replaced or removed, and
    // nothing of this export is left behind.
    ExportTransactionRequest racedDir = request;
    racedDir.directoryStem = "raced-dir-" + tag;
    racedDir.failurePoint = ExportFailurePoint::DestinationAppearsAsEmptyDirectory;
    const auto racedDirResult = publishVerifiedExport(racedDir);
    assert(!racedDirResult.success);
    assert(racedDirResult.error.find("appeared before publication") != std::string::npos);
    assert(isDirectory(root + "/raced-dir-" + tag));
    assert(rootEntries(root + "/raced-dir-" + tag).empty());
    assert(rootEntries(root).size() == 2);
    ::rmdir((root + "/raced-dir-" + tag).c_str());

    ExportTransactionRequest racedFile = request;
    racedFile.directoryStem = "raced-file-" + tag;
    racedFile.failurePoint = ExportFailurePoint::DestinationAppearsAsFile;
    const auto racedFileResult = publishVerifiedExport(racedFile);
    assert(!racedFileResult.success);
    assert(racedFileResult.error.find("appeared before publication") != std::string::npos);
    assert(isEmptyRegularFile(root + "/raced-file-" + tag));
    assert(rootEntries(root).size() == 2);
    std::remove((root + "/raced-file-" + tag).c_str());
    assert(rootEntries(root).size() == 1);

    // A clean low-level publish leaves no temporary directory beside the final one.
    ExportTransactionRequest clean = request;
    clean.directoryStem = "clean-" + tag;
    const auto cleanResult = publishVerifiedExport(clean);
    assert(cleanResult.success);
    assert(readBytes(cleanResult.directory + "/edited.srm") == edited);
    assert(readBytes(cleanResult.directory + "/original_backup.srm") == source);
    assert(exists(cleanResult.directory + "/EDIT_MANIFEST.txt"));
    for (const auto& name : rootEntries(root)) assert(name.find(".tmp") == std::string::npos);
    removeExport(cleanResult.directory);

    // 0x2009 is inside the checksum-covered payload for both G/S and Crystal fixtures.
    // Flipping it without updating the stored checksum must be rejected by strict parse before
    // any export directory is created. Do not weaken this to accept successful parsing.
    auto corrupt = edited;
    corrupt[0x2009] ^= 0x01;
    assert(!parse(corrupt, game));
    ExportTransactionRequest invalid = request;
    invalid.directoryStem = "invalid-" + tag;
    invalid.editedBytes = corrupt;
    const auto invalidResult = publishVerifiedExport(invalid);
    assert(!invalidResult.success);
    assert(!exists(root + "/invalid-" + tag));
    assert(!exists(root + "/invalid-" + tag + ".tmp"));

    // Exporting is app-owned only. The source object and StagedEditor original bytes never change.
    assert(source == std::vector<uint8_t>(parsed.save->sourceBytes().begin(), parsed.save->sourceBytes().end()));
    assert(source == std::vector<uint8_t>(editor->originalBytes().begin(), editor->originalBytes().end()));

    removeExport(result.directory);
    ::rmdir(root.c_str());
}


// Phase 4: a Box edit and a Trainer (money) edit exported SEPARATELY. Neither touches the party
// list or the RTC trailer; the Box edit never touches money; the money edit never touches PC box
// banks. Both reparse strictly. Truncated, oversized, empty and checksum-broken images are refused.
void phase4(const L& layout, SourceGame game, const std::string& tag) {
    const auto source = fixture(layout, true);
    const size_t partyLength = listLen(6, 48, 11);
    const std::string root = PokeBankTest::freshTempBase("pokebank-gen2-phase4");  // fresh per run
    auto exportWith = [&](auto&& stage, const std::string& stem) {
        auto parsed = parse(source, game);
        assert(parsed);
        std::string error;
        auto editor = StagedEditor::create(*parsed.save, error);
        assert(editor && stage(*editor, error));
        StagedEditorExportRequest request;
        request.rootDirectory = root;
        request.directoryStem = stem + "-" + tag;
        request.sourcePath = "/retroarch/saves/" + tag + ".srm";
        request.gameId = sourceGameId(game);
        request.timestamp = "20261015-120000";
        const auto result = publishVerifiedStagedEditorExport(*editor, request);
        assert(result.success);
        assert(readBytes(result.directory + "/original_backup.srm") == source);
        auto edited = readBytes(result.directory + "/edited.srm");
        removeExport(result.directory);
        assert(edited.size() == source.size());
        assert(parse(edited, game));
        // Party list and RTC trailer are never written by either edit.
        assert(std::equal(edited.begin() + static_cast<std::ptrdiff_t>(layout.party),
                          edited.begin() + static_cast<std::ptrdiff_t>(layout.party + partyLength),
                          source.begin() + static_cast<std::ptrdiff_t>(layout.party)));
        assert(std::equal(edited.begin() + 0x8000, edited.end(), source.begin() + 0x8000));
        return edited;
    };

    const auto box = exportWith([](StagedEditor& editor, std::string& error) {
        BoxPokemonEdit edit;
        edit.nickname = "SPARK";
        return editor.stageBoxPokemonEdit(1, 0, edit, error);
    }, "box");
    assert(std::equal(box.begin() + static_cast<std::ptrdiff_t>(layout.money),
                      box.begin() + static_cast<std::ptrdiff_t>(layout.money + 3),
                      source.begin() + static_cast<std::ptrdiff_t>(layout.money)));
    {
        auto reparsed = parse(box, game);
        std::string error;
        auto reopened = StagedEditor::create(*reparsed.save, error);
        assert(reopened);
        const auto mon = reopened->boxedPokemon(1, 0, error);
        assert(mon && mon->nickname == "SPARK");
        assert(reparsed.save->party().size() == parse(source, game).save->party().size());
    }

    const auto money = exportWith([](StagedEditor& editor, std::string& error) {
        return editor.stageMoney(123457, error);
    }, "money");
    // PC box banks are untouched by a money edit. The only bytes there that may change are the
    // Gold/Silver secondary checksum (0x7E6D), which covers the mirrored backup copy of the
    // main data; Crystal keeps its secondary checksum outside the banks (0x1F0D).
    for (size_t i = 0x4000; i < 0x8000; ++i)
        if (money[i] != source[i]) assert(i == layout.sum2 || i == layout.sum2 + 1);

    auto truncated = source; truncated.resize(0x7FFF);
    assert(!parse(truncated, game));
    assert(!parse(std::vector<uint8_t>{}, game));
    auto oversized = source; oversized.resize(0x9000);
    assert(!parse(oversized, game));
    auto broken = source; broken[layout.money] ^= 0x01;
    assert(!parse(broken, game));
    ::rmdir(root.c_str());
}

// Round 8 automated QA: Export Copy of a battery save padded to 128 KiB (upstream PKSE 9a9a731),
// for a 32 KiB core with 0xFF fill and a 64 KiB core with 0x00 fill. Synthetic fixtures only.
// The published copy keeps the source size and every padding byte; original_backup is the source
// byte for byte; the stored checksums equal a fresh recomputation; it reparses strictly. An edited
// image whose padding was disturbed is refused before any export directory exists.
void paddedExport(const L& layout, SourceGame game, const std::string& tag, size_t core, uint8_t fill) {
    auto base = fixture(layout, true);
    base.resize(core, 0x00);
    auto source = base;
    source.resize(kPaddedSaveSize128K, fill);
    const auto pristine = source;
    auto parsed = parse(source, game);
    assert(parsed && parsed.save->metadata().payloadSize == core &&
           parsed.save->metadata().paddingSize == kPaddedSaveSize128K - core);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor && editor->stageMoney(123457, error));
    const auto edited = editor->finalizedBytes(error);
    assert(!edited.empty() && edited.size() == source.size());
    {
        auto recomputed = std::vector<uint8_t>(edited.begin(), edited.begin() + 0x8000);
        checksum(recomputed, layout);
        assert(std::equal(recomputed.begin(), recomputed.end(), edited.begin()));  // stored == recomputed
    }
    const std::string root = PokeBankTest::freshTempBase("pokebank-gen2-padded-export");
    StagedEditorExportRequest request;
    request.rootDirectory = root;
    request.directoryStem = "padded-" + tag;
    request.sourcePath = "/retroarch/saves/" + tag + "-padded.srm";
    request.gameId = sourceGameId(game);
    request.timestamp = "20261028-120000";
    request.applicationSha = "test-sha";
    const auto result = publishVerifiedStagedEditorExport(*editor, request);
    if (!result.success) std::fprintf(stderr, "padded export failed for %s: %s\n", tag.c_str(), result.error.c_str());
    assert(result.success);
    const auto written = readBytes(result.directory + "/edited.srm");
    assert(readBytes(result.directory + "/original_backup.srm") == pristine);
    assert(written == edited && written.size() == kPaddedSaveSize128K);
    assert(std::all_of(written.begin() + static_cast<std::ptrdiff_t>(core), written.end(),
                       [fill](uint8_t b) { return b == fill; }));
    if (core > 0x8000)  // the 64 KiB core's upper half is carried verbatim
        assert(std::equal(written.begin() + 0x8000, written.begin() + static_cast<std::ptrdiff_t>(core),
                          pristine.begin() + 0x8000));
    auto reparsed = parse(written, game);
    assert(reparsed && reparsed.save->trainer().money == 123457);
    removeExport(result.directory);

    // Deliberately failing: one padding byte changed in the edited image.
    auto disturbed = edited;
    disturbed[kPaddedSaveSize128K - 1] ^= 0xFF;
    ExportTransactionRequest bad;
    bad.rootDirectory = root;
    bad.directoryStem = "padded-bad-" + tag;
    bad.sourcePath = request.sourcePath;
    bad.gameId = request.gameId;
    bad.timestamp = request.timestamp;
    bad.originalBytes = pristine;
    bad.editedBytes = disturbed;
    bad.sourceGame = game;
    bad.applicationSha = request.applicationSha;
    bad.pendingChanges = editor->pendingChanges();
    const auto refused = publishVerifiedExport(bad);
    assert(!refused.success && refused.error.find("padded to 0x20000") != std::string::npos);
    assert(!exists(root + "/padded-bad-" + tag));
    assert(source == pristine);  // the source buffer is never written
    ::rmdir(root.c_str());
}

} // namespace

int main() {
    runExport(GS, SourceGame::Gold, "gold");
    runExport(GS, SourceGame::Silver, "silver");
    runExport(C, SourceGame::Crystal, "crystal");
    phase4(GS, SourceGame::Gold, "gold");
    phase4(GS, SourceGame::Silver, "silver");
    phase4(C, SourceGame::Crystal, "crystal");
    paddedExport(GS, SourceGame::Gold, "gold-ff32", 0x8000, 0xFF);
    paddedExport(C, SourceGame::Crystal, "crystal-0064", 0x10000, 0x00);
    std::puts("GSC verified staged UI export + transaction: files, provenance, read-back, strict reparse, cleanup and source immutability PASS");
}
