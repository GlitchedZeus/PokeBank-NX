#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    const std::string header = read("include/UI/TrainerViewScreenBase.h");
    const std::string source = read("src/UI/TrainerViewScreenBase.inc");

    // A03 regression: destination adaptation must be represented as a separate
    // prepared candidate. The carried moveMon object is authoritative custody
    // until the destination slot actually owns the prepared candidate.
    assert(header.find("struct PreparedPlacement") != std::string::npos);
    assert(header.find("prepareForPane(") != std::string::npos);
    assert(header.find("const Pokemon::Pokemon& original") != std::string::npos);
    assert(header.find("bool convertForPane(std::unique_ptr<Pokemon::Pokemon>&") ==
           std::string::npos);

    const auto prepareBegin = source.find("TrainerViewScreen::prepareForPane(");
    const auto prepareEnd = source.find(
        "bool TrainerViewScreen::lgpeConversionInvolved", prepareBegin);
    assert(prepareBegin != std::string::npos);
    assert(prepareEnd != std::string::npos && prepareEnd > prepareBegin);
    const std::string prepare = source.substr(prepareBegin, prepareEnd - prepareBegin);

    // Bank placement must keep the native payload and defer moving the original
    // until the slot commit.
    assert(prepare.find("useOriginal = true") != std::string::npos);

    // Same-game normalization must happen on a clone/candidate, never on the
    // authoritative object still held in moveMon.
    assert(prepare.find("original.clone()") != std::string::npos);
    assert(prepare.find("normalizeAffixedRibbon(*candidate)") != std::string::npos);
    assert(prepare.find("candidate->refreshChecksum()") != std::string::npos);

    // Cross-game conversion consumes a const source and returns a separate candidate.
    assert(prepare.find("Conversion::convert(original") != std::string::npos);
    assert(prepare.find("pk = std::move(converted)") == std::string::npos);

    const auto putBegin = source.find("void TrainerViewScreen::putDownBlock()");
    const auto putEnd = source.find(
        "void TrainerViewScreen::storagePickup()", putBegin);
    assert(putBegin != std::string::npos);
    assert(putEnd != std::string::npos && putEnd > putBegin);
    const std::string put = source.substr(putBegin, putEnd - putBegin);

    assert(put.find("prepareForPane(*moveMon[idx], pane)") != std::string::npos);
    assert(put.find("prepared.useOriginal") != std::string::npos);
    assert(put.find("dst = std::move(prepared.candidate)") != std::string::npos);
    assert(put.find("moveMon[idx] = std::move(displaced)") != std::string::npos);

    // Candidate ownership must reach the destination before original custody is
    // retired/replaced by the displaced slot occupant.
    const auto destCommit = put.find("dst = std::move(prepared.candidate)");
    const auto custodyRetire = put.find("moveMon[idx] = std::move(displaced)", destCommit);
    assert(destCommit != std::string::npos);
    assert(custodyRetire != std::string::npos);
    assert(destCommit < custodyRetire);

    // Native Bank deposits keep the old direct swap semantics.
    assert(put.find("std::swap(dst, moveMon[idx])") != std::string::npos);

    // A02 rollback protection must remain present while A03 changes placement.
    const auto returnBegin = source.find("void TrainerViewScreen::returnHeldToOrigin()");
    const auto returnEnd = source.find(
        "std::unique_ptr<Pokemon::Pokemon>& TrainerViewScreen::storageSlot", returnBegin);
    assert(returnBegin != std::string::npos);
    assert(returnEnd != std::string::npos && returnEnd > returnBegin);
    const std::string rollback = source.substr(returnBegin, returnEnd - returnBegin);
    assert(rollback.find("if (!returnPlan.complete)") != std::string::npos);
    assert(rollback.find("custody retained") != std::string::npos);

    std::cout << "Storage conversion custody/candidate contract: PASS\n";
}
