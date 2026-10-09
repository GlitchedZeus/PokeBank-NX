#include "Legality/Gen34EggBallEvidence.h"
#include <cassert>

int main() {
    using namespace Legality::Gen34EggBallEvidence;
    static_assert(analyze(3,true,0,0,4)==Kind::NativeEggPokeBall);
    static_assert(analyze(4,true,2000,0,4)==Kind::NativeEggPokeBall);
    static_assert(analyze(4,false,2000,0,4)==Kind::NativeEggPokeBall);
    static_assert(analyze(3,false,0,0,4)==Kind::Unresolved);
    static_assert(analyze(4,false,0,0,4)==Kind::Unresolved);
    static_assert(analyze(3,true,0,0,5)==Kind::Unresolved);
    static_assert(analyze(4,true,2000,0,24)==Kind::Unresolved);
    static_assert(analyze(4,true,0,0,4)==Kind::Unresolved);
    static_assert(analyze(3,true,0,1,4)==Kind::Unresolved);
    static_assert(analyze(4,true,2000,1,4)==Kind::Unresolved);
    static_assert(analyze(5,true,2000,0,4)==Kind::Unresolved);
    assert(analyze(4,true,2000,0,4)==Kind::NativeEggPokeBall);
}
