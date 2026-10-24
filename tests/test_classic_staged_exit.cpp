#include "UI/ClassicStagedExit.h"
#include "UI/ClassicTrainerSession.h"
#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    namespace E = PokeBank::UIModel::ClassicStagedExit;
    using A = E::Action;
    assert(E::choose(false, false, false, false) == A::None);
    assert(E::choose(true, false, false, false) == A::Review);
    assert(E::choose(false, true, false, false) == A::ContinueEditing);
    assert(E::choose(false, false, true, false) == A::ExportCopy);
    assert(E::choose(false, false, false, true) == A::DiscardExit);
    for (int mask = 0; mask < 16; ++mask) {
        if (mask & 2) assert(E::choose(mask & 1, true, mask & 4, mask & 8) == A::ContinueEditing);
    }
    assert(E::choose(false, false, false, false, 0) == A::ContinueEditing);
    assert(E::choose(false, false, false, false, 1) == A::DiscardExit);
    assert(E::choose(false, false, false, false, 2) == A::Review);
    assert(E::choose(false, false, false, false, 3) == A::ExportCopy);
    assert(E::choose(false, false, false, false, 90) == A::None);
    assert(E::choose(false, true, false, false, 1) == A::ContinueEditing);
    namespace T = PokeBank::UIModel::ClassicTrainerSession;
    assert(std::string_view(T::trainerPageInstructions(true)).find("Y: Review") != std::string_view::npos);
    assert(std::string_view(T::trainerPageInstructions(false)).find("Review") == std::string_view::npos);
    std::cout << "Classic staged exit: Review/Continue/Export/Y Discard, controller-touch parity and B priority PASS\n";
}
