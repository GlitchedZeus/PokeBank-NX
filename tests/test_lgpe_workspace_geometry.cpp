#include "Trainer/Trainer7LGPE.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

int main() {
    using namespace Trainer;

    std::vector<uint8_t> active(SAVE_SIZE7_LGPE, 0x11);
    std::vector<uint8_t> full(LGPE_FULL_FILE_SIZE, 0x22);

    assert(isSupportedLGPEWorkspaceSize(active.size()));
    assert(isSupportedLGPEWorkspaceSize(full.size()));
    assert(!isSupportedLGPEWorkspaceSize(SAVE_SIZE7_LGPE - 1));
    assert(!isSupportedLGPEWorkspaceSize(SAVE_SIZE7_LGPE + 1));
    assert(!isSupportedLGPEWorkspaceSize(LGPE_FULL_FILE_SIZE + 1));

    const auto activeView = lgpeActiveRegion(active);
    assert(activeView.size() == SAVE_SIZE7_LGPE);
    assert(activeView.data() == active.data());

    // AUDIT-023: authentic 1 MiB savedata.bin validates only its Beluga active region.
    full[SAVE_SIZE7_LGPE - 1] = 0xA5;
    full[SAVE_SIZE7_LGPE] = 0x5A; // first trailing byte must remain outside the active view.
    const auto fullView = lgpeActiveRegion(full);
    assert(fullView.size() == SAVE_SIZE7_LGPE);
    assert(fullView.data() == full.data());
    assert(fullView.back() == 0xA5);

    std::vector<uint8_t> intermediate(SAVE_SIZE7_LGPE + 1, 0);
    assert(lgpeActiveRegion(intermediate).empty());

    std::cout << "LGPE active/full workspace geometry: PASS\n";
    return 0;
}
