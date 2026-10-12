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
    const std::string source = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto begin = source.find("bool beginSelectedGroup(");
    const auto end = source.find("bool cancel(", begin);
    assert(begin != std::string::npos);
    assert(end != std::string::npos && end > begin);
    const std::string body = source.substr(begin, end - begin);

    // AUDIT-037: both group-pickup generations must cancel backend custody when
    // presentation refresh fails after a successful beginPackedGroupMove().
    const auto gen1Begin = body.find("editor->beginPackedGroupMove");
    const auto gen1Refresh = body.find("refreshGen1(screen, refreshError)", gen1Begin);
    const auto gen1Cancel = body.find("editor->cancelPackedMove(cancelError)", gen1Refresh);
    const auto gen1Restore = body.find("refreshGen1(screen, restoreError)", gen1Cancel);
    assert(gen1Begin != std::string::npos);
    assert(gen1Refresh != std::string::npos);
    assert(gen1Cancel != std::string::npos);
    assert(gen1Restore != std::string::npos);
    assert(gen1Begin < gen1Refresh && gen1Refresh < gen1Cancel && gen1Cancel < gen1Restore);

    const auto gen2Begin = body.find("editor->beginPackedGroupMove", gen1Begin + 1);
    const auto gen2Refresh = body.find("refreshGen2(screen, refreshError)", gen2Begin);
    const auto gen2Cancel = body.find("editor->cancelPackedMove(cancelError)", gen2Refresh);
    const auto gen2Restore = body.find("refreshGen2(screen, restoreError)", gen2Cancel);
    assert(gen2Begin != std::string::npos);
    assert(gen2Refresh != std::string::npos);
    assert(gen2Cancel != std::string::npos);
    assert(gen2Restore != std::string::npos);
    assert(gen2Begin < gen2Refresh && gen2Refresh < gen2Cancel && gen2Cancel < gen2Restore);

    // If cancellation itself fails, UI custody must remain active so B can retry.
    const auto rollbackBranch = body.find("if (rollbackFailed)");
    assert(rollbackBranch != std::string::npos);
    assert(body.find("state.active = true", rollbackBranch) != std::string::npos);
    assert(body.find("state.generation = attemptedGeneration", rollbackBranch) != std::string::npos);
    assert(body.find("setLegacyHoldingPresentation(screen, true)", rollbackBranch) != std::string::npos);

    std::cout << "Classic packed-move refresh rollback contract: PASS\n";
    return 0;
}
