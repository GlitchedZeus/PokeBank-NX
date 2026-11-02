// Mirrors Legality.cpp's named Candidate initialized from Pokemon::otName().
// ASan must diagnose stack-use-after-scope at commonFields. No Gen III production fix.
#include "Legality/Gen3NegaiBoshiEventTemplate.h"
#include <string>
std::u16string otName() { return u"ネガイボシ"; }
int main() {
    const Legality::Gen3NegaiBoshiEvent::Candidate candidate{
        385, 30719, 0, 2, 1, 0, 5, 255, 4, false, false, false, otName()
    };
    return Legality::Gen3NegaiBoshiEvent::commonFields(candidate) ? 0 : 1;
}
