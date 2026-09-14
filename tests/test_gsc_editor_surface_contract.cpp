#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "Gen II editor foundation source must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    const auto source = readFile("src/UI/Gen2PokemonEditorFoundation.inc");

    // Hardware swkbd safety: Level and EXP are separate rows/actions, so one A press never
    // deliberately chains the two library applets in the same update.
    assert(source.find("{\"Level\", std::to_string(p.level)}") != std::string::npos);
    assert(source.find("{\"EXP\", std::to_string(p.experience)}") != std::string::npos);
    assert(source.find("Level / EXP") == std::string::npos);

    // Transactional Edit snapshots the current staged entry, edits a local working copy, and
    // exposes the accepted Keep / discard-this-session / Continue interaction.
    assert(source.find("state.baseline = *pokemon") != std::string::npos);
    assert(source.find("state.working = *pokemon") != std::string::npos);
    assert(source.find("Keep staged Pokemon edits?") != std::string::npos);
    assert(source.find("This Edit session discarded; earlier staged work preserved") != std::string::npos);
    assert(source.find("{\"A\", \"Keep\"}") != std::string::npos);
    assert(source.find("{\"X\", \"Discard\"}") != std::string::npos);
    assert(source.find("{\"B\", \"Continue\"}") != std::string::npos);

    // Create is explicitly staged and cancellable; party mutation remains called out as deferred.
    assert(source.find("Stage Add blocked: correct red incompatible move rows first") != std::string::npos);
    assert(source.find("Create draft cancelled; staged save unchanged") != std::string::npos);
    assert(source.find("Party mutation deferred") != std::string::npos);

    // Exact-format capability hygiene stays visible in the shared shell.
    assert(source.find("Nature / Ability / Ribbons / Marks hidden") != std::string::npos);
    assert(source.find("HP DV is derived") != std::string::npos);
    assert(source.find("one Special DV feeds SpA + SpD") != std::string::npos);
    assert(source.find("Held Item — Generation II") != std::string::npos);

    // The real GSC Review -> Export UI is only a metadata/path wrapper around the verified staged
    // transaction. Keep a small source-surface guard here in addition to the behavioral export test:
    // no second fopen/fwrite implementation is allowed to creep back into the overlay.
    const auto overlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(overlay.find("publishVerifiedStagedEditorExport(editor, request)") != std::string::npos);
    assert(overlay.find("Verified staged export: ") != std::string::npos);
    assert(overlay.find("source remains unchanged") != std::string::npos);
    assert(overlay.find("std::fopen") == std::string::npos);
    assert(overlay.find("std::fwrite") == std::string::npos);
    assert(overlay.find("writeBytes(") == std::string::npos);
    assert(overlay.find("writeText(") == std::string::npos);

    std::cout << "GSC shared editor surface/session/export contract: PASS\n";
    return 0;
}
