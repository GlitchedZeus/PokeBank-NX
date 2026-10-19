// PokeBank NX application QA: reproductions of OPEN defects (Thread 4, QA-owned).
//
// NOT part of host-test / host-sanitize / `make qa`: each case FAILS (non-zero exit) while its defect
// is open, so it can be ported verbatim into the owning thread's suite as the regression test for
// the fix. When the fix lands, the case moves to test_qa_regressions.cpp. Run:
//   make -f Makefile.host qa-known-failures
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"


#include <iostream>

using namespace qacontract;

namespace {


// No C++ defect is open: QA-F4..F8b are fixed and live in test_qa_regressions.cpp.

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    std::cout << "  (no open C++ defect)\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known C++ defect reproduces any more: move these cases into test_qa_regressions.cpp.\n";
    return 0;
}
