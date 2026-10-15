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

// No C++ defect is open. Fixed cases live in test_qa_regressions.cpp:
//   QA-F4 (Gen V PID replay links, fixed bc83815), QA-F5 (Gen V Home party strip form, fixed on #307).

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    std::cout << "  (no open C++ defect; QA-F2 is reproduced by tests/test_qa_known_failures_assets.py)\n";
    return failures() == 0 ? 0 : 1;
}
