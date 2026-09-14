#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2ExportTransaction.h"

#include <cassert>
#include <cstdio>
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

    const std::string root = "/tmp/pokebank-gen2-export-" + std::to_string(::getpid());
    ::mkdir(root.c_str(), 0777);
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

} // namespace

int main() {
    runExport(GS, SourceGame::Gold, "gold");
    runExport(GS, SourceGame::Silver, "silver");
    runExport(C, SourceGame::Crystal, "crystal");
    std::puts("GSC verified staged UI export + transaction: files, provenance, read-back, strict reparse, cleanup and source immutability PASS");
}
