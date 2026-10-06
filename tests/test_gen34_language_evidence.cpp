#include "Legality/Gen34LanguageEvidence.h"

#include <cassert>

int main() {
    using Legality::Gen34Language::Result;
    using Legality::Gen34Language::classify;

    // Unwired/unknown language data must remain unresolved, not Invalid.
    assert(classify(3, 0) == Result::Unresolved);
    assert(classify(4, 0) == Result::Unresolved);

    // PKHeX Gen III maximum: Spanish (7); language id 6 is unused.
    assert(classify(3, 5) == Result::Valid);
    assert(classify(3, 6) == Result::Invalid);
    assert(classify(3, 7) == Result::Valid);
    assert(classify(3, 8) == Result::Invalid);
    assert(classify(3, 10) == Result::Invalid);

    // PKHeX Gen IV maximum: Korean (8); language id 6 remains unused.
    assert(classify(4, 5) == Result::Valid);
    assert(classify(4, 6) == Result::Invalid);
    assert(classify(4, 7) == Result::Valid);
    assert(classify(4, 8) == Result::Valid);
    assert(classify(4, 9) == Result::Invalid);
    assert(classify(4, 10) == Result::Invalid);

    // This helper is deliberately scoped to Gen III/IV only.
    assert(classify(1, 7) == Result::Unresolved);
    assert(classify(2, 7) == Result::Unresolved);
    assert(classify(5, 7) == Result::Unresolved);

    return 0;
}
