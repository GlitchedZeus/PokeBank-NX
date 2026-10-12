#include "Trainer/Gen9MyStatusValidation.h"

#include <cassert>
#include <cstddef>
#include <iostream>

int main() {
    using Trainer::Gen9MyStatus::hasCoreFields;
    using Trainer::Gen9MyStatus::kCoreFieldBytes;

    static_assert(kCoreFieldBytes == 6);
    static_assert(!hasCoreFields(0));
    static_assert(!hasCoreFields(4));
    static_assert(!hasCoreFields(5));
    static_assert(hasCoreFields(6));

    for (std::size_t size = 0; size < kCoreFieldBytes; ++size)
        assert(!hasCoreFields(size));
    assert(hasCoreFields(kCoreFieldBytes));
    assert(hasCoreFields(kCoreFieldBytes + 1));

    std::cout << "Gen IX MyStatus core-field bounds contract: PASS\n";
    return 0;
}
